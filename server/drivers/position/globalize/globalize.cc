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
 * This driver replaces local (odometry) position data with global position.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_Globalize Globalize
 * @brief This driver replaces local (odometry) position data with global position.

@par Provides

- @ref interface_position2d
  - commands will be passed to "local" position2d device (VELOCITY COMMANDS ONLY!)

@par Requires

- @ref interface_position2d
  - key "local" - local position2d device
  - key "global" - global positioning device (amcl or something)

@par Configuration requests

- cmd_interval (float)
  - default: -1.0
  - if greater than zero, commands will be sent with this interval

@par Configuration file options

- None

@par Example

@verbatim
driver
(
  name "globalize"
  provides ["position2d:10"]
  requires ["local:::position2d:0" "global:::position2d:1"]
)
@endverbatim

@author Paul Osmialowski

*/

/** @} */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libplayercore/playercore.h>

#define RQ_QUEUE_LEN 10

class Globalize : public Driver
{
  public:
    Globalize(ConfigFile * cf, int section);
    virtual ~Globalize();
    virtual int Setup();
    virtual int Shutdown();
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);
  private:
    Device * r_local_pos_dev;
    Device * r_global_pos_dev;
    player_devaddr_t r_local_pos_addr;
    player_devaddr_t r_global_pos_addr;
    player_devaddr_t p_pos_addr;
    player_msghdr_t rq_hdrs[RQ_QUEUE_LEN];
    QueuePointer rq_ptrs[RQ_QUEUE_LEN];
    void * payloads[RQ_QUEUE_LEN];
    int rq[RQ_QUEUE_LEN];
    int last_rq;
    double cmd_interval;
    double cmd_time;
    uint8_t stall;
};

Globalize::Globalize(ConfigFile* cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  int i;

  this->r_local_pos_dev = NULL;
  this->r_global_pos_dev = NULL;
  memset(&(this->r_local_pos_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->r_global_pos_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->p_pos_addr), 0, sizeof(player_devaddr_t));
  memset(this->rq_hdrs, 0, sizeof this->rq_hdrs);
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    this->payloads[i] = NULL;
    this->rq[i] = 0;
  }
  this->last_rq = -1;
  this->cmd_interval = 0.0;
  this->cmd_time = 0.0;
  this->stall = 0;
  if (cf->ReadDeviceAddr(&(this->p_pos_addr), section, "provides",
      PLAYER_POSITION2D_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->p_pos_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->r_local_pos_addr), section, "requires",
      PLAYER_POSITION2D_CODE, -1, "local"))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->r_global_pos_addr), section, "requires",
      PLAYER_POSITION2D_CODE, -1, "global"))
  {
    this->SetError(-1);
    return;
  }
  this->cmd_interval = cf->ReadFloat(section, "cmd_interval", -1.0);
}

Globalize::~Globalize()
{
  int i;

  for (i = 0; i < RQ_QUEUE_LEN; i++) if (this->payloads[i])
  {
    free(this->payloads[i]);
    this->payloads[i] = NULL;
  }
}

int Globalize::Setup()
{
  int i;

  this->r_local_pos_dev = NULL;
  this->r_global_pos_dev = NULL;
  this->cmd_time = 0.0;
  this->stall = 0;
  memset(this->rq_hdrs, 0, sizeof this->rq_hdrs);
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    this->payloads[i] = NULL;
    this->rq[i] = 0;
  }
  this->last_rq = -1;
  if ((Device::MatchDeviceAddress(this->r_local_pos_addr, this->p_pos_addr))
      || (Device::MatchDeviceAddress(this->r_global_pos_addr, this->p_pos_addr)))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return -1;
  }
  this->r_local_pos_dev = deviceTable->GetDevice(this->r_local_pos_addr);
  if (!(this->r_local_pos_dev))
  {
    PLAYER_ERROR("unable to locate suitable local position2d device");
    return -1;
  }
  if (this->r_local_pos_dev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to local position2d device");
    this->r_local_pos_dev = NULL;
    return -1;
  }
  this->r_global_pos_dev = deviceTable->GetDevice(this->r_global_pos_addr);
  if (!(this->r_global_pos_dev))
  {
    PLAYER_ERROR("unable to locate suitable global position2d device");
    this->r_local_pos_dev->Unsubscribe(this->InQueue);
    this->r_local_pos_dev = NULL;
    return -1;
  }
  if (this->r_global_pos_dev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to global position2d device");
    this->r_local_pos_dev->Unsubscribe(this->InQueue);
    this->r_local_pos_dev = NULL;
    this->r_global_pos_dev = NULL;
    return -1;
  }
  return 0;
}

