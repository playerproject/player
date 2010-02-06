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
// Desc: Opaque commands sender
// Author: Paul Osmialowski
// Date: 02 Jan 2010
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_opaquecmd opaquecmd
 * @brief Opaque commands sender

The opaquecmd driver keeps on repeating configured opaque command.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_opaque

@par Requires

- @ref interface_opaque

@par Configuration requests

- none

@par Configuration file options

- sleep_sec (integer)
  - Default: 0
  - timespec value for nanosleep() (sec resolution)
- sleep_nsec (integer)
  - Default: 100000000 (however, when sleep_sec is greater than 0, default sleep_nsec is 0)
  - timespec value for nanosleep() (nanosec resolution)
- hexstring (string)
  - Default: NONE
  - Opaque command bytes given by two-digit hex numbers (up to 32 numbers)
- init_req (string)
  - Default: ""
  - If given, these bytes will be sent at startup as a request; two-digit hex numbers (up to 32 numbers)

@par Example

@verbatim
driver
(
  name "opaquecmd"
  provides ["opaque:100"]
  requires ["opaque:0"]
  hexstring "0000"
  init_req "0100034D205E206F20"
  sleep_sec 2
  alwayson 1
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL and size_t
#include <stdlib.h> // for malloc() and free()
#include <string.h> // for memset()
#include <pthread.h>
#include <libplayercore/playercore.h>

class OpaqueCmd : public ThreadedDriver
{
  public:
    // Constructor; need that
    OpaqueCmd(ConfigFile * cf, int section);

    virtual ~OpaqueCmd();

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
    player_devaddr_t required_opaque_addr;
    // Handle for the device that have the address given above
    Device * required_opaque_dev;

    int sleep_sec;
    int sleep_nsec;
    player_opaque_data_t cmd_data;
    player_opaque_data_t req_data;
    int init_req;
    static int hexfromstring(const char * s, player_opaque_data_t * data);
};

int OpaqueCmd::hexfromstring(const char * s, player_opaque_data_t * data)
{
  size_t len;
  const char * hex;
  char hexbuff[3];
  unsigned u;

  memset(data, 0, sizeof(player_opaque_data_t));
  len = 0;
  for (hex = s; *hex; hex += 2, len++)
  {
    if (!(hex[1]))
    {
      PLAYER_ERROR("hexstring too short");
      return -1;
    }
  }
  data->data_count = len;
  if (!(data->data_count))
  {
    data->data = NULL;
    return 0;
  }
  data->data = reinterpret_cast<uint8_t *>(malloc(data->data_count));
  if (!(data->data))
  {
    PLAYER_ERROR("out of memory");
    memset(data, 0, sizeof(player_opaque_data_t));
    return -1;
  }
  len = 0;
  for (hex = s; *hex; hex += 2, ++len)
  {
    if (!(hex[1]))
    {
      PLAYER_ERROR("serious internal error");
      free(data->data);
      memset(data, 0, sizeof(player_opaque_data_t));
      return -1;
    }
    hexbuff[0] = hex[0];
    hexbuff[1] = hex[1];
    hexbuff[2] = '\0';
    sscanf(hexbuff, "%x", &u);
    data->data[len] = u;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
//
OpaqueCmd::OpaqueCmd(ConfigFile * cf, int section) : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  int default_nsec;
  const char * str;

  memset(&(this->provided_opaque_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->required_opaque_addr), 0, sizeof(player_devaddr_t));
  this->required_opaque_dev = NULL;
  this->sleep_sec = 0;
  this->sleep_nsec = 0;
  memset(&(this->cmd_data), 0, sizeof this->cmd_data);
  memset(&(this->req_data), 0, sizeof this->req_data);
  this->cmd_data.data = NULL;
  this->req_data.data = NULL;
  this->init_req = 0;
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
  if (cf->ReadDeviceAddr(&(this->required_opaque_addr), section, "requires",
                         PLAYER_OPAQUE_CODE, -1, NULL))
  {
    PLAYER_ERROR("cannot require opaque device");
    this->SetError(-1);
    return;
  }
  this->sleep_sec = cf->ReadInt(section, "sleep_sec", 0);
  if ((this->sleep_sec) < 0)
  {
    PLAYER_ERROR("Invalid sleep_sec value");
    this->SetError(-1);
    return;
  }
  default_nsec = 100000000;
  if ((this->sleep_sec) > 0) default_nsec = 0;
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", default_nsec);
  if ((this->sleep_nsec) < 0)
  {
    PLAYER_ERROR("Invalid sleep_nsec value");
    this->SetError(-1);
    return;
  }
  str = cf->ReadString(section, "hexstring", "");
  if (!str)
  {
    PLAYER_ERROR("NULL hexstring");
    this->SetError(-1);
    return;
  }
  if (OpaqueCmd::hexfromstring(str, &(this->cmd_data)))
  {
    this->SetError(-1);
    return;
  }
  str = cf->ReadString(section, "init_req", NULL);
  if (str)
  {
    this->init_req = !0;
    if (OpaqueCmd::hexfromstring(str, &(this->req_data)))
    {
      this->SetError(-1);
      return;
    }
  } else PLAYER_WARN("As intented, initial request will not be sent");
}

OpaqueCmd::~OpaqueCmd()
{
  if (this->cmd_data.data) free(this->cmd_data.data);
  this->cmd_data.data = NULL;
  if (this->req_data.data) free(this->req_data.data);
  this->req_data.data = NULL;
}

int OpaqueCmd::MainSetup()
{
  this->required_opaque_dev = deviceTable->GetDevice(this->required_opaque_addr);
  if (!(this->required_opaque_dev))
  {
    PLAYER_ERROR("unable to locate suitable opaque device");
    return -1;
  }
  if (this->required_opaque_dev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to opaque device");
    this->required_opaque_dev = NULL;
    return -1;
  }
  return 0;
}

void OpaqueCmd::MainQuit()
{
  if (this->required_opaque_dev) this->required_opaque_dev->Unsubscribe(this->InQueue);
  this->required_opaque_dev = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void OpaqueCmd::Main()
{
  struct timespec tspec;
  player_opaque_data_t data;
  Message * msg = NULL;

  if (this->init_req)
  {
    msg = this->required_opaque_dev->Request(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_OPAQUE_REQ_DATA, reinterpret_cast<void *>(&(this->req_data)), 0, NULL, true); // threaded = true
    if (!msg) PLAYER_WARN("failed to send request on opaque interface");
    else delete msg;
    msg = NULL;
  }
  for (;;)
  {
    // Test if we are supposed to cancel
    pthread_testcancel();

    // Process incoming messages
    this->ProcessMessages();

    // Test if we are supposed to cancel
    pthread_testcancel();

    this->required_opaque_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, reinterpret_cast<void *>(&(this->cmd_data)), 0, NULL);

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

    if (((this->sleep_sec) > 0) || ((this->sleep_nsec) > 0))
    {
      // sleep for a while
      tspec.tv_sec = this->sleep_sec;
      tspec.tv_nsec = this->sleep_nsec;
      nanosleep(&tspec, NULL);
    }
  }
}

int OpaqueCmd::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  // Process messages here.  Send a response if necessary, using Publish().
  // If you handle the message successfully, return 0.  Otherwise,
  // return -1, and a NACK will be sent for you, if a response is required.

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  // handle opaque data
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            -1, // -1 means 'all message subtypes'
                            this->required_opaque_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL opaque data");
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
Driver * OpaqueCmd_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new OpaqueCmd(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void opaquecmd_Register(DriverTable * table)
{
  table->AddDriver("opaquecmd", OpaqueCmd_Init);
}
