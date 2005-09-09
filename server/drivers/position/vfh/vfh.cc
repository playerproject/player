#include <assert.h>
#include <math.h>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <libplayercore/playercore.h>
#include "vfh_algorithm.h"

extern PlayerTime *GlobalTime;

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_vfh vfh

The vfh driver implements the Vector Field Histogram Plus local
navigation method by Ulrich and Borenstein.  VFH+ provides real-time
obstacle avoidance and path following capabilities for mobile robots.
Layered on top of a laser-equipped robot, vfh works great as a local
navigation system (for global navigation, you can layer the @ref
player_driver_wavefront driver on top of vfh).

The primary parameters to tweak to get reliable performance are
safety_dist and free_space_cutoff.  In general, safety_dist determines how
close the robot will come to an obstacle while turning (around a corner
for instance) and free_space_cutoff determines how close a robot will
get to an obstacle in the direction of motion before turning to avoid.
From experience, it is recommeded that max_turnrate should be at least
15% of max_speed.

To get initiated to VFH, I recommend starting with the default
values for all parameters and experimentally adjusting safety_dist
and free_space_cutoff to get a feeling for how the parameters affect
performance.  Once comfortable, increase max_speed and max_turnrate.
Unless you are familiar with the VFH algorithm, I don't recommend
deviating from the default values for cell_size, window_diameter,
or sector_angle.

@par Compile-time dependencies

- none

@par Provides

- @ref player_interface_position : accepts target poses, to which vfh will
  attempt to drive the robot.  Also passes through the data from the
  underlying @ref player_interface_position device.  All data and commands
  are in the odometric frame of the underlying device.

@par Requires

- @ref player_interface_position : the underlying robot that will be
  controlled by vfh.

- @ref player_interface_laser : the laser that will be used to avoid
  obstacles

- @todo : add support for getting the robot's true global pose via the 
  @ref player_interface_simulation interface

@par Configuration requests

- all position2d requests (as long as the underlying position2d device 
  supports them)

@par Configuration file options

- cell_size (length)
  - Default: 0.1 m
  - Local occupancy map grid size
- window_diameter (integer)
  - Default: 61
  - Dimensions of occupancy map (map consists of window_diameter X
    window_diameter cells).
- sector_angle (integer)
  - Default: 5
  - Histogram angular resolution, in degrees.
- safety_dist (length)
  - Default: 0.1 m
  - The minimum distance the robot is allowed to get to obstacles.
- max_speed (length / sec)
  - Default: 0.2 m/sec
  - The maximum allowable speed of the robot.
- max_acceleration (length / sec / sec)
  - Default: 0.2 m/sec/sec
  - The maximum allowable acceleration of the robot.
- min_turnrate (angle / sec)
  - Default: 10 deg/sec
  - The minimum allowable turnrate of the robot.
- max_turnrate (angle / sec)
  - Default: 40 deg/sec
  - The maximum allowable turnrate of the robot.
- free_space_cutoff (float)
  - Default: 2000000.0
  - Unitless value.  The higher the value, the closer the robot will
    get to obstacles before avoiding.
- obs_cutoff (float)
  - Default: free_space_cutoff
  - ???
- weight_desired_dir (float)
  - Default: 5.0  
  - Bias for the robot to turn to move toward goal position.
- weight_current_dir (float)
  - Default: 3.0
  - Bias for the robot to continue moving in current direction of travel.
- distance_epsilon (length)
  - Default: 0.5 m
  - Planar distance from the target position that will be considered
    acceptable.
- angle_epsilon (angle)
  - Default: 10 deg
  - Angular difference from target angle that will considered acceptable.

@par Example
@verbatim
driver
(
  name "p2os"
  provides ["odometry::position:1"]
  port "/dev/ttyS0"
)
driver
(
  name "sicklms200"
  provides ["laser:0"]
  port "/dev/ttyS1"
)
driver
(
  name "vfh"
  requires ["position:1" "laser:0"]
  provides ["position:0"]
  safety_dist 0.10
  distance_epsilon 0.3
  angle_epsilon 5
)
@endverbatim

@par Authors

Chris Jones, Brian Gerkey, Alex Brooks

*/

/** @} */

class VFH_Class : public Driver 
{
  public:
    // Constructor
    VFH_Class( ConfigFile* cf, int section);

    // Destructor
    virtual ~VFH_Class();

    // Setup/shutdown routines.
    virtual int Setup();
    virtual int Shutdown();

