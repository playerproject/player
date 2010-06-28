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
// Desc: Command flow splitter
// Author: Paul Osmialowski
// Date: 16 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_cmdsplitter cmdsplitter
 * @brief Command flow splitter

The splitter device sends received command to n subscribed devices.
Data packets are sent back only from the first subscribed device.

@par Compile-time dependencies

- none

@par Provides

- any kind of interface

@par Requires

- the same interface as provided (repeated n times with numbered keys, 0-based)

@par Configuration requests

- none

@par Configuration file options

- devices
  - Default: 1
  - Number of subscriptions to be done
- rq_first_device_only
  - Default: 0
  - If set to non-zero, requests will be forwarded only to the first subscribed device

@par Example

@verbatim
driver
(
  name "cmdsplitter"
  provides ["position2d:0"]
  devices 2
  requires ["0::6665:position2d:0" "1::6666:position2d:0"]
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL and size_t
#include <stdlib.h> // for malloc() and free()
#include <stdio.h> // for snprintf()
#include <string.h> // for memset()
#include <assert.h>
#include <libplayercore/playercore.h>

#define RQ_QUEUE_LEN 10
#define MAX_DEVICES 16

#if defined (WIN32)
  #define snprintf _snprintf
#endif

class CmdSplitter : public Driver
{
  public: CmdSplitter(ConfigFile * cf, int section);
  public: virtual ~CmdSplitter();
  public: virtual int Setup();
  public: virtual int Shutdown();
  public: virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data);
  private: player_devaddr_t provided_addr;
  private: player_devaddr_t required_addrs[MAX_DEVICES];
  private: Device * required_devs[MAX_DEVICES];
  private: int devices;
  private: int rq_first_device_only;
  private: int last_rq;
  private: player_msghdr_t rq_hdrs[RQ_QUEUE_LEN];
  private: QueuePointer rq_ptrs[RQ_QUEUE_LEN];
  private: void * payloads[RQ_QUEUE_LEN];
  private: int rq[RQ_QUEUE_LEN];
};

CmdSplitter::CmdSplitter(ConfigFile * cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  char key[7];
  int i;

  memset(&(this->provided_addr), 0, sizeof(player_devaddr_t));
  memset(this->required_addrs, 0, sizeof this->required_addrs);
  for (i = 0; i < MAX_DEVICES; i++) this->required_devs[i] = NULL;
  this->devices = 0;
  this->rq_first_device_only = 0;
  this->last_rq = -1;
  memset(this->rq_hdrs, 0, sizeof this->rq_hdrs);
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    this->payloads[i] = NULL;
    this->rq[i] = 0;
  }
  if (cf->ReadDeviceAddr(&(this->provided_addr), section, "provides", -1, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->provided_addr))
  {
    this->SetError(-1);
    return;
  }
  this->devices = cf->ReadInt(section, "devices", 1);
  if ((this->devices < 1) || (this->devices > MAX_DEVICES))
  {
    PLAYER_ERROR("Invalid number of devices to subscribe");
    this->SetError(-1);
    return;
  }
  for (i = 0; i < (this->devices); i++)
  {
    snprintf(key, sizeof key, "%d", i);
    if (cf->ReadDeviceAddr(&(this->required_addrs[i]), section, "requires", this->provided_addr.interf, -1, key))
    {
      PLAYER_ERROR1("Cannot require configured device [source %d]", i);
      this->SetError(-1);
      return;
    }
  }
  this->rq_first_device_only = cf->ReadInt(section, "rq_first_device_only", 0);
}

CmdSplitter::~CmdSplitter()
{
  int i;

  for (i = 0; i < RQ_QUEUE_LEN; i++) if (this->payloads[i])
  {
    free(this->payloads[i]);
    this->payloads[i] = NULL;
  }
}

int CmdSplitter::Setup()
{
  int i;

  this->last_rq = -1;
  memset(this->rq_hdrs, 0, sizeof this->rq_hdrs);
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    this->payloads[i] = NULL;
    this->rq[i] = 0;
  }
  for (i = 0; i < (this->devices); i++)
  {
    if (Device::MatchDeviceAddress(this->required_addrs[i], this->provided_addr))
    {
      PLAYER_ERROR1("attempt to subscribe to self (device %i)", i);
      return -1;
    }
  }
  for (i = 0; i < (this->devices); i++)
  {
    this->required_devs[i] = deviceTable->GetDevice(this->required_addrs[i]);
    if (!(this->required_devs[i])) return -1;
    if (this->required_devs[i]->Subscribe(this->InQueue))
    {
      this->required_devs[i] = NULL;
      for (i = 0; i < (this->devices); i++)
      {
        if (this->required_devs[i]) this->required_devs[i]->Unsubscribe(this->InQueue);
        this->required_devs[i] = NULL;
      }
      return -1;
    }
  }
  return 0;
}

int CmdSplitter::Shutdown()
{
  int i;

  for (i = 0; i < (this->devices); i++)
  {
    if (this->required_devs[i]) this->required_devs[i]->Unsubscribe(this->InQueue);
    this->required_devs[i] = NULL;
  }
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

int CmdSplitter::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_msghdr_t newhdr;
  QueuePointer null;
  int i;
  int n;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->provided_addr))
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
      for (i = 0; i < ((this->rq_first_device_only) ? 1 : (this->devices)); i++)
      {
        newhdr = this->rq_hdrs[n];
        newhdr.addr = this->required_addrs[i];
        if ((newhdr.size) > 0) assert(this->payloads[n]);
        this->required_devs[i]->PutMsg(this->InQueue, &newhdr, this->payloads[n], true); // copy = true
      }
      this->last_rq = n;
      return 0;
    }
  }
  for (i = 0; i < ((this->rq_first_device_only) ? 1 : (this->devices)); i++)
  {
    if ((Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK, -1, this->required_addrs[i])) || (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_NACK, -1, this->required_addrs[i])))
    {
      if (i) return 0;
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
      this->Publish(this->provided_addr, this->rq_ptrs[this->last_rq], hdr->type, hdr->subtype, data, 0, &(hdr->timestamp), true); // copy = true
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
        n = i;
        for (i = 0; i < ((this->rq_first_device_only) ? 1 : (this->devices)); i++)
        {
          newhdr = this->rq_hdrs[n];
          newhdr.addr = this->required_addrs[i];
          if ((newhdr.size) > 0) assert(this->payloads[n]);
          this->required_devs[i]->PutMsg(this->InQueue, &newhdr, this->payloads[n], true); // copy = true;
        }
        this->last_rq = n;
        return 0;
      }
      return 0;
    }
  }
  for (i = 0; i < (this->devices); i++)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, -1, this->required_addrs[i]))
    {
      assert(data);
      if (!i)
      {
        this->Publish(this->provided_addr,
                      PLAYER_MSGTYPE_DATA, hdr->subtype,
                      data, 0, &(hdr->timestamp), true); // copy = true
      }
      return 0;
    }
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, -1, this->provided_addr))
  {
    for (i = 0; i < (this->devices); i++)
    {
      newhdr = *hdr;
      newhdr.addr = this->required_addrs[i];
      this->required_devs[i]->PutMsg(this->InQueue, &newhdr, data, true); // copy = true
    }
    return 0;
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * CmdSplitter_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new CmdSplitter(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void cmdsplitter_Register(DriverTable * table)
{
  table->AddDriver("cmdsplitter", CmdSplitter_Init);
}
