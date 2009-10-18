/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004  Brian Gerkey gerkey@stanford.edu
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
 *
 * Bitwise logic functions for dio interface.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_bitlogic bitlogic
 * @brief Bitwise logic functions for dio interface.

@par Provides

- @ref interface_dio

@par Requires

- @ref interface_dio (optionally)
  - If set, results are sent as commands

@par Configuration requests

- None

@par Configuration file options

- function (string)
  - Default: "NONE"
  - One of: "and", "or", "xor", "nand", "nor", "nxor" (case sensitive)
- slots
  - Default: 1
  - Number of slots (greater than 0)
- init_bits (string)
  - Default: "00000000000000000000000000000000"
  - Initial bits for each slot, last character is the lowest bit (length greater than 0, max. 32 characters)
- wait_for_all (integer)
  - Default: 1
  - If set to 1, this driver waits for data from all slots before issuing a command to subscribed dio interface

@par Example

@verbatim
driver
(
  name "bitlogic"
  function "and"
  slots 2
  provides ["0:::dio:0" "1:::dio:1"]
  init_bits "101"
)
@endverbatim

@author Paul Osmialowski

*/

/** @} */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <libplayercore/playercore.h>

#define MAX_SLOTS 16

class Bitlogic : public Driver
{
  public:
    Bitlogic(ConfigFile * cf, int section);
    virtual ~Bitlogic();
    virtual int Setup();
    virtual int Shutdown();
    virtual int Subscribe(player_devaddr_t addr);
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);
  private:
    uint32_t bits[MAX_SLOTS];
    player_devaddr_t dio_provided_addrs[MAX_SLOTS];
    player_devaddr_t dio_required_addr;
    Device * dio_required_dev;
    size_t slots;
    uint32_t init_bits;
    uint32_t init_count;
    uint32_t count;
    int cmd_mode;
    enum { AND = 0, OR = 1, XOR = 2, NAND = 3, NOR = 4, NXOR = 5 } function;
    int wait_for_all;
    bool data_valid[MAX_SLOTS];
    int ith;
    int jth;
    uint32_t compute() const throw (const char *);
};

Bitlogic::Bitlogic(ConfigFile * cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  const char * _init_bits;
  const char * fun;
  char key[7];
  int i;

  memset(this->bits, 0, sizeof this->bits);
  memset(this->dio_provided_addrs, 0, sizeof this->dio_provided_addrs);
  memset(&(this->dio_required_addr), 0, sizeof(player_devaddr_t));
  this->dio_required_dev = NULL;
  this->slots = 0;
  this->init_bits = 0;
  this->init_count = 0;
  this->count = 0;
  this->cmd_mode = 0;
  this->function = AND;
  for (i = 0; i < MAX_SLOTS; i++) this->data_valid[i] = false;
  this->ith = 0;
  this->jth = 0;
  if (cf->ReadDeviceAddr(&(this->dio_required_addr), section, "requires", PLAYER_DIO_CODE, -1, NULL))
  {
    PLAYER_WARN("ignore \"missing field [requires]\" error message");
  } else
  {
    this->cmd_mode = !0;
  }
  this->slots = static_cast<size_t>(cf->ReadInt(section, "slots", 1));
  if ((!((this->slots) > 0)) || ((this->slots) > MAX_SLOTS))
  {
    PLAYER_ERROR("invalid number of slots");
    this->SetError(-1);
    return;
  }
  for (i = 0; i < static_cast<int>(this->slots); i++)
  {
#if defined (WIN32)
    _snprintf(key, sizeof key, "%d", i);
#else
    snprintf(key, sizeof key, "%d", i);
#endif
    if (cf->ReadDeviceAddr(&(this->dio_provided_addrs[i]), section, "provides", PLAYER_DIO_CODE, -1, key))
    {
      this->SetError(-1);
      return;
    }
    if (this->AddInterface(this->dio_provided_addrs[i]))
    {
      this->SetError(-1);
      return;
    }
  }
  _init_bits = cf->ReadString(section, "init_bits", "00000000000000000000000000000000");
  if (!_init_bits)
  {
    this->SetError(-1);
    return;
  }
  this->init_count = strlen(_init_bits);
  if ((!((this->init_count) > 0)) || ((this->init_count) > 32))
  {
    PLAYER_ERROR("invalid length of init_bits string");
    this->SetError(-1);
    return;
  }
  this->count = this->init_count;
  this->init_bits = 0;
  for (i = 0; _init_bits[i]; i++)
  {
    if (i) ((this->init_bits) <<= 1);
    switch(_init_bits[i])
    {
    case '0':
      break;
    case '1':
      ((this->init_bits) |= 1);
      break;
    default:
      PLAYER_ERROR("invalid init_bits string");
      this->SetError(-1);
      return;
    }
  }
  fun = cf->ReadString(section, "function", "NONE");
  if (!(strcmp(fun, "and")))
  {
    this->function = AND;
  } else if (!(strcmp(fun, "or")))
  {
    this->function = OR;
  } else if (!(strcmp(fun, "xor")))
  {
    this->function = XOR;
  } else if (!(strcmp(fun, "namd")))
  {
    this->function = NAND;
  } else if (!(strcmp(fun, "nor")))
  {
    this->function = NOR;
  } else if (!(strcmp(fun, "nxor")))
  {
    this->function = NXOR;
  } else
  {
    PLAYER_ERROR1("unknown function [%s]", fun);
    this->SetError(-1);
    return;
  }
  this->wait_for_all = cf->ReadInt(section, "wait_for_all", 1);
}