int Globalize::Shutdown()
{
  int i;

  if (this->r_local_pos_dev) this->r_local_pos_dev->Unsubscribe(this->InQueue);
  this->r_local_pos_dev = NULL;
  if (this->r_global_pos_dev) this->r_global_pos_dev->Unsubscribe(this->InQueue);
  this->r_global_pos_dev = NULL;
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

int Globalize::ProcessMessage(QueuePointer &resp_queue,
                              player_msghdr * hdr,
                              void * data)
{
  player_msghdr_t newhdr;
  player_position2d_data_t pos_data;
  double d;
  QueuePointer null;
  int i;
  int n;

  if (!hdr)
  {
    PLAYER_ERROR("NULL hdr");
    return -1;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, this->p_pos_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL data");
      return -1;
    }
    if ((this->cmd_interval) > 0.0)
    {
      GlobalTime->GetTimeDouble(&d);
      if ((d - (this->cmd_time)) < (this->cmd_interval)) return 0;
    }
    this->r_local_pos_dev->PutMsg(this->InQueue, hdr, data);
    GlobalTime->GetTimeDouble(&(this->cmd_time));
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->p_pos_addr))
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
      newhdr.addr = this->r_local_pos_addr;
      if ((newhdr.size) > 0) assert(this->payloads[n]);
      this->r_local_pos_dev->PutMsg(this->InQueue, &newhdr, this->payloads[n], true); // copy = true
      this->last_rq = n;
    }
    return 0;
  }
  if ((Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK, -1, this->r_local_pos_addr)) || (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_NACK, -1, this->r_local_pos_addr)))
  {
    if ((this->last_rq) < 0)
    {
      PLAYER_ERROR("last_rq < 0");
      return -1;
    }
    assert((hdr->subtype) == (this->rq_hdrs[this->last_rq].subtype));
    this->Publish(this->p_pos_addr, this->rq_ptrs[this->last_rq], hdr->type, hdr->subtype, data, 0, &(hdr->timestamp), true); // copy = true
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
      newhdr.addr = this->r_local_pos_addr;
      if ((newhdr.size) > 0) assert(this->payloads[i]);
      this->r_local_pos_dev->PutMsg(this->InQueue, &newhdr, this->payloads[i], true); // copy = true;
      this->last_rq = i;
      break;
    }
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            PLAYER_POSITION2D_DATA_STATE,
                            this->r_global_pos_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL data");
      return -1;
    }
    pos_data = *(reinterpret_cast<player_position2d_data_t *>(data));
    pos_data.stall = this->stall;
    newhdr = *hdr;
    newhdr.addr = this->p_pos_addr;
    this->Publish(&newhdr, reinterpret_cast<void *>(&pos_data));
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            PLAYER_POSITION2D_DATA_STATE,
                            this->r_local_pos_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL data");
      return -1;
    }
    pos_data = *(reinterpret_cast<player_position2d_data_t *>(data));
    this->stall = pos_data.stall;
    return 0;
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * Globalize_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new Globalize(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void globalize_Register(DriverTable * table)
{
  table->AddDriver("globalize", Globalize_Init);
}