    // Process incoming messages from clients 
    virtual int ProcessMessage(MessageQueue* resp_queue, 
                               player_msghdr * hdr, 
                               void * data);
    // Main function for device thread.
    virtual void Main();

  private:
    bool active_goal;
    bool turninginplace;
    
    // Set up the odometry device.
    int SetupOdom();
    int ShutdownOdom();
    void ProcessOdom(player_msghdr_t* hdr, player_position2d_data_t &data);

    // Class to handle the internal VFH algorithm
    // (like maintaining histograms etc)
    VFH_Algorithm *vfh_Algorithm;

    // Process requests.  Returns 1 if the configuration has changed.
    //int HandleRequests();
    // Handle motor power requests
    void HandlePower(void *client, void *req, int reqlen);
    // Handle geometry requests.
    void HandleGetGeom(void *client, void *req, int reqlen);

    // Set up the laser device.
    int SetupLaser();
    int ShutdownLaser();
    void ProcessLaser(player_laser_data_t &);

    // Send commands to underlying position device
    void PutCommand( int speed, int turnrate );

    // Check for new commands from server
    void ProcessCommand(player_msghdr_t* hdr, player_position2d_cmd_t &);

    // Computes the signed minimum difference between the two angles.  Inputs
    // and return values are in degrees.
    double angle_diff(double a, double b);
    
    // Odometry device info
    Device *odom;
    player_devaddr_t odom_addr;
    double dist_eps;
    double ang_eps;

    // how fast and how long to back up to escape from a stall
    double escape_speed;
    double escape_max_turnspeed;
    double escape_time;

    // Pose and velocity of robot in odometric cs (mm,mm,deg)
    double odom_pose[3];
    double odom_vel[3];

    // Stall flag and counter
    int odom_stall;

    // Laser device info
    Device *laser;
    player_devaddr_t laser_addr;

    // Laser range and bearing values
    int laser_count;
    double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2];

    // Control velocity
    double con_vel[3];

    int speed, turnrate;
    double reset_odom_x, reset_odom_y, reset_odom_t;
    int32_t goal_x, goal_y, goal_t;
    int cmd_state, cmd_type;
};

// Initialization function
Driver* 
VFH_Init(ConfigFile* cf, int section) 
{
  return ((Driver*) (new VFH_Class( cf, section)));
} 

// a driver registration function
void VFH_Register(DriverTable* table)
{ 
  table->AddDriver("vfh",  VFH_Init);
  return;
} 

