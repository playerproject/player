/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/* Copyright (C) 2010
 *   Paul Osmialowski
 * Copyright (C) 2004
 *   Toby Collett, University of Auckland Robotics Group
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_hemisson hemisson
 * @brief K-Team Hemisson mobile robot

The hemisson driver is used to interface to the K-Team hemisson robot.

This driver is experimental and should be treated with caution. At
this point it supports the @ref interface_position2d and
@ref interface_ranger interfaces.

This driver was tested with RS-232C cable link (for example /dev/ttyS0)
and bluetooth radio link (/dev/rfcomm0 or /dev/ttyUB0).

@par Compile-time dependencies

- none

@par Provides

- @ref interface_position2d
- @ref interface_ranger

@par Requires

- none

@par Configuration file options

- port (string)
  - Default: "/dev/rfcomm0"
  - Serial port used to communicate with the robot.
- sleep_nsec (integer)
  - Default: 100000000
  - timespec value for nanosleep().
- init_motor_state (integer)
  - Default: 0
  - Initial motor state.
- speed_factor (float)
  - Default: 18.0
  - Speed scale factor.
- aspeed_factor (float)
  - Default: 16.0
  - Angular speed scale factor.
- publish_ranges (integer)
  - Default: 8
  - Vaild values: 1..8
  - Number of ranger scans to publish.
- set_stall (integer)
  - Default: 0
  - If set to non-zero, stall field of provided position2d interface
    will be filled according to sensor readings.
- stall_threshold (float)
  - Default: 0.025
  - Distance below this value causes robot to be stalled.

Since initialization routine takes some time to do its job, it may be
worth to set alwayson to 1, as some libplayerc client programs may
refuse to work due to communication timeout at the startup.

@par Example

@verbatim
driver
(
  name "hemisson"
  provides ["position2d:0" "ranger:0"]
  port "/dev/ttyS1"
  alwayson 1
)
@endverbatim

@author Paul Osmialowski based on Khepera driver by Toby Collett
*/
/** @} */

#include "hemisson_serial.h"

#include <stddef.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <libplayercore/playercore.h>

#define EPS 0.00000000001

#define HEMISSON_RANGES 8
#define HEMISSON_WIDTH 0.11

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

// Convert degrees to radians
#ifndef DTOR
#define DTOR(d) ((d) * (M_PI) / 180.0)
#endif

#ifndef RTOD
#define RTOD(r) ((r) * 180.0 / M_PI)
#endif

class Hemisson : public ThreadedDriver
{
public:
  Hemisson(ConfigFile * cf, int section);
  virtual ~Hemisson();

  virtual void Main();

  virtual int MainSetup();
  virtual void MainQuit();
  virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data);
private:
  HemissonSerial * serial;
  int debug;
  player_devaddr_t position2d_addr;
  player_devaddr_t ranger_addr;
  const char * port;
  int sleep_nsec;
  int init_motor_state;
  int motor_state;
  double speed_factor;
  double aspeed_factor;
  int publish_ranges;
  int set_stall;
  double stall_threshold;
  int stalled;
  int prev_speed[2];
  double prev_vel;
  static double btm(int i);
};