Bitlogic::~Bitlogic() { }

int Bitlogic::Setup()
{
  int i;

  this->count = this->init_count;
  for (i = 0; i < MAX_SLOTS; i++) this->data_valid[i] = false;
  this->ith = 0;
  this->jth = 0;
  for (i = 0; i < static_cast<int>(this->slots); i++)
  {
    this->bits[i] = this->init_bits;
    if (this->cmd_mode)
    {
      if (Device::MatchDeviceAddress(this->dio_required_addr, this->dio_provided_addrs[i]))
      {
        PLAYER_ERROR("attempt to subscribe to self");
        return -1;
      }
    }
  }
  this->dio_required_dev = NULL;
  if (this->cmd_mode)
  {
    this->dio_required_dev = deviceTable->GetDevice(this->dio_required_addr);
    if (!(this->dio_required_dev))
    {
      PLAYER_ERROR("unable to locate suitable dio device");
      return -1;
    }
    if (this->dio_required_dev->Subscribe(this->InQueue))
    {
      PLAYER_ERROR("unable to subscribe to dio device");
      this->dio_required_dev = NULL;
      return -1;
    }
  }
  return 0;
}

int Bitlogic::Shutdown()
{
  if ((this->cmd_mode) && (this->dio_required_dev))
  {
    this->dio_required_dev->Unsubscribe(this->InQueue);
  }
  this->dio_required_dev = NULL;
  return 0;
}

int Bitlogic::Subscribe(player_devaddr_t addr)
{
  player_dio_data_t dio_data;
  int retval;

  retval = this->Driver::Subscribe(addr);
  if (retval) return retval;
  if (!(this->cmd_mode))
  {
    memset(&dio_data, 0, sizeof dio_data);
    dio_data.count = this->count;
    try
    {
      dio_data.bits = this->compute();
    } catch (const char * e)
    {
      PLAYER_ERROR1("%s", e);
      return -1;
    }
    this->Publish(addr,
                  PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                  reinterpret_cast<void *>(&dio_data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
  }
  return 0;
}

uint32_t Bitlogic::compute() const throw (const char *)
{
  uint32_t b;
  int i;

  b = this->bits[0];
  for (i = 1; i < static_cast<int>(this->slots); i++)
  {
    switch (this->function)
    {
    case AND:
    case NAND:
      b &= this->bits[i];
      break;
    case OR:
    case NOR:
      b |= this->bits[i];
      break;
    case XOR:
    case NXOR:
      b ^= this->bits[i];
      break;
    default:
      throw "unknown function";
    }
  }
  switch (this->function)
  {
  case NAND:
  case NOR:
  case NXOR:
    b = ~b;
  default:
    b = b; // prevents warnings
  }
  return b;
}

int Bitlogic::ProcessMessage(QueuePointer &resp_queue,
                             player_msghdr * hdr,
                             void * data)
{
  int i;
  player_msghdr newhdr;
  player_dio_cmd_t cmd;
  player_dio_data_t dio_data;

  if (!hdr) return -1;
  if (this->cmd_mode)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                              -1, // -1 means 'all message subtypes'
                              this->dio_required_addr))
    {
      if (!data) return -1;
      newhdr = *hdr;
      newhdr.addr = this->dio_provided_addrs[this->ith];
      this->Publish(&newhdr, data); // *data remains unchanged
      this->ith++;
      if ((this->ith) >= (static_cast<int>(this->slots)))
      {
        this->ith = 0;
      }
      return 0;
    }
  }
  for (i = 0; i < static_cast<int>(this->slots); i++)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, this->dio_provided_addrs[i]))
    {
      if (!data) return -1;
      cmd = *(reinterpret_cast<player_dio_cmd_t *>(data));
      if ((cmd.count) > (this->count)) this->count = cmd.count;
      this->bits[i] = cmd.digout;
      this->data_valid[i] = true;
      memset(&cmd, 0, sizeof cmd);
      memset(&dio_data, 0, sizeof dio_data);
      cmd.count = this->count;
      try
      {
        cmd.digout = this->compute();
      } catch (const char * e)
      {
        PLAYER_ERROR1("%s", e);
        return -1;
      }
      dio_data.count = cmd.count;
      dio_data.bits = cmd.digout;
      if (this->cmd_mode)
      {
        for (i = 0; i < static_cast<int>(this->slots); i++) if (!(this->data_valid[i])) break;
        if ((!(this->wait_for_all)) || (i >= static_cast<int>(this->slots)))
        {
          this->dio_required_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, reinterpret_cast<void *>(&cmd), 0, NULL);
          for (i = 0; i < static_cast<int>(this->slots); i++) this->data_valid[i] = false;
        }
      } else
      {
        this->Publish(this->dio_provided_addrs[this->jth],
                      PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                      reinterpret_cast<void *>(&dio_data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
        this->jth++;
        if ((this->jth) >= (static_cast<int>(this->slots)))
        {
          this->jth = 0;
        }
      }
      return 0;
    }
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * Bitlogic_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new Bitlogic(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void bitlogic_Register(DriverTable * table)
{
  table->AddDriver("bitlogic", Bitlogic_Init);
}
