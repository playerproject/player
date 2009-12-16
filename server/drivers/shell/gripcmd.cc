/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al.
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
/////////////////////////////////////////////////////////////////////////////
//
// Desc: Gripper commands sender
// Author: Paul Osmialowski
// Date: 17 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_gripcmd gripcmd
 * @brief Gripper commands sender

The gripcmd driver keeps on repeating configured gripper command.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_opaque

@par Requires

- @ref interface_gripper

@par Configuration requests

- none

@par Configuration file options

- cmd (string)
  - Default: "close"
  - One of: "open", "close", "stop", "store", "retrieve"
- sleep_nsec (integer)
  - Default: 100000000 (10 sends per second)
  - timespec value for nanosleep()

@par Example

@verbatim
driver
(
  name "gripcmd"
  provides ["opaque:0"]
  requires ["gripper:0"]
  cmd "open"
  alwayson 1
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL and size_t
#include <string.h> // for memset()
#if !defined (WIN32)
#include <strings.h> // for strcasecmp()
#endif
#include <pthread.h>
#include <libplayercore/playercore.h>

class GripCmd : public ThreadedDriver
{
  public:
    // Constructor; need that
    GripCmd(ConfigFile * cf, int section);

    virtual ~GripCmd();

    // Must implement the following methods.
    virtual int MainSetup();
    virtual void MainQuit();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);

  private:
    virtual void Main();
    player_devaddr_t provided_opaque_addr;
    player_devaddr_t required_gripper_addr;
    Device * required_gripper_dev;
    uint8_t cmd;
    int sleep_nsec;
};

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
//
GripCmd::GripCmd(ConfigFile * cf, int section) : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  const char * _cmd;

  memset(&(this->provided_opaque_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->required_gripper_addr), 0, sizeof(player_devaddr_t));
  this->required_gripper_dev = NULL;
  this->cmd = 0;
  this->sleep_nsec = 0;
  if (cf->ReadDeviceAddr(&(this->provided_opaque_addr), section, "provides",
                         PLAYER_OPAQUE_CODE, -1, NULL))
  {
    PLAYER_ERROR("Nothing is provided");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->provided_opaque_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->required_gripper_addr), section, "requires",
                         PLAYER_GRIPPER_CODE, -1, NULL))
  {
    PLAYER_ERROR("Cannot require gripper device");
    this->SetError(-1);
    return;
  }
  _cmd = cf->ReadString(section, "cmd", "close");
  if (!_cmd)
  {
    this->SetError(-1);
    return;
  }
  this->cmd = 0;
#if defined (WIN32)
  if (!(_strnicmp(_cmd, "open", strlen(_cmd)))) this->cmd = PLAYER_GRIPPER_CMD_OPEN;
  else if (!(_strnicmp(_cmd, "close", strlen(_cmd)))) this->cmd = PLAYER_GRIPPER_CMD_CLOSE;
  else if (!(_strnicmp(_cmd, "stop", strlen(_cmd)))) this->cmd = PLAYER_GRIPPER_CMD_STOP;
  else if (!(_strnicmp(_cmd, "store", strlen(_cmd)))) this->cmd = PLAYER_GRIPPER_CMD_STORE;
  else if (!(_strnicmp(_cmd, "retrieve", strlen(_cmd)))) this->cmd = PLAYER_GRIPPER_CMD_RETRIEVE;
#else
  if (!(strcasecmp(_cmd, "open"))) this->cmd = PLAYER_GRIPPER_CMD_OPEN;
  else if (!(strcasecmp(_cmd, "close"))) this->cmd = PLAYER_GRIPPER_CMD_CLOSE;
  else if (!(strcasecmp(_cmd, "stop"))) this->cmd = PLAYER_GRIPPER_CMD_STOP;
  else if (!(strcasecmp(_cmd, "store"))) this->cmd = PLAYER_GRIPPER_CMD_STORE;
  else if (!(strcasecmp(_cmd, "retrieve"))) this->cmd = PLAYER_GRIPPER_CMD_RETRIEVE;
#endif
  else
  {
    PLAYER_ERROR("Invalid command");
    this->SetError(-1);
    return;
  }
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 100000000);
  if ((this->sleep_nsec) <= 0)
  {
    PLAYER_ERROR("Invalid sleep_nsec value");
    this->SetError(-1);
    return;
  }
}

GripCmd::~GripCmd() { }

int GripCmd::MainSetup()
{
  this->required_gripper_dev = deviceTable->GetDevice(this->required_gripper_addr);
  if (!(this->required_gripper_dev))
  {
    PLAYER_ERROR("unable to locate suitable gripper device");
    return -1;
  }
  if (this->required_gripper_dev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to gripper device");
    this->required_gripper_dev = NULL;
    return -1;
  }
  return 0;
}

void GripCmd::MainQuit()
{
  if (this->required_gripper_dev) this->required_gripper_dev->Unsubscribe(this->InQueue);
  this->required_gripper_dev = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void GripCmd::Main() 
{
  struct timespec tspec;
  player_opaque_data_t data;

  for (;;)
  {
    // Test if we are supposed to cancel
    pthread_testcancel();

    // Process incoming messages
    this->ProcessMessages();

    // Test if we are supposed to cancel
    pthread_testcancel();

    this->required_gripper_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, this->cmd, NULL, 0, NULL);

    // Test if we are supposed to cancel
    pthread_testcancel();

    memset(&data, 0, sizeof data);
    data.data_count = 0;
    data.data = NULL;
    this->Publish(this->provided_opaque_addr,
                  PLAYER_MSGTYPE_DATA, PLAYER_OPAQUE_DATA_STATE,
                  reinterpret_cast<void *>(&data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
    // Test if we are supposed to cancel
    pthread_testcancel();

    // sleep for a while
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec;
    nanosleep(&tspec, NULL);
  }
}

int GripCmd::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  // Process messages here.  Send a response if necessary, using Publish().
  // If you handle the message successfully, return 0.  Otherwise,
  // return -1, and a NACK will be sent for you, if a response is required.

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            -1, // -1 means 'all message subtypes'
                            this->required_gripper_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL gripper data");
      return -1;
    }
    return 0;
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * GripCmd_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new GripCmd(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void gripcmd_Register(DriverTable * table)
{
  table->AddDriver("gripcmd", GripCmd_Init);
}
