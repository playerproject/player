/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  Brian Gerkey   gerkey@robotics.stanford.edu
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

// TODO:
//   - allow a waypoint to be skipped, if the robot gets near a later
//      waypoint
//
//  - allow for computing a path, without actually executing it.
//
//  - compute and return path length

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <netinet/in.h>
#include <playertime.h>

#include "player.h"
#include "driver.h"
#include "drivertable.h"
#include "devicetable.h"
#include "plan.h"

// TODO: monitor localize timestamps, and slow or stop robot accordingly

// time to sleep between loops (us)
#define CYCLE_TIME_US 50000
// number of past poses to use when low-pass filtering localize data
#define LOCALIZE_WINDOW_SIZE 20
// skip poses that are more than this far away from the current window avg
// (meters)
#define LOCALIZE_WINDOW_EPSILON 3.0
// if localize gets more than this far behind, stop the robot to let it
// catch up (seconds)
#define LOCALIZE_MAX_LAG 2.0

extern PlayerTime *GlobalTime;

extern int global_playerport;

class Wavefront : public Driver
{
  private: 
    // Main function for device thread.
    virtual void Main();

    bool newData;

    // bookkeeping
    player_device_id_t position_id;
    player_device_id_t localize_id;
    int position_index;
    int localize_index;
    double map_res;
    double robot_radius;
    double safety_dist;
    double max_radius;
    double dist_penalty;
    double dist_eps;
    double ang_eps;
    int map_index;
    //const char* cspace_fname;

    // for filtering localize poses
    double lx_window[LOCALIZE_WINDOW_SIZE];
    double ly_window[LOCALIZE_WINDOW_SIZE];
    int l_window_size;
    int l_window_ptr;

    // the plan object
    plan_t* plan;

    // pointers to the underlying devices
    Driver* position;
    Driver* localize;

    // current target (m,m,rad)
    double target_x, target_y, target_a;
    // index of current waypoint;
    int curr_waypoint;
    // current waypoint (m,m,rad)
    double waypoint_x, waypoint_y, waypoint_a;
    // are we pursuing a new goal?
    bool new_goal;
    // current odom pose
    double position_x, position_y, position_a;
    // current odom velocities, NOT byteswapped or unit converted, because 
    // we're just passing them through and don't need to use them
    //int32_t position_xspeed_be, position_yspeed_be, position_aspeed_be;
    // last timestamp from localize
    unsigned int localize_timesec, localize_timeusec;
    // last timestamp from position
    unsigned int position_timesec, position_timeusec;
    // current localize pose
    double localize_x, localize_y, localize_a;
    // current localize pose, not byteswapped or unit converted, for
    // passing through.
    int32_t localize_x_be, localize_y_be, localize_a_be;
    bool stopped;

    // methods for internal use
    int SetupLocalize();
    int SetupPosition();
    int SetupMap();
    int ShutdownPosition();
    int ShutdownLocalize();

    void GetCommand();
    void GetLocalizeData();
    void GetPositionData();
    void PutPositionCommand(double x, double y, double a);
    void PutPlannerData();
    void StopPosition();
    void LocalizeToPosition(double* px, double* py, double* pa,
                            double lx, double ly, double la);
    void SetWaypoint(double wx, double wy, double wa);

  public:
    // Constructor
    Wavefront( ConfigFile* cf, int section);

    // Setup/shutdown routines.
    virtual int Setup();
    virtual int Shutdown();

    int PutConfig(player_device_id_t id, void *client, 
                  void* src, size_t len,
                  struct timeval* timestamp);
};


// Initialization function
Driver* Wavefront_Init( ConfigFile* cf, int section)
{
  return ((Driver*) (new Wavefront( cf, section)));
}


