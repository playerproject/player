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

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_linuxjoystick linuxjoystick

The linuxjoystick driver reads data from a standard Linux joystick and
provides the data via the @ref player_interface_joystick interface.
This driver can also control a position device by converting joystick
positions to velocity commands.

@par Compile-time dependencies

- &lt;linux/joystick.h&gt;

@par Provides

- @ref player_interface_joystick

@par Requires

- @ref player_interface_position : if present, joystick positions will be
  interpreted as velocities and sent as commands to this position device.
  See also max_xspeed and max_yawspeed options below.

@par Configuration requests

- PLAYER_PTZ_GENERIC_CONFIG_REQ

@par Configuration file options

- port (string)
  - Default: "/dev/js0"
  - The joystick to be used.
- axes (integer tuple)
  - Default: [0 1]
  - Which joystick axes to call the "X" and "Y" axes, respectively.
- axis_maxima (integer tuple)
  - Default: [32767 32767]
  - Maximum absolute values attainable on the X and Y axes, respectively.
- axis_minima (integer tuple)
  - Default: [0 0]
  - Minimum values on the X and Y axes, respectively.  Anything smaller
    in absolute value than this limit will be reported as zero.
    Useful for implementing a dead zone on a touchy joystick.
- max_xspeed (length / sec)
  - Default: 0.5 m/sec
  - The maximum translational velocity to be used when commanding a
    position device.
- max_yawspeed (angle / sec)
  - Default: 30 deg/sec
  - The maximum rotational velocity to be used when commanding a
    position device.
- timeout (float)
  - Default: 5.0
  - Time (in seconds) since receiving a new joystick event after which
    the underlying position device will be stopped, for safety.  Set to
    0.0 for no timeout.

@par Example 

@verbatim
driver
(
  name "linuxjoystick"
  provides ["joystick:0"]
  port "/dev/js0"
)
@endverbatim

@todo
Add support for continuously sending commands, which might be needed for 
position devices that use watchdog timers.

@par Authors

Andrew Howard

*/
/** @} */

#include <linux/joystick.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>

#include "replace.h" // for poll(2)

#include "playercommon.h"
#include "drivertable.h"
#include "driver.h"
#include "devicetable.h"
#include "error.h"
#include "player.h"
#include "playertime.h"

extern PlayerTime *GlobalTime;

#define XAXIS 0
#define YAXIS 1

