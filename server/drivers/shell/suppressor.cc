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
// Desc: Command flow suppressor
// Author: Paul Osmialowski
// Date: 13 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_suppressor suppressor
 * @brief Command flow suppressor

The suppressor device blocks commands sent from one (slave) source at the time
the other (master) source sends commands to the same subscribed device.

WARNING! Using this device we no more can say that each sent command is
guaranteed to be delivered to the receiver.

@par Compile-time dependencies

- none

@par Provides

- any kind of interface
- the same interface as chosen above

Roles of provided interfaces are distinguished by given key (master or slave)

@par Requires

- the same interface as provided

@par Configuration requests

- none

@par Configuration file options

- fadeout_time (float)
  - Default: 0.0 (no suppression)
  - fadeout_time (in seconds) starts after any command is forwarded from
    master input to the subscribed device; during this all commands
    received by slave input are lost

@par Example

@verbatim
driver
(
  name "suppressor"
  provides ["master:::position2d:10" "slave:::position2d:11"]
  requires ["position2d:0"]
  fadeout_time 0.333
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

#define RQ_QUEUE_LEN 10

class Suppressor : public Driver
{
  public: Suppressor(ConfigFile * cf, int section);
  public: virtual ~Suppressor();
  public: virtual int Setup();
  public: virtual int Shutdown();
  public: virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data);
  private: player_devaddr_t master_provided_addr;
  private: player_devaddr_t slave_provided_addr;
  private: player_devaddr_t required_addr;
  private: Device * required_dev;
  private: double fadeout_time;
  private: double fadeout_start;
  private: int fading_out;
  private: int rq[RQ_QUEUE_LEN];
  private: int last_rq;
  private: player_msghdr_t rq_hdrs[RQ_QUEUE_LEN];
  private: QueuePointer rq_ptrs[RQ_QUEUE_LEN];
  private: void * payloads[RQ_QUEUE_LEN];
};

Suppressor::Suppressor(ConfigFile * cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  int i;

  memset(&(this->master_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->slave_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->required_addr), 0, sizeof(player_devaddr_t));
  this->required_dev = NULL;
  this->fadeout_time = 0.0;
  this->fadeout_start = 0.0;
  this->fading_out = 0;
  this->last_rq = -1;
  memset(this->rq_hdrs, 0, sizeof this->rq_hdrs);
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    this->payloads[i] = NULL;
    this->rq[i] = 0;
  }
  if (cf->ReadDeviceAddr(&(this->master_provided_addr), section, "provides", -1, -1, "master"))
  {
    PLAYER_ERROR("cannot provide master slot");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->master_provided_addr))
  {
    PLAYER_ERROR("AddInterface failed for master slot");
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->slave_provided_addr), section, "provides", this->master_provided_addr.interf, -1, "slave"))
  {
    PLAYER_ERROR("cannot provide slave slot");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->slave_provided_addr))
  {
    PLAYER_ERROR("AddInterface failed for slave slot");
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->required_addr), section, "requires", this->master_provided_addr.interf, -1, NULL))
  {
    PLAYER_ERROR("cannot require configured device");
    this->SetError(-1);
    return;
  }
  this->fadeout_time = cf->ReadFloat(section, "fadeout_time", 0.0);
  if (this->fadeout_time < 0.0)
  {
    PLAYER_ERROR("Invalid fadeout_time value");
    this->SetError(-1);
    return;
  }
}

Suppressor::~Suppressor()
{
  int i;

  for (i = 0; i < RQ_QUEUE_LEN; i++) if (this->payloads[i])
  {
    free(this->payloads[i]);
    this->payloads[i] = NULL;
  }
}

int Suppressor::Setup()
{
  int i;

  this->fadeout_start = 0.0;
  this->fading_out = 0;
  this->last_rq = -1;
  memset(this->rq_hdrs, 0, sizeof this->rq_hdrs);
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    this->payloads[i] = NULL;
    this->rq[i] = 0;
  }
  if (Device::MatchDeviceAddress(this->required_addr, this->master_provided_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self (master)");
    return -1;
  }
  if (Device::MatchDeviceAddress(this->required_addr, this->slave_provided_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self (slave)");
    return -1;
  }
  this->required_dev = deviceTable->GetDevice(this->required_addr);
  if (!(this->required_dev)) return -1;
  if (this->required_dev->Subscribe(this->InQueue))
  {
    this->required_dev = NULL;
    return -1;
  }
  return 0;
}

int Suppressor::Shutdown()
{
  int i;

  if (this->required_dev) this->required_dev->Unsubscribe(this->InQueue);
  this->required_dev = NULL;
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

int Suppressor::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_msghdr_t newhdr;
  QueuePointer null;
  double d;
  int i;
  int n;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, -1, this->required_addr))
  {
    assert(data);
    d = hdr->timestamp;
    this->Publish(this->master_provided_addr,
                  PLAYER_MSGTYPE_DATA, hdr->subtype,
                  data, 0, &d, true); // copy = true
    d = hdr->timestamp;
    this->Publish(this->slave_provided_addr,
                  PLAYER_MSGTYPE_DATA, hdr->subtype,
                  data, 0, &d, true); // copy = true
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, -1, this->master_provided_addr))
  {
    newhdr = *hdr;
    newhdr.addr = this->required_addr;
    this->required_dev->PutMsg(this->InQueue, &newhdr, data, true); // copy = true
    GlobalTime->GetTimeDouble(&(this->fadeout_start));
    this->fading_out = !0;
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, -1, this->slave_provided_addr))
  {
    if (this->fading_out)
    {
      GlobalTime->GetTimeDouble(&d);
      if ((d - (this->fadeout_start)) >= (this->fadeout_time)) this->fading_out = 0;
    }
    if (!(this->fading_out))
    {
      newhdr = *hdr;
      newhdr.addr = this->required_addr;
      this->required_dev->PutMsg(this->InQueue, &newhdr, data, true); // copy = true
    }
    return 0;
  }
  if ((Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->master_provided_addr)) || (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->slave_provided_addr)))
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
      } else (this->payloads[i]) = NULL;
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
      newhdr.addr = this->required_addr;
      if ((newhdr.size) > 0) assert(this->payloads[n]);
      this->required_dev->PutMsg(this->InQueue, &newhdr, this->payloads[n], true); // copy = true
      this->last_rq = n;
    }
    return 0;
  }
  if ((Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK, -1, this->required_addr)) || (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_NACK, -1, this->required_addr)))
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
    this->Publish(this->rq_hdrs[this->last_rq].addr, this->rq_ptrs[this->last_rq], hdr->type, hdr->subtype, data, 0, &(hdr->timestamp), true); // copy = true
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
      newhdr.addr = this->required_addr;
      if ((newhdr.size) > 0) assert(this->payloads[i]);
      this->required_dev->PutMsg(this->InQueue, &newhdr, this->payloads[i], true); // copy = true;
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
Driver * Suppressor_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new Suppressor(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void suppressor_Register(DriverTable * table)
{
  table->AddDriver("suppressor", Suppressor_Init);
}