////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int VFH_Class::Setup() 
{
  this->active_goal = false;
  this->turninginplace = false;
  this->goal_x = this->goal_y = this->goal_t = 0;

  
  // Initialise the underlying position device.
  if (this->SetupOdom() != 0)
    return -1;

  // Initialise the laser.
  if (this->SetupLaser() != 0)
    return -1;

  // FIXME
  // Allocate and intialize
  vfh_Algorithm->Init();

  // Start the driver thread.
  this->StartThread();

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int VFH_Class::Shutdown() {
  // Stop the driver thread.
  this->StopThread();

  // Stop the laser
  this->ShutdownLaser();

  // Stop the odom device.
  this->ShutdownOdom();

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the underlying odom device.
int VFH_Class::SetupOdom() 
{
  if(!(this->odom = deviceTable->GetDevice(this->odom_addr)))
  {
    PLAYER_ERROR("unable to locate suitable position device");
    return -1;
  }
  if(this->odom->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to position device");
    return -1;
  }

  // Get the odometry geometry
  Message* msg;
  if(!(msg = this->odom->Request(this->InQueue,
                                 PLAYER_MSGTYPE_REQ,
                                 PLAYER_POSITION2D_REQ_GET_GEOM,
                                 NULL, 0, NULL,false)) ||
     (msg->GetHeader()->size != sizeof(player_position2d_geom_t)))
  {
    PLAYER_ERROR("failed to get geometry of underlying position device");
    if(msg)
      delete msg;
    return(-1);
  }
  player_position2d_geom_t* geom = (player_position2d_geom_t*)msg->GetPayload();

  // take the bigger of the two dimensions and halve to get a radius
  float robot_radius = MAX(geom->size.sl,geom->size.sw);
  robot_radius /= 2.0;

  delete msg;

  vfh_Algorithm->SetRobotRadius( robot_radius );

  this->odom_pose[0] = this->odom_pose[1] = this->odom_pose[2] = 0.0;
  this->odom_vel[0] = this->odom_vel[1] = this->odom_vel[2] = 0.0;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying odom device.
int VFH_Class::ShutdownOdom() 
{

  // Stop the robot before unsubscribing
  this->speed = 0;
  this->turnrate = 0;
  this->PutCommand( speed, turnrate );
  
  this->odom->Unsubscribe(this->InQueue);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the laser
int VFH_Class::SetupLaser() 
{
  if(!(this->laser = deviceTable->GetDevice(this->laser_addr)))
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return -1;
  }
  if (this->laser->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return -1;
  }

  this->laser_count = 0;
  for(int i=0;i<PLAYER_LASER_MAX_SAMPLES;i++)
  {
    this->laser_ranges[i][0] = 0;
    this->laser_ranges[i][1] = 0;
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the laser
int VFH_Class::ShutdownLaser() 
{
  this->laser->Unsubscribe(this->InQueue);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Process new odometry data
void 
VFH_Class::ProcessOdom(player_msghdr_t* hdr, player_position2d_data_t &data) 
{

  // Cache the new odometric pose, velocity, and stall info
  // NOTE: this->odom_pose is in (mm,mm,deg), as doubles
  this->odom_pose[0] = data.pos.px * 1e3;
  this->odom_pose[1] = data.pos.py * 1e3;
  this->odom_pose[2] = RTOD(data.pos.pa);
  this->odom_vel[0] = data.vel.px * 1e3;
  this->odom_vel[1] = data.vel.py * 1e3;
  this->odom_vel[2] = RTOD(data.vel.pa);
  this->odom_stall = data.stall;

  // Also change this info out for use by others
  hdr->addr = this->device_addr;
  this->Publish(NULL, hdr, (void*)&data);
}

////////////////////////////////////////////////////////////////////////////////
// Process new laser data
void 
VFH_Class::ProcessLaser(player_laser_data_t &data) 
{
  int i;
  double b, db, r;

  b = RTOD(data.min_angle);
  db = RTOD(data.resolution);

  this->laser_count = data.ranges_count;
  assert(this->laser_count < 
         (int)sizeof(this->laser_ranges) / (int)sizeof(this->laser_ranges[0]));

  for(i = 0; i < PLAYER_LASER_MAX_SAMPLES; i++) 
    this->laser_ranges[i][0] = -1;

  b += 90.0;
  for(i = 0; i < this->laser_count; i++)
  {
    this->laser_ranges[(int)rint(b * 2)][0] = data.ranges[i] * 1e3;
    this->laser_ranges[(int)rint(b * 2)][1] = b;
    b += db;
  }

  r = 1000000.0;
  for (i = 0; i < PLAYER_LASER_MAX_SAMPLES; i++) 
  {
    if (this->laser_ranges[i][0] != -1) {
      r = this->laser_ranges[i][0];
    } else {
      this->laser_ranges[i][0] = r;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Send commands to the underlying position device
void 
VFH_Class::PutCommand( int cmd_speed, int cmd_turnrate ) 
{
  player_position2d_cmd_t cmd;

//printf("Command: speed: %d turnrate: %d\n", cmd_speed, cmd_turnrate);

  this->con_vel[0] = (double)cmd_speed;
  this->con_vel[1] = 0;
  this->con_vel[2] = (double)cmd_turnrate;

  memset(&cmd, 0, sizeof(cmd));

  // Stop the robot (locks the motors) if the motor state is set to
  // disabled.  The P2OS driver does not respect the motor state.
  if (this->cmd_state == 0)
  {
    cmd.vel.px = 0;
    cmd.vel.py = 0;
    cmd.vel.pa = 0;
  }
  // Velocity mode: the command was already passed through in
  // ProcessCommand.
  else if (this->cmd_type == 0)
  {
  }
  // Position mode
  else
  {
    cmd.vel.px =  this->con_vel[0] / 1e3;
    cmd.vel.py =  this->con_vel[1] / 1e3;
    cmd.vel.pa =  DTOR(this->con_vel[2]);
  }

  if(fabs(this->con_vel[2]) > 
     (double)vfh_Algorithm->GetMaxTurnrate((int)this->con_vel[0]))
    PLAYER_WARN1("fast turn %d", RTOD(cmd.vel.pa));

  this->odom->PutMsg(this->InQueue,
                     PLAYER_MSGTYPE_CMD,
                     PLAYER_POSITION2D_CMD_STATE,
                     (void*)&cmd,sizeof(cmd),NULL);
}


////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int VFH_Class::ProcessMessage(MessageQueue* resp_queue, 
                              player_msghdr * hdr, 
                              void * data)
{
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 
                           PLAYER_POSITION2D_DATA_STATE, this->odom_addr))
  {
    assert(hdr->size == sizeof(player_position2d_data_t));
    ProcessOdom(hdr, *reinterpret_cast<player_position2d_data_t *> (data));
    return(0);
  }
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 
                                PLAYER_LASER_DATA_SCAN, this->laser_addr))
  {
    // It's not always that big...
    //assert(hdr->size == sizeof(player_laser_data_t));
    ProcessLaser(*reinterpret_cast<player_laser_data_t *> (data));
    return 0;
  }
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, 
                                PLAYER_POSITION2D_CMD_STATE, 
                                this->device_addr))
  {
    assert(hdr->size == sizeof(player_position2d_cmd_t));
    ProcessCommand(hdr, *reinterpret_cast<player_position2d_cmd_t *> (data));
    return 0;
  }
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->device_addr))
  {
    // Pass the request on to the underlying position device and wait for
    // the reply.
    Message* msg;

    if(!(msg = this->odom->Request(this->InQueue,
                                   hdr->type,
                                   hdr->subtype,
                                   (void*)data, 
                                   hdr->size, 
                                   &hdr->timestamp)))
    {
      PLAYER_WARN1("failed to forward config request with subtype: %d\n",
                   hdr->subtype);
      return(-1);
    }

    player_msghdr_t* rephdr = msg->GetHeader();
    void* repdata = msg->GetPayload();
    // Copy in our address and forward the response
    rephdr->addr = this->device_addr;
    this->Publish(resp_queue, rephdr, repdata);
    delete msg;
    return(0);
  }
  else
  {
    puts("unhandled command");
    return -1;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void VFH_Class::Main() 
{
  float dist;
  double angdiff;
  struct timeval startescape, curr;
  bool escaping = false;
  double timediff;
  int escape_turnrate_deg;

  // bookkeeping to implement hysteresis when rotating at the goal
  int rotatedir = 1;

  // bookkeeping to implement smarter escape policy
  int escapedir;

  escapedir = 1;
  while (true)
  {
    // Wait till we get new data
    this->Wait();

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    this->ProcessMessages();

    if(!this->active_goal)
      continue;

    // Figure how far, in distance and orientation, we are from the goal
    dist = sqrt(pow((goal_x - this->odom_pose[0]),2) + 
                pow((goal_y - this->odom_pose[1]),2));
    angdiff = this->angle_diff((double)this->goal_t,this->odom_pose[2]);

    // If we're currently escaping after a stall, check whether we've done
    // so for long enough.
    if(escaping)
    {
      GlobalTime->GetTime(&curr);
      timediff = (curr.tv_sec + curr.tv_usec/1e6) -
              (startescape.tv_sec + startescape.tv_usec/1e6);
      if(timediff > this->escape_time)
      {
        // if we're still stalled, try escaping the other direction
        if(this->odom_stall)
          escapedir *= -1;
        escaping = false;
      }
    }

    // CASE 1: The robot has stalled, so escape if the user specified
    //         a non-zero escape velocity.
    if(escaping || 
       (this->escape_speed && this->escape_time && this->odom_stall))
    {
      if(!escaping)
      {
        GlobalTime->GetTime(&startescape);
        escaping = true;
      }

      this->speed = (int)rint(this->escape_speed * escapedir * 1e3);
      if(this->escape_max_turnspeed)
      {
        // pick a random turnrate in
        // [-escape_max_turnspeed,escape_max_turnspeed]
        escape_turnrate_deg = (int)rint(RTOD(this->escape_max_turnspeed));
        this->turnrate = (int)(2.0 * escape_turnrate_deg *
                               rand()/(RAND_MAX+1.0)) - 
                escape_turnrate_deg/2 + 1;
      }
      else
        this->turnrate = 0;
      PutCommand(this->speed, this->turnrate);

      this->turninginplace = false;
    }
    // CASE 2: The robot is at the goal, within user-specified tolerances, so
    //         stop.
    else if((dist < (this->dist_eps * 1e3)) && 
            (fabs(DTOR(angdiff)) < this->ang_eps))
    {
      this->active_goal = false;
      this->speed = this->turnrate = 0;
      PutCommand( this->speed, this->turnrate );
      this->turninginplace = false;
    }
    // CASE 3: The robot is too far from the goal position, so invoke VFH to
    //         get there.
    else if (dist > (this->dist_eps * 1e3))
    {
      float Desired_Angle = (90 + atan2((goal_y - this->odom_pose[1]),
                                        (goal_x - this->odom_pose[0]))
                             * 180 / M_PI - this->odom_pose[2]);

      while (Desired_Angle > 360.0)
        Desired_Angle -= 360.0;
      while (Desired_Angle < 0)
        Desired_Angle += 360.0;

      vfh_Algorithm->Update_VFH( this->laser_ranges, 
                                 (int)(this->odom_vel[0]),
                                 Desired_Angle,
                                 dist,
                                 this->dist_eps * 1e3,
                                 this->speed, 
                                 this->turnrate );
      PutCommand( this->speed, this->turnrate );
      this->turninginplace = false;
    }
    // CASE 4: The robot is at the goal position, but still needs to turn
    //         in place to reach the desired orientation.
    else
    {
      // At goal, stop
      speed = 0;

      // Turn in place in the appropriate direction, with speed
      // proportional to the angular distance to the goal orientation.
      turnrate = (int)rint(fabs(angdiff/180.0) * 
                           vfh_Algorithm->GetMaxTurnrate(speed));

      // If we've just gotten to the goal, pick a direction to turn;
      // otherwise, keep turning the way we started (to prevent
      // oscillation)
      if(!this->turninginplace)
      {
        this->turninginplace = true;
        if(angdiff < 0)
          rotatedir = -1;
        else
          rotatedir = 1;
      }

      turnrate *= rotatedir;

      // Threshold to make sure we don't send arbitrarily small turn speeds
      // (which may not cause the robot to actually move).
      if(turnrate < 0)
        turnrate = MIN(turnrate,-this->vfh_Algorithm->GetMinTurnrate());
      else
        turnrate = MAX(turnrate,this->vfh_Algorithm->GetMinTurnrate());

      this->PutCommand( this->speed, this->turnrate );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Check for new commands from the server
void 
VFH_Class::ProcessCommand(player_msghdr_t* hdr, player_position2d_cmd_t &cmd) 
{
  int x,y,t;

  // Velocity mode; just pass it through
  if (cmd.type == 0)
  {
    this->odom->PutMsg(this->InQueue, hdr, (void*)&cmd);
    this->cmd_type = 0;
    this->active_goal = false;
  }
  // Position mode
  else
  {
    x = (int)rint(cmd.pos.px * 1e3);
    y = (int)rint(cmd.pos.py * 1e3);
    t = (int)rint(RTOD(cmd.pos.pa));

    this->cmd_type = 1;
    this->cmd_state = cmd.state;

    if((x != this->goal_x) || (y != this->goal_y) || (t != this->goal_t))
    {
      this->active_goal = true;
      this->turninginplace = false;
      this->goal_x = x;
      this->goal_y = y;
      this->goal_t = t;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Constructor
VFH_Class::VFH_Class( ConfigFile* cf, int section)
  : Driver(cf, section, true, 
           PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_POSITION2D_CODE)
{
  double cell_size;
  int window_diameter;
  int sector_angle;
  double safety_dist_0ms;
  double safety_dist_1ms; 
  int max_speed;
  int max_speed_narrow_opening;
  int max_speed_wide_opening;
  int max_acceleration;
  int min_turnrate;
  int max_turnrate_0ms;
  int max_turnrate_1ms;
  double min_turn_radius_safety_factor;
  double free_space_cutoff_0ms;
  double obs_cutoff_0ms;
  double free_space_cutoff_1ms;
  double obs_cutoff_1ms;
  double weight_desired_dir;
  double weight_current_dir;


  this->speed = 0;
  this->turnrate = 0;

  cell_size = cf->ReadLength(section, "cell_size", 0.1) * 1e3;
  window_diameter = cf->ReadInt(section, "window_diameter", 61);
  sector_angle = cf->ReadInt(section, "sector_angle", 5);
  safety_dist_0ms = cf->ReadLength(section, "safety_dist_0ms", 0.1) * 1e3;
  safety_dist_1ms = cf->ReadLength(section, "safety_dist_1ms", 
                                   safety_dist_0ms/1e3) * 1e3;
  max_speed = (int)rint(1e3 * cf->ReadLength(section, "max_speed", 0.2));
  max_speed_narrow_opening = 
          (int)rint(1e3 * cf->ReadLength(section, 
                                         "max_speed_narrow_opening", 
                                         max_speed/1e3));
  max_speed_wide_opening = 
          (int)rint(1e3 * cf->ReadLength(section, 
                                         "max_speed_wide_opening", 
                                         max_speed/1e3));
  max_acceleration = (int)rint(1e3 * cf->ReadLength(section, "max_acceleration", 0.2));
  min_turnrate = (int)rint(RTOD(cf->ReadAngle(section, "min_turnrate", DTOR(10))));
  max_turnrate_0ms = (int) rint(RTOD(cf->ReadAngle(section, "max_turnrate_0ms", DTOR(40))));
  max_turnrate_1ms = 
          (int) rint(RTOD(cf->ReadAngle(section, 
                                        "max_turnrate_1ms", 
                                        DTOR(max_turnrate_0ms))));
  min_turn_radius_safety_factor = 
          cf->ReadFloat(section, "min_turn_radius_safety_factor", 1.0);
  free_space_cutoff_0ms = cf->ReadFloat(section, 
                                        "free_space_cutoff_0ms", 
                                        2000000.0);
  obs_cutoff_0ms = cf->ReadFloat(section, 
                                 "obs_cutoff_0ms", 
                                 free_space_cutoff_0ms);
  free_space_cutoff_1ms = cf->ReadFloat(section, 
                                        "free_space_cutoff_1ms", 
                                        free_space_cutoff_0ms);
  obs_cutoff_1ms = cf->ReadFloat(section, 
                                 "obs_cutoff_1ms", 
                                 free_space_cutoff_1ms);
  weight_desired_dir = cf->ReadFloat(section, "weight_desired_dir", 5.0);
  weight_current_dir = cf->ReadFloat(section, "weight_current_dir", 3.0);

  this->dist_eps = cf->ReadLength(section, "distance_epsilon", 0.5);
  this->ang_eps = cf->ReadAngle(section, "angle_epsilon", DTOR(10.0));

  this->escape_speed = cf->ReadLength(section, "escape_speed", 0.0);
  this->escape_time = cf->ReadFloat(section, "escape_time", 0.0);
  this->escape_max_turnspeed = cf->ReadAngle(section, "escape_max_turnrate", 0.0);

  // Instantiate the classes that handles histograms
  // and chooses directions
  this->vfh_Algorithm = new VFH_Algorithm(cell_size,
                                          window_diameter,
                                          sector_angle,
                                          safety_dist_0ms,
                                          safety_dist_1ms, 
                                          max_speed,
                                          max_speed_narrow_opening,
                                          max_speed_wide_opening,
                                          max_acceleration,
                                          min_turnrate,
                                          max_turnrate_0ms,
                                          max_turnrate_1ms,
                                          min_turn_radius_safety_factor,
                                          free_space_cutoff_0ms,
                                          obs_cutoff_0ms,
                                          free_space_cutoff_1ms,
                                          obs_cutoff_1ms,
                                          weight_desired_dir,
                                          weight_current_dir);
  this->odom = NULL;
  if (cf->ReadDeviceAddr(&this->odom_addr, section, "requires",
                         PLAYER_POSITION2D_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);    
    return;
  }
  
  this->laser = NULL;
  if (cf->ReadDeviceAddr(&this->laser_addr, section, "requires",
                         PLAYER_LASER_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);    
    return;
  }
  
  // Laser settings
  //TODO this->laser_max_samples = cf->ReadInt(section, "laser_max_samples", 10);
  
  return;
}


///////////////////////////////////////////////////////////////////////////
// VFH Code


VFH_Class::~VFH_Class() {
  delete this->vfh_Algorithm;

  return;
}

// computes the signed minimum difference between the two angles.  inputs
// and return values are in degrees.
double
VFH_Class::angle_diff(double a, double b)
{
  double ra, rb;
  double d1, d2;

  ra = NORMALIZE(DTOR(a));
  rb = NORMALIZE(DTOR(b));

  d1 = ra-rb;
  d2 = 2*M_PI - fabs(d1);
  if(d1 > 0)
    d2 *= -1.0;

  if(fabs(d1) < fabs(d2))
    return(RTOD(d1));
  else
    return(RTOD(d2));
}

