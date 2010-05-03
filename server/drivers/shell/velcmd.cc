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
// Desc: Velocity commands sender
// Author: Paul Osmialowski
// Date: 13 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_velcmd velcmd
 * @brief Velocity commands sender

The velcmd driver keeps on repeating configured velocity command.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_opaque

@par Requires

- @ref interface_position2d
- optionally: @ref interface_ranger

@par Configuration requests

- none

@par Configuration file options

- px (float)
  - Default: 0.0 (m/s)
- py (float)
  - Default: 0.0 (m/s)
- pa (float)
  - Default: 0.0 (rad/s)
- power_on (integer)
  - Default: 1
  - If set to 1, send power-on request at startup
- sleep_nsec (integer)
  - Default: 100000000 (10 sends per second)
  - timespec value for nanosleep()
- first_idx (integer) (only if ranger in use)
  - Default: 0
  - Index of the first ranger scan to use
- last_idx (integer) (only if ranger in use)
  - Default: -1 (the last scan)
  - Index of the last ranger scan to use
- ranger_power_on (integer) (only if ranger in use)
  - Default: 0
  - If set to 1, send power-on request at startup

If ranger device is in use, mean from readings will be multiplied by these values:

- pxr (float)
  - Default: 0.0
  - resulting value will be kept lower or equal px given above
- pyr (float)
  - Default: 0.0
  - resulting value will be kept lower or equal py given above
- par (float)
  - Default: 0.0
  - resulting value will be kept lower or equal pa given above

@par Example

@verbatim
driver
(
  name "velcmd"
  provides ["opaque:0"]
  requires ["position2d:0" "ranger:0"]
  px 3.0
  py 0.0
  pa 0.0
  pxr 0.2
  first_idx 1
  last_idx 6
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

class VelCmd : public ThreadedDriver
{
  public:
    // Constructor; need that
    VelCmd(ConfigFile * cf, int section);

    virtual ~VelCmd();

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
    player_devaddr_t required_pos2d_addr;
    player_devaddr_t required_ranger_addr;
    // Handle for the device that have the address given above
    Device * required_pos2d_dev;
    Device * required_ranger_dev;
    int use_ranger;
    double px;
    double py;
    double pa;
    int power_on;
    int sleep_nsec;
    int first_idx;
    int last_idx;
    int ranger_power_on;
    double pxr;
    double pyr;
    double par;
    double mean_dist;
    int mean_dist_valid;
};

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
//
VelCmd::VelCmd(ConfigFile * cf, int section) : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  memset(&(this->provided_opaque_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->required_pos2d_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->required_ranger_addr), 0, sizeof(player_devaddr_t));
  this->required_pos2d_dev = NULL;
  this->required_ranger_dev = NULL;
  this->use_ranger = 0;
  this->px = 0.0;
  this->py = 0.0;
  this->pa = 0.0;
  this->power_on = 0;
  this->sleep_nsec = 0;
  this->first_idx = 0;
  this->last_idx = 0;
  this->ranger_power_on = 0;
  this->pxr = 0.0;
  this->pyr = 0.0;
  this->par = 0.0;
  this->mean_dist = 0.0;
  this->mean_dist_valid = 0;
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
  if (cf->ReadDeviceAddr(&(this->required_pos2d_addr), section, "requires",
                         PLAYER_POSITION2D_CODE, -1, NULL))
  {
    PLAYER_ERROR("cannot require position2d device");
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->required_ranger_addr), section, "requires",
                         PLAYER_RANGER_CODE, -1, NULL))
  {
    PLAYER_WARN("ranger device is not in use");
    this->use_ranger = 0;
  } else
  {
    PLAYER_WARN("ranger device is in use");
    this->use_ranger = !0;
  }
  this->px = cf->ReadFloat(section, "px", 0.0);
  this->py = cf->ReadFloat(section, "py", 0.0);
  this->pa = cf->ReadFloat(section, "pa", 0.0);
  this->power_on = cf->ReadInt(section, "power_on", 1);
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 100000000);
  if ((this->sleep_nsec) <= 0)
  {
    PLAYER_ERROR("Invalid sleep_nsec value");
    this->SetError(-1);
    return;
  }
  this->first_idx = cf->ReadInt(section, "first_idx", 0);
  if ((this->first_idx) < 0)
  {
    PLAYER_ERROR("Invalid first_idx value");
    this->SetError(-1);
    return;
  }
  this->last_idx = cf->ReadInt(section, "last_idx", 0);
  if (!((this->last_idx = -1) || (this->last_idx >= this->first_idx)))
  {
    PLAYER_ERROR("Invalid last_idx value");
    this->SetError(-1);
    return;    
  }
  this->ranger_power_on = cf->ReadInt(section, "ranger_power_on", 0);
  this->pxr = cf->ReadFloat(section, "pxr", 0.0);
  this->pyr = cf->ReadFloat(section, "pyr", 0.0);
  this->par = cf->ReadFloat(section, "par", 0.0);
}

VelCmd::~VelCmd() { }

