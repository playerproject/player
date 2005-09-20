/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al.
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

/** Mixed mode driver for Garcia robot by Acroname
  * @author Brad Kratochvil
  * @date 20050915
  * @addtogroup drivers Drivers
  * @{ */

/**
  *@defgroup player_driver_sphere garcia


The garcia driver captures

@par Compile-time dependencies

- &lt;acpGarcia.h&gt;

@par Provides

- @ref player_interface_position2d
- @ref player_interface_ir
- @ref player_interface_speech
- @ref player_interface_dio

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- port (string)
  - Default: "/dev/ttyS0"
  - Serial port to communicate with the device

@par Example

@verbatim
driver
(
  name "garciadriver"
  provides ["position2d:0"
            "ir:0"
            "dio:0"
            "speech:0"]
  port "/dev/ttyS0"
)
@endverbatim

@par Authors

Brad Kratochvil

*/
/** @} */

// ONLY if you need something that was #define'd as a result of configure
// (e.g., HAVE_CFMAKERAW), then #include <config.h>, like so:
/*
#if HAVE_CONFIG_H
  #include <config.h>
#endif
*/
#include "garcia_mixed.h"

#define DEG2RAD(x) (((double)(x))*0.01745329251994)
#define RAD2DEG(x) (((double)(x))*57.29577951308232)

#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <math.h>
using namespace std;

const timespec NSLEEP_TIME = {0, 10000000}; // (0s, 10 ms) => max 100 fps

////////////////////////////////////////////////////////////////////////////////
// Now the driver

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver*
GarciaDriver_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return ((Driver*)(new GarciaDriver(cf, section)));

}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void
GarciaDriver_Register(DriverTable* table)
{
  table->AddDriver("garcia", GarciaDriver_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
GarciaDriver::GarciaDriver(ConfigFile* cf, int section)
    : Driver(cf, section)
{
  // Create position2d interface
  if (0 != cf->ReadDeviceAddr(&(mPos2dAddr),section,"provides",
                              PLAYER_POSITION2D_CODE,-1,NULL))
  {
    PLAYER_ERROR("Could not read position2d ID ");
    SetError(-1);
    return;
  }
  if (0 != AddInterface(mPos2dAddr))
  {
    PLAYER_ERROR("Could not add position2d interface ");
    SetError(-1);
    return;
  }

  // Create ir interface
  if (0 != cf->ReadDeviceAddr(&(mIrAddr),section,"provides",
                              PLAYER_IR_CODE,-1,NULL))
  {
    PLAYER_ERROR("Could not read ir ID ");
    SetError(-1);
    return;
  }
  if (0 != AddInterface(mIrAddr))
  {
    PLAYER_ERROR("Could not add ir interface ");
    SetError(-1);
    return;
  }

  // Create speech interface
  if (0 != cf->ReadDeviceAddr(&(mSpeechAddr),section,"provides",
                              PLAYER_SPEECH_CODE,-1,NULL))
  {
    PLAYER_ERROR("Could not read speech ID ");
    SetError(-1);
    return;
  }
  if (0 != AddInterface(mSpeechAddr))
  {
    PLAYER_ERROR("Could not add speech interface ");
    SetError(-1);
    return;
  }

  // Create dio interface
  if (0 != cf->ReadDeviceAddr(&(mDioAddr),section,"provides",
                              PLAYER_Dio_CODE,-1,NULL))
  {
    PLAYER_ERROR("Could not read dio ID ");
    SetError(-1);
    return;
  }
  if (0 != AddInterface(mDioAddr))
  {
    PLAYER_ERROR("Could not add dio interface ");
    SetError(-1);
    return;
  }

  /// @todo is there a replacement clear command?
  //ClearCommand(mPosition2dAddr);

  // Read options from the configuration file
  mConfigPath = cf->ReadString(section, "config_path", "garcia.config");

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int
GarciaDriver::Setup()
{

  // Start the device thread; spawns a new thread and executes
  // GarciaDriver::Main(), which contains the main loop for the driver.
  StartThread();

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int
GarciaDriver::Shutdown()
{
  puts("Shutting Garcia driver down");

  // Stop and join the driver thread
  StopThread();

  // Here you would shut the device down by, for example, closing a
  // serial port.

  delete mGarcia;

  puts("Garcia driver has been shutdown");

  return(0);
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void
GarciaDriver::Main()
{
  puts("Setting up Garcia driver");
  mGarcia = new acpRobot("garcia", mConfigPath);

  puts("waiting for garcia");
  while (!mGarcia.isActive())
  {
    puts("still waiting");
    nanosleep(&NSLEEP_TIME, NULL);
  }
  puts("Garcia driver ready");


  // The main loop; interact with the device here
  for(;;)
  {
    // test if we are supposed to cancel
    pthread_testcancel();

    // Go to sleep for a while (this is a polling loop)
    nanosleep(&NSLEEP_TIME, NULL);

    // Process incoming messages
    ProcessMessages();

    // Write outgoing data
    RefreshData();

  }
  return;
}

// Process an incoming message
int
GarciaDriver::ProcessMessage(MessageQueue* resp_queue,
                             player_msghdr* hdr,
                             void* data)
{
  assert(resp_queue);
  assert(hdr);
  assert(data);

  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                           PLAYER_PTZ_CMD_STATE, mPos2dAddr))
  {
    assert(hdr->size == sizeof(player_position2d_cmd_t));
    ProcessPos2dCommand(hdr, *reinterpret_cast<player_position2d_cmd_t *>(data));
    return(0);
  }
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                                PLAYER_SPEECH_CMD_STATE, mSpeechAddr))
  {
    assert(hdr->size == sizeof(player_speech_cmd_t));
    ProcessSpeechCommand(hdr, *reinterpret_cast<player_speech_cmd_t *>(data));
    return(0);
  }
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                                PLAYER_DIO_CMD_STATE, mDioAddr))
  {
    assert(hdr->size == sizeof(player_dio_cmd_t));
    ProcessDioCommand(hdr, *reinterpret_cast<player_dio_cmd_t *>(data));
    return(0);
  }
  else
  {
    PLAYER_ERROR1("GarciaDriver received unknown message: %s", hdr->type);
  }

  return -1;
}

void
GarciaDriver::ProcessPos2dCommand(player_msghdr_t* hdr,
                                  player_position2d_cmd_t &data)
{

}

void
GarciaDriver::ProcessSpeechCommand(player_msghdr_t* hdr,
                                   player_speech_cmd_t &data)
{

}

void
GarciaDriver::ProcessDioCommand(player_msghdr_t* hdr,
                                player_dio_cmd_t &data)
{

}

void
GarciaDriver::RefreshData()
{
  Publish(mPos2dAddr, NULL,
          PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
          reinterpret_cast<void*>(&mPos2dData), size, NULL);

  Publish(mIrAddr, NULL,
          PLAYER_MSGTYPE_DATA, PLAYER_IR_DATA_STATE,
          reinterpret_cast<void*>(&mIrData), sizeof(mIrData), NULL);

  Publish(mSpeechAddr, NULL,
          PLAYER_MSGTYPE_DATA, PLAYER_SPEECH_DATA_STATE,
          reinterpret_cast<void*>(&mSpeechData), sizeof(mSpeechData), NULL);

  Publish(mDioAddr, NULL,
          PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_STATE,
          reinterpret_cast<void*>(&mDioData), sizeof(mDioData), NULL);

}