#define MAX_XSPEED 0.5
#define MAX_YAWSPEED DTOR(30.0)
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

  // Put new position command
  private: void PutPositionCommand();

  // Joystick device
  private: const char *dev;
  private: int fd;
  private: int16_t xpos, ypos;
  private: uint16_t buttons;
  private: int xaxis_max, yaxis_max;
  private: int xaxis_min, yaxis_min;
  private: double timeout;
  private: struct timeval lastread;

  // These are used when we send commands to a position device
  private: bool command_position;
  private: double max_xspeed, max_yawspeed;
  private: int xaxis, yaxis;
  private: player_device_id_t position_id;
  private: Driver* position;

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
  this->dev = cf->ReadString(section, "port", "/dev/js0");
  this->xaxis = cf->ReadTupleInt(section,"axes", 0, XAXIS);
  this->yaxis = cf->ReadTupleInt(section,"axes", 1, YAXIS);
  this->xaxis_max = cf->ReadTupleInt(section, "axis_maxima", 0, AXIS_MAX);
  this->yaxis_max = cf->ReadTupleInt(section, "axis_maxima", 1, AXIS_MAX);
  this->xaxis_min = cf->ReadTupleInt(section, "axis_minima", 0, 0);
  this->yaxis_min = cf->ReadTupleInt(section, "axis_minima", 1, 0);

  this->command_position = false;
  // Do we talk to a position device?
  if(cf->GetTupleCount(section, "requires"))
  {
    if(cf->ReadDeviceId(&(this->position_id), section, "requires", 
                        PLAYER_POSITION_CODE, -1, NULL) == 0)
    {
      this->command_position = true;
      this->max_xspeed = cf->ReadLength(section, "max_xspeed", MAX_XSPEED);
      this->max_yawspeed = cf->ReadAngle(section, "max_yawspeed", MAX_YAWSPEED);
      this->timeout = cf->ReadFloat(section, "timeout", 5.0);
    }
  }

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

  this->lastread.tv_sec = this->lastread.tv_usec = 0;

  // If we're asked, open the position device
  if(this->command_position)
  {
    player_position_power_config_t motorconfig;
    unsigned short reptype;
    player_position_cmd_t cmd;

    if(!(this->position = deviceTable->GetDriver(this->position_id)))
    {
      PLAYER_ERROR("unable to open position device");
      return(-1);
    }
    if(this->position->Subscribe(this->position_id) != 0)
    {
      PLAYER_ERROR("unable to subscribe to position device");
      return(-1);
    }

    // Enable the motors
    motorconfig.request = PLAYER_POSITION_MOTOR_POWER_REQ;
    motorconfig.value = 1;
    this->position->Request(this->position_id, this, 
                            &motorconfig, sizeof(motorconfig), NULL,
                            &reptype, NULL, 0, NULL);
    if(reptype != PLAYER_MSGTYPE_RESP_ACK)
      PLAYER_WARN("failed to enable motors");

    // Stop the robot
    memset(&cmd,0,sizeof(cmd));
    this->position->PutCommand(this->position_id,
                               (unsigned char*)&cmd,sizeof(cmd),NULL);
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

  if(this->command_position)
    this->position->Unsubscribe(this->position_id);

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
    // Sleep (you might, for example, block on a read() instead)
    // REMOVE usleep(100000);

    // test if we are supposed to cancel
    pthread_testcancel();

    // Check for and handle configuration requests
    this->CheckConfig();

    // Run and process output
    this->ReadJoy();
    
    // Write outgoing data
    this->RefreshData();

    // Send new commands to position device
    if(this->command_position)
      this->PutPositionCommand();
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Read the joystick
void LinuxJoystick::ReadJoy()
{
  struct pollfd fd;
  struct js_event event;
  int count;
  
  fd.fd = this->fd;
  fd.events = POLLIN | POLLHUP;
  fd.revents = 0;

  count = poll(&fd, 1, 10);
  if (count < 0)
    PLAYER_ERROR1("poll returned error [%s]", strerror(errno));
  else if(count > 0)
  {
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
          if(event.number == this->xaxis)
          {
            this->xpos = event.value;
            if(abs(this->xpos) < this->xaxis_min)
              this->xpos = 0;
            GlobalTime->GetTime(&this->lastread);
          }
          else if(event.number == this->yaxis)
          {
            this->ypos = event.value;
            if(abs(this->ypos) < this->yaxis_min)
              this->ypos = 0;
            GlobalTime->GetTime(&this->lastread);
          }
        }	  
        break;
    }
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
  this->joy_data.xscale = htons(this->xaxis_max);
  this->joy_data.yscale = htons(this->yaxis_max);
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

void LinuxJoystick::PutPositionCommand()
{
  double scaled_x, scaled_y;
  double xspeed, yawspeed;
  player_position_cmd_t cmd;
  struct timeval curr;
  double diff;

  scaled_x = this->xpos / (double) this->xaxis_max;
  scaled_y = this->ypos / (double) this->yaxis_max;

  // sanity check
  if((scaled_x > 1.0) || (scaled_x < -1.0))
  {
    PLAYER_ERROR2("X position (%d) outside of axis max (+-%d); ignoring", 
                  this->xpos, this->xaxis_max);
    return;
  }
  if((scaled_y > 1.0) || (scaled_y < -1.0))
  {
    PLAYER_ERROR2("Y position (%d) outside of axis max (+-%d); ignoring", 
                  this->ypos, this->yaxis_max);
    return;
  }

  // Note that joysticks use X for side-to-side and Y for forward-back, and
  // also that their axes are backwards with respect to intuitive driving
  // controls.
  xspeed = -scaled_y * this->max_xspeed;
  yawspeed = -scaled_x * this->max_yawspeed;

  // Make sure we've gotten a joystick fairly recently.
  GlobalTime->GetTime(&curr);
  diff = (curr.tv_sec - curr.tv_usec/1e6) -
          (this->lastread.tv_sec - this->lastread.tv_usec/1e6);
  if(this->timeout && (diff > this->timeout) && (xspeed || yawspeed))
  {
    PLAYER_WARN("Timeout on joystick; stopping robot");
    xspeed = yawspeed = 0.0;
  }


  PLAYER_MSG2(2,"sending speeds: (%f,%f)", xspeed, yawspeed);

  memset(&cmd,0,sizeof(cmd));
  cmd.xspeed = htonl((int)rint(xspeed*1e3));
  cmd.yawspeed = htonl((int)rint(RTOD(yawspeed)));
  cmd.type=0;
  cmd.state=1;

  this->position->PutCommand(this->position_id,
                             (unsigned char*)&cmd,sizeof(cmd),NULL);
}
