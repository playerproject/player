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

#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <netinet/in.h>

#include "player.h"
#include "playertime.h"
#include "device.h"
#include "drivertable.h"
#include "devicetable.h"
#include "plan.h"

extern PlayerTime* GlobalTime;

// TODO: monitor localize timestamps, and slow or stop robot accordingly

// time to sleep between loops (us)
#define CYCLE_TIME_US 100000
// number of past poses to use when low-pass filtering localize data
#define LOCALIZE_WINDOW_SIZE 20
// skip poses that are more than this far away from the current window avg
// (meters)
#define LOCALIZE_WINDOW_EPSILON 3.0
// if localize gets more than this far behind, stop the robot to let it
// catch up (seconds)
#define LOCALIZE_MAX_LAG 2.0

class Wavefront : public CDevice
{
  private: 
    // Main function for device thread.
    virtual void Main();

    // bookkeeping
    int position_index;
    int localize_index;
    double map_res;
    double robot_radius;
    double safety_dist;
    double max_radius;
    double dist_penalty;
    double dist_eps;
    double ang_eps;
    const char* map_fname;
    const char* cspace_fname;

    // for filtering localize poses
    double lx_window[LOCALIZE_WINDOW_SIZE];
    double ly_window[LOCALIZE_WINDOW_SIZE];
    int l_window_size;
    int l_window_ptr;

    // for monitoring localize timestamps
    double l_lag;

    // the plan object
    plan_t* plan;

    // pointers to the underlying devices
    CDevice* position;
    CDevice* localize;

    // current target (m,m,rad)
    double target_x, target_y, target_a;
    // current waypoint (m,m,rad)
    double waypoint_x, waypoint_y;
    // are we pursuing a new goal?
    bool new_goal;
    // current odom pose
    double position_x, position_y, position_a;
    // current localize pose
    double localize_x, localize_y, localize_a;

    // methods for internal use
    int SetupLocalize();
    int SetupPosition();
    int ShutdownPosition();
    int ShutdownLocalize();

    void GetCommand();
    void GetLocalizeData();
    void GetPositionData();
    void PutPositionCommand(double x, double y, double a);
    void StopPosition();
    void LocalizeToPosition(double* px, double* py, double* pa,
                            double lx, double ly, double la);

  public:
    // Constructor
    Wavefront(char* interface, ConfigFile* cf, int section);

    // Setup/shutdown routines.
    virtual int Setup();
    virtual int Shutdown();
};


// Initialization function
CDevice* Wavefront_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_POSITION_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"wavefront\" does not support interface \"%s\"\n",
                  interface);
    return (NULL);
  }
  return ((CDevice*) (new Wavefront(interface, cf, section)));
}


