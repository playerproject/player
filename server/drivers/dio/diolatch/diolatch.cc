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
// Desc: Dio bits latch
// Author: Paul Osmialowski
// Date: 17 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_diolatch diolatch
 * @brief Dio bits latch

This device latches configured state of bits (zeros or ones, according to given
pattern)

@par Compile-time dependencies

- none

@par Provides

- @ref interface_dio (with key 'set')
- @ref interface_dio (with key 'reset')

Roles of provided interfaces are distinguished by given key (set or reset)

@par Requires

- (optional) @ref interface_dio (with key 'set')
- (optional) @ref interface_dio (with key 'reset')

@par Configuration requests

- none

@par Configuration file options

- pattern (string)
  - Default: "11111111111111111111111111111111"
  - What states of bits to catch (number of bits is significant)
  - Last character is the lowest bit (length greater than 0, max. 32 characters)
- send_commands (integer) (only when dio interface with 'set' key is subscribed)
  - Default: 0
  - If set to 1, send commands instead of reading data on subscribed
    dio interface with 'set' key
- neg (integer)
  - Default: 0
  - If set to 1, negate reset before use

@par Example

@verbatim
driver
(
  name "diolatch"
  provides ["set:::dio:0" "reset:::dio:1"]
  pattern "010"
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL and size_t
#include <stdlib.h> // for malloc() and free()
#include <string.h> // for memset()
#include <assert.h>
#include <libplayercore/playercore.h>

class DioLatch : public Driver
{
  public: DioLatch(ConfigFile * cf, int section);
  public: virtual ~DioLatch();
  public: virtual int Setup();
  public: virtual int Shutdown();
  public: virtual int Subscribe(player_devaddr_t addr);
  public: virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data);
  private: player_devaddr_t dio_set_provided_addr;
  private: player_devaddr_t dio_reset_provided_addr;
  private: player_devaddr_t dio_set_required_addr;
  private: player_devaddr_t dio_reset_required_addr;
  private: Device * dio_set_required_dev;
  private: Device * dio_reset_required_dev;
  private: int use_set;
  private: int use_reset;
  private: uint32_t pattern;
  private: uint32_t pattern_count;
  private: int send_commands;
  private: int neg;
  private: uint32_t latch;
  private: uint32_t reset;

  private: void process_set(uint32_t _bits, int count);
  private: void process_reset(uint32_t _bits, int count);
  private: void publish_set();
  private: void publish_reset();
};

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
//
DioLatch::DioLatch(ConfigFile * cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  const char * _pattern;
  int i;
  uint32_t u;

  memset(&(this->dio_set_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_reset_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_set_required_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_reset_required_addr), 0, sizeof(player_devaddr_t));
  this->dio_set_required_dev = NULL;
  this->dio_reset_required_dev = NULL;
  this->use_set = 0;
  this->use_reset = 0;
  this->pattern = 0;
  this->pattern_count = 0;
  this->send_commands = 0;
  this->neg = 0;
  this->latch = 0;
  this->reset = 0;
  if (cf->ReadDeviceAddr(&(this->dio_set_provided_addr), section, "provides", PLAYER_DIO_CODE, -1, "set"))
  {
    PLAYER_ERROR("cannot provide dio set interface");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->dio_set_provided_addr))
  {
    PLAYER_ERROR("AddInterface failed for dio set interface");
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->dio_reset_provided_addr), section, "provides", PLAYER_DIO_CODE, -1, "reset"))
  {
    PLAYER_ERROR("cannot provide dio reset interface");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->dio_reset_provided_addr))
  {
    PLAYER_ERROR("AddInterface failed for dio reset interface");
    this->SetError(-1);
    return;
  }
  if (cf->GetTupleCount(section, "requires") > 0)
  {
    if (cf->ReadDeviceAddr(&(this->dio_set_required_addr), section, "requires", PLAYER_DIO_CODE, -1, "set"))
    {
      PLAYER_WARN("dio set device not required");
      this->use_set = 0;
    } else
    {
      PLAYER_WARN("dio set device will be subscribed");
      this->use_set = !0;
    }
    if (cf->ReadDeviceAddr(&(this->dio_reset_required_addr), section, "requires", PLAYER_DIO_CODE, -1, "reset"))
    {
      PLAYER_WARN("dio reset device not required");
      this->use_reset = 0;
    } else
    {
      PLAYER_WARN("dio reset device will be subscribed");
      this->use_reset = !0;
    }
  }
  _pattern = cf->ReadString(section, "pattern", "11111111111111111111111111111111");
  if (!_pattern)
  {
    this->SetError(-1);
    return;
  }
  this->pattern_count = strlen(_pattern);
  this->pattern = 0;
  for (i = 0; _pattern[i]; i++)
  {
    if (i) ((this->pattern) <<= 1);
    switch(_pattern[i])
    {
    case '0':
      break;
    case '1':
      ((this->pattern) |= 1);
      break;
    default:
      PLAYER_ERROR("invalid pattern string");
      this->SetError(-1);
      return;
    }
  }
  this->latch = 0;
  u = 1;
  for (i = 0; i < static_cast<int>(this->pattern_count); i++)
  {
    (this->latch) |= ((~(this->pattern)) & u);
    u <<= 1;
  }
  this->send_commands = cf->ReadInt(section, "send_commands", 0);
  this->neg = cf->ReadInt(section, "neg", 0);
}

