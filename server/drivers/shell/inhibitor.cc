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
// Desc: Command flow inhibitor
// Author: Paul Osmialowski
// Date: 16 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_inhibitor inhibitor
 * @brief Command flow inhibitor

The inhibitor device which can be turned on/off by selected bits (AND bitmask)
of provided dio interface blocks commands sent from provided interface to the
subscribed device.

WARNING! Using this device we no more can say that each sent command is
guaranteed to be delivered to the receiver.

@par Compile-time dependencies

- none

@par Provides

- any kind of interface ('comm' key)
- @ref interface_dio ('switch' key)

Roles of provided interfaces are distinguished by given key (switch or comm)

@par Requires

- the same interface as provided with 'comm' key (again, 'comm' key)
- optionally, @ref interface_dio ('switch' key) to read switch state from other device

@par Configuration requests

- none

@par Configuration file options

- init_state (integer)
  - Default: 1
  - if set to 1, the switch is initially set to ON (see 'neg' below)
- bitmask (string)
  - Default: "00000000000000000000000000000001"
  - Last character is the lowest bit (length greater than 0, max. 32 characters)
- neg (integer)
  - Default: 0
  - If set to 1, invert switch meaning (pass commands when it is OFF)
- block_data
  - Default: 0
  - If set to 1, data flow is also blocked
- msg_interval
  - Default: 0.0
  - If greater than 0.0 (seconds), commands will be passed with this interval
    (all commands sent more frequently will be thrown away). When this interval
    is set to 0.0, all commands are passed (as long as inhibitor state is right).

@par Example

@verbatim
driver
(
  name "inhibitor"
  provides ["switch:::dio:0" "comm:::position2d:10"]
  requires ["comm:::position2d:0"]
  bitmask "111"
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL and size_t
#include <stdlib.h> // for malloc() and free()
#include <string.h> // for memset()
#include <math.h> // for fabs()
#include <assert.h>
#include <libplayercore/playercore.h>

#define RQ_QUEUE_LEN 10
#define EPS 0.0000001

class Inhibitor : public Driver
{
  public: Inhibitor(ConfigFile * cf, int section);
  public: virtual ~Inhibitor();
  public: virtual int Setup();
  public: virtual int Shutdown();
  public: virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data);
  private: player_devaddr_t dio_provided_addr;
  private: player_devaddr_t comm_provided_addr;
  private: player_devaddr_t dio_required_addr;
  private: player_devaddr_t comm_required_addr;
  private: Device * dio_required_dev;
  private: Device * comm_required_dev;
  private: int use_dio;
  private: int init_state;
  private: uint32_t bitmask;
  private: int neg;
  private: int block_data;
  private: double msg_interval;
  private: double lastTime;
  private: int switch_state;
  private: int rq[RQ_QUEUE_LEN];
  private: int last_rq;
  private: player_msghdr_t rq_hdrs[RQ_QUEUE_LEN];
  private: QueuePointer rq_ptrs[RQ_QUEUE_LEN];
  private: void * payloads[RQ_QUEUE_LEN];
};

Inhibitor::Inhibitor(ConfigFile * cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  const char * _bitmask;
  size_t count;
  int i;

  memset(&(this->dio_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->comm_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_required_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->comm_required_addr), 0, sizeof(player_devaddr_t));
  this->dio_required_dev = NULL;
  this->comm_required_dev = NULL;
  this->use_dio = 0;
  this->init_state = 0;
  this->bitmask = 0;
  this->neg = 0;
  this->block_data = 0;
  this->msg_interval = 0.0;
  this->lastTime = 0.0;
  this->switch_state = 0;
  this->last_rq = -1;
  memset(this->rq_hdrs, 0, sizeof this->rq_hdrs);
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    this->payloads[i] = NULL;
    this->rq[i] = 0;
  }
  if (cf->ReadDeviceAddr(&(this->dio_provided_addr), section, "provides", PLAYER_DIO_CODE, -1, "switch"))
  {
    PLAYER_ERROR("cannot provide switch interface");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->dio_provided_addr))
  {
    PLAYER_ERROR("AddInterface failed for switch interface");
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->comm_provided_addr), section, "provides", -1, -1, "comm"))
  {
    PLAYER_ERROR("cannot provide comm interface");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->comm_provided_addr))
  {
    PLAYER_ERROR("AddInterface failed for comm interface");
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->dio_required_addr), section, "requires", PLAYER_DIO_CODE, -1, "switch"))
  {
    PLAYER_WARN("switch dio device not required");
    this->use_dio = 0;
  } else
  {
    PLAYER_WARN("switch dio device will be subscribed");
    this->use_dio = !0;
  }
  if (cf->ReadDeviceAddr(&(this->comm_required_addr), section, "requires", this->comm_provided_addr.interf, -1, "comm"))
  {
    PLAYER_ERROR("cannot require configured comm device");
    this->SetError(-1);
    return;
  }
  this->init_state = cf->ReadInt(section, "init_state", 1);
  this->switch_state = this->init_state;
  _bitmask = cf->ReadString(section, "bitmask", "00000000000000000000000000000001");
  if (!_bitmask)
  {
    this->SetError(-1);
    return;
  }
  count = strlen(_bitmask);
  if ((!(count > 0)) || (count > 32))
  {
    PLAYER_ERROR("invalid length of bitmask string");
    this->SetError(-1);
    return;
  }
  this->bitmask = 0;
  for (i = 0; _bitmask[i]; i++)
  {
    if (i) ((this->bitmask) <<= 1);
    switch(_bitmask[i])
    {
    case '0':
      break;
    case '1':
      ((this->bitmask) |= 1);
      break;
    default:
      PLAYER_ERROR("invalid bitmask string");
      this->SetError(-1);
      return;
    }
  }
  this->neg = cf->ReadInt(section, "neg", 0);
  this->block_data = cf->ReadInt(section, "block_data", 0);
  this->msg_interval = cf->ReadFloat(section, "msg_interval", 0.0);
  if ((this->msg_interval) < 0.0)
  {
    PLAYER_ERROR("invalid msg_interval");
    this->SetError(-1);
    return;
  }
}

