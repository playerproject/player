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
// Desc: Dio commands sender
// Author: Paul Osmialowski
// Date: 17 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_diocmd diocmd
 * @brief Dio commands sender

The diocmd driver keeps on repeating configured dio interface command.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_dio

@par Requires

- (optional) @ref interface_dio (if configured, commands will be sent there)

@par Configuration requests

- none

@par Configuration file options

- bits (string)
  - Default: "00000000000000000000000000000000"
  - Initial state (number of bits is significant)
  - Last character is the lowest bit (length greater than 0, max. 32 characters)
- read_only (integer)
  - Default: 1
  - If set to 1, received commands will not change the state
- sleep_nsec (integer)
  - Default: 100000000 (10 sends per second)
  - timespec value for nanosleep()

@par Example

@verbatim
driver
(
  name "diocmd"
  provides ["dio:100"]
  requires ["dio:0"]
  bits "010"
  alwayson 1
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL and size_t
#include <string.h> // for memset()
#include <pthread.h>
#include <libplayercore/playercore.h>

class DioCmd : public ThreadedDriver
{
  public:
    // Constructor; need that
    DioCmd(ConfigFile * cf, int section);

    virtual ~DioCmd();

    // Must implement the following methods.
    virtual int MainSetup();
    virtual void MainQuit();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);

  private:
    virtual void Main();
    player_devaddr_t provided_dio_addr;
    player_devaddr_t required_dio_addr;
    Device * required_dio_dev;
    int use_dio;
    uint32_t bits;
    uint32_t bits_count;
    int read_only;
    int sleep_nsec;
    uint32_t state;
    uint32_t state_count;
};

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
//
DioCmd::DioCmd(ConfigFile * cf, int section) : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  const char * _bits;
  int i;

  memset(&(this->provided_dio_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->required_dio_addr), 0, sizeof(player_devaddr_t));
  this->required_dio_dev = NULL;
  this->use_dio = 0;
  this->bits = 0;
  this->bits_count = 0;
  this->read_only = 0;
  this->sleep_nsec = 0;
  this->state = 0;
  this->state_count = 0;
  if (cf->ReadDeviceAddr(&(this->provided_dio_addr), section, "provides",
                         PLAYER_DIO_CODE, -1, NULL))
  {
    PLAYER_ERROR("Nothing is provided");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->provided_dio_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->GetTupleCount(section, "requires") > 0)
  {
    if (cf->ReadDeviceAddr(&(this->required_dio_addr), section, "requires",
                           PLAYER_DIO_CODE, -1, NULL))
    {
      PLAYER_WARN("dio device will not be subscribed");
      this->use_dio = 0;
    } else
    {
      PLAYER_WARN("dio device will be subscribed");
      this->use_dio = !0;
    }
  }
  _bits = cf->ReadString(section, "bits", "00000000000000000000000000000000");
  if (!_bits)
  {
    this->SetError(-1);
    return;
  }
  this->bits_count = strlen(_bits);
  if ((!((this->bits_count) > 0)) || ((this->bits_count) > 32))
  {
    PLAYER_ERROR("Invalid bits string");
    this->SetError(-1);
    return;
  }
  this->bits = 0;
  for (i = 0; _bits[i]; i++)
  {
    if (i) ((this->bits) <<= 1);
    switch(_bits[i])
    {
    case '0':
      break;
    case '1':
      ((this->bits) |= 1);
      break;
    default:
      PLAYER_ERROR("invalid bits string");
      this->SetError(-1);
      return;
    }
  }
  this->state = this->bits;
  this->state_count = this->bits_count;
  this->read_only = cf->ReadInt(section, "read_only", 1);
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 100000000);
  if ((this->sleep_nsec) <= 0)
  {
    PLAYER_ERROR("Invalid sleep_nsec value");
    this->SetError(-1);
    return;
  }
}

DioCmd::~DioCmd() { }

int DioCmd::MainSetup()
{
  if (this->use_dio)
  {
    if (Device::MatchDeviceAddress(this->required_dio_addr, this->provided_dio_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self");
      return -1;
    }
    this->required_dio_dev = deviceTable->GetDevice(this->required_dio_addr);
    if (!(this->required_dio_dev))
    {
      PLAYER_ERROR("unable to locate suitable dio device");
      return -1;
    }
    if (this->required_dio_dev->Subscribe(this->InQueue))
    {
      PLAYER_ERROR("unable to subscribe to dio device");
      this->required_dio_dev = NULL;
      return -1;
    }
  }
  return 0;
}

void DioCmd::MainQuit()
{
  if (this->required_dio_dev) this->required_dio_dev->Unsubscribe(this->InQueue);
  this->required_dio_dev = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void DioCmd::Main() 
{
  struct timespec tspec;
  player_dio_cmd_t dio_cmd;
  player_dio_data_t dio_data;

  this->state = this->bits;
  this->state_count = this->bits_count;
  for (;;)
  {
    // Test if we are supposed to cancel
    pthread_testcancel();

    // Process incoming messages
    this->ProcessMessages();

    // Test if we are supposed to cancel
    pthread_testcancel();

    if (this->use_dio)
    {
      memset(&dio_cmd, 0, sizeof dio_cmd);
      dio_cmd.count = this->state_count;
      dio_cmd.digout = this->state;
      this->required_dio_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, reinterpret_cast<void *>(&dio_cmd), 0, NULL);
    }

    // Test if we are supposed to cancel
    pthread_testcancel();

    memset(&dio_data, 0, sizeof dio_data);
    dio_data.count = this->state_count;
    dio_data.bits = this->state;
    this->Publish(this->provided_dio_addr,
                  PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                  reinterpret_cast<void *>(&dio_data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!

    // Test if we are supposed to cancel
    pthread_testcancel();

    // sleep for a while
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec;
    nanosleep(&tspec, NULL);
  }
}

int DioCmd::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_dio_cmd_t * dio_cmd;

  // Process messages here.  Send a response if necessary, using Publish().
  // If you handle the message successfully, return 0.  Otherwise,
  // return -1, and a NACK will be sent for you, if a response is required.

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  if (this->use_dio)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                              -1, // -1 means 'all message subtypes'
                              this->required_dio_addr))
    {
      if (!data)
      {
        PLAYER_ERROR("NULL dio data");
        return -1;
      }
      return 0;
    }
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
                            PLAYER_DIO_CMD_VALUES,
                            this->provided_dio_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL dio command");
      return -1;
    }
    if (this->read_only) return 0;
    dio_cmd = reinterpret_cast<player_dio_cmd_t *>(data);
    if (!dio_cmd) return -1;
    this->state = dio_cmd->digout;
    this->state_count = dio_cmd->count;
    return 0;
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * DioCmd_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new DioCmd(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void diocmd_Register(DriverTable * table)
{
  table->AddDriver("diocmd", DioCmd_Init);
}
