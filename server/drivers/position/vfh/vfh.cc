#include <assert.h>
#include <math.h>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "player.h"
#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"
#include "vfh_algorithm.h"

#ifndef MIN
  #define MIN(a,b) ((a < b) ? (a) : (b))
#endif
#ifndef MAX
  #define MAX(a,b) ((a > b) ? (a) : (b))
#endif

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

    // Main function for device thread.
    virtual void Main();


  private:
     bool active_goal;
    
    // Set up the truth device (optional).
    int SetupTruth();
    int ShutdownTruth();
    int GetTruth();

    // Set up the odometry device.
    int SetupOdom();
    int ShutdownOdom();
    int GetOdom();

    // Class to handle the internal VFH algorithm
    // (like maintaining histograms etc)
    VFH_Algorithm *vfh_Algorithm;

    // Process requests.  Returns 1 if the configuration has changed.
    int HandleRequests();
    // Handle motor power requests
    void HandlePower(void *client, void *req, int reqlen);
    // Handle geometry requests.
    void HandleGetGeom(void *client, void *req, int reqlen);

    // Set up the laser device.
    int SetupLaser();
    int ShutdownLaser();
    int GetLaser();

    // Write the pose data (the data going back to the client).
    void PutPose();

    // Send commands to underlying position device
    void PutCommand( int speed, int turnrate );

    // Check for new commands from server
    void GetCommand();
    
    // Truth device info
    Driver *truth;
    int truth_index;
    double truth_time;

    // Odometry device info
    Driver *odom;
    int odom_index;
    double odom_time;
    double dist_eps;
    double ang_eps;

    // Odometric geometry (robot size and pose in robot cs)
    double odom_geom_pose[3];
    double odom_geom_size[3];

    // Pose of robot in odometric cs (mm,mm,deg)
    double odom_pose[3];

    // Stall flag
    int odom_stall;

    // Velocity of robot in robot cs, NOT byteswapped, just for passing
    // through
    int32_t odom_vel_be[3];

    // Laser device info
    Driver *laser;
    int laser_index;
    double laser_time;

    // Laser geometry (pose of laser in robot cs)
    double laser_geom_pose[3];

    // Laser range and bearing values
    int laser_count;
    double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2];

    // Control velocity
    double con_vel[3];

    int speed, turnrate;
    double reset_odom_x, reset_odom_y, reset_odom_t;
    int32_t goal_x, goal_y, goal_t;
    int32_t goal_vx, goal_vy, goal_vt;
    int cmd_state, cmd_type;
};