Hemisson::Hemisson(ConfigFile * cf, int section)
  : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  this->serial = NULL;
  this->debug = 0;
  memset(&(this->position2d_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->ranger_addr), 0, sizeof(player_devaddr_t));
  this->port = NULL;
  this->sleep_nsec = 0;
  this->init_motor_state = 0;
  this->motor_state = 0;
  this->speed_factor = 0.0;
  this->publish_ranges = 0;
  this->set_stall = 0;
  this->stall_threshold = 0.0;
  this->stalled = 0;
  this->prev_speed[0] = 0;
  this->prev_speed[1] = 0;
  this->prev_vel = 0.0;
  if (cf->ReadDeviceAddr(&(this->position2d_addr), section, "provides", PLAYER_POSITION2D_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->position2d_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->ranger_addr), section, "provides", PLAYER_RANGER_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->ranger_addr))
  {
    this->SetError(-1);
    return;
  }
  this->port = cf->ReadString(section, "port", HEMISSON_DEFAULT_SERIAL_PORT);
  if (!(this->port))
  {
    this->SetError(-1);
    return;
  }
  if (!(strlen(this->port) > 0))
  {
    this->SetError(-1);
    return;
  }
  this->debug = cf->ReadInt(section, "debug", 0);
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 100000000);
  if ((this->sleep_nsec) < 0)
  {
    this->SetError(-1);
    return;
  }
  this->init_motor_state = cf->ReadInt(section, "init_motor_state", 0);
  this->speed_factor = cf->ReadFloat(section, "speed_factor", 18.0);
  this->aspeed_factor = cf->ReadFloat(section, "aspeed_factor", 16.0);
  this->publish_ranges = cf->ReadInt(section, "publish_ranges", 8);
  if (((this->publish_ranges) < 1) || ((this->publish_ranges) > HEMISSON_RANGES))
  {
    PLAYER_ERROR("Invalid number of ranges to publish");
    this->SetError(-1);
    return;
  }
  this->set_stall = cf->ReadInt(section, "set_stall", 0);
  if ((this->set_stall) && (this->publish_ranges < 6))
  {
    PLAYER_ERROR("Stall detection needs at least 6 ir ranger sensors");
    this->SetError(-1);
    return;
  }
  this->stall_threshold = cf->ReadFloat(section, "stall_threshold", 0.025);
  if ((this->stall_threshold) < 0.0)
  {
    this->SetError(-1);
    return;
  }
}

Hemisson::~Hemisson()
{
  if (this->serial) delete this->serial;
  this->serial = NULL;
}

int Hemisson::MainSetup()
{
  try
  {
    this->serial = new HemissonSerial(this->debug, this->port);
  } catch (...)
  {
    this->serial = NULL;
  }
  if (!(this->serial)) return -1;
  return 0;
}

void Hemisson::MainQuit()
{
  if (this->serial) delete this->serial;
  this->serial = NULL;
}

void Hemisson::Main()
{
  struct timespec tspec;
  double rrval[HEMISSON_RANGES];
  player_ranger_data_range_t ranges = { static_cast<uint32_t>(this->publish_ranges), rrval };
  int rval[HEMISSON_RANGES];
  player_position2d_data_t pose;
  int i;

  this->motor_state = this->init_motor_state;
  this->prev_speed[0] = -1000;
  this->prev_speed[1] = -1000;
  this->prev_vel = 0.0;
  this->stalled = 0;
  for (;;)
  {
    pthread_testcancel();
    this->ProcessMessages();
    pthread_testcancel();
    if (!(this->serial->HemissonCommand('N', 0, NULL, HEMISSON_RANGES, rval)))
    {
      assert((ranges.ranges_count) == static_cast<uint32_t>(this->publish_ranges));
      assert(ranges.ranges);
      for (i = 0; i < (this->publish_ranges); i++)
      {
        ranges.ranges[i] = Hemisson::btm(rval[i]);
      }
      if (this->set_stall)
      {
        if ((this->prev_vel) > EPS)
        {
          if (((ranges.ranges[0]) < (this->stall_threshold)) || ((ranges.ranges[1]) < (this->stall_threshold)) || ((ranges.ranges[2]) < (this->stall_threshold))) this->stalled = !0;
          else this->stalled = 0;
        } else if ((this->prev_vel) < (-EPS))
        {
          if ((ranges.ranges[5]) < (this->stall_threshold)) this->stalled = !0;
          else this->stalled = 0;
        }
      }
      this->Publish(this->ranger_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE, reinterpret_cast<void *>(&ranges));
    }
    pthread_testcancel();
    memset(&(pose), 0, sizeof pose);
    pose.stall = (this->set_stall) ? (this->stalled) : 0;
    this->Publish(this->position2d_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, reinterpret_cast<void *>(&pose));
    pthread_testcancel();
    if (this->sleep_nsec > 0)
    {
      tspec.tv_sec = 0;
      tspec.tv_nsec = this->sleep_nsec;
      nanosleep(&tspec, NULL);
    }
  }
}