DioLatch::~DioLatch() { }

int DioLatch::Setup()
{
  int i;
  uint32_t u;

  this->latch = 0;
  u = 1;
  for (i = 0; i < static_cast<int>(this->pattern_count); i++)
  {
    (this->latch) |= ((~(this->pattern)) & u);
    u <<= 1;
  }
  this->reset = 0;
  if (this->use_set)
  {
    if (Device::MatchDeviceAddress(this->dio_set_required_addr, this->dio_set_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self (set->set)");
      return -1;
    }
    if (Device::MatchDeviceAddress(this->dio_set_required_addr, this->dio_reset_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self (set->reset)");
      return -1;
    }
    this->dio_set_required_dev = deviceTable->GetDevice(this->dio_set_required_addr);
    if (!(this->dio_set_required_dev))
    {
      PLAYER_ERROR("unable to locate suitable set:::dio device");
      return -1;
    }
    if (this->dio_set_required_dev->Subscribe(this->InQueue))
    {
      PLAYER_ERROR("unable to subscribe to set:::dio device");
      this->dio_set_required_dev = NULL;
      return -1;
    }
  }
  if (this->use_reset)
  {
    if (Device::MatchDeviceAddress(this->dio_reset_required_addr, this->dio_set_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self (reset->set)");
      if (this->dio_set_required_dev) this->dio_set_required_dev->Unsubscribe(this->InQueue);
      this->dio_set_required_dev = NULL;
      return -1;
    }
    if (Device::MatchDeviceAddress(this->dio_reset_required_addr, this->dio_reset_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self (reset->reset)");
      if (this->dio_set_required_dev) this->dio_set_required_dev->Unsubscribe(this->InQueue);
      this->dio_set_required_dev = NULL;
      return -1;
    }
    this->dio_reset_required_dev = deviceTable->GetDevice(this->dio_reset_required_addr);
    if (!(this->dio_reset_required_dev))
    {
      PLAYER_ERROR("unable to locate suitable reset:::dio device");
      if (this->dio_set_required_dev) this->dio_set_required_dev->Unsubscribe(this->InQueue);
      this->dio_set_required_dev = NULL;
      return -1;
    }
    if (this->dio_reset_required_dev->Subscribe(this->InQueue))
    {
      PLAYER_ERROR("unable to subscribe to reset:::dio device");
      this->dio_reset_required_dev = NULL;
      if (this->dio_set_required_dev) this->dio_set_required_dev->Unsubscribe(this->InQueue);
      this->dio_set_required_dev = NULL;
      return -1;
    }
  }
  return 0;
}

int DioLatch::Shutdown()
{
  if (this->dio_set_required_dev) this->dio_set_required_dev->Unsubscribe(this->InQueue);
  this->dio_set_required_dev = NULL;
  if (this->dio_reset_required_dev) this->dio_reset_required_dev->Unsubscribe(this->InQueue);
  this->dio_reset_required_dev = NULL;
  return 0;
}

void DioLatch::process_set(uint32_t _bits, int count)
{
  uint32_t u;
  int i;

  u = 1;
  for (i = 0; i < count; i++)
  {
    if ((this->pattern) & u)
    {
      if (_bits & u)
      {
        ((this->latch) |= u);
        (this->reset) &= (~u);
      }
    } else
    {
      if (!(_bits & u))
      {
        ((this->latch) &= (~u));
        (this->reset) &= (~u);
      }
    }
    u <<= 1;
  }
}

void DioLatch::process_reset(uint32_t _bits, int count)
{
  uint32_t u;
  int i;

  u = 1;
  for (i = 0; i < count; i++)
  {
    if ((this->pattern) & u)
    {
      if (this->neg)
      {
        if (!(_bits & u)) ((this->latch) &= (~u));
      } else
      {
        if (_bits & u) ((this->latch) &= (~u));
      }
    } else
    {
      if (this->neg)
      {
        if (!(_bits & u)) ((this->latch) |= u);
      } else
      {
        if (_bits & u) ((this->latch) |= u);
      }
    }
    (this->reset) = (((this->reset) & (~u)) | (_bits & u));
    u <<= 1;
  }
}

void DioLatch::publish_set()
{
  player_dio_cmd_t dio_cmd;
  player_dio_data_t dio_data;

  if ((this->use_set) && (this->send_commands))
  {
    memset(&dio_cmd, 0, sizeof dio_cmd);
    dio_cmd.count = this->pattern_count;
    dio_cmd.digout = this->latch;
    this->dio_set_required_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, reinterpret_cast<void *>(&dio_cmd), 0, NULL);
  }
  memset(&dio_data, 0, sizeof dio_data);
  dio_data.count = this->pattern_count;
  dio_data.bits = this->latch;
  this->Publish(this->dio_set_provided_addr,
                PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                reinterpret_cast<void *>(&dio_data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
}

void DioLatch::publish_reset()
{
  player_dio_data_t dio_data;

  memset(&dio_data, 0, sizeof dio_data);
  dio_data.count = this->pattern_count;
  dio_data.bits = this->reset;
  this->Publish(this->dio_reset_provided_addr,
                PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                reinterpret_cast<void *>(&dio_data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
}

int DioLatch::Subscribe(player_devaddr_t addr)
{
  int retval;

  retval = this->Driver::Subscribe(addr);
  if (retval) return retval;
  if (!(this->use_set))
  {
    publish_set();
    publish_reset();
  }
  return 0;
}

int DioLatch::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_dio_cmd_t * dio_cmd;
  player_dio_data_t * dio_data;
  int count;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, this->dio_set_provided_addr))
  {
    assert(data);
    dio_cmd = reinterpret_cast<player_dio_cmd_t *>(data);
    assert(dio_cmd);
    count = static_cast<int>(dio_cmd->count);
    if (!(count > 0)) return 0;
    if (count > static_cast<int>(this->pattern_count)) count = static_cast<int>(this->pattern_count);
    this->process_set(dio_cmd->digout, count);
    this->publish_set();
    this->publish_reset();
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, this->dio_reset_provided_addr))
  {
    assert(data);
    dio_cmd = reinterpret_cast<player_dio_cmd_t *>(data);
    assert(dio_cmd);
    count = static_cast<int>(dio_cmd->count);
    if (!(count > 0)) return 0;
    if (count > static_cast<int>(this->pattern_count)) count = static_cast<int>(this->pattern_count);
    this->process_reset(dio_cmd->digout, count);
    this->publish_set();
    this->publish_reset();
    return 0;
  }
  if (this->use_set)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES, this->dio_set_required_addr))
    {
      assert(data);
      if (!(this->send_commands))
      {
        dio_data = reinterpret_cast<player_dio_data_t *>(data);
        assert(dio_data);
        count = static_cast<int>(dio_data->count);
        if (!(count > 0)) return 0;
        if (count > static_cast<int>(this->pattern_count)) count = static_cast<int>(this->pattern_count);
        this->process_set(dio_data->bits, count);
        this->publish_reset();
      }
      this->publish_set();
      return 0;
    }
  }
  if (this->use_reset)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES, this->dio_reset_required_addr))
    {
      assert(data);
      dio_data = reinterpret_cast<player_dio_data_t *>(data);
      assert(dio_data);
      count = static_cast<int>(dio_data->count);
      if (!(count > 0)) return 0;
      if (count > static_cast<int>(this->pattern_count)) count = static_cast<int>(this->pattern_count);
      this->process_reset(dio_data->bits, count);
      this->publish_set();
      this->publish_reset();
      return 0;
    }
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * DioLatch_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new DioLatch(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void diolatch_Register(DriverTable * table)
{
  table->AddDriver("diolatch", DioLatch_Init);
}
