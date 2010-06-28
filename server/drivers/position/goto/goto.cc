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
 * A robot controller that drives a robot to a given target.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_Goto Goto
 * @brief A robot controller that drives a robot to a given target.

@par Provides

- @ref interface_position2d
- @ref interface_dio

@par Requires

- @ref interface_position2d

@par Configuration requests

- None

@par Configuration file options

- dist_tol (float)
  - Default: DEFAULT_DIST_TOL
  - Distance to the target tollerance
- angle_tol (float)
  - Default: DEFAULT_ANGLE_TOL
  - Angle difference tollerance
- max_dist (float)
  - Default: 0.5
  - Maximum distance between checkpoints
- debug (integer)
  - Default: 0
  - Debug
- reactive (integer)
  - Default: 0
  - Shall we react for stall states? 0 - no, 1 - yes (see 'reaction_turn_vel', 'reaction_time'), 2 - yes, with random parameters
- reaction_turn_vel (float)
  - Default: 0.0 (rad/sec)
  - When 'reactive' is set to 1, this driver causes a robot to go back on stall state for a while ('reaction_time')
    turning it with 'reaction_turn_vel' angular velocity
- reaction_time (float)
  - Default: 1.5 (seconds)
  - When 'reactive' is set to 1, this driver causes a robot to go back on stall state for a while ('reaction_time')
    turning it with 'reaction_turn_vel' angular velocity
- forward_enabled (integer)
  - Default: 0
  - Shall we forward position2d velocity commands?
- early_check (integer)
  - Default: 1
  - If set to 1, do not wait for newer position data to check if at target
- send_everything (integer)
  - Default: 1
  - If set to 1, data and commands are sent at once
- max_vel
  - Default: 0.7
  - Maximum speed forward
- min_vel
  - Default: 0.1
  - Minimum speed forward
- max_angular_vel
  - Default: 45.0
  - Maximum rotation speed
- min_angular_vel
  - Default: 10.0 
  - Minimum rotation speed

@par Example

@verbatim
driver
(
  name "goto"
  provides ["position2d:100" "dio:0"]
  requires ["position2d:0"]
  debug 1
  reactive 1
)
@endverbatim

@author Paul Osmialowski

*/

/** @} */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <libplayercore/playercore.h>

#define EPS 0.000000000000001

#define DEFAULT_DIST_TOL 0.04
#define DEFAULT_ANGLE_TOL 3.0
#define AMIN 5.0
#define AMAX 20.0
#define AVMIN 10.0
#define AVMAX 45.0
#define MAXD 2.0
#define TVMIN 0.1
#define TVMAX 0.7

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#ifndef M_SQRT2
#define M_SQRT2        1.41421356237309504880
#endif

// Convert degrees to radians
#ifndef DTOR
#define DTOR(d) ((d) * (M_PI) / 180.0)
#endif

// Normalize angle to domain -pi, pi
#ifndef NORMALIZE
#define NORMALIZE(z) atan2(sin(z), cos(z))
#endif

#define RQ_QUEUE_LEN 10

class Goto : public Driver
{
  public:
    Goto(ConfigFile * cf, int section);
    virtual ~Goto();
    virtual int Setup();
    virtual int Shutdown();
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);
  private:
    Device * position2d_required_dev;
    player_devaddr_t position2d_required_addr;
    player_devaddr_t position2d_provided_addr;
    player_devaddr_t dio_provided_addr;
    player_position2d_cmd_pos_t position2d_cmd_pos;
    player_position2d_data_t prev_pos_data;
    bool prev_pos_data_valid;
    double stopping_time;
    double stall_start_time;
    double stall_length;
    double stall_turn;
    double dist_tol;
    double angle_tol;
    double max_dist;
    double max_vel;
    double min_vel;
    double max_angular_vel;
    double min_angular_vel;
    int debug;
    int reactive;
    double reaction_turn_vel;
    double reaction_time;
    int forward_enabled;
    int early_check;
    int send_everything;
    int enabled;
    int stopping;
    int last_dir;
    int stall_state;
    int skip;
    player_msghdr_t rq_hdrs[RQ_QUEUE_LEN];
    QueuePointer rq_ptrs[RQ_QUEUE_LEN];
    void * payloads[RQ_QUEUE_LEN];
    int rq[RQ_QUEUE_LEN];
    int last_rq;
    static double mod(double x, double y);
    static double angle_map(double d);
    static double angle_diff(double a, double b);
};

