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
 * Desc: A simple example of how to write a driver that supports multiple interface.
 * Also demonstrates use of a driver as a loadable object.
 * Author: Andrew Howard
 * Date: 25 July 2004
 * CVS: $Id$
 */


// ONLY if you need something that was #define'd as a result of configure 
// (e.g., HAVE_CFMAKERAW), then #include <config.h>, like so:
/*
#if HAVE_CONFIG_H
  #include <config.h>
#endif
*/

#include <unistd.h>
#include <string.h>

#include "playercommon.h"
#include "drivertable.h"
#include "deviceregistry.h"
#include "player.h"


////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class MultiDriver : public CDevice
{
  // Constructor; need that
  public: MultiDriver(ConfigFile* cf, int section);

  // Must implement the following methods.
  public: int Setup();
  public: int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  private: void CheckConfig();
  private: void CheckCommands();
  private: void RefreshData();

  // Position interface
  private: player_device_id_t position_id;
  private: player_position_data_t position_data;
  private: player_position_cmd_t position_cmd;

  // Laser interface
  private: player_device_id_t laser_id;
  private: player_laser_data_t laser_data;
};


// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic CDevice*) a pointer to a new instance of this driver.
CDevice* MultiDriver_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return ((CDevice*) (new MultiDriver(cf, section)));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void MultiDriver_Register(DriverTable* table)
{
  table->AddDriverEx("multidriver", MultiDriver_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Extra stuff for building a shared object.

// Need access to the global driver table
#include <drivertable.h>
extern DriverTable* driverTable;

/* need the extern to avoid C++ name-mangling  */
extern "C"
{
  void _init(void)
    //void __attribute__ ((constructor)) example_init(void)
  {
    puts("pluging init");
    MultiDriver_Register(driverTable);
  }

  void _fini(void)
    //void __attribute__ ((destructor)) example_fini(void)
  {
    puts("plugin fini");

    /* probably don't need any code here; the destructor for the device
     * will be called when Player shuts down.  this function is only useful
     * if you want to dlclose() the shared object before Player exits
     */
  }
}


////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
MultiDriver::MultiDriver(ConfigFile* cf, int section)
    : CDevice(0, 0, 10, 10)
{
  // Create position interface
  if (cf->ReadDeviceId(section, 0, PLAYER_POSITION_CODE, &this->position_id) != 0)
  {
    this->SetError(-1);
    return;
  }  
  if (this->AddInterfaceEx(this->position_id, "multidriver", PLAYER_ALL_MODE,
                           sizeof(player_position_data_t),
                           sizeof(player_position_cmd_t), 10, 10) != 0)
  {
    this->SetError(-1);    
    return;
  }

  // Create laser interface
  if (cf->ReadDeviceId(section, 1, PLAYER_LASER_CODE, &this->laser_id) != 0)
  {
    this->SetError(-1);
    return;
  }    
  if (this->AddInterfaceEx(this->laser_id, "multidriver", PLAYER_READ_MODE,
                           sizeof(player_laser_data_t), 0, 10, 10) != 0)
  {
    this->SetError(-1);        
    return;
  }

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int MultiDriver::Setup()
{   
  puts("Example driver initialising");

  // Here you do whatever is necessary to setup the device, like open and
  // configure a serial port.
    
  puts("Example driver ready");

  // Start the device thread; spawns a new thread and executes
  // MultiDriver::Main(), which contains the main loop for the driver.
  this->StartThread();

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int MultiDriver::Shutdown()
{
  puts("Shutting example driver down");

  // Stop and join the driver thread
  this->StopThread();

  // Here you would shut the device down by, for example, closing a
  // serial port.

  puts("Example driver has been shutdown");

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void MultiDriver::Main() 
{
  // The main loop; interact with the device here
  for(;;)
  {
    // test if we are supposed to cancel
    pthread_testcancel();

    // Check for and handle configuration requests
    this->CheckConfig();

    // Check for commands
    this->CheckCommands();

    // Write outgoing data
    this->RefreshData();
    
    // Sleep (you might, for example, block on a read() instead)
    usleep(100000);
  }
  return;
}


void MultiDriver::CheckConfig()
{
  void *client;
  unsigned char buffer[PLAYER_MAX_REQREP_SIZE];
  
  while (this->GetConfigEx(this->position_id, &client, &buffer, sizeof(buffer)) > 0)
  {
    printf("got position request\n");
    if (this->PutReplyEx(this->position_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  while (this->GetConfigEx(this->laser_id, &client, &buffer, sizeof(buffer)) > 0)
  {
    printf("got laser request\n");
    if (this->PutReplyEx(this->laser_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  return;
}

void MultiDriver::CheckCommands()
{
  this->GetCommandEx(this->position_id, &this->position_cmd, sizeof(this->position_cmd));

  printf("%d %d\n", ntohl(this->position_cmd.xspeed), ntohl(this->position_cmd.yawspeed));
  
  return;
}

void MultiDriver::RefreshData()
{

  // Write position data
  memset(&this->position_data, 0, sizeof(this->position_data));
  this->PutDataEx(this->position_id, &this->position_data, sizeof(this->position_data), 0, 0);

  // Write laser data
  memset(&this->laser_data, 0, sizeof(this->laser_data));
  this->PutDataEx(this->laser_id, &this->laser_data, sizeof(this->laser_data), 0, 0);

  return;
}
