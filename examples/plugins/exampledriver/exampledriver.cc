/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  
 *     Brian Gerkey
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
 * A simple example of how to write a driver that will be built as a
 * shared object.
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
#include "player.h"

////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class ExampleDriver : public CDevice
{
  public:
    
    // Constructor; need that
    ExampleDriver(char* interface, ConfigFile* cf, int section);

    // Must implement the following methods.
    int Setup();
    int Shutdown();

  private:

    // Main function for device thread.
    virtual void Main();

    int foop;
};

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic CDevice*) a pointer to a new instance of this driver.
CDevice* 
ExampleDriver_Init(char* interface, ConfigFile* cf, int section)
{
  // Check whether we can support the requested interface; return NULL to
  // indicate that we can't.
  if(strcmp(interface, PLAYER_POSITION_STRING))
  {
    PLAYER_ERROR1("driver \"exampledriver\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
  {
    // Create and return a new instance of this driver
    return((CDevice*)(new ExampleDriver(interface, cf, section)));
  }
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void ExampleDriver_Register(DriverTable* table)
{
  table->AddDriver("exampledriver", PLAYER_READ_MODE, ExampleDriver_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
ExampleDriver::ExampleDriver(char* interface, ConfigFile* cf, int section)
: CDevice(sizeof(player_position_data_t),sizeof(player_position_cmd_t),10,10)
{
  // Read an option from the configuration file
  this->foop = cf->ReadInt(section, "foo", 0);

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int ExampleDriver::Setup()
{   
  puts("Example driver initialising");

  // Here you do whatever is necessary to setup the device, like open and
  // configure a serial port.

  printf("Was foo option given in config file? %d\n", this->foop);
    
  puts("Example driver ready");

  // Start the device thread; spawns a new thread and executes
  // ExampleDriver::Main(), which contains the main loop for the driver.
  StartThread();

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int ExampleDriver::Shutdown()
{
  puts("Shutting example driver down");

  // Stop and join the driver thread
  StopThread();

  // Here you would shut the device down by, for example, closing a
  // serial port.

  puts("Example driver has been shutdown");

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void ExampleDriver::Main() 
{
  // The main loop; interact with the device here
  for(;;)
  {
    // test if we are supposed to cancel
    pthread_testcancel();

    // Check for and handle configuration requests, using CDevice::GetConfig()

    // Check for and execute commands, using CDevice::GetCommand()

    // Interact with the device, and push out the resulting data, using
    // CDevice::PutData()

    // Sleep (you might, for example, block on a read() instead)
    usleep(100000);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Extra stuff for building a shared object.

// Need access to the global driver table
#include <drivertable.h>

/* need the extern to avoid C++ name-mangling  */
extern "C" {
  int player_driver_init(DriverTable* table)
  {
    puts("Example driver initializing");
    ExampleDriver_Register(table);
    puts("Example driver done");
    return(0);
  }
}