// Initialization function
Driver* VFH_Init( ConfigFile* cf, int section) 
{
  if (strcmp(interface, PLAYER_POSITION_STRING) != 0) { 
    PLAYER_ERROR1("driver \"vfh\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((Driver*) (new VFH_Class(interface, cf, section)));
} 

// a driver registration function
void VFH_Register(DriverTable* table)
{ 
  table->AddDriver("vfh", PLAYER_ALL_MODE, VFH_Init);
  return;
} 

////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int VFH_Class::Setup() 
{
  player_position_cmd_t cmd;

  memset(&cmd,0,sizeof(cmd));
  Driver::PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));

  this->active_goal = false;
  this->goal_x = this->goal_y = this->goal_t = 0;

  
  // Initialise the underlying truth device.
  if(this->SetupTruth() != 0)
    return -1;

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
  
  // Stop the truth device.
  this->ShutdownTruth();

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the underlying truth device (optional).
int VFH_Class::SetupTruth() 
{
  //struct timeval ts;
  player_device_id_t id;
  
  // if the user didn't specify a truth device, don't do anything
  if(this->truth_index < 0)
    return(0);

// EDIT?
//  id.robot = this->device_id.robot;
  id.port = this->device_id.port;
  id.code = PLAYER_TRUTH_CODE;
  id.index = this->truth_index;


  if(!(this->truth = deviceTable->GetDevice(id)))
  {
    PLAYER_ERROR("unable to locate suitable truth device");
    return -1;
  }

  if(this->truth->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to truth device");
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying truth device.
int VFH_Class::ShutdownTruth() 
{

  if(this->truth)
    this->truth->Unsubscribe(this);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the underlying odom device.
int VFH_Class::SetupOdom() 
{
  uint8_t req;
  size_t replen;
  unsigned short reptype;
  struct timeval ts;
  player_position_geom_t geom;
  player_device_id_t id;

// EDIT?
//  id.robot = this->device_id.robot;
  id.port = this->device_id.port;
  id.code = PLAYER_POSITION_CODE;
  id.index = this->odom_index;

  this->odom = deviceTable->GetDevice(id);
  if (!this->odom)
  {
    PLAYER_ERROR("unable to locate suitable position device");
    return -1;
  }

  if (this->odom->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to position device");
    return -1;
  }

  // Get the odometry geometry
  req = PLAYER_POSITION_GET_GEOM_REQ;
  replen = this->odom->Request(&this->odom->device_id, this, &req, sizeof(req),
                               &reptype, &ts, &geom, sizeof(geom));

  geom.pose[0] = ntohs(geom.pose[0]);
  geom.pose[1] = ntohs(geom.pose[1]);
  geom.pose[2] = ntohs(geom.pose[2]);
  geom.size[0] = ntohs(geom.size[0]);
  geom.size[1] = ntohs(geom.size[1]);

  this->odom_geom_pose[0] = (int16_t) geom.pose[0] / 1000.0;
  this->odom_geom_pose[1] = (int16_t) geom.pose[1] / 1000.0;
  this->odom_geom_pose[2] = (int16_t) geom.pose[2] / 180 * M_PI;

  this->odom_geom_size[0] = (int16_t) geom.size[0] / 1000.0;
  this->odom_geom_size[1] = (int16_t) geom.size[1] / 1000.0;

  // take the bigger of the two dimensions and halve to get a radius
  float robot_radius = MAX(geom.size[0],geom.size[1]);
  robot_radius /= 2.0;

  vfh_Algorithm->SetRobotRadius( robot_radius );

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
  
  this->odom->Unsubscribe(this);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the laser
int VFH_Class::SetupLaser() {
  uint8_t req;
  size_t replen;
  unsigned short reptype;
  struct timeval ts;
  player_laser_geom_t geom;
  player_device_id_t id;

// EDIT ?
//  id.robot = this->device_id.robot;
  id.port = this->device_id.port;
  id.code = PLAYER_LASER_CODE;
  id.index = this->laser_index;

  this->laser = deviceTable->GetDevice(id);
  if (!this->laser)
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return -1;
  }
  if (this->laser->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return -1;
  }

  // Get the laser geometry
  req = PLAYER_LASER_GET_GEOM;
  replen = this->laser->Request(&this->laser->device_id, this, &req, sizeof(req),
                                &reptype, &ts, &geom, sizeof(geom));

  geom.pose[0] = ntohs(geom.pose[0]);
  geom.pose[1] = ntohs(geom.pose[1]);
  geom.pose[2] = ntohs(geom.pose[2]);
  geom.size[0] = ntohs(geom.size[0]);
  geom.size[1] = ntohs(geom.size[1]);

  this->laser_geom_pose[0] = (int16_t) geom.pose[0] / 1000.0;
  this->laser_geom_pose[1] = (int16_t) geom.pose[1] / 1000.0;
  this->laser_geom_pose[2] = (int16_t) geom.pose[2] / 180 * M_PI;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the laser
int VFH_Class::ShutdownLaser() {
  this->laser->Unsubscribe(this);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Check for new odometry data
int VFH_Class::GetOdom() {
  //int i;
  size_t size;
  player_position_data_t data;
  uint32_t timesec, timeusec;
  double time;

  // Get the odom device data.
  size = this->odom->GetData(this,(unsigned char*) &data, sizeof(data), &timesec, &timeusec);
  if (size == 0)
    return 0;
  time = (double) timesec + ((double) timeusec) * 1e-6;

  // Dont do anything if this is old data.
  if (time - this->odom_time < 0.001)
    return 0;
  this->odom_time = time;

  // Byte swap
  data.xpos = ntohl(data.xpos);
  data.ypos = ntohl(data.ypos);
  data.yaw = ntohl(data.yaw);

  this->odom_pose[0] = (double)((int32_t)data.xpos);
  this->odom_pose[1] = (double)((int32_t)data.ypos);
  this->odom_pose[2] = (double)((int32_t)data.yaw);

  // don't byteswap velocities, we're just passing them through
  this->odom_vel_be[0] = data.xspeed;
  this->odom_vel_be[1] = data.yspeed;
  this->odom_vel_be[2] = data.yawspeed;

  this->odom_stall = data.stall;

  return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Check for new truth data (optional)
int VFH_Class::GetTruth() 
{
  //int i;
  size_t size;
  player_truth_data_t data;
  uint32_t timesec, timeusec;
  double time;

  // Get the truth device data.
  size = this->truth->GetData(this,(unsigned char*) &data, sizeof(data), &timesec, &timeusec);
  if (size == 0)
    return 0;
  time = (double) timesec + ((double) timeusec) * 1e-6;

  // Dont do anything if this is old data.
  if (time - this->truth_time < 0.001)
    return 0;
  this->truth_time = time;

  // Byte swap
  data.px = ntohl(data.px);
  data.py = ntohl(data.py);
  data.pa = (ntohl(data.pa) + 360) % 360;

  this->odom_pose[0] = (double) ((int32_t) data.px);
  this->odom_pose[1] = (double) ((int32_t) data.py);
  this->odom_pose[2] = (double) ((int32_t) data.pa);

  return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Check for new laser data
int VFH_Class::GetLaser() {
  int i;
  size_t size;
  player_laser_data_t data;
  uint32_t timesec, timeusec;
  double time;
  double r, b, db, range_res;

  // Get the laser device data.
  size = this->laser->GetData(this,(unsigned char*) &data, sizeof(data), &timesec, &timeusec);
  if (size == 0)
    return 0;
  time = (double) timesec + ((double) timeusec) * 1e-6;

  // Dont do anything if this is old data.
  if (time - this->laser_time < 0.001)
    return 0;
  this->laser_time = time;
  /*
  this->laser->Wait();
  this->laser->GetData(this,(unsigned char*) &data, sizeof(data), NULL, NULL);
  */

//  b = ((int16_t) ntohs(data.min_angle)) / 100.0 * M_PI / 180.0;
//  db = ((int16_t) ntohs(data.resolution)) / 100.0 * M_PI / 180.0;

  b = ((int16_t) ntohs(data.min_angle)) / 100.0;
  db = ((int16_t) ntohs(data.resolution)) / 100.0;
  range_res = ((int16_t) ntohs(data.range_res));

  this->laser_count = ntohs(data.range_count);
  assert(this->laser_count < (int)sizeof(this->laser_ranges) / (int)sizeof(this->laser_ranges[0]));

  // Read and byteswap the range data
  for (i = 0; i < PLAYER_LASER_MAX_SAMPLES; i++) {
    this->laser_ranges[i][0] = -1;
  }

  // what are units of min_angle and resolution
  //printf("LASER %d %d\n", (int)b, (int)db);

  b += 90;
  for (i = 0; i < this->laser_count; i++)
  {
//printf("b: %f\n", b);
//    r = ((int16_t) ntohs(data.ranges[i])) / 1000.0;
//if ((i % 20) == 0) {
//printf("b: %f\n", b);
    r = ((int16_t) ntohs(data.ranges[i])) * range_res;
    this->laser_ranges[(int)rint(b * 2)][0] = r;
    this->laser_ranges[(int)rint(b * 2)][1] = b;
//}
    b += db;
  }

  r = 1000000;
  for (i = 0; i < PLAYER_LASER_MAX_SAMPLES; i++) {
    if (this->laser_ranges[i][0] != -1) {
      r = this->laser_ranges[i][0];
    } else {
      this->laser_ranges[i][0] = r;
    }
  }
 

/*
  // Read and byteswap the range data
  for (i = 0; i < this->laser_count; i++)
  {
printf("b: %f\n", b);
//    r = ((int16_t) ntohs(data.ranges[i])) / 1000.0;
    r = ((int16_t) ntohs(data.ranges[i]));
    this->laser_ranges[i][0] = r;
    this->laser_ranges[i][1] = b;
    b += db;
  }
*/

  return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Send commands to the underlying position device
void VFH_Class::PutCommand( int cmd_speed, int cmd_turnrate ) {
  player_position_cmd_t cmd;

//printf("Command: speed: %d turnrate: %d\n", cmd_speed, cmd_turnrate);

  this->con_vel[0] = (double)cmd_speed;
  this->con_vel[1] = 0;
  this->con_vel[2] = (double)cmd_turnrate;

  memset(&cmd, 0, sizeof(cmd));

  // Stop the robot (locks the motors) if the motor state is set to
  // disabled.  The P2OS driver does not respect the motor state.
  if (this->cmd_state == 0)
  {
    cmd.xspeed = 0;
    cmd.yspeed = 0;
    cmd.yawspeed = 0;
  }

  // Velocity mode: pass through the commands.  There is some added
  // latency here, perhaps move this to GetCommand()?
  else if (this->cmd_type == 0)
  {
    cmd.xspeed = this->goal_vx;
    cmd.yspeed = this->goal_vy;
    cmd.yawspeed = this->goal_vt;
  }

  // Position mode
  else
  {
    cmd.xspeed = (int32_t) (this->con_vel[0]);
    cmd.yspeed = (int32_t) (this->con_vel[1]);
    cmd.yawspeed = (int32_t) (this->con_vel[2]);
  }

  if (abs(cmd.yawspeed) > vfh_Algorithm->GetMaxTurnrate())
    PLAYER_WARN1("fast turn %d", cmd.yawspeed);

  cmd.xspeed = htonl(cmd.xspeed);
  cmd.yspeed = htonl(cmd.yspeed);
  cmd.yawspeed = htonl(cmd.yawspeed);

  this->odom->PutCommand(this, (unsigned char*) &cmd, sizeof(cmd));

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Handle motor power requests
void VFH_Class::HandlePower(void *client, void *req, int reqlen)
{
  unsigned short reptype;
  struct timeval ts;

  this->odom->Request(&this->odom->device_id, this, req, reqlen, &reptype, &ts, NULL, 0);
  if (PutReply(client, reptype, &ts, NULL, 0) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Handle geometry requests.
void VFH_Class::HandleGetGeom(void *client, void *req, int reqlen)
{
  player_position_geom_t geom;

  geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
  geom.pose[0] = htons((int) (this->odom_geom_pose[0] * 1000));
  geom.pose[1] = htons((int) (this->odom_geom_pose[1] * 1000));
  geom.pose[2] = htons((int) (this->odom_geom_pose[2] * 180 / M_PI));
  geom.size[0] = htons((int) (this->odom_geom_size[0] * 1000));
  geom.size[1] = htons((int) (this->odom_geom_size[1] * 1000));

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int VFH_Class::HandleRequests()
{
  int len;
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];

  while ((len = GetConfig(&client, &request, sizeof(request))) > 0)
  {
    printf( "VFH Got a request\n");
    switch (request[0])
    {
      case PLAYER_POSITION_GET_GEOM_REQ:
        HandleGetGeom(client, request, len);
        break;

      case PLAYER_POSITION_MOTOR_POWER_REQ:
        HandlePower(client, request, len);
        break;

      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void VFH_Class::Main() 
{
  struct timespec sleeptime;
  float dist;
  //struct timeval stime, time;
  //int gt, ct;
  //bool newodom,newtruth;
  double angdiff;

  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = 1000000L;

  // Wait till we get new odometry data; this may block indefinitely
  // on older versions of Stage and Gazebo.
  //this->odom->Wait();

  this->GetOdom();
  if(this->truth)
    this->GetTruth();

  while (true)
  {
    // Wait till we get new odometry data; this may block indefinitely
    // on older versions of Stage and Gazebo.
   // this->odom->Wait();

    // Process any pending requests.
    this->HandleRequests();


    // Sleep for 1ms (will actually take longer than this).
    nanosleep(&sleeptime, NULL);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    if(this->truth)
    {
      // first get odometry, which fills in odom_pose, among other things
      this->GetOdom();
      
      // now get truth, which will overwrite odom_pose
      if(this->GetTruth() == 0)
        continue;
    }
    else
    {        
      // Get new odometric data
      if (this->GetOdom() == 0)
        continue;
    }

    // Write odometric data (so we emulate the underlying odometric device)
    this->PutPose();


    // Read client command
    this->GetCommand();

    if(!this->active_goal)
    {
      //puts("VFH: no active goal");
      continue;
    }

    // gettimeofday(&time, 0);
    // printf("Before VFH Time: %d %d\n",time.tv_sec - stime.tv_sec, time.tv_usec - stime.tv_usec);

    dist = sqrt(pow((goal_x - this->odom_pose[0]),2) + 
                pow((goal_y - this->odom_pose[1]),2));

    
    /*
    printf("VFH: goal : %d,%d,%d\n",
           this->goal_x, this->goal_y, this->goal_t);
    printf("VFH: pose: %f,%f,%f\n",
           this->odom_pose[0], this->odom_pose[1], this->odom_pose[2]);
    
    printf("VFH: dist: %f\n", dist);
    */

    if((dist < (this->dist_eps * 1e3)) && 
       (fabs(NORMALIZE(DTOR(goal_t)-DTOR(this->odom_pose[2]))) < this->ang_eps))
    {
      //puts("VFH: goal reached");
      this->active_goal = false;
      this->speed = this->turnrate = 0;
      PutCommand( this->speed, this->turnrate );
    }
    else if (dist > (this->dist_eps * 1e3))
    {
      float Desired_Angle = 90 + atan2((goal_y - this->odom_pose[1]), (goal_x - this->odom_pose[0]))
              * 180 / M_PI - this->odom_pose[2];

      while (Desired_Angle > 360.0)
      {
        Desired_Angle -= 360.0;
      }
      while (Desired_Angle < 0)
      {
        Desired_Angle += 360.0;
      }

      vfh_Algorithm->SetDesiredAngle( Desired_Angle );

      //printf("vfh dist %.3f angle %.3f\n", dist, Desired_Angle);

      // Get new laser data.
      this->GetLaser();
      vfh_Algorithm->Update_VFH( this->laser_ranges, this->speed, this->turnrate );
      PutCommand( this->speed, this->turnrate );
    }
    else
    {
      // At goal, stop
      //      goal_t = 90;

      /*
      gt = goal_t % 360;
      if (gt > 180)
        gt -= 360;

      ct = (int)rint(this->odom_pose[2]) % 360;
      if (ct > 180)
        ct -= 360;

      if (((gt - ct) > 10) && ((gt - ct) <= 180)) {
        speed = 0;
        turnrate = MAX_TURNRATE;
      } else if (((gt - ct) < -10) &&
                 ((gt - ct) >= -180)) {
        speed = 0;
        turnrate = -1 * MAX_TURNRATE;
      } else {
        speed = 0;
        turnrate = 0;
      }
      */

      angdiff = this->goal_t - this->odom_pose[2];
      while (angdiff > 180)
        angdiff -= 360.0;
      while (angdiff < -180)
        angdiff += 360.0;

      speed = 0;
      if(fabs(angdiff) > RTOD(this->ang_eps))
      {
        turnrate = (int)rint((angdiff/180.0) * vfh_Algorithm->GetMaxTurnrate());
        if(turnrate < 0)
          turnrate = MIN(turnrate,-this->vfh_Algorithm->GetMinTurnrate());
        else
          turnrate = MAX(turnrate,this->vfh_Algorithm->GetMinTurnrate());
      }
      else
        turnrate = 0;

      this->PutCommand( this->speed, this->turnrate );
    }
    // gettimeofday(&time, 0);
    // printf("After VFH Time: %d %d\n",time.tv_sec - stime.tv_sec, time.tv_usec - stime.tv_usec);
  }
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Check for new commands from the server
void VFH_Class::GetCommand() 
{
  player_position_cmd_t cmd;
  int x,y,t;

  if(Driver::GetCommand(&cmd, sizeof(cmd)) != 0) 
  {
    // Velocity mode
    if (cmd.type == 0)
    {
      x = (int)ntohl(cmd.xspeed);
      y = (int)ntohl(cmd.yspeed);
      t = (int)ntohl(cmd.yawspeed);

      this->cmd_type = 0;
      this->cmd_state = cmd.state;
      this->goal_vx = x;
      this->goal_vy = y;
      this->goal_vt = t;
    }

    // Position mode
    else
    {
      x = (int)ntohl(cmd.xpos);
      y = (int)ntohl(cmd.ypos);
      t = (int)ntohl(cmd.yaw);

      this->cmd_type = 1;
      this->cmd_state = cmd.state;

      if((x != this->goal_x) || (y != this->goal_y) || (t != this->goal_t))
      {
        this->active_goal = true;
        this->goal_x = x;
        this->goal_y = y;
        this->goal_t = t;
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
VFH_Class::VFH_Class( ConfigFile* cf, int section)
    : Driver(cf, section, sizeof(player_position_data_t), sizeof(player_position_cmd_t), 10, 10)
{
  //double size;
  double cell_size, robot_radius, safety_dist, free_space_cutoff, obs_cutoff;
  double weight_desired_dir, weight_current_dir;
  int window_diameter, sector_angle, max_speed, max_turnrate, min_turnrate;

  this->speed = 0;
  this->turnrate = 0;

  // AlexB: This all seems to get over-written just below..
//   this->CELL_WIDTH = 200.0;
//   this->WINDOW_DIAMETER = 41;
//   this->SECTOR_ANGLE = 5;
//   this->ROBOT_RADIUS = 200.0;
//   this->SAFETY_DIST = 200.0;

//   this->CENTER_X = (int)floor(this->WINDOW_DIAMETER / 2.0);
//   this->CENTER_Y = this->CENTER_X;
//   this->HIST_SIZE = (int)rint(360.0 / this->SECTOR_ANGLE);

//   this->MAX_SPEED = 200;
//   this->MAX_TURNRATE = 40;

//   this->U1 = 5;
//   this->U2 = 3;

//   this->Desired_Angle = 90;
//   this->Picked_Angle = 90;
//   this->Last_Picked_Angle = this->Picked_Angle;

  cell_size = cf->ReadLength(section, "cell_size", 0.1) * 1000.0;
  window_diameter = cf->ReadInt(section, "window_diameter", 61);
  sector_angle = cf->ReadInt(section, "sector_angle", 5);
  //robot_radius = cf->ReadLength(section, "robot_radius", 0.25) * 1000.0;
  safety_dist = cf->ReadLength(section, "safety_dist", 0.1) * 1000.0;
  max_speed = (int) rint(1000 * cf->ReadLength(section, "max_speed", 0.2));
  max_turnrate = (int) rint(RTOD(cf->ReadAngle(section, "max_turnrate", DTOR(40))));
  min_turnrate = (int) rint(RTOD(cf->ReadAngle(section, "min_turnrate", DTOR(10))));
  free_space_cutoff = cf->ReadLength(section, "free_space_cutoff", 2000000.0);
  obs_cutoff = cf->ReadLength(section, "obs_cutoff", free_space_cutoff);
  weight_desired_dir = cf->ReadLength(section, "weight_desired_dir", 5.0);
  weight_current_dir = cf->ReadLength(section, "weight_current_dir", 3.0);

  this->dist_eps = cf->ReadLength(section, "distance_epsilon", 0.5);
  this->ang_eps = cf->ReadAngle(section, "angle_epsilon", DTOR(10.0));

  // Instantiate the classes that handles histograms
  // and chooses directions
  this->vfh_Algorithm = new VFH_Algorithm( cell_size,
                                           window_diameter,
                                           sector_angle,
                                           safety_dist,
                                           max_speed,
                                           min_turnrate,
                                           max_turnrate,
                                           free_space_cutoff,
                                           obs_cutoff,
                                           weight_desired_dir,
                                           weight_current_dir );

  this->truth = NULL;
  this->truth_index = cf->ReadInt(section, "truth_index", -1);
  this->truth_time = 0.0;
  
  this->odom = NULL;
  this->odom_index = cf->ReadInt(section, "position_index", 0);
  this->odom_time = 0.0;
  
  // The actual odometry geometry is read from the odometry device
  this->odom_geom_pose[0] = 0.0;
  this->odom_geom_pose[1] = 0.0;
  this->odom_geom_pose[2] = 0.0;

  this->odom_geom_size[0] = 0.0;
  this->odom_geom_size[1] = 0.0;
  this->odom_geom_size[2] = 0.0;

  this->laser = NULL;
  this->laser_index = cf->ReadInt(section, "laser_index", 0);
  this->laser_time = 0.0;

  // The actual laser geometry is read from the laser device
  this->laser_geom_pose[0] = 0.0;
  this->laser_geom_pose[1] = 0.0;
  this->laser_geom_pose[2] = 0.0;
  
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

////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void VFH_Class::PutPose()
{
  uint32_t timesec, timeusec;
  player_position_data_t data;

  data.xpos = (int32_t)rint(this->odom_pose[0]);
  data.ypos = (int32_t)rint(this->odom_pose[1]);
  data.yaw = (int32_t)rint(this->odom_pose[2]);

  data.xspeed = this->odom_vel_be[0];
  data.yspeed = this->odom_vel_be[1];
  data.yawspeed = this->odom_vel_be[2];
  data.stall = this->odom_stall;

  // Byte swap
  data.xpos = htonl(data.xpos);
  data.ypos = htonl(data.ypos);
  data.yaw = htonl(data.yaw);
  // don't byteswap velocities; they're already in network byteorder

  // Compute timestamp
  timesec = (uint32_t) this->odom_time;
  timeusec = (uint32_t) (fmod(this->odom_time, 1.0) * 1e6);

  // Copy data to server.
  PutData((unsigned char*) &data, sizeof(data), timesec, timeusec);

  return;
}