// a driver registration function
void Wavefront_Register(DriverTable* table)
{
  table->AddDriver("wavefront",  Wavefront_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
Wavefront::Wavefront( ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_PLANNER_CODE, PLAYER_ALL_MODE,
             sizeof(player_planner_data_t), sizeof(player_planner_cmd_t), 1, 1)
{
  this->position_index = cf->ReadInt(section,"position_index",-1);
  this->localize_index = cf->ReadInt(section,"localize_index",-1);
  this->map_index = cf->ReadInt(section,"map_index",-1);
  //this->cspace_fname = cf->ReadString(section,"cspace_filename",NULL);
  //this->robot_radius = cf->ReadLength(section,"robot_radius",0.15);
  this->safety_dist = cf->ReadLength(section,"safety_dist", 0.25);
  this->max_radius = cf->ReadLength(section,"max_radius",1.0);
  this->dist_penalty = cf->ReadFloat(section,"dist_penalty",1.0);
  this->dist_eps = cf->ReadLength(section,"distance_epsilon", 0.5);
  this->ang_eps = cf->ReadAngle(section,"angle_epsilon",DTOR(10));
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int 
Wavefront::Setup()
{
  player_planner_cmd_t cmd;
  player_planner_data_t data;

  memset(&cmd,0,sizeof(cmd));
  memset(&data,0,sizeof(data));
  PutCommand(this->device_id,(unsigned char*)&cmd,sizeof(cmd),NULL);
  PutData((unsigned char*)&data,sizeof(data),NULL);

  this->stopped = true;
  this->target_x = this->target_y = this->target_a = 0.0;
  this->position_x = this->position_y = this->position_a = 0.0;
  this->localize_x = this->localize_y = this->localize_a = 0.0;
  this->waypoint_x = this->waypoint_y = this->waypoint_a = 0.0;

  this->newData = false;

  /*
  this->position_xspeed_be = 
          this->position_yspeed_be = 
          this->position_aspeed_be = 0;
          */
  this->localize_x_be = this->localize_y_be = this->localize_a_be = 0;
  this->localize_timesec = this->localize_timeusec = 0;
  this->position_timesec = this->position_timeusec = 0;
  this->new_goal = false;
  memset(this->lx_window,0,sizeof(this->lx_window));
  memset(this->ly_window,0,sizeof(this->ly_window));
  this->l_window_size = 0;
  this->l_window_ptr = 0;

  if(this->position_index < 0)
  {
    PLAYER_ERROR("must specify position index");
    return(-1);
  }
  if(this->localize_index < 0)
  {
    PLAYER_ERROR("must specify localize index");
    return(-1);
  }
  if(this->map_index < 0)
  {
    PLAYER_ERROR("must specify map index");
    return(-1);
  }

  if(SetupPosition() < 0)
    return(-1);
  if(SetupMap() < 0)
    return(-1);
  if(SetupLocalize() < 0)
    return(-1);

  // Start the driver thread.
  this->StartThread();
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int 
Wavefront::Shutdown()
{
  // Stop the driver thread.
  this->StopThread();

  if(this->plan)
    plan_free(this->plan);

  ShutdownPosition();
  ShutdownLocalize();

  return 0;
}

void
Wavefront::GetCommand()
{
  player_planner_cmd_t cmd;
  double new_x, new_y, new_a;
  double eps = 1e-3;

  Driver::GetCommand((unsigned char*)&cmd,sizeof(cmd),NULL);

  new_x = ((int)ntohl(cmd.gx))/1e3;
  new_y = ((int)ntohl(cmd.gy))/1e3;
  new_a = DTOR((int)ntohl(cmd.ga));
  if((fabs(new_x - this->target_x) > eps) ||
     (fabs(new_y - this->target_y) > eps) ||
     (fabs(NORMALIZE(new_a - this->target_a)) > eps))
  {
    this->target_x = new_x;
    this->target_y = new_y;
    this->target_a = new_a;
    //printf("new goal: %f, %f, %f\n", target_x, target_y, target_a);
    this->new_goal = true;
  }
}

void
Wavefront::GetLocalizeData()
{
  player_localize_data_t data;
  struct timeval timestamp;
  double lx,ly,la;
  double dist;
  double lx_sum, ly_sum;
  double lx_avg, ly_avg;
  double la_tmp;
  //struct timeval curr;

  memset(&data,0,sizeof(data));
  if(!this->localize->GetData(this->localize_id,(void*)&data,sizeof(data),
                              &timestamp) || !data.hypoth_count)
    return;

  // is this new data?
  if((this->localize_timesec == timestamp.tv_sec) && 
     (this->localize_timeusec == timestamp.tv_usec))
    return;

  this->localize_timesec = timestamp.tv_sec;
  this->localize_timeusec = timestamp.tv_usec;

  // just take the first hypothesis, on the assumption that it's the
  // highest weight.
  lx = ((int)ntohl(data.hypoths[0].mean[0]))/1e3;
  ly = ((int)ntohl(data.hypoths[0].mean[1]))/1e3;
  la = DTOR((int)ntohl(data.hypoths[0].mean[2])/3600.0);
  //printf("GetLocalizeData: %f, %f, %f\n",
         //localize_x,localize_y,RTOD(localize_a));

  // is the filter window full yet?
  if(this->l_window_size >= LOCALIZE_WINDOW_SIZE)
  {
    // compute window averages
    lx_sum = ly_sum = 0.0;
    for(int i=0;i<this->l_window_size;i++)
    {
      lx_sum += this->lx_window[i];
      ly_sum += this->ly_window[i];
    }
    // compute distance of new pose from window average
    lx_avg = lx_sum / (double)this->l_window_size;
    ly_avg = ly_sum / (double)this->l_window_size;
    dist = sqrt((lx-lx_avg)*(lx-lx_avg) + (ly-ly_avg)*(ly-ly_avg));
  }
  else
  {
    // filter window isn't full yet; just take this pose
    dist = 0.0;
  }

  // should we use this pose?
  if(dist < LOCALIZE_WINDOW_EPSILON)
  {
    this->localize_x = lx;
    this->localize_y = ly;
    this->localize_a = la;

    // also store it un-byteswapped
    this->localize_x_be = data.hypoths[0].mean[0];
    this->localize_y_be = data.hypoths[0].mean[1];
    la_tmp = NORMALIZE(la);
    if(la_tmp < 0)
      la_tmp += 2*M_PI;
    this->localize_a_be = (int32_t)htonl((int)rint(RTOD(la_tmp)));
  }
  else
    PLAYER_WARN3("discarding pose %f,%f,%f", lx,ly,la);

  // regardless, add it to the running window sum
  this->lx_window[this->l_window_ptr] = lx;
  this->ly_window[this->l_window_ptr] = ly;
  if(this->l_window_size < LOCALIZE_WINDOW_SIZE)
    this->l_window_size++;
  this->l_window_ptr = (this->l_window_ptr + 1) % LOCALIZE_WINDOW_SIZE;
}

void
Wavefront::GetPositionData()
{
  player_position_data_t data;
  struct timeval timestamp;

  if(!this->position->GetData(this->position_id,(void*)&data,sizeof(data),
                              &timestamp))
    return;

  // is this new data?
  if((this->position_timesec == timestamp.tv_sec) && 
     (this->position_timeusec == timestamp.tv_usec))
    return;

  this->position_timesec = timestamp.tv_sec;
  this->position_timeusec = timestamp.tv_usec;

  this->position_x = ((int)ntohl(data.xpos))/1e3;
  this->position_y = ((int)ntohl(data.ypos))/1e3;
  this->position_a = DTOR((int)(ntohl(data.yaw)));
  
  // current odom velocities are NOT byteswapped or unit converted, because 
  // we're just passing them through and don't need to use them
  /*
  this->position_xspeed_be = data.xspeed;
  this->position_yspeed_be = data.yspeed;
  this->position_aspeed_be = data.yawspeed;
  */
}

void
Wavefront::PutPlannerData()
{
  player_planner_data_t data;
  
  memset(&data,0,sizeof(data));

  if(this->plan->waypoint_count > 0)
    data.valid = 1;
  else
    data.valid = 0;

  if((this->plan->waypoint_count > 0) && (this->curr_waypoint < 0))
    data.done = 1;
  else
    data.done = 0;

  // put the current localize pose (these fields are already big-endian)
  data.px = this->localize_x_be;
  data.py = this->localize_y_be;
  data.pa = this->localize_a_be;

  data.gx = htonl((int)rint(this->target_x * 1e3));
  data.gy = htonl((int)rint(this->target_y * 1e3));
  data.ga = htonl((int)rint(this->target_a * 1e3));

  data.wx = htonl((int)rint(this->waypoint_x * 1e3));
  data.wy = htonl((int)rint(this->waypoint_y * 1e3));
  data.wa = htonl((int)rint(RTOD(this->waypoint_a)));

  data.curr_waypoint = (short)htons(this->curr_waypoint);
  data.waypoint_count = htons(this->plan->waypoint_count);

  // We should probably send new data even if we haven't moved.
  if (this->newData)
  {
    struct timeval time;
    GlobalTime->GetTime(&time);

    PutData((unsigned char*)&data,sizeof(data),&time);
  } else {
    /* use the localizer's timestamp */
    struct timeval ts;
    ts.tv_sec = this->localize_timesec;
    ts.tv_usec = this->localize_timeusec;
    PutData((unsigned char*)&data,sizeof(data),&ts);
  }
}

void
Wavefront::PutPositionCommand(double x, double y, double a)
{
  player_position_cmd_t cmd;

  memset(&cmd,0,sizeof(cmd));

  cmd.xpos = htonl((int)rint(x*1e3));
  cmd.ypos = htonl((int)rint(y*1e3));
  cmd.yaw = htonl((int)rint(RTOD(a)));
  cmd.type=1;
  cmd.state=1;

  this->position->PutCommand(this->position_id,
                             (unsigned char*)&cmd,sizeof(cmd),NULL);
}

void
Wavefront::LocalizeToPosition(double* px, double* py, double* pa,
                              double lx, double ly, double la)
{
  double offset_x, offset_y, offset_a;
  double lx_rot, ly_rot;

  offset_a = NORMALIZE(this->position_a - this->localize_a);
  lx_rot = this->localize_x * cos(offset_a) - this->localize_y * sin(offset_a);
  ly_rot = this->localize_x * sin(offset_a) + this->localize_y * cos(offset_a);

  offset_x = this->position_x - lx_rot;
  offset_y = this->position_y - ly_rot;

  //printf("offset: %f, %f, %f\n", offset_x, offset_y, RTOD(offset_a));

  *px = lx * cos(offset_a) - ly * sin(offset_a) + offset_x;
  *py = lx * sin(offset_a) + ly * cos(offset_a) + offset_y;
  *pa = la + offset_a;
}

void
Wavefront::StopPosition()
{
  if(!this->stopped)
  {
    //puts("stopping robot");
    PutPositionCommand(this->position_x,this->position_y,this->position_a);
    this->stopped = true;
  }
}

void
Wavefront::SetWaypoint(double wx, double wy, double wa)
{
  double wx_odom, wy_odom, wa_odom;

  // transform to odometric frame
  LocalizeToPosition(&wx_odom, &wy_odom, &wa_odom, wx, wy, wa);
  // hand down waypoint
  PutPositionCommand(wx_odom, wy_odom, wa_odom);
  this->stopped = false;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Wavefront::Main() 
{
  double dist, angle;
  //double lag;
  static bool rotate_waypoint=false;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

  // block until we get initial data from underlying devices
  this->position->Wait();
  GetPositionData();
  
  // HACK!
  // Blocking here means that the planner won't start until the localizer
  // generates a *new* pose estimate, even if the localizer already has
  // converged to a good value.  That's a pain. So we'll try sleeping
  // briefly instead.  Clearly not a good solution.
  
  //this->localize->Wait();
  usleep(2000000);
  GetLocalizeData();
  StopPosition();

  this->curr_waypoint = -1;
  for(;;)
  {
    pthread_testcancel();

    GetLocalizeData();
    GetPositionData();
    PutPlannerData();
    GetCommand();

    this->newData = false;

    if(this->new_goal)
    {
      this->newData = true;

      // compute costs to the new goal
      plan_update_plan(this->plan, this->target_x, this->target_y);

      // compute a path to the goal from the current position
      plan_update_waypoints(this->plan, this->localize_x, this->localize_y);

      if(!this->plan->waypoint_count)
      {
        PLAYER_WARN("no path!");
        this->curr_waypoint = -1;
        this->new_goal=false;
      }
      else if(!plan_get_waypoint(this->plan,0,
            &this->waypoint_x,
            &this->waypoint_y))
      {
        PLAYER_WARN("no waypoints!");
        this->curr_waypoint = -1;
        this->new_goal=false;
      }
      else
      {
        this->curr_waypoint = 0;

        /*
           double wx,wy;
           for(int i=0;plan_get_waypoint(this->plan,i++,&wx,&wy);)
           printf("waypoint %d: %f,%f\n", i, wx, wy);
           */
      }
    }

    dist = sqrt(((this->localize_x - this->target_x) *
          (this->localize_x - this->target_x)) +
        ((this->localize_y - this->target_y) *
         (this->localize_y - this->target_y)));
    angle = fabs(NORMALIZE(this->localize_a - this->target_a));
    if(dist < this->dist_eps && angle < this->ang_eps)
    {
      // we're at the final target, so stop
      StopPosition();
      this->curr_waypoint = -1;
      this->new_goal = false;

      this->newData = true;
    }
    else if(this->curr_waypoint < 0)
    {
      // no more waypoints, so stop
      StopPosition();
    }
    else
    {

      // are we there yet?  ignore angle, cause this is just a waypoint
      dist = sqrt(((this->localize_x - this->waypoint_x) * 
            (this->localize_x - this->waypoint_x)) +
          ((this->localize_y - this->waypoint_y) *
           (this->localize_y - this->waypoint_y)));
      if(this->new_goal ||
          ((dist < this->dist_eps) &&
           (!rotate_waypoint ||
            (fabs(NORMALIZE(this->localize_a - this->waypoint_a))
             < this->ang_eps))))
      {
        this->new_goal = false;


        this->newData = true;
        // get next waypoint
        if(!plan_get_waypoint(this->plan,this->curr_waypoint,
              &this->waypoint_x,&this->waypoint_y))
        {
          // no more waypoints, so wait for target achievement

          //puts("waiting for goal achievement");
          usleep(CYCLE_TIME_US);
          continue;
        }
        this->curr_waypoint++;

        this->waypoint_a = this->target_a;
        dist = sqrt((this->waypoint_x - this->localize_x) *
            (this->waypoint_x - this->localize_x) +
            (this->waypoint_y - this->localize_y) *
            (this->waypoint_y - this->localize_y));
        angle = atan2(this->waypoint_y - this->localize_y, 
            this->waypoint_x - this->localize_x);
        if((dist > this->dist_eps) &&
            fabs(NORMALIZE(angle - this->localize_a)) > M_PI/4.0)
        {
          //puts("adding rotational waypoint");
          this->waypoint_x = this->localize_x;
          this->waypoint_y = this->localize_y;
          this->waypoint_a = angle;
          this->curr_waypoint--;
          rotate_waypoint=true;

        }
        else
          rotate_waypoint=false;

        SetWaypoint(this->waypoint_x, this->waypoint_y, this->waypoint_a);
      }
      if(!rotate_waypoint)
        SetWaypoint(this->waypoint_x, this->waypoint_y, this->waypoint_a);
    }


    usleep(CYCLE_TIME_US);
  }
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying position device.
int 
Wavefront::SetupPosition()
{
  player_position_geom_t geom;
  struct timeval ts;
  unsigned short reptype;

  this->position_id.code = PLAYER_POSITION_CODE;
  this->position_id.index = this->position_index;
  this->position_id.port = this->device_id.port;

  // Subscribe to the position device.
  if(!(this->position = deviceTable->GetDriver(this->position_id)))
  {
    PLAYER_ERROR("unable to locate suitable position device");
    return(-1);
  }
  if(this->position->Subscribe(this->position_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to position device");
    return(-1);
  }

  // Get the robot's geometry, and infer the radius
  geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
  if(this->position->Request(this->position_id, this, 
                             &geom, sizeof(geom.subtype), NULL,
                             &reptype, &geom, sizeof(geom), &ts) <
     (int)sizeof(player_position_geom_t))
  {
    PLAYER_ERROR("failed to get robot geometry");
    return(-1);
  }

  // take the bigger of the two dimensions, convert to meters, and halve 
  // to get a radius
  this->robot_radius = MAX(htons(geom.size[0]), htons(geom.size[1])) / 1e3;
  this->robot_radius /= 2.0;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the underlying localize device.
int 
Wavefront::SetupLocalize()
{
  this->localize_id.code = PLAYER_LOCALIZE_CODE;
  this->localize_id.index = this->localize_index;
  this->localize_id.port = this->device_id.port;

  // Subscribe to the localize device.
  if(!(this->localize = deviceTable->GetDriver(this->localize_id)))
  {
    PLAYER_ERROR("unable to locate suitable localize device");
    return(-1);
  }
  if(this->localize->Subscribe(this->localize_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to localize device");
    return(-1);
  }

  return 0;
}

// setup the underlying map device (i.e., get the map)
// TODO: should Unsubscribe from the map on error returns in the function
int
Wavefront::SetupMap()
{
  player_device_id_t map_id;
  Driver* mapdevice;
  plan_cell_t* cell;

  // Subscribe to the map device
  map_id.port = global_playerport;
  map_id.code = PLAYER_MAP_CODE;
  map_id.index = this->map_index;

  if(!(mapdevice = deviceTable->GetDriver(map_id)))
  {
    PLAYER_ERROR("unable to locate suitable map device");
    return -1;
  }
  if(mapdevice->Subscribe(map_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to map device");
    return -1;
  }

  if(!(this->plan = plan_alloc(this->robot_radius+this->safety_dist,
                               this->robot_radius+this->safety_dist,
                               this->max_radius,
                               this->dist_penalty)))
  {
    PLAYER_ERROR("failed to allocate plan");
    return(-1);
  }

  printf("Wavefront: Loading map from map:%d...\n", this->map_index);
  fflush(NULL);

  // Fill in the map structure (I'm doing it here instead of in libmap, 
  // because libmap is written in C, so it'd be a pain to invoke the internal 
  // device API from there)

  // first, get the map info
  int replen;
  unsigned short reptype;
  player_map_info_t info;
  struct timeval ts;
  info.subtype = PLAYER_MAP_GET_INFO_REQ;
  if((replen = mapdevice->Request(map_id, this, 
                                  &info, sizeof(info.subtype), NULL,
                                  &reptype, &info, sizeof(info),&ts)) <= 0)
  {
    PLAYER_ERROR("failed to get map info");
    return(-1);
  }
  
  // copy in the map info
  this->plan->scale = 1/(ntohl(info.scale) / 1e3);
  this->plan->size_x = ntohl(info.width);
  this->plan->size_y = ntohl(info.height);

  // allocate space for map cells
  assert(this->plan->cells = (plan_cell_t*)malloc(sizeof(plan_cell_t) *
                                                  this->plan->size_x *
                                                  this->plan->size_y));

  // Reset the grid
  plan_reset(this->plan);

  // now, get the map data
  player_map_data_t data_req;
  int reqlen;
  int i,j;
  int oi,oj;
  int sx,sy;
  int si,sj;

  data_req.subtype = PLAYER_MAP_GET_DATA_REQ;
  
  // Tile size
  sy = sx = (int)sqrt(sizeof(data_req.data));
  assert(sx * sy < (int)sizeof(data_req.data));
  oi=oj=0;
  while((oi < this->plan->size_x) && (oj < this->plan->size_y))
  {
    si = MIN(sx, this->plan->size_x - oi);
    sj = MIN(sy, this->plan->size_y - oj);

    data_req.col = htonl(oi);
    data_req.row = htonl(oj);
    data_req.width = htonl(si);
    data_req.height = htonl(sj);

    reqlen = sizeof(data_req) - sizeof(data_req.data);

    if((replen = mapdevice->Request(map_id, this, 
                                    &data_req, reqlen, NULL,
                                    &reptype, &data_req, sizeof(data_req),&ts)) == 0)
    {
      PLAYER_ERROR("failed to get map info");
      return(-1);
    }
    else if(replen != (reqlen + si * sj))
    {
      PLAYER_ERROR2("got less map data than expected (%d != %d)",
                    replen, reqlen + si*sj);
      return(-1);
    }

    // copy the map data
    for(j=0;j<sj;j++)
    {
      for(i=0;i<si;i++)
      {
        cell = this->plan->cells + PLAN_INDEX(this->plan,oi+i,oj+j);
        cell->occ_dist = this->plan->max_radius;
        if((cell->occ_state = data_req.data[j*si + i]) >= 0)
          cell->occ_dist = 0;
      }
    }

    oi += si;
    if(oi >= this->plan->size_x)
    {
      oi = 0;
      oj += sj;
    }
  }

  // we're done with the map device now
  if(mapdevice->Unsubscribe(map_id) != 0)
    PLAYER_WARN("unable to unsubscribe from map device");

  printf("Wavefront: Generating C-space...");
  fflush(NULL);
  plan_update_cspace(this->plan);
  puts("done.");
  return(0);
}



int 
Wavefront::ShutdownPosition()
{
  return(this->position->Unsubscribe(this->position_id));
}

int 
Wavefront::ShutdownLocalize()
{
  return(this->localize->Unsubscribe(this->localize_id));
}

int 
Wavefront::PutConfig(player_device_id_t id, void *client, 
                     void* src, size_t len,
                     struct timeval* timestamp)
{
  player_planner_waypoints_req_t reply;
  double wx,wy;
  size_t replylen;
  
  // TODO: figure out why locking here causes a deadlock.
  //Lock();
  memset(&reply,0,sizeof(player_planner_waypoints_req_t));
  if(len > 0)
  {
    switch(((unsigned char*)src)[0])
    {
      case PLAYER_PLANNER_GET_WAYPOINTS_REQ:
        // return the list of waypoints
        reply.subtype = PLAYER_PLANNER_GET_WAYPOINTS_REQ;
        if(plan->waypoint_count > PLAYER_PLANNER_MAX_WAYPOINTS)
        {
          PLAYER_WARN("too many waypoints; truncating list");
          reply.count = htons((unsigned short)PLAYER_PLANNER_MAX_WAYPOINTS);
        }
        else
          reply.count = htons((unsigned short)plan->waypoint_count);
        for(int i=0;i<(int)ntohs(reply.count);i++)
        {
          if(!plan_get_waypoint(plan, i, &wx, &wy))
          {
            PLAYER_WARN("fewer waypoints than expected!");
            reply.count = htons((unsigned short)(i));
            break;
          }
          reply.waypoints[i].x = htonl((int)rint(wx * 1e3));
          reply.waypoints[i].y = htonl((int)rint(wy * 1e3));
        }
        replylen = sizeof(player_planner_waypoints_req_t) - 
                (PLAYER_PLANNER_MAX_WAYPOINTS - ntohs(reply.count)) * 
                sizeof(player_planner_waypoint_t);

        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, 
                    (void*)&reply, replylen,NULL))
          PLAYER_ERROR("PutReply() failed");
        break;
      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
    }
  }
  //Unlock();
  return(0);
}
