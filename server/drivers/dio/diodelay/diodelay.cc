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
// Desc: Dio state delay
// Author: Paul Osmialowski
// Date: 17 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_diodelay diodelay
 * @brief Dio state delay

The diodelay defers dio bits state change.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_dio

@par Requires

- (optional) @ref interface_dio to which diodelay will be sending dio commands
  containing current state ('state' key)
- (optional) @ref interface_dio from which dio data will be read to track down
  ('bits' key)

Roles of subscribed interfaces are distinguished by given key (state or bits)

@par Configuration requests

- none

@par Configuration file options

- wait_on_0 (double)
  - Default: 0.0 (no effect)
  - Wait time in secs.
- wait_on_1 (double)
  - Default: 0.0 (no effect)
  - Wait time in secs.
- fade_out (double)
  - Default: 0.0 (no effect)
  - Fade out time in secs.
- init_state (string)
  - Default: "00000000000000000000000000000000"
  - Initial state (number of bits is significant)
  - Last character is the lowest bit (length greater than 0, max. 32 characters)
- sleep_nsec (integer)
  - Default: 10000000
  - timespec value for nanosleep()

@par Example

@verbatim
driver
(
  name "diodelay"
  provides ["dio:0"]
  init_state "000"
  wait_on_1 2.0
  wait_on_0 0.7
  fade_out 0.333
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL
#include <string.h> // for str* and mem* functions
#include <time.h> // for nanosleep()
#include <assert.h> // for assert()
#include <pthread.h> // for pthread_testcancel()
#include <libplayercore/playercore.h>

class DioDelay : public ThreadedDriver
{
  public:    
    // Constructor; need that
    DioDelay(ConfigFile * cf, int section);

    virtual ~DioDelay();

    // Must implement the following methods.
    virtual int MainSetup();
    virtual void MainQuit();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer & resp_queue, 
                               player_msghdr * hdr,
                               void * data);

  private:
    // Main function for device thread.
    virtual void Main();

    player_devaddr_t dio_provided_addr;
    player_devaddr_t dio_bits_required_addr;
    player_devaddr_t dio_state_required_addr;
    Device * dio_bits_required_dev;
    Device * dio_state_required_dev;
    int dio_bits_in_use;
    int dio_state_in_use;

    // some configuration settings:
    double wait_on_0;
    double wait_on_1;
    double fade_out;
    uint32_t init_state;
    uint32_t state_count;
    int wait_on;
    int sleep_nsec;
    uint32_t state;
    uint32_t bits;
    int waiting0[32];
    int waiting1[32];
    double start_time0[32];
    double start_time1[32];

    void process(uint32_t _bits, int count);
};

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
//
DioDelay::DioDelay(ConfigFile * cf, int section) : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  const char * _init_state;
  int i;

  memset(&(this->dio_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_bits_required_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_state_required_addr), 0, sizeof(player_devaddr_t));
  this->dio_bits_required_dev = NULL;
  this->dio_state_required_dev = NULL;
  this->dio_bits_in_use = 0;
  this->dio_state_in_use = 0;
  this->wait_on_0 = 0.0;
  this->wait_on_1 = 0.0;
  this->fade_out = 0.0;
  this->init_state = 0;
  this->state_count = 0;
  this->sleep_nsec = 0;
  this->state = 0;
  this->bits = 0;
  for (i = 0; i < 32; i++)
  {
    this->waiting0[i] = 0;
    this->waiting1[i] = 0;
    this->start_time0[i] = 0.0;
    this->start_time1[i] = 0.0;
  }
  if (cf->ReadDeviceAddr(&(this->dio_provided_addr), section, "provides",
                         PLAYER_DIO_CODE, -1, NULL))
  {
    PLAYER_ERROR("Nothing is provided");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->dio_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->GetTupleCount(section, "requires") > 0)
  {
    if (cf->ReadDeviceAddr(&(this->dio_bits_required_addr), section, "requires",
                           PLAYER_DIO_CODE, -1, "bits"))
    {
      PLAYER_WARN("dio bits not in use");
      this->dio_bits_in_use = 0;
    } else
    {
      PLAYER_WARN("dio bits in use");
      this->dio_bits_in_use = !0;
    }
    if (cf->ReadDeviceAddr(&(this->dio_state_required_addr), section, "requires",
                           PLAYER_DIO_CODE, -1, "state"))
    {
      PLAYER_WARN("dio state not in use");
      this->dio_state_in_use = 0;
    } else
    {
      PLAYER_WARN("dio state in use");
      this->dio_state_in_use = !0;
    }
  }
  this->wait_on_0 = cf->ReadFloat(section, "wait_on_0", 0.0);
  if (this->wait_on_0 < 0.0)
  {
    PLAYER_ERROR("Invalid wait_on_0 value");
    this->SetError(-1);
    return;
  }
  this->wait_on_1 = cf->ReadFloat(section, "wait_on_1", 0.0);
  if (this->wait_on_1 < 0.0)
  {
    PLAYER_ERROR("Invalid wait_on_1 value");
    this->SetError(-1);
    return;
  }
  this->fade_out = cf->ReadFloat(section, "fade_out", 0.0);
  if (this->fade_out < 0.0)
  {
    PLAYER_ERROR("Invalid fade_out value");
    this->SetError(-1);
    return;
  }
  _init_state = cf->ReadString(section, "init_state", "00000000000000000000000000000000");
  if (!_init_state)
  {
    this->SetError(-1);
    return;
  }
  this->state_count = strlen(_init_state);
  if ((!((this->state_count) > 0)) || ((this->state_count) > 32))
  {
    PLAYER_ERROR("Invalid init_state string");
    this->SetError(-1);
    return;
  }
  this->init_state = 0;
  for (i = 0; _init_state[i]; i++)
  {
    if (i) ((this->init_state) <<= 1);
    switch(_init_state[i])
    {
    case '0':
      break;
    case '1':
      ((this->init_state) |= 1);
      break;
    default:
      PLAYER_ERROR("invalid init_state string");
      this->SetError(-1);
      return;
    }
  }
  this->state = this->init_state;
  this->bits = this->init_state;
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 10000000);
  if ((this->sleep_nsec) <= 0)
  {
    PLAYER_ERROR("Invalid sleep_nsec value");
    this->SetError(-1);
    return;
  }
}