// a driver registration function
void Wavefront_Register(DriverTable* table)
{
  table->AddDriver("wavefront", PLAYER_ALL_MODE, Wavefront_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
Wavefront::Wavefront(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_position_data_t), sizeof(player_position_cmd_t), 
              1, 1)
{
  this->position_index = cf->ReadInt(section,"position_index",-1);
  this->localize_index = cf->ReadInt(section,"localize_index",-1);
  this->map_fname = cf->ReadString(section,"map_filename",NULL);
  this->map_res = cf->ReadFloat(section,"map_scale",-1.0);
  this->cspace_fname = cf->ReadString(section,"cspace_filename",NULL);
  this->robot_radius = cf->ReadLength(section,"robot_radius",0.15);
  this->safety_dist = cf->ReadLength(section,"safety_dist", this->robot_radius);
  this->max_radius = cf->ReadLength(section,"max_radius",1.0);
  this->dist_penalty = cf->ReadFloat(section,"dist_penalty",1.0);
  this->dist_eps = cf->ReadLength(section,"distance_epsilon", 
                                 3*this->robot_radius);
  this->ang_eps = cf->ReadAngle(section,"angle_epsilon",DTOR(10));
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int Wavefront::Setup()
{
  player_position_cmd_t cmd;
  player_position_data_t data;

  memset(&cmd,0,sizeof(cmd));
  memset(&data,0,sizeof(data));
  PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));
  PutData((unsigned char*)&data,sizeof(data),0,0);

  this->target_x = this->target_y = this->target_a = 0.0;
  this->position_x = this->position_y = this->position_a = 0.0;
  this->localize_x = this->localize_y = this->localize_a = 0.0;
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
  if(this->position_index == this->device_id.index)
  {
    PLAYER_ERROR("must specify *different* position index");
    return(-1);
  }
  if(this->localize_index < 0)
  {
    PLAYER_ERROR("must specify localize index");
    return(-1);
  }
  if(this->map_res < 0)
  {
    PLAYER_ERROR("must specify map resolution");
    return(-1);
  }
  if(!(this->map_fname))
  {
    PLAYER_ERROR("must specify map");
    return(-1);
  }

  if(SetupPosition() < 0)
    return(-1);
  if(SetupLocalize() < 0)
    return(-1);

  if(!(this->plan = plan_alloc(this->robot_radius,
                               this->robot_radius+this->safety_dist,
                               this->max_radius,
                               this->dist_penalty)))
  {
    PLAYER_ERROR("failed to allocate plan");
    return(-1);
  }

  printf("Wavefront: Loading map from file \"%s\"...", this->map_fname);
  fflush(NULL);
  if(plan_load_occ(this->plan,this->map_fname,this->map_res) < 0)
  {
    PLAYER_ERROR("failed to load map");
    plan_free(this->plan);
    return(-1);
  }
  puts("done.");

  if(this->cspace_fname)
  {
    printf("Wavefront: Loading C-space from file \"%s\"...", this->cspace_fname);
    fflush(NULL);
    if(plan_read_cspace(this->plan,this->cspace_fname) < 0)
    {
      PLAYER_ERROR("failed to load C-space");
      plan_free(this->plan);
      return(-1);
    }
    puts("done.");
  }
  else
  {
    printf("Wavefront: Generating C-space...");
    fflush(NULL);
    plan_update_cspace(this->plan);
    puts("done.");
  }

  // Start the driver thread.
  this->StartThread();
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int Wavefront::Shutdown()
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
  player_position_cmd_t cmd;
  double new_x, new_y, new_a;
  double eps = 0.001;

  CDevice::GetCommand((unsigned char*)&cmd,sizeof(cmd));

  new_x = ((int)ntohl(cmd.xpos))/1e3;
  new_y = ((int)ntohl(cmd.ypos))/1e3;
  new_a = DTOR(ntohl(cmd.yaw));
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
  unsigned int timesec, timeusec;
  static unsigned int last_timesec = 0;
  static unsigned int last_timeusec = 0;
  double lx,ly,la;
  double dist;
  double lx_sum, ly_sum;
  double lx_avg, ly_avg;
  struct timeval curr;

  if(!this->localize->GetData(this,(unsigned char*)&data,sizeof(data),
                              &timesec, &timeusec) || !data.hypoth_count)
    return;

  // is this new data?
  /*
  if((last_timesec == timesec) && (last_timeusec == timeusec))
    return;

  last_timesec = timesec;
  last_timeusec = timeusec;

  if(GlobalTime->GetTime(&curr) < 0)
    PLAYER_ERROR("GetTime() failed!");

  this->l_lag = (curr.tv_sec + curr.tv_usec / 1e6) - 
          (timesec + timeusec / 1e6);
          */

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
  unsigned int timesec, timeusec;

  if(!this->position->GetData(this,(unsigned char*)&data,sizeof(data),
                              &timesec, &timeusec))
    return;
  
  this->position_x = ((int)ntohl(data.xpos))/1e3;
  this->position_y = ((int)ntohl(data.ypos))/1e3;
  this->position_a = DTOR(ntohl(data.yaw));
  //printf("GetPositionData: %f, %f, %f\n",
         //position_x,position_y,RTOD(position_a));
}

