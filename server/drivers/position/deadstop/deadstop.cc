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
// Desc: This forwards and tracks commands through position2d interface,
//       if no command is sent for too long it repeats on sending stop
//       velocity command in order to cause emergency stop.
// Author: Paul Osmialowski
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_deadstop deadstop
 * @brief stop while dead

This forwards and tracks commands through position2d interface,
if no command is sent for too long it repeats on sending stop
velocity command in order to cause emergency stop.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_position2d

@par Requires

- @ref interface_position2d

@par Configuration requests

- none

@par Configuration file options

- timeout (float)
  - Default: 0.5 (half a second)
  - how long to wait for new command to forward

- cheat_time (float)
  - Default: 0.0 (no effect)
  - how long to send stop command before start to cheat

@par Example

@verbatim
driver
(
  name "deadstop"
  requires ["position2d:1"]
  provides ["position2d:0"]
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <libplayercore/playercore.h>

#define EPS 0.000000001

class DeadStop : public ThreadedDriver
{
  public: DeadStop(ConfigFile * cf, int section);
  public: virtual ~DeadStop();

  public: virtual int MainSetup();
  public: virtual void MainQuit();

  // This method will be invoked on each incoming message
  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
			             void * data);

  private: virtual void Main();

  private: player_devaddr_t position2d_provided_addr, position2d_required_addr;
  private: Device * ppos;

  private: double timeout;
  private: double cheat_time;
  private: double last_time;
  private: int stop_command;
};

Driver * DeadStop_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new DeadStop(cf, section));
}

void deadstop_Register(DriverTable * table)
{
  table->AddDriver("deadstop", DeadStop_Init);
}

DeadStop::DeadStop(ConfigFile * cf, int section)
  : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  memset(&(this->position2d_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->position2d_required_addr), 0, sizeof(player_devaddr_t));
  this->ppos = NULL;
  this->timeout = 0.0;
  this->cheat_time = 0.0;
  this->last_time = 0.0;
  this->stop_command = 0;
  if (cf->ReadDeviceAddr(&(this->position2d_provided_addr), section, "provides", PLAYER_POSITION2D_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->position2d_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->position2d_required_addr), section, "requires", PLAYER_POSITION2D_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  this->timeout = cf->ReadFloat(section, "timeout", 0.5);
  if ((this->timeout) < 0.0)
  {
    PLAYER_ERROR("Invalid timeout value");
    this->SetError(-1);
    return;
  }
  this->cheat_time = cf->ReadFloat(section, "cheat_time", 0.0);
  if ((this->cheat_time) < 0.0)
  {
    PLAYER_ERROR("Invalid cheat_time value");
    this->SetError(-1);
    return;
  }
}

DeadStop::~DeadStop() { }

int DeadStop::MainSetup()
{
  if (Device::MatchDeviceAddress(this->position2d_required_addr, this->position2d_provided_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return -1;
  }
  this->ppos = deviceTable->GetDevice(this->position2d_required_addr);
  if (!(this->ppos))
  {
    PLAYER_ERROR("unable to locate suitable position2d device");
    return -1;
  }
  if (this->ppos->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to position2d device");
    this->ppos = NULL;
    return -1;
  }
  return 0;
}

void DeadStop::MainQuit()
{
  if (this->ppos) this->ppos->Unsubscribe(this->InQueue);
  this->ppos = NULL;
}

void DeadStop::Main()
{
  player_position2d_cmd_vel_t vel_cmd;
  struct timespec tspec;
  double d;
  int stopping;
  double stop_time;

  GlobalTime->GetTimeDouble(&(this->last_time));
  GlobalTime->GetTimeDouble(&(stop_time));
  stopping = 0;
  this->stop_command = !0;
  for (;;)
  {
    pthread_testcancel();
    this->ProcessMessages();
    pthread_testcancel();
    GlobalTime->GetTimeDouble(&d);
    if ((d - (this->last_time)) >= (this->timeout))
    {
      memset(&vel_cmd, 0, sizeof vel_cmd);
      vel_cmd.vel.px = 0.0;
      vel_cmd.vel.py = 0.0;
      vel_cmd.vel.pa = 0.0;
      switch (stopping)
      {
      case 0:
        stopping = 1;
	stop_time = d;
	break;
      case 1:
        if (((this->cheat_time) > 0.0) && (!(this->stop_command)))
	{
	  if ((d - stop_time) >= (this->cheat_time))
	  {
	    vel_cmd.vel.pa = 0.4;
	    stopping = 2;
	  }
	}
	break;
      case 2:
        assert((this->cheat_time) > 0.0);
	if ((d - stop_time) >= ((this->cheat_time) + (this->timeout)))
	{
	  vel_cmd.vel.pa = 0.0;
	  stopping = 0;
	} else vel_cmd.vel.pa = 0.4;
	break;
      default:
        assert(!"internal error");
      }
      vel_cmd.state = !0;
      this->ppos->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, reinterpret_cast<void *>(&vel_cmd), 0, NULL);
    } else stopping = 0;
    pthread_testcancel();
    tspec.tv_sec = 0;
    tspec.tv_nsec = 1000000;
    nanosleep(&tspec, NULL);
  }
}

int DeadStop::ProcessMessage(QueuePointer &resp_queue,
                             player_msghdr * hdr,
			     void * data)
{
  Message * msg;
  player_msghdr_t newhdr;
  player_position2d_cmd_vel_t vel_cmd;

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            -1, // -1 means 'all message subtypes'
			    this->position2d_required_addr))
  {
    if (!data) return -1;
    this->Publish(this->position2d_provided_addr,
                  PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
		  data, 0, NULL, true); // copy = true, do not dispose data placed who knows where!
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, -1, this->position2d_provided_addr))
  {
    if (!data) return -1;
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, this->position2d_provided_addr))
    {
      vel_cmd = *(reinterpret_cast<player_position2d_cmd_vel_t *>(data));
      if ((fabs(vel_cmd.vel.px) < EPS) && (fabs(vel_cmd.vel.py) < EPS) && (fabs(vel_cmd.vel.pa) < EPS)) this->stop_command = !0;
      else this->stop_command = 0;
    }
    GlobalTime->GetTimeDouble(&(this->last_time));
    this->ppos->PutMsg(this->InQueue, hdr, data);
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->position2d_provided_addr))
  {
    if (!data) PLAYER_WARN2("NULL data request %d:%d", hdr->type, hdr->subtype);
    hdr->addr = this->position2d_required_addr;
    msg = this->ppos->Request(this->InQueue, hdr->type, hdr->subtype, data, 0, NULL, true); // threaded = true
    if (!msg)
    {
      PLAYER_WARN("failed to forward request");
      return -1;
    }
    newhdr = *(msg->GetHeader());
    newhdr.addr = this->position2d_provided_addr;
    this->Publish(resp_queue, &newhdr, msg->GetPayload(), true); // copy = true, do not dispose published data as we're disposing whole source message in the next line
    delete msg;
    return 0;
  }
  return -1;
}