int VelCmd::MainSetup()
{
  this->required_pos2d_dev = deviceTable->GetDevice(this->required_pos2d_addr);
  if (!(this->required_pos2d_dev))
  {
    PLAYER_ERROR("unable to locate suitable position2d device");
    return -1;
  }
  if (this->required_pos2d_dev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to position2d device");
    this->required_pos2d_dev = NULL;
    return -1;
  }
  if (this->use_ranger)
  {
    this->required_ranger_dev = deviceTable->GetDevice(this->required_ranger_addr);
    if (!(this->required_ranger_dev))
    {
      PLAYER_ERROR("unable to locate suitable ranger device");
      if (this->required_pos2d_dev) this->required_pos2d_dev->Unsubscribe(this->InQueue);
      this->required_pos2d_dev = NULL;
      return -1;
    }
    if (this->required_ranger_dev->Subscribe(this->InQueue))
    {
      PLAYER_ERROR("unable to subscribe to ranger device");
      this->required_ranger_dev = NULL;
      if (this->required_pos2d_dev) this->required_pos2d_dev->Unsubscribe(this->InQueue);
      this->required_pos2d_dev = NULL;
      return -1;
    }
  }
  return 0;
}

void VelCmd::MainQuit()
{
  if (this->required_ranger_dev) this->required_ranger_dev->Unsubscribe(this->InQueue);
  this->required_ranger_dev = NULL;
  if (this->required_pos2d_dev) this->required_pos2d_dev->Unsubscribe(this->InQueue);
  this->required_pos2d_dev = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void VelCmd::Main() 
{
  struct timespec tspec;
  player_ranger_power_config_t r_pwr_rq;
  player_position2d_power_config_t pwr_rq;
  player_position2d_cmd_vel_t vel_cmd;
  player_opaque_data_t data;
  Message * msg = NULL;

  if (this->ranger_power_on)
  {
    memset(&r_pwr_rq, 0, sizeof r_pwr_rq);
    r_pwr_rq.state = 1;
    msg = required_ranger_dev->Request(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_POWER, reinterpret_cast<void *>(&r_pwr_rq), 0, NULL, true); // threaded = true
    if (!msg) PLAYER_WARN("failed to send power request on ranger interface");
    else delete msg;
    msg = NULL;
  }
  if (this->power_on)
  {
    memset(&pwr_rq, 0, sizeof pwr_rq);
    pwr_rq.state = 1;
    msg = required_pos2d_dev->Request(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_MOTOR_POWER, reinterpret_cast<void *>(&pwr_rq), 0, NULL, true); // threaded = true
    if (!msg) PLAYER_WARN("failed to send power request on position2d interface");
    else delete msg;
    msg = NULL;
  }
  this->mean_dist = 0.0;
  this->mean_dist_valid = 0;
  for (;;)
  {
    // Test if we are supposed to cancel
    pthread_testcancel();

    // Process incoming messages
    this->ProcessMessages();

    // Test if we are supposed to cancel
    pthread_testcancel();

    memset(&vel_cmd, 0, sizeof vel_cmd);
    vel_cmd.state = !0;
    if (this->mean_dist_valid)
    {
      vel_cmd.vel.px = this->mean_dist * this->pxr;
      vel_cmd.vel.py = this->mean_dist * this->pyr;
      vel_cmd.vel.pa = this->mean_dist * this->par;
      if (vel_cmd.vel.px > this->px) vel_cmd.vel.px = this->px;
      if (vel_cmd.vel.py > this->py) vel_cmd.vel.py = this->py;
      if (vel_cmd.vel.pa > this->pa) vel_cmd.vel.pa = this->pa;
    } else
    {
      vel_cmd.vel.px = this->px;
      vel_cmd.vel.py = this->py;
      vel_cmd.vel.pa = this->pa;
    }
    this->required_pos2d_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, reinterpret_cast<void *>(&vel_cmd), 0, NULL);

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

    // sleep for a while
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec;
    nanosleep(&tspec, NULL);
  }
}

int VelCmd::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_ranger_data_range_t * ranges;
  int lastidx;
  int i;
  double sum;

  // Process messages here.  Send a response if necessary, using Publish().
  // If you handle the message successfully, return 0.  Otherwise,
  // return -1, and a NACK will be sent for you, if a response is required.

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  // handle position2d data
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            -1, // -1 means 'all message subtypes'
                            this->required_pos2d_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL position2d data");
      return -1;
    }
    return 0;
  }
  if (this->use_ranger)
  {
    // handle ranger data
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                              PLAYER_RANGER_DATA_RANGE,
                              this->required_ranger_addr))
    {
      if (!data)
      {
        PLAYER_ERROR("NULL ranger data");
        return -1;
      }
      ranges = reinterpret_cast<player_ranger_data_range_t *>(data);
      if (!ranges) return -1;
      if (static_cast<int>(ranges->ranges_count) <= (this->first_idx))
      {
        PLAYER_ERROR("Not enough ranger data");
        return -1;
      }
      if ((this->last_idx) < 0) lastidx = ((ranges->ranges_count) - 1);
      else lastidx = (this->last_idx);
      if (lastidx < (this->first_idx))
      {
        PLAYER_ERROR("Invalid indices");
        return -1;
      }
      sum = 0.0;
      for (i = (this->first_idx); i <= lastidx; i++)
      {
        sum += ranges->ranges[i];
      }
      this->mean_dist = sum / static_cast<double>((lastidx - (this->first_idx)) + 1);
      this->mean_dist_valid = !0;
      return 0;
    }
    // handle other ranger data
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, -1, this->required_ranger_addr))
    {
      if (!data)
      {
        PLAYER_ERROR("NULL other ranger data");
        return -1;
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
Driver * VelCmd_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new VelCmd(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void velcmd_Register(DriverTable * table)
{
  table->AddDriver("velcmd", VelCmd_Init);
}