void
Wavefront::PutPositionCommand(double x, double y, double a)
{
  player_position_cmd_t cmd;

  memset(&cmd,0,sizeof(cmd));

  cmd.xpos = htonl((int)rint(x*1e3));
  cmd.ypos = htonl((int)rint(y*1e3));
  cmd.yaw = htonl((int)rint(RTOD(a)));

  this->position->PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));
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
  puts("stopping robot");
  PutPositionCommand(this->position_x,this->position_y,this->position_a);
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Wavefront::Main() 
{
  int curr_waypoint;
  double dist;
  double wx_odom, wy_odom, wa_odom;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

  // block until we get initial data from underlying devices
  this->position->Wait();
  GetPositionData();
  this->localize->Wait();
  GetLocalizeData();
  StopPosition();

  curr_waypoint = -1;
  for(;;)
  {
    pthread_testcancel();

    GetPositionData();
    GetLocalizeData();
    GetCommand();

    // if localize gets too far behind, stop the robot to let it catch up
/*
    if(this->l_lag > LOCALIZE_MAX_LAG)
    {
      PLAYER_WARN("stopping robot to let localize catch up");
      printf("lag: %f\n", this->l_lag);
      StopPosition();
      usleep(CYCLE_TIME_US);
      continue;
    }
*/

    if(this->new_goal)
    {
      // compute costs to the new goal
      plan_update_plan(this->plan, this->target_x, this->target_y);
      
      // compute a path to the goal from the current position
      plan_update_waypoints(this->plan, this->localize_x, this->localize_y);

      curr_waypoint = 0;
      if(!plan_get_waypoint(this->plan,curr_waypoint++,
                            &this->waypoint_x,&this->waypoint_y))
      {
        PLAYER_WARN("no waypoints!");
        curr_waypoint = -1;
      }
      else
      {
        // transform to odometric frame
        // TODO: deal with angle
        LocalizeToPosition(&wx_odom, &wy_odom, &wa_odom, 
                           this->waypoint_x, this->waypoint_y, 0.0);
        //printf("w_odom: %f, %f, %f\n", wx_odom, wy_odom, RTOD(wa_odom));
        // hand down next waypoint
        PutPositionCommand(wx_odom, wy_odom, wa_odom);
      }

      double wx,wy;
      for(int i=0;plan_get_waypoint(this->plan,i++,&wx,&wy);)
        printf("waypoint %d: %f,%f\n", i, wx, wy);
      
      this->new_goal = false;
    }
      
    // TODO: check angle
    dist = sqrt(((this->localize_x - this->target_x) *
                 (this->localize_x - this->target_x)) +
                ((this->localize_y - this->target_y) *
                 (this->localize_y - this->target_y)));
    //printf("distance to goal: %f\n", dist);
    if(dist < this->dist_eps)
    {
      // we're at the final target, so stop
      StopPosition();
      curr_waypoint = -1;
    }
    else if(curr_waypoint < 0)
    {
      // no more waypoints, so stop
      StopPosition();
    }
    else
    {
      // are we there yet?
      dist = sqrt(((this->localize_x - this->waypoint_x) * 
                   (this->localize_x - this->waypoint_x)) +
                  ((this->localize_y - this->waypoint_y) *
                   (this->localize_y - this->waypoint_y)));
      //printf("distance to next waypoint: %f\n", dist);
      // TODO: check angle
      if(dist < this->dist_eps)
      {
        // get next waypoint
        if(!plan_get_waypoint(this->plan,curr_waypoint++,
                              &this->waypoint_x,&this->waypoint_y))
        {
          // no more waypoints, so stop
          StopPosition();
          curr_waypoint = -1;
          usleep(CYCLE_TIME_US);
          continue;
        }
      }
      //printf("waypoint: %f, %f\n", this->waypoint_x,this->waypoint_y);
      // transform to odometric frame
      // TODO: deal with angle
      LocalizeToPosition(&wx_odom, &wy_odom, &wa_odom, 
                         this->waypoint_x, this->waypoint_y, 0.0);
      //printf("w_odom: %f, %f, %f\n", wx_odom, wy_odom, wa_odom);
      // hand down next waypoint
      PutPositionCommand(wx_odom, wy_odom, wa_odom);
    }

    usleep(CYCLE_TIME_US);
  }
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying position device.
int 
Wavefront::SetupPosition()
{
  player_device_id_t id;

  id.code = PLAYER_POSITION_CODE;
  id.index = this->position_index;
  id.port = this->device_id.port;

  // Subscribe to the position device.
  if(!(this->position = deviceTable->GetDevice(id)))
  {
    PLAYER_ERROR("unable to locate suitable position device");
    return(-1);
  }
  if(this->position->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to position device");
    return(-1);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the underlying localize device.
int 
Wavefront::SetupLocalize()
{
  player_device_id_t id;

  id.code = PLAYER_LOCALIZE_CODE;
  id.index = this->localize_index;
  id.port = this->device_id.port;

  // Subscribe to the localize device.
  if(!(this->localize = deviceTable->GetDevice(id)))
  {
    PLAYER_ERROR("unable to locate suitable localize device");
    return(-1);
  }
  if(this->localize->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to localize device");
    return(-1);
  }

  return 0;
}

int 
Wavefront::ShutdownPosition()
{
  return(this->position->Unsubscribe(this));
}

int 
Wavefront::ShutdownLocalize()
{
  return(this->localize->Unsubscribe(this));
}