Inhibitor::~Inhibitor()
{
  int i;

  for (i = 0; i < RQ_QUEUE_LEN; i++) if (this->payloads[i])
  {
    free(this->payloads[i]);
    this->payloads[i] = NULL;
  }
}

int Inhibitor::Setup()
{
  int i;

  this->switch_state = this->init_state;
  this->last_rq = -1;
  memset(this->rq_hdrs, 0, sizeof this->rq_hdrs);
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    this->payloads[i] = NULL;
    this->rq[i] = 0;
  }
  if (Device::MatchDeviceAddress(this->comm_required_addr, this->comm_provided_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self (comm->comm)");
    return -1;
  }
  if (Device::MatchDeviceAddress(this->comm_required_addr, this->dio_provided_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self (comm->switch)");
    return -1;
  }
  if (this->use_dio)
  {
    if (Device::MatchDeviceAddress(this->dio_required_addr, this->comm_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self (switch->comm)");
      return -1;
    }
    if (Device::MatchDeviceAddress(this->dio_required_addr, this->dio_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self (switch->switch)");
      return -1;
    }
  }
  this->comm_required_dev = deviceTable->GetDevice(this->comm_required_addr);
  if (!(this->comm_required_dev)) return -1;
  if (this->comm_required_dev->Subscribe(this->InQueue))
  {
    this->comm_required_dev = NULL;
    return -1;
  }
  if (this->use_dio)
  {
    this->dio_required_dev = deviceTable->GetDevice(this->dio_required_addr);
    if (!(this->dio_required_dev))
    {
      if (this->comm_required_dev) this->comm_required_dev->Unsubscribe(this->InQueue);
      this->comm_required_dev = NULL;
      return -1;
    }
    if (this->dio_required_dev->Subscribe(this->InQueue))
    {
      this->dio_required_dev = NULL;
      if (this->comm_required_dev) this->comm_required_dev->Unsubscribe(this->InQueue);
      this->comm_required_dev = NULL;
      return -1;
    }
  }
  this->lastTime = 0.0;
  return 0;
}

int Inhibitor::Shutdown()
{
  int i;

  if (this->comm_required_dev) this->comm_required_dev->Unsubscribe(this->InQueue);
  this->comm_required_dev = NULL;
  if (this->dio_required_dev) this->dio_required_dev->Unsubscribe(this->InQueue);
  this->dio_required_dev = NULL;
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    if (this->payloads[i])
    {
      free(this->payloads[i]);
      this->payloads[i] = NULL;
    }
    this->rq[i] = 0;
  }
  return 0;
}

int Inhibitor::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  int pass;
  player_dio_data_t * dio_data;
  player_dio_cmd_t * dio_cmd;
  player_dio_data_t new_data;
  player_msghdr_t newhdr;
  QueuePointer null;
  int i;
  int n;
  double t;

  assert(hdr);
  if (this->use_dio)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES, this->dio_required_addr))
    {
      assert(data);
      dio_data = reinterpret_cast<player_dio_data_t *>(data);
      assert(dio_data);
      if ((dio_data->count) > 0) this->switch_state = ((dio_data->bits) & (this->bitmask)) ? 1 : 0;
      memset(&new_data, 0, sizeof new_data);
      new_data.count = 1;
      new_data.bits = static_cast<uint32_t>(this->switch_state);
      this->Publish(this->dio_provided_addr,
                    PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                    reinterpret_cast<void *>(&new_data), 0, NULL, true); // copy = true
      return 0;
    }
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, this->dio_provided_addr))
  {
    assert(data);
    dio_cmd = reinterpret_cast<player_dio_cmd_t *>(data);
    assert(dio_cmd);
    if ((dio_cmd->count) > 0) this->switch_state = ((dio_cmd->digout) & (this->bitmask)) ? 1 : 0;
    memset(&new_data, 0, sizeof new_data);
    new_data.count = 1;
    new_data.bits = static_cast<uint32_t>(this->switch_state);
    this->Publish(this->dio_provided_addr,
                  PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                  reinterpret_cast<void *>(&new_data), 0, NULL, true); // copy = true
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, -1, this->comm_required_addr))
  {
    assert(data);
    memset(&new_data, 0, sizeof new_data);
    new_data.count = 1;
    new_data.bits = static_cast<uint32_t>(this->switch_state);
    this->Publish(this->dio_provided_addr,
                  PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                  reinterpret_cast<void *>(&new_data), 0, NULL, true); // copy = true
    if (this->block_data)
    {
      pass = (this->neg) ? (!(this->switch_state)) : (this->switch_state);
      if (!pass) return 0;
    }
    this->Publish(this->comm_provided_addr,
                  PLAYER_MSGTYPE_DATA, hdr->subtype,
                  data, 0, &(hdr->timestamp), true); // copy = true
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, -1, this->comm_provided_addr))
  {
    pass = (this->neg) ? (!(this->switch_state)) : (this->switch_state);
    if (!pass) return 0;
    if ((this->msg_interval) > EPS)
    {
      GlobalTime->GetTimeDouble(&t);
      if ((fabs(t - (this->lastTime))) > (this->msg_interval))
      {
        this->lastTime = t;
      } else return 0;
    }
    newhdr = *hdr;
    newhdr.addr = this->comm_required_addr;
    this->comm_required_dev->PutMsg(this->InQueue, &newhdr, data, true); // copy = true
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->comm_provided_addr))
  {
    for (i = 0; i < RQ_QUEUE_LEN; i++) if (!(this->rq[i]))
    {
      this->rq_hdrs[i] = *hdr;
      this->rq_ptrs[i] = resp_queue;
      if ((hdr->size) > 0)
      {
        this->payloads[i] = malloc(hdr->size);
        assert(this->payloads[i]);
        memcpy(this->payloads[i], data, hdr->size);
      } else this->payloads[i] = NULL;
      this->rq[i] = !0;
      break;
    }
    if (!(i < RQ_QUEUE_LEN)) return -1;
    n = -1;
    for (i = 0; i < RQ_QUEUE_LEN; i++) if (this->rq[i]) n = i;
    assert(n >= 0);
    if (!n)
    {
      newhdr = this->rq_hdrs[n];
      newhdr.addr = this->comm_required_addr;
      if ((newhdr.size) > 0) assert(this->payloads[n]);
      this->comm_required_dev->PutMsg(this->InQueue, &newhdr, this->payloads[n], true); // copy = true
      this->last_rq = n;
    }
    return 0;
  }
  if ((Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK, -1, this->comm_required_addr)) || (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_NACK, -1, this->comm_required_addr)))
  {
    if ((this->last_rq) < 0)
    {
      PLAYER_ERROR("last_rq < 0");
      return -1;
    }
    if ((hdr->subtype) != (this->rq_hdrs[this->last_rq].subtype))
    {
      PLAYER_ERROR("ACK/NACK subtype does not match");
      return -1;
    }
    this->Publish(this->comm_provided_addr, this->rq_ptrs[this->last_rq], hdr->type, hdr->subtype, data, 0, &(hdr->timestamp), true); // copy = true
    this->rq_ptrs[this->last_rq] = null;
    assert(this->rq[this->last_rq]);
    if (this->payloads[this->last_rq])
    {
      assert(this->payloads[this->last_rq]);
      free(this->payloads[this->last_rq]);
    }
    this->payloads[this->last_rq] = NULL;
    this->rq[this->last_rq] = 0;
    this->last_rq = -1;
    for (i = 0; i < RQ_QUEUE_LEN; i++) if (this->rq[i])
    {
      newhdr = this->rq_hdrs[i];
      newhdr.addr = this->comm_required_addr;
      if ((newhdr.size) > 0) assert(this->payloads[i]);
      this->comm_required_dev->PutMsg(this->InQueue, &newhdr, this->payloads[i], true); // copy = true;
      this->last_rq = i;
      break;
    }
    return 0;
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * Inhibitor_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new Inhibitor(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void inhibitor_Register(DriverTable * table)
{
  table->AddDriver("inhibitor", Inhibitor_Init);
}