double Goto::mod(double x, double y)
{
    return x - (static_cast<double>((static_cast<int>(x / y))) * y);
}

double Goto::angle_map(double d)
{
  if ((d + M_PI) < 0.0)
  {
    return M_PI - Goto::mod(-(d + M_PI), M_PI + M_PI);
  } else return Goto::mod(d + M_PI, M_PI + M_PI) - M_PI;
}

double Goto::angle_diff(double a, double b)
{
  double d1, d2;
  a = NORMALIZE(a);
  b = NORMALIZE(b);
  d1 = a - b;
  d2 = 2 * M_PI - fabs(d1);
  if (d1 > 0.0) d2 *= -1.0;
  if (fabs(d1) < fabs(d2)) return d1;
  return d2;
}

Goto::Goto(ConfigFile* cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  int i;

  srand(time(NULL));
  this->stopping_time = 0.0;
  this->stall_start_time = 0.0;
  this->stall_length = 0.0;
  this->stall_turn = 0.0;
  this->dist_tol = DEFAULT_DIST_TOL;
  this->angle_tol = DEFAULT_ANGLE_TOL;
  this->max_dist = 0.0;
  this->debug = 0;
  this->reactive = 0;
  this->reaction_turn_vel = 0.0;
  this->reaction_time = 0.0;
  this->forward_enabled = 0;
  this->early_check = 0;
  this->send_everything = 0;
  this->enabled = 0;
  this->stopping = 0;
  this->last_dir = 0;
  this->stall_state = 0;
  this->skip = 0;
  this->position2d_required_dev = NULL;
  memset(&(this->position2d_required_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->position2d_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->prev_pos_data), 0, sizeof(this->prev_pos_data));
  this->prev_pos_data_valid = false;
  memset(&(this->rq_hdrs), 0, sizeof this->rq_hdrs);
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    this->payloads[i] = NULL;
    this->rq[i] = 0;
  }
  this->last_rq = -1;
  if (cf->ReadDeviceAddr(&(this->position2d_provided_addr), section, "provides",
                         PLAYER_POSITION2D_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->position2d_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->dio_provided_addr), section, "provides",
                         PLAYER_DIO_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->dio_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->position2d_required_addr), section, "requires",
                         PLAYER_POSITION2D_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  this->dist_tol = cf->ReadFloat(section, "dist_tol", DEFAULT_DIST_TOL);
  if ((this->dist_tol) < 0.0)
  {
    PLAYER_ERROR1("Invalid dist_tol %.4f", this->dist_tol);
    this->SetError(-1);
    return;
  }
  this->angle_tol = cf->ReadFloat(section, "angle_tol", DEFAULT_ANGLE_TOL);
  if ((this->angle_tol) < 0.0)
  {
    PLAYER_ERROR1("Invalid angle_tol %.4f", this->angle_tol);
    this->SetError(-1);
    return;
  }
  this->max_dist = cf->ReadFloat(section, "max_dist", 0.5);
  if ((this->max_dist) < 0.0)
  {
    PLAYER_ERROR1("Invalid max_dist %.4f", this->max_dist);
    this->SetError(-1);
    return;
  }
  if ((this->dist_tol) >= (this->max_dist))
  {
    PLAYER_ERROR("dist_tol should not be greater or equal to max_dist");
    this->SetError(-1);
    return;
  }
  this->debug = cf->ReadInt(section, "debug", 0);
  this->reactive = cf->ReadInt(section, "reactive", 0);
  switch (this->reactive)
  {
  case 0:
  case 1:
  case 2:
    break;
  default:
    PLAYER_ERROR1("Invalid reactive value %d", this->reactive);
    this->SetError(-1);
    return;
  }
  this->reaction_turn_vel = cf->ReadFloat(section, "reaction_turn_vel", 0.0);
  this->reaction_time = cf->ReadInt(section, "reaction_time", 1.5);
  if ((this->reaction_time) < 0.0)
  {
    PLAYER_ERROR1("Invalid reaction_time %.4f", this->reaction_time);
    this->SetError(-1);
    return;
  }
  this->forward_enabled = cf->ReadInt(section, "forward_enabled", 0);
  this->early_check = cf->ReadInt(section, "early_check", 1);
  this->send_everything = cf->ReadInt(section, "send_everything", 1);

  this->max_vel = cf->ReadFloat(section, "max_vel", TVMAX);
  if ((this->max_vel) < 0.0)
  {
    PLAYER_ERROR1("Invalid max_vel %.4f", this->max_vel);
    this->SetError(-1);
    return;
  }

  this->min_vel = cf->ReadFloat(section, "min_vel", TVMIN);
  if ((this->min_vel) < 0.0)
  {
    PLAYER_ERROR1("Invalid min_vel %.4f", this->min_vel);
    this->SetError(-1);
    return;
  }

  this->max_angular_vel = cf->ReadFloat(section, "max_angular_vel", AVMAX);
  if ((this->max_angular_vel) < 0.0)
  {
    PLAYER_ERROR1("Invalid max_angular_vel %.4f", this->max_angular_vel);
    this->SetError(-1);
    return;
  }

  this->min_angular_vel = cf->ReadFloat(section, "min_angular_vel", AVMIN);
  if ((this->min_angular_vel) < 0.0)
  {
    PLAYER_ERROR1("Invalid min_angular_vel %.4f", this->min_angular_vel);
    this->SetError(-1);
    return;
  }
}

