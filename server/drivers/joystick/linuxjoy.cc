/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  
 *     Brian Gerkey, Andrew Howard
 *                      
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Desc: Read data from a standard linux joystick
 * Author: Andrew Howard
 * Date: 25 July 2004
 * CVS: $Id$
 *
 */

#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "playercommon.h"
#include "drivertable.h"
#include "driver.h"
#include "player.h"


/////////////////////////////////////////////////////////////////////
// this is the event structure from the linux joystick driver v2.0.0
// I have blind-copied this from Brian's playerjoy, but shouldnt this
// be in header file somewhere?

struct js_event
{
  uint32_t time;     /* event timestamp in milliseconds */
  int16_t value;    /* value */
  uint8_t type;      /* event type */
  uint8_t number;    /* axis/button number */
};

#define JS_EVENT_BUTTON         0x01    /* button pressed/released */
#define JS_EVENT_AXIS           0x02    /* joystick moved */
#define JS_EVENT_INIT           0x80    /* initial state of device */

#define XAXIS 0
#define YAXIS 1
#define XAXIS2 2
#define YAXIS2 3
#define PAN    4
#define TILT   5
#define XAXIS4 6
#define YAXIS4 7

#define AXIS_MAX ((int16_t) 32767)


////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class LinuxJoystick : public Driver
{
  // Constructor; need that
  public: LinuxJoystick(ConfigFile* cf, int section);

  // Must implement the following methods.
  public: int Setup();
  public: int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Read the joystick
  private: void ReadJoy();

  // Write new data to server
  private: void RefreshData();

  // Check for new configuration requests
  private: void CheckConfig();

  // Joystick device
  private: const char *dev;
  private: int fd;
  private: int16_t xpos, ypos;
  private: uint16_t buttons;

  // Joystick
  private: player_joystick_data_t joy_data;
};


////////////////////////////////////////////////////////////////////////////////
// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver* LinuxJoystick_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return ((Driver*) (new LinuxJoystick(cf, section)));
}


////////////////////////////////////////////////////////////////////////////////
// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void LinuxJoystick_Register(DriverTable* table)
{
  table->AddDriver("linuxjoystick", LinuxJoystick_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
LinuxJoystick::LinuxJoystick(ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_JOYSTICK_CODE, PLAYER_READ_MODE,
             sizeof(player_joystick_data_t), 0, 10, 10)
{
  // Ethernet interface to monitor
  this->dev = cf->ReadString(section, "port", "/dev/js0");

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int LinuxJoystick::Setup()
{
  // Open the joystick device
  this->fd = open(this->dev, O_RDONLY);
  if (this->fd < 1)
  {
    PLAYER_ERROR2("unable to open joystick [%s]; %s",
                  this->dev, strerror(errno));
    return -1;
  }
  
  // Start the device thread; spawns a new thread and executes
  // LinuxJoystick::Main(), which contains the main loop for the driver.
  this->StartThread();

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int LinuxJoystick::Shutdown()
{
  // Stop and join the driver thread
  this->StopThread();

  // Close the joystick
  close(this->fd);

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void LinuxJoystick::Main() 
{
  // The main loop; interact with the device here
  while (true)
  {    
    // test if we are supposed to cancel
    pthread_testcancel();

    // Check for and handle configuration requests
    this->CheckConfig();

    // Run and process output
    this->ReadJoy();
    
    // Write outgoing data
    this->RefreshData();
    
    // Sleep (you might, for example, block on a read() instead)
    //usleep(100000);
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Read the joystick
void LinuxJoystick::ReadJoy()
{
  struct js_event event;
  
  //puts("Reading from joystick");

  // get the next event from the joystick
  read(this->fd, &event, sizeof(struct js_event));

  //printf( "value % d type %u  number %u state %X \n", 
  //        event.value, event.type, event.number, this->joy_data.buttons );

  // Update buttons
  if ((event.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON)
  {
    if (event.value)
      this->buttons |= (1 << event.number);
    else
      this->buttons &= ~(1 << event.number);
  }
            
  // ignore the startup events
  if (event.type & JS_EVENT_INIT)
    return;

  switch( event.type )
  {
    case JS_EVENT_AXIS:
    {
      switch( event.number )
      {
        case XAXIS:
          this->xpos = event.value;
          break;
        case YAXIS:
          this->ypos = event.value;
          break;
      }
    }	  
    break;
  }
      
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Send new data to server
void LinuxJoystick::RefreshData()
{ 
  //printf("%d %d\n", this->xpos, this->ypos);
  
  // Do byte reordering
  this->joy_data.xpos = htons(this->xpos);
  this->joy_data.ypos = htons(this->ypos);
  this->joy_data.xscale = htons(AXIS_MAX);
  this->joy_data.yscale = htons(AXIS_MAX);
  this->joy_data.buttons = htons(this->buttons);
  this->PutData(&this->joy_data, sizeof(this->joy_data), NULL);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process configuration requests
void LinuxJoystick::CheckConfig()
{
  void *client;
  unsigned char buffer[PLAYER_MAX_REQREP_SIZE];
  
  while(this->GetConfig(&client, &buffer, sizeof(buffer), NULL) > 0)
  {
    if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  return;
}