DioDelay::~DioDelay() { }

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int DioDelay::MainSetup()
{
  if (this->dio_bits_in_use)
  {
    if (Device::MatchDeviceAddress(this->dio_bits_required_addr, this->dio_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self (bits)");
      return -1;
    }
    this->dio_bits_required_dev = deviceTable->GetDevice(this->dio_bits_required_addr);
    if (!(this->dio_bits_required_dev))
    {
      PLAYER_ERROR("unable to locate suitable bits:::dio device");
      return -1;
    }
    if (this->dio_bits_required_dev->Subscribe(this->InQueue))
    {
      PLAYER_ERROR("unable to subscribe to bits:::dio device");
      this->dio_bits_required_dev = NULL;
      return -1;
    }
  }
  if (this->dio_state_in_use)
  {
    if (Device::MatchDeviceAddress(this->dio_state_required_addr, this->dio_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self (state)");
      if (this->dio_bits_required_dev) this->dio_bits_required_dev->Unsubscribe(this->InQueue);
      this->dio_bits_required_dev = NULL;
      return -1;
    }
    this->dio_state_required_dev = deviceTable->GetDevice(this->dio_state_required_addr);
    if (!(this->dio_state_required_dev))
    {
      PLAYER_ERROR("unable to locate suitable state:::dio device");
      if (this->dio_bits_required_dev) this->dio_bits_required_dev->Unsubscribe(this->InQueue);
      this->dio_bits_required_dev = NULL;
      return -1;
    }
    if (this->dio_state_required_dev->Subscribe(this->InQueue))
    {
      PLAYER_ERROR("unable to subscribe to state:::dio device");
      this->dio_state_required_dev = NULL;
      if (this->dio_bits_required_dev) this->dio_bits_required_dev->Unsubscribe(this->InQueue);
      this->dio_bits_required_dev = NULL;
      return -1;
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void DioDelay::MainQuit()
{
  if (this->dio_bits_required_dev) this->dio_bits_required_dev->Unsubscribe(this->InQueue);
  this->dio_bits_required_dev = NULL;
  if (this->dio_state_required_dev) this->dio_state_required_dev->Unsubscribe(this->InQueue);
  this->dio_state_required_dev = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void DioDelay::Main() 
{
  player_dio_data_t dio_data;
  player_dio_cmd_t dio_cmd;
  struct timespec tspec;
  double d;
  int i;
  uint32_t u;

  this->state = this->init_state;
  this->bits = this->init_state;
  for (i = 0; i < 32; i++)
  {
    this->waiting0[i] = 0;
    this->waiting1[i] = 0;
    this->start_time0[i] = 0.0;
    this->start_time1[i] = 0.0;
  }
  for (;;)
  {
    // Test if we are supposed to cancel
    pthread_testcancel();

    // Process incoming messages
    this->ProcessMessages();

    // Test if we are supposed to cancel
    pthread_testcancel();

    GlobalTime->GetTimeDouble(&d);
    u = 1;
    for (i = 0; i < static_cast<int>(this->state_count); i++) 
    {
      if (this->waiting0[i])
      {
        if ((this->waiting1[i]) && ((this->start_time1[i]) > (this->start_time0[i])) && ((d - (this->start_time1[i])) >= (this->fade_out)))
        {
          this->waiting0[i] = 0;
          this->start_time0[i] = 0.0;
        } else if ((d - (this->start_time0[i])) >= (this->wait_on_0))
        {
          this->waiting0[i] = 0;
          this->start_time0[i] = 0.0;
          if ((!((this->bits) & u)) && ((this->state) & u))
          {
            (this->state) &= (~u);
            this->waiting1[i] = 0;
            this->start_time1[i] = 0.0;
          }
        }
      }
      if (this->waiting1[i])
      {
        if ((this->waiting0[i]) && ((this->start_time0[i]) > (this->start_time1[i])) && ((d - (this->start_time0[i])) >= (this->fade_out)))
        {
          this->waiting1[i] = 0;
          this->start_time1[i] = 0.0;
        } else if ((d - (this->start_time1[i])) >= (this->wait_on_1))
        {
          this->waiting1[i] = 0;
          this->start_time1[i] = 0.0;
          if (((this->bits) & u) && (!((this->state) & u)))
          {
            (this->state) |= u;
            this->waiting0[i] = 0;
            this->start_time0[i] = 0.0;
          }
        }
      }
      u <<= 1;
    }

    // Test if we are supposed to cancel
    pthread_testcancel();

    memset(&(dio_data), 0, sizeof dio_data);
    dio_data.count = this->state_count;
    dio_data.bits = this->state;
    this->Publish(this->dio_provided_addr,
                  PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                  reinterpret_cast<void *>(&dio_data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
    if (this->dio_state_in_use)
    {
      memset(&(dio_cmd), 0, sizeof dio_cmd);
      dio_cmd.count = this->state_count;
      dio_cmd.digout = this->state;
      assert(this->dio_state_required_dev);
      this->dio_state_required_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, reinterpret_cast<void *>(&dio_cmd), 0, NULL);
    }

    // Test if we are supposed to cancel
    pthread_testcancel();

    // sleep for a while
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec;
    nanosleep(&tspec, NULL);
  }
}

void DioDelay::process(uint32_t _bits, int count)
{
  uint32_t u;
  int i;

  u = 1;
  for (i = 0; i < count; i++)
  {
    this->bits = (((this->bits) & (~u)) | (_bits & u));
    if (_bits & u)
    {
      if (!(waiting1[i]))
      {
        this->waiting1[i] = !0;
        GlobalTime->GetTimeDouble(&(this->start_time1[i]));
      }
    } else
    {
      if (!(waiting0[i]))
      {
        this->waiting0[i] = !0;
        GlobalTime->GetTimeDouble(&(this->start_time0[i]));
      }
    }
    u <<= 1;
  }
}

int DioDelay::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_dio_cmd_t * dio_cmd;
  player_dio_data_t * dio_data;
  int count;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, this->dio_provided_addr))
  {
    assert(data);
    dio_cmd = reinterpret_cast<player_dio_cmd_t *>(data);
    assert(dio_cmd);
    count = static_cast<int>(dio_cmd->count);
    if (!(count > 0)) return 0;
    if (count > static_cast<int>(this->state_count)) count = static_cast<int>(this->state_count);
    this->process(dio_cmd->digout, count);
    return 0;
  }
  if (this->dio_bits_in_use)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES, this->dio_bits_required_addr))
    {
      assert(data);
      dio_data = reinterpret_cast<player_dio_data_t *>(data);
      assert(dio_data);
      count = static_cast<int>(dio_data->count);
      if (!(count > 0)) return 0;
      if (count > static_cast<int>(this->state_count)) count = static_cast<int>(this->state_count);
      this->process(dio_data->bits, count);
      return 0;
    }
  }
  if (this->dio_state_in_use)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES, this->dio_state_required_addr))
    {
      assert(data);
      return 0;
    }
  }
  return -1; // The message does not seem to fit to our needs
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * DioDelay_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new DioDelay(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void diodelay_Register(DriverTable * table)
{
  table->AddDriver("diodelay", DioDelay_Init);
}