Goto::~Goto()
{
  int i;

  for (i = 0; i < RQ_QUEUE_LEN; i++) if (this->payloads[i])
  {
    free(this->payloads[i]);
    this->payloads[i] = NULL;
  }
}

int Goto::Setup()
{
  int i;

  this->position2d_required_dev = NULL;
  memset(&(this->prev_pos_data), 0, sizeof(this->prev_pos_data));
  this->prev_pos_data_valid = false;
  memset(&(this->rq_hdrs), 0, sizeof this->rq_hdrs);
  for (i = 0; i < RQ_QUEUE_LEN; i++)
  {
    this->payloads[i] = NULL;
    this->rq[i] = 0;
  }
  this->last_rq = -1;
  // Only for driver that provides the same interface as it requires
  if (Device::MatchDeviceAddress(this->position2d_required_addr, this->position2d_provided_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return -1;
  }
  this->position2d_required_dev = deviceTable->GetDevice(this->position2d_required_addr);
  if (!(this->position2d_required_dev))
  {
    PLAYER_ERROR("unable to locate suitable position2d device");
    return -1;
  }
  if (this->position2d_required_dev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to position2d device");
    this->position2d_required_dev = NULL;
    return -1;
  }
  this->stall_start_time = 0.0;
  this->stall_length = 0.0;
  this->stall_turn = 0.0;
  this->stall_state = 0;
  this->skip = 0;
  this->enabled = 0;
  this->stopping = !0;
  this->last_dir = 0;
  GlobalTime->GetTimeDouble(&(this->stopping_time));
  return 0;
}

int Goto::Shutdown()
{
  int i;

  if (this->position2d_required_dev) this->position2d_required_dev->Unsubscribe(this->InQueue);
  this->position2d_required_dev = NULL;
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

int Goto::ProcessMessage(QueuePointer &resp_queue,
                                player_msghdr * hdr,
                                void * data)
{
  player_dio_data_t dio_data;
  player_position2d_cmd_vel_t vel_cmd;
  player_position2d_data_t pos_data;
  player_msghdr_t newhdr;
  QueuePointer null;
  double t;
  double newtx, newty;
  double dist;
  double ad, av, tv;
  int i;
  int n;

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_POS, this->position2d_provided_addr))
  {
    if (!data) return -1;
    this->position2d_cmd_pos = *(reinterpret_cast<player_position2d_cmd_pos_t *>(data));
    if (this->debug) PLAYER_WARN3("position command: px = %.4f, py = %.4f pa = %.4f", this->position2d_cmd_pos.pos.px, this->position2d_cmd_pos.pos.py, this->position2d_cmd_pos.pos.pa);
    if ((this->early_check) && (this->prev_pos_data_valid))
    {
      dist = sqrt((((this->position2d_cmd_pos.pos.px) - (this->prev_pos_data.pos.px)) * \
                    ((this->position2d_cmd_pos.pos.px) - (this->prev_pos_data.pos.px))) + \
                    (((this->position2d_cmd_pos.pos.py) - (this->prev_pos_data.pos.py)) * \
                    ((this->position2d_cmd_pos.pos.py) - (this->prev_pos_data.pos.py))));
      if (!(dist >= (this->dist_tol)))
      {
        ad = Goto::angle_diff(this->prev_pos_data.pos.pa, Goto::angle_map(this->position2d_cmd_pos.pos.pa));
        if (!(fabs(ad) > DTOR(this->angle_tol)))
        {
          if (this->debug) PLAYER_WARN6("==> (early check) at target: px = %.4f, py = %.4f pa = %.4f; prev pos: px = %.4f, py = %.4f pa = %.4f", this->position2d_cmd_pos.pos.px, this->position2d_cmd_pos.pos.py, this->position2d_cmd_pos.pos.pa, this->prev_pos_data.pos.px, this->prev_pos_data.pos.py, this->prev_pos_data.pos.pa);
          this->stopping = !0;
          GlobalTime->GetTimeDouble(&(this->stopping_time));
          return 0;
        }
      }
    }
    this->enabled = !0;
    this->stopping = 0;
    this->last_dir = 0;
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, this->position2d_provided_addr))
  {
    if (!data) return -1;
    vel_cmd = *(reinterpret_cast<player_position2d_cmd_vel_t *>(data));
    hdr->addr = this->position2d_required_addr;
    if (this->forward_enabled)
    {
      this->position2d_required_dev->PutMsg(this->InQueue, hdr, data);
    }
    if ((this->enabled) && (fabs(vel_cmd.vel.px) < EPS) && (fabs(vel_cmd.vel.py) < EPS) && (fabs(vel_cmd.vel.pa) < EPS))
    {
      if (this->debug) PLAYER_WARN3("STOP COMMAND while going to target: px = %.4f, py = %.4f pa = %.4f", this->position2d_cmd_pos.pos.px, this->position2d_cmd_pos.pos.py, this->position2d_cmd_pos.pos.pa);
      this->enabled = 0;
      this->stopping = !0;
      GlobalTime->GetTimeDouble(&(this->stopping_time));
    }
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->position2d_provided_addr))
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
      newhdr.addr = this->position2d_required_addr;
      if ((newhdr.size) > 0) assert(this->payloads[n]);
      this->position2d_required_dev->PutMsg(this->InQueue, &newhdr, this->payloads[n], true); // copy = true
      this->last_rq = n;
    }
    return 0;
  }
  if ((Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK, -1, this->position2d_required_addr)) || (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_NACK, -1, this->position2d_required_addr)))
  {
    if ((this->last_rq) < 0)
    {
      PLAYER_ERROR("last_rq < 0");
      return -1;
    }
    assert((hdr->subtype) == (this->rq_hdrs[this->last_rq].subtype));
    this->Publish(this->position2d_provided_addr, this->rq_ptrs[this->last_rq], hdr->type, hdr->subtype, data, 0, &(hdr->timestamp), true); // copy = true
    this->rq_ptrs[this->last_rq] = null;
    assert(this->rq[this->last_rq]);
    if (this->payloads[this->last_rq]) free(this->payloads[this->last_rq]);
    this->payloads[this->last_rq] = NULL;
    this->rq[this->last_rq] = 0;
    this->last_rq = -1;
    for (i = 0; i < RQ_QUEUE_LEN; i++) if (this->rq[i])
    {
      newhdr = this->rq_hdrs[i];
      newhdr.addr = this->position2d_required_addr;
      if ((newhdr.size) > 0) assert(this->payloads[i]);
      this->position2d_required_dev->PutMsg(this->InQueue, &newhdr, this->payloads[i], true); // copy = true;
      this->last_rq = i;
      break;
    }
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            PLAYER_POSITION2D_DATA_STATE,
                            this->position2d_required_addr))
  {
    if (!data) return -1;
    pos_data = *(reinterpret_cast<player_position2d_data_t *>(data));
    if ((this->enabled) && (this->prev_pos_data_valid) && (!(this->stall_state)))
    {
      dist = sqrt(((pos_data.pos.px - (this->prev_pos_data.pos.px)) * \
                    (pos_data.pos.px - (this->prev_pos_data.pos.px))) + \
                    ((pos_data.pos.py - (this->prev_pos_data.pos.py)) * \
                    (pos_data.pos.py - (this->prev_pos_data.pos.py))));
      this->prev_pos_data_valid = false;
      if (dist >= (this->max_dist))
      {
        if (this->debug) PLAYER_WARN("position changed too much; STOPPING!");
        GlobalTime->GetTimeDouble(&(this->stopping_time));
        this->stopping = !0;
      }
    }
    this->prev_pos_data = pos_data;
    this->prev_pos_data_valid = true;
    // main routine here:
    memset(&vel_cmd, 0, sizeof vel_cmd);    
    if ((this->enabled) && (!(this->stopping)))
    {
      newtx = this->position2d_cmd_pos.pos.px;
      newty = this->position2d_cmd_pos.pos.py;
      for (i = 0; i < 100; i++)
      {
        dist = sqrt(((pos_data.pos.px - newtx) * \
                    (pos_data.pos.px - newtx)) + \
                    ((pos_data.pos.py - newty) * \
                    (pos_data.pos.py - newty)));
        if (dist <= (this->max_dist)) break;
        newtx = pos_data.pos.px + ((newtx - pos_data.pos.px) / 2.0);
        newty = pos_data.pos.py + ((newty - pos_data.pos.py) / 2.0);
        if ((this->debug) && ((i + 1) == 100)) PLAYER_WARN("100 times divided in half and still nothing?!");
      }
      if (dist >= (this->dist_tol))
      {
        ad = Goto::angle_diff(atan2(newty - (pos_data.pos.py), newtx - (pos_data.pos.px)), pos_data.pos.pa);
        if (fabs(ad) > (DTOR(AMIN) + (dist / (MAXD)) * (DTOR(AMAX) - DTOR(AMIN))))
        {
          tv = 0.0;
        } else
        {
          tv = (this->min_vel) + (dist / ((M_SQRT2) * (MAXD))) * ((this->max_vel) - (this->min_vel));
        }
        av = DTOR(this->min_angular_vel) + (fabs(ad) / (M_PI)) * (DTOR(this->max_angular_vel) - DTOR(this->min_angular_vel));
        if (ad < 0.0) av = -av;
        vel_cmd.vel.px = tv;
        vel_cmd.vel.py = 0.0;
        vel_cmd.vel.pa = av;
        this->last_dir = 0;
      } else
      {
        ad = Goto::angle_diff(pos_data.pos.pa, Goto::angle_map(this->position2d_cmd_pos.pos.pa));
        if (fabs(ad) > DTOR(this->angle_tol))
        {
          switch (this->last_dir)
          {
          case -1:
            av = 0.8;
            break;
          case 0:
            if (ad < 0.0)
            {
              this->last_dir = -1;
              av = 0.8;
            } else
            {
              this->last_dir = 1;
              av = -0.8;
            }
            break;
          case 1:
            av = -0.8;
            break;
          default:
            PLAYER_ERROR("Invalid internal state!");
            return -1;
          }
          if (this->debug) PLAYER_WARN2("angle diff: %.4f, av: %.4f", ad, av);
          vel_cmd.vel.px = 0.0;
          vel_cmd.vel.py = 0.0;
          vel_cmd.vel.pa = av;
        } else
        {
          if (this->debug) PLAYER_WARN6("==> at target: px = %.4f, py = %.4f pa = %.4f; current pos: px = %.4f, py = %.4f pa = %.4f", this->position2d_cmd_pos.pos.px, this->position2d_cmd_pos.pos.py, this->position2d_cmd_pos.pos.pa, pos_data.pos.px, pos_data.pos.py, pos_data.pos.pa);
          GlobalTime->GetTimeDouble(&(this->stopping_time));
          this->stopping = !0;
          this->enabled = 0;
          this->last_dir = 0;
        }
      }
    }
    if ((this->reactive) && (this->enabled) && (!(this->stopping)))
    {
      switch (this->stall_state)
      {
      case 0:
        if (pos_data.stall)
        {
          GlobalTime->GetTimeDouble(&(this->stall_start_time));
          this->stall_length = ((this->reactive) == 1) ? (this->reaction_time) : ((static_cast<double>(rand()) / static_cast<double>(RAND_MAX)) + 0.3);
          this->stall_turn = ((this->reactive) == 1) ? (this->reaction_turn_vel) : (((static_cast<double>(rand()) / static_cast<double>(RAND_MAX)) * 4.0) - 2.0);
          vel_cmd.vel.px = -(this->max_vel);
          vel_cmd.vel.py = 0.0;
          vel_cmd.vel.pa = this->stall_turn;
          this->stall_state++;
        }
        break;
      case 1:
        GlobalTime->GetTimeDouble(&t);
        if ((t - (this->stall_start_time)) >= (this->stall_length))
        {
          this->stall_start_time = t;
          vel_cmd.vel.px = ((this->max_vel) * 0.65);
          vel_cmd.vel.py = 0.0;
          vel_cmd.vel.pa = 0.0;
          this->stall_state++;
        } else
        {
          vel_cmd.vel.px = -(this->max_vel);
          vel_cmd.vel.py = 0.0;
          vel_cmd.vel.pa = this->stall_turn;
        }
        break;
      case 2:
        GlobalTime->GetTimeDouble(&t);
        if ((t - (this->stall_start_time)) >= ((this->stall_length) / 3.0))
        {
          this->stall_start_time = t;
          vel_cmd.vel.px = 0.0;
          vel_cmd.vel.py = 0.0;
          vel_cmd.vel.pa = 0.0;
          this->stall_state++;
        } else
        {
          vel_cmd.vel.px = ((this->max_vel) * 0.65);
          vel_cmd.vel.py = 0.0;
          vel_cmd.vel.pa = 0.0;
        }
        break;
      case 3:
        GlobalTime->GetTimeDouble(&t);
        if ((t - (this->stall_start_time)) >= 0.4)
        {
          this->stall_state = 0;
          GlobalTime->GetTimeDouble(&(this->stopping_time));
          this->stopping = !0; // will never go back to this->stall_state switch until it's fully stopped
        } else
        {
          vel_cmd.vel.px = 0.0;
          vel_cmd.vel.py = 0.0;
          vel_cmd.vel.pa = 0.0;
        }
        break;
      default:
        PLAYER_ERROR("Invalid stall_state");
        return -1;
      }
    }
    if (this->stopping)
    {
      vel_cmd.vel.px = 0.0;
      vel_cmd.vel.py = 0.0;
      vel_cmd.vel.pa = 0.0;
      GlobalTime->GetTimeDouble(&t);
      if ((t - (this->stopping_time)) >= 1.5) (this->stopping) = 0;
    }
    if (this->send_everything) this->skip = 0;
    switch (this->skip)
    {
    case 0:
      if ((this->stopping) || (this->enabled))
      {
        vel_cmd.state = !0;
        this->position2d_required_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, reinterpret_cast<void *>(&vel_cmd), 0, NULL);
        if (!(this->send_everything))
        {
          this->skip = 1;
          break;
        }
      }
    case 1:
      if (this->reactive) pos_data.stall = 0; // let's manage stall state with reactive manner
      this->Publish(this->position2d_provided_addr,
                    PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
                    reinterpret_cast<void *>(&pos_data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
      if (!(this->send_everything))
      {
        this->skip = 2;
        break;
      }
    case 2:
      memset(&dio_data, 0, sizeof dio_data);
      dio_data.count = 1;
      dio_data.bits |= ((this->enabled) || (this->stopping)) ? 1 : 0;
      this->Publish(this->dio_provided_addr, PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                    reinterpret_cast<void *>(&dio_data), 0, NULL, true); // copy = true
      this->skip = 0;
      break;
    default:
      PLAYER_ERROR("Internal error: invalid switch value");
      return -1;
    }
    return 0;
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * Goto_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new Goto(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void goto_Register(DriverTable * table)
{
  table->AddDriver("goto", Goto_Init);
}