int Hemisson::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_ranger_geom_t ranger_geom;
  player_pose3d_t ranger_poses[HEMISSON_RANGES];
  player_bbox3d_t ranger_sizes[HEMISSON_RANGES];
  player_ranger_config_t ranger_config;
  player_position2d_geom_t position_geom;
  player_position2d_power_config_t * position_power;
  player_position2d_cmd_vel_t * position_cmd;
  int i, rotvel, speed[2];
  double d;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_GEOM, this->ranger_addr))
  {
    memset(&ranger_geom, 0, sizeof ranger_geom);
    ranger_geom.size.sw = HEMISSON_WIDTH;
    ranger_geom.size.sl = 0.13;
    ranger_geom.size.sh = 0.006;
    ranger_geom.element_poses_count = static_cast<uint32_t>(this->publish_ranges);
    ranger_geom.element_poses = ranger_poses;
    assert(ranger_geom.element_poses);
    memset(&(ranger_geom.element_poses[0]), 0, sizeof ranger_geom.element_poses[0]);
    ranger_geom.element_poses[0].px = 0.06;
    ranger_geom.element_poses[0].py = 0.0;
    ranger_geom.element_poses[0].pz = 0.0;
    ranger_geom.element_poses[0].pyaw = DTOR(0.0);
    memset(&(ranger_geom.element_poses[1]), 0, sizeof ranger_geom.element_poses[1]);
    ranger_geom.element_poses[1].px = 0.055;
    ranger_geom.element_poses[1].py = -0.04;
    ranger_geom.element_poses[1].pz = 0.0;
    ranger_geom.element_poses[1].pyaw = DTOR(-45.0);
    memset(&(ranger_geom.element_poses[2]), 0, sizeof ranger_geom.element_poses[2]);
    ranger_geom.element_poses[2].px = 0.055;
    ranger_geom.element_poses[2].py = 0.04;
    ranger_geom.element_poses[2].pz = 0.0;
    ranger_geom.element_poses[2].pyaw = DTOR(45.0);
    memset(&(ranger_geom.element_poses[3]), 0, sizeof ranger_geom.element_poses[3]);
    ranger_geom.element_poses[3].px = 0.0;
    ranger_geom.element_poses[3].py = -0.04;
    ranger_geom.element_poses[3].pz = 0.0;
    ranger_geom.element_poses[3].pyaw = DTOR(-90.0);
    memset(&(ranger_geom.element_poses[4]), 0, sizeof ranger_geom.element_poses[4]);
    ranger_geom.element_poses[4].px = 0.0;
    ranger_geom.element_poses[4].py = 0.04;
    ranger_geom.element_poses[4].pz = 0.0;
    ranger_geom.element_poses[4].pyaw = DTOR(90.0);
    memset(&(ranger_geom.element_poses[5]), 0, sizeof ranger_geom.element_poses[5]);
    ranger_geom.element_poses[5].px = -0.06;
    ranger_geom.element_poses[5].py = 0.0;
    ranger_geom.element_poses[5].pz = 0.0;
    ranger_geom.element_poses[5].pyaw = DTOR(180.0);
    memset(&(ranger_geom.element_poses[6]), 0, sizeof ranger_geom.element_poses[6]);
    ranger_geom.element_poses[6].ppitch = DTOR(90.0);
    memset(&(ranger_geom.element_poses[7]), 0, sizeof ranger_geom.element_poses[7]);
    ranger_geom.element_poses[7].ppitch = DTOR(-90.0);
    ranger_geom.element_sizes_count = static_cast<uint32_t>(this->publish_ranges);
    ranger_geom.element_sizes = ranger_sizes;
    assert(ranger_geom.element_sizes);
    for (i = 0; i < HEMISSON_RANGES; i++)
    {
      memset(&(ranger_geom.element_sizes[i]), 0, sizeof ranger_geom.element_sizes[i]);
      ranger_geom.element_sizes[i].sw = 0.003;
      ranger_geom.element_sizes[i].sl = 0.007;
      ranger_geom.element_sizes[i].sh = 0.006;
    }
    this->Publish(this->ranger_addr, resp_queue,  PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_GET_GEOM, reinterpret_cast<void *>(&ranger_geom));
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_CONFIG, this->ranger_addr))
  {
    // as in sonartoranger.cc: no config for this device, so send back a pile of zeroes
    memset(&ranger_config, 0, sizeof ranger_config);
    this->Publish(this->ranger_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_GET_CONFIG, reinterpret_cast<void *>(&ranger_config));
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_GET_GEOM, this->position2d_addr))
  {
    memset(&position_geom, 0, sizeof position_geom);
    position_geom.size.sw = HEMISSON_WIDTH;
    position_geom.size.sl = 0.13;
    position_geom.size.sh = 0.05;
    this->Publish(this->position2d_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_GET_GEOM, reinterpret_cast<void *>(&position_geom));
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_MOTOR_POWER, this->position2d_addr))
  {
    assert(data);
    position_power = reinterpret_cast<player_position2d_power_config_t *>(data);
    assert(position_power);
    this->motor_state = position_power->state;
    this->Publish(this->position2d_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_MOTOR_POWER);
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, this->position2d_addr))
  {
    assert(data);
    position_cmd = reinterpret_cast<player_position2d_cmd_vel_t *>(data);
    assert(position_cmd);
    d = (position_cmd->vel.px) * (this->speed_factor);
    i = static_cast<int>(d);
    if (d > EPS)
    {
      if (static_cast<double>(i) < d) i++;
    } else if (d < (-EPS))
    {
      if (static_cast<double>(i) > d) i--;
    } else i = 0;
    speed[0] = i;
    speed[1] = i;
    if (fabs(position_cmd->vel.pa) > EPS)
    {
      rotvel = static_cast<int>(RTOD(position_cmd->vel.pa) * (this->aspeed_factor) * M_PI * (HEMISSON_WIDTH * 10.0) / 360.0);
      speed[0] -= rotvel;
      speed[1] += rotvel;
    }
    if (this->debug) PLAYER_WARN4("vel: %.4f, %.4f - speed: %d, %d", position_cmd->vel.px, position_cmd->vel.pa, speed[0], speed[1]);
    if (speed[0] > 9) speed[0] = 9;
    if (speed[0] < -9) speed[0] = -9;
    if (speed[1] > 9) speed[1] = 9;
    if (speed[1] < -9) speed[1] = -9;
    if (this->motor_state)
    {
      if (((this->prev_speed[0]) != speed[0]) || ((this->prev_speed[1]) != speed[1]))
      {
        this->prev_speed[0] = speed[0];
        this->prev_speed[1] = speed[1];
        this->serial->HemissonCommand('D', 2, speed, 0, NULL);
      }
      this->prev_vel = position_cmd->vel.px;
    }
    return 0;
  }
  return -1;
}

double Hemisson::btm(int i)
{
  if (i > 250) return EPS;
  if (i > 100) return 0.01;
  if (i > 50) return 0.02 - (static_cast<double>(i) / 100000.0);
  if (i > 20) return 0.03 - (static_cast<double>(i) / 10000.0);
  return 0.08 - (static_cast<double>(i) / 1000.0);
}

Driver * Hemisson_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new Hemisson(cf, section));
}

void hemisson_Register(DriverTable * table)
{
  table->AddDriver("hemisson", Hemisson_Init);
}

