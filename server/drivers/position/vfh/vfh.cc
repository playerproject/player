#include <assert.h>
#include <math.h>
#include <fstream.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

class VFH_Class : public CDevice {
  public:
    // Constructor
    VFH_Class(char* interface, ConfigFile* cf, int section);

    // Destructor
    virtual ~VFH_Class();

    // Setup/shutdown routines.
    virtual int Setup();
    virtual int Shutdown();

    // Main function for device thread.
    virtual void Main();


  private:
    // Set up the odometry device.
    int SetupOdom();
    int ShutdownOdom();
    int GetOdom();

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
    void PutCommand();

    // Check for new commands from server
    void GetCommand();

    // Odometry device info
    CDevice *odom;
    int odom_index;
    double odom_time;

    // Odometric geometry (robot size and pose in robot cs)
    double odom_geom_pose[3];
    double odom_geom_size[3];

    // Pose of robot in odometric cs
    double odom_pose[3];

    // Velocity of robot in robot cs
    double odom_vel[3];

    // Laser device info
    CDevice *laser;
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


    // VFH Member variables
    float CELL_WIDTH;           // millimeters
    int SECTOR_ANGLE;           // degrees
    float ROBOT_RADIUS;         // millimeters
    float SAFETY_DIST;          // millimeters
    int WINDOW_DIAMETER;        // cells
    int CENTER_X;
    int CENTER_Y;
    int HIST_SIZE;
    int MAX_SPEED;
    int MAX_TURNRATE;

    vector<vector<float> > Cell_Direction;
    vector<vector<float> > Cell_Base_Mag;
    vector<vector<float> > Cell_Mag;
    vector<vector<float> > Cell_Dist;
    vector<vector<float> > Cell_Enlarge;
    vector<vector<vector<int> > > Cell_Sector;
    vector<float> Candidate_Angle;
    float Desired_Angle, Picked_Angle, Last_Picked_Angle;
    float U1, U2;

    float *Hist, *Last_Binary_Hist;

    float Binary_Hist_Low, Binary_Hist_High;

    // Minimum turning radius at different speeds, in millimeters
    int *Min_Turning_Radius;
    int Init(double cell_width, int window_diameter, int sector_angle,
                double robot_radius, double safety_dist, int max_speed, 
		int max_turnrate, double binary_hist_low, 
		double binary_hist_high, double u1, double u2); 
    int VFH_Allocate();
    int Update_VFH();

    float Delta_Angle(int a1, int a2);
    float Delta_Angle(float a1, float a2);
    int Bisect_Angle(int angle1, int angle2);

    int Calculate_Cells_Mag();
    int Build_Primary_Polar_Histogram();
    int Build_Binary_Polar_Histogram();
    int Build_Masked_Polar_Histogram(int speed);
    int Read_Min_Turning_Radius_From_File(char *filename);
    int Select_Candidate_Angle();
    int Select_Direction();
    int Set_Motion();

    void Print_Cells_Dir();
    void Print_Cells_Mag();
    void Print_Cells_Dist();
    void Print_Cells_Sector();
    void Print_Cells_Enlargement_Angle();
    void Print_Hist();
};

// Initialization function
CDevice* VFH_Init(char* interface, ConfigFile* cf, int section) {
  if (strcmp(interface, PLAYER_POSITION_STRING) != 0) { 
    PLAYER_ERROR1("driver \"vfh\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new VFH_Class(interface, cf, section)));
} 

// a driver registration function
void VFH_Register(DriverTable* table)
{ 
  table->AddDriver("vfh", PLAYER_ALL_MODE, VFH_Init);
  return;
} 

////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int VFH_Class::Setup() {
  player_position_cmd_t cmd;

  // Initialise the underlying position device.
  if (this->SetupOdom() != 0)
    return -1;

  // Initialise the laser.
  if (this->SetupLaser() != 0)
    return -1;

  // Initialize the command buffer
/*
  cmd.xpos = htonl((int32_t) (this->inc_pose.v[0] * 1000));
  cmd.ypos = htonl((int32_t) (this->inc_pose.v[1] * 1000));
  cmd.yaw = htonl((int32_t) (this->inc_pose.v[2] * 180 / M_PI));
  CDevice::PutCommand(NULL, (unsigned char*) &cmd, sizeof(cmd));
*/

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
int VFH_Class::SetupOdom() {
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

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying odom device.
int VFH_Class::ShutdownOdom() {
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
  int i;
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

  data.xspeed = ntohl(data.xspeed);
  data.yspeed = ntohl(data.yspeed);
  data.yawspeed = ntohl(data.yawspeed);

//  this->odom_pose.v[0] = (double) ((int32_t) data.xpos) / 1000.0;
//  this->odom_pose.v[1] = (double) ((int32_t) data.ypos) / 1000.0;
//  this->odom_pose.v[2] = (double) ((int32_t) data.yaw) * M_PI / 180;

  this->odom_pose[0] = (double) ((int32_t) data.xpos);
  this->odom_pose[1] = (double) ((int32_t) data.ypos);
  this->odom_pose[2] = (double) ((int32_t) data.yaw);

  this->odom_vel[0] = (double) ((int32_t) data.xspeed) / 1000.0;
  this->odom_vel[1] = (double) ((int32_t) data.yspeed) / 1000.0;
  this->odom_vel[2] = (double) ((int32_t) data.yawspeed) * M_PI / 180;

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
  double r, b, db;

  // Get the laser device data.
  size = this->laser->GetData(this,(unsigned char*) &data, sizeof(data), &timesec, &timeusec);
  if (size == 0)
    return 0;
  time = (double) timesec + ((double) timeusec) * 1e-6;

  // Dont do anything if this is old data.
  if (time - this->laser_time < 0.001)
    return 0;
  this->laser_time = time;

  b = ((int16_t) ntohs(data.min_angle)) / 100.0 * M_PI / 180.0;
  db = ((int16_t) ntohs(data.resolution)) / 100.0 * M_PI / 180.0;
  this->laser_count = ntohs(data.range_count);
  assert(this->laser_count < sizeof(this->laser_ranges) / sizeof(this->laser_ranges[0]));

  // Read and byteswap the range data
  for (i = 0; i < this->laser_count; i++)
  {
//    r = ((int16_t) ntohs(data.ranges[i])) / 1000.0;
    r = ((int16_t) ntohs(data.ranges[i]));
    this->laser_ranges[i][0] = r;
    this->laser_ranges[i][1] = b;
    b += db;
  }

  return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Send commands to the underlying position device
void VFH_Class::PutCommand() {
  player_position_cmd_t cmd;

//printf("Command: speed: %d turnrate: %d\n", speed, turnrate);

  this->con_vel[0] = (double)speed;
  this->con_vel[1] = 0;
  this->con_vel[2] = (double)turnrate;

  memset(&cmd, 0, sizeof(cmd));

  cmd.xspeed = (int32_t) (this->con_vel[0]);
  cmd.yspeed = (int32_t) (this->con_vel[1]);
  cmd.yawspeed = (int32_t) (this->con_vel[2]);

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
void VFH_Class::Main() {
  struct timespec sleeptime;
  float dist;
  struct timeval stime, time;

  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = 1000000L;

  this->GetOdom();

  /*REMOVE
  reset_odom_x = this->odom_pose[0];
  reset_odom_y = this->odom_pose[1];
  reset_odom_t = this->odom_pose[2];
  */

  while (true) {
//    gettimeofday(&stime, 0);
    // Sleep for 1ms (will actually take longer than this).
    nanosleep(&sleeptime, NULL);

//    gettimeofday(&time, 0);
//    printf("After sleep Time: %d %d\n",time.tv_sec - stime.tv_sec, time.tv_usec - stime.tv_usec);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    this->GetOdom();

    // Process any pending requests.
    this->HandleRequests();

    // Read client command
    this->GetCommand();

//    gettimeofday(&time, 0);
//    printf("Before VFH Time: %d %d\n",time.tv_sec - stime.tv_sec, time.tv_usec - stime.tv_usec);

    dist = sqrt(powf((goal_x - this->odom_pose[0]),2) + 
                powf((goal_y - this->odom_pose[1]),2));
    
    if (dist > 500) {
      Desired_Angle = atan2((goal_y - this->odom_pose[1]),
                            (goal_x - this->odom_pose[0])) * 180 / M_PI - this->odom_pose[2];

      while (Desired_Angle > 360.0) {
        Desired_Angle -= 360.0;
      }
      while (Desired_Angle < 0) {
        Desired_Angle += 360.0;
      }
//Desired_Angle = 90;
      printf("ANGLE: %f\tDIST: %f\n", Desired_Angle, dist);

      // Get new laser data.
      this->GetLaser();
      Update_VFH();
    } else {
      // At goal, stop
      speed = 0;
      turnrate = 0;
      this->PutCommand();
    }

//    gettimeofday(&time, 0);
//    printf("After VFH Time: %d %d\n",time.tv_sec - stime.tv_sec, time.tv_usec - stime.tv_usec);

    this->PutPose();
  }
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Check for new commands from the server
void VFH_Class::GetCommand() {
  player_position_cmd_t cmd;

  if (CDevice::GetCommand(&cmd, sizeof(cmd)) != 0) {
    goal_x = ntohl(cmd.xpos);
    goal_y = ntohl(cmd.ypos);
    goal_t = ntohl(cmd.yaw);

//    reset_odom_x = this->odom_pose[0];
//    reset_odom_y = this->odom_pose[1];
//    reset_odom_t = this->odom_pose[2];

    //printf("Received command to go to: (%d, %d)\n", goal_x, goal_y);
  }
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
VFH_Class::VFH_Class(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_position_data_t), sizeof(player_position_cmd_t), 10, 10)
{
  double size;
  double cell_size, robot_radius, safety_dist, free_space_cutoff, obs_cutoff;
  double weight_desired_dir, weight_current_dir;
  int window_diameter, sector_angle, max_speed, max_turnrate;

  this->speed = 0;
  this->turnrate = 0;

  this->CELL_WIDTH = 200.0;
  this->WINDOW_DIAMETER = 41;
  this->SECTOR_ANGLE = 5;
  this->ROBOT_RADIUS = 200.0;
  this->SAFETY_DIST = 200.0;

  this->CENTER_X = (int)floor(this->WINDOW_DIAMETER / 2.0);
  this->CENTER_Y = this->CENTER_X;
  this->HIST_SIZE = (int)rint(360.0 / this->SECTOR_ANGLE);

  this->MAX_SPEED = 200;
  this->MAX_TURNRATE = 40;

  this->U1 = 5;
  this->U2 = 3;

  this->Desired_Angle = 90;
  this->Picked_Angle = 90;
  this->Last_Picked_Angle = this->Picked_Angle;

  cell_size = cf->ReadLength(section, "cell_size", 100.0);
  window_diameter = cf->ReadInt(section, "window_diameter", 61);
  sector_angle = cf->ReadInt(section, "sector_angle", 5);
  robot_radius = cf->ReadLength(section, "robot_radius", 250.0);
  safety_dist = cf->ReadLength(section, "safety_dist", 100.0);
  max_speed = cf->ReadInt(section, "max_speed", 200);
  max_turnrate = cf->ReadInt(section, "max_turnrate", 40);
  free_space_cutoff = cf->ReadLength(section, "free_space_cutoff", 2000000.0);
  obs_cutoff = cf->ReadLength(section, "obs_cutoff", free_space_cutoff);
  weight_desired_dir = cf->ReadLength(section, "weight_desired_dir", 5.0);
  weight_current_dir = cf->ReadLength(section, "weight_current_dir", 3.0);

  // Allocate and intialize with defaults, for now
  this->Init(cell_size, window_diameter, sector_angle, robot_radius,
		  safety_dist, max_speed, max_turnrate, free_space_cutoff,
		  obs_cutoff, weight_desired_dir, weight_current_dir);

  
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
  return;
}

int VFH_Class::VFH_Allocate() {
  vector<float> temp_vec;
  vector<int> temp_vec3;
  vector<vector<int> > temp_vec2;
  int x;

  Cell_Direction.clear();
  Cell_Base_Mag.clear();
  Cell_Mag.clear();
  Cell_Dist.clear();
  Cell_Enlarge.clear();
  Cell_Sector.clear();

  temp_vec.clear();
  for(x=0;x<WINDOW_DIAMETER;x++) {
    temp_vec.push_back(0);
  }

  temp_vec2.clear();
  temp_vec3.clear();
  for(x=0;x<WINDOW_DIAMETER;x++) {
    temp_vec2.push_back(temp_vec3);
  }

  for(x=0;x<WINDOW_DIAMETER;x++) {
    Cell_Direction.push_back(temp_vec);
    Cell_Base_Mag.push_back(temp_vec);
    Cell_Mag.push_back(temp_vec);
    Cell_Dist.push_back(temp_vec);
    Cell_Enlarge.push_back(temp_vec);
    Cell_Sector.push_back(temp_vec2);
  }

  Hist = new float[HIST_SIZE];
  Last_Binary_Hist = new float[HIST_SIZE];
  Min_Turning_Radius = new int[MAX_SPEED+1];

  return(1);
}

int VFH_Class::Init(double cell_width, int window_diameter, int sector_angle, double robot_radius,
		double safety_dist, int max_speed, int max_turnrate, double binary_hist_low, double binary_hist_high,
		double u1, double u2) {
  int x, y, i;
  float plus_dir, neg_dir, plus_sector, neg_sector;
  bool plus_dir_bw, neg_dir_bw, dir_around_sector;
  float neg_sector_to_neg_dir, neg_sector_to_plus_dir;
  float plus_sector_to_neg_dir, plus_sector_to_plus_dir;

  CELL_WIDTH = cell_width;
  WINDOW_DIAMETER = window_diameter;
  SECTOR_ANGLE = sector_angle;
  ROBOT_RADIUS = robot_radius;
  SAFETY_DIST = safety_dist;

  CENTER_X = (int)floor(WINDOW_DIAMETER / 2.0);
  CENTER_Y = CENTER_X;
  HIST_SIZE = (int)rint(360.0 / SECTOR_ANGLE);

  MAX_SPEED = max_speed;
  MAX_TURNRATE = max_turnrate;

  Binary_Hist_Low = binary_hist_low;
  Binary_Hist_High = binary_hist_high;

  U1 = u1;
  U2 = u2;

  printf("CELL_WIDTH: %1.1f\tWINDOW_DIAMETER: %d\tSECTOR_ANGLE: %d\tROBOT_RADIUS: %1.1f\tSAFETY_DIST: %1.1f\tMAX_SPEED: %d\tMAX_TURNRATE: %d\tFree Space Cutoff: %1.1f\tObs Cutoff: %1.1f\tWeight Desired Dir: %1.1f\tWeight Current_Dir:%1.1f\n", CELL_WIDTH, WINDOW_DIAMETER, SECTOR_ANGLE, ROBOT_RADIUS, SAFETY_DIST, MAX_SPEED, MAX_TURNRATE, Binary_Hist_Low, Binary_Hist_High, U1, U2);

  VFH_Allocate();

  for(x=0;x<HIST_SIZE;x++) {
    Hist[x] = 0;
    Last_Binary_Hist[x] = 1;
  }

  for(x=0;x<=MAX_SPEED;x++) {
//    Min_Turning_Radius[x] = (int)rint(150.0 * x / 200.0);
    Min_Turning_Radius[x] = 150;
  }

  for(x=0;x<WINDOW_DIAMETER;x++) {
    for(y=0;y<WINDOW_DIAMETER;y++) {
      Cell_Mag[x][y] = 0;
      Cell_Dist[x][y] = sqrt(powf((CENTER_X - x), 2) + powf((CENTER_Y - y), 2)) * CELL_WIDTH;

      Cell_Base_Mag[x][y] = powf((3000.0 - Cell_Dist[x][y]), 4) / 100000000.0;

      if (x < CENTER_X) {
        if (y < CENTER_Y) {
          Cell_Direction[x][y] = atan((float)(CENTER_Y - y) / (float)(CENTER_X - x));
          Cell_Direction[x][y] *= (360.0 / 6.28);
          Cell_Direction[x][y] = 180.0 - Cell_Direction[x][y];
        } else if (y == CENTER_Y) {
          Cell_Direction[x][y] = 180.0;
        } else if (y > CENTER_Y) {
          Cell_Direction[x][y] = atan((float)(y - CENTER_Y) / (float)(CENTER_X - x));
          Cell_Direction[x][y] *= (360.0 / 6.28);
          Cell_Direction[x][y] = 180.0 + Cell_Direction[x][y];
        }
      } else if (x == CENTER_X) {
        if (y < CENTER_Y) {
          Cell_Direction[x][y] = 90.0;
        } else if (y == CENTER_Y) {
          Cell_Direction[x][y] = -1.0;
        } else if (y > CENTER_Y) {
          Cell_Direction[x][y] = 270.0;
        }
      } else if (x > CENTER_X) {
        if (y < CENTER_Y) {
          Cell_Direction[x][y] = atan((float)(CENTER_Y - y) / (float)(x - CENTER_X));
          Cell_Direction[x][y] *= (360.0 / 6.28);
        } else if (y == CENTER_Y) {
          Cell_Direction[x][y] = 0.0;
        } else if (y > CENTER_Y) {
          Cell_Direction[x][y] = atan((float)(y - CENTER_Y) / (float)(x - CENTER_X));
          Cell_Direction[x][y] *= (360.0 / 6.28);
          Cell_Direction[x][y] = 360.0 - Cell_Direction[x][y];
        }
      }

      if (Cell_Dist[x][y] > 0)
        Cell_Enlarge[x][y] = (float)atan((ROBOT_RADIUS + SAFETY_DIST) / Cell_Dist[x][y]) * 
		(360.0 / 6.28);
      else
        Cell_Enlarge[x][y] = 0;

      Cell_Sector[x][y].clear();
      for(i=0;i<(360 / SECTOR_ANGLE);i++) {
        plus_dir = Cell_Direction[x][y] + Cell_Enlarge[x][y];
        neg_dir = Cell_Direction[x][y] - Cell_Enlarge[x][y];
        plus_sector = (i + 1) * (float)SECTOR_ANGLE;
        neg_sector = i * (float)SECTOR_ANGLE;

        if ((neg_sector - neg_dir) > 180) {
          neg_sector_to_neg_dir = neg_dir - (neg_sector - 360);
        } else {
          if ((neg_dir - neg_sector) > 180) {
            neg_sector_to_neg_dir = neg_sector - (neg_dir + 360);
          } else {
            neg_sector_to_neg_dir = neg_dir - neg_sector;
          }
        }

        if ((plus_sector - neg_dir) > 180) {
          plus_sector_to_neg_dir = neg_dir - (plus_sector - 360);
        } else {
          if ((neg_dir - plus_sector) > 180) {
            plus_sector_to_neg_dir = plus_sector - (neg_dir + 360);
          } else {
            plus_sector_to_neg_dir = neg_dir - plus_sector;
          }
        }

        if ((plus_sector - plus_dir) > 180) {
          plus_sector_to_plus_dir = plus_dir - (plus_sector - 360);
        } else {
          if ((plus_dir - plus_sector) > 180) {
            plus_sector_to_plus_dir = plus_sector - (plus_dir + 360);
          } else {
            plus_sector_to_plus_dir = plus_dir - plus_sector;
          }
        }

        if ((neg_sector - plus_dir) > 180) {
          neg_sector_to_plus_dir = plus_dir - (neg_sector - 360);
        } else {
          if ((plus_dir - neg_sector) > 180) {
            neg_sector_to_plus_dir = neg_sector - (plus_dir + 360);
          } else {
            neg_sector_to_plus_dir = plus_dir - neg_sector;
          }
        }

        plus_dir_bw = 0;
        neg_dir_bw = 0;
        dir_around_sector = 0;

        if ((neg_sector_to_neg_dir >= 0) && (plus_sector_to_neg_dir <= 0)) {
          neg_dir_bw = 1; 
        }

        if ((neg_sector_to_plus_dir >= 0) && (plus_sector_to_plus_dir <= 0)) {
          plus_dir_bw = 1; 
        }

        if ((neg_sector_to_neg_dir <= 0) && (neg_sector_to_plus_dir >= 0)) {
          dir_around_sector = 1; 
        }

        if ((plus_sector_to_neg_dir <= 0) && (plus_sector_to_plus_dir >= 0)) {
          plus_dir_bw = 1; 
        }

        if ((plus_dir_bw) || (neg_dir_bw) || (dir_around_sector)) {
          Cell_Sector[x][y].push_back(i);
        }
      }
    }
  }
  return(1);
}

int VFH_Class::Update_VFH() {
  int print = 0;

  Build_Primary_Polar_Histogram();
  if (print) {
    printf("Primary Histogram\n");
    Print_Hist();
  }

  Build_Binary_Polar_Histogram();
  if (print) {
    printf("Binary Histogram\n");
    Print_Hist();
  }

  speed += 20;
  if (speed > MAX_SPEED) {
    speed = MAX_SPEED;
  }

  Build_Masked_Polar_Histogram(speed);
  if (print) {
    printf("Masked Histogram\n");
    Print_Hist();
  }

  Select_Direction();

  printf("Picked Angle: %f\n", Picked_Angle);

  if (Picked_Angle != -9999) {
    Set_Motion();
  } else {
    speed = 0;
    Set_Motion();
    return(1);
  }

  if (print)
    printf("SPEED: %d\t TURNRATE: %d\n", speed, turnrate);

  return(1);
}

float VFH_Class::Delta_Angle(int a1, int a2) {
  return(Delta_Angle((float)a1, (float)a2));
}

float VFH_Class::Delta_Angle(float a1, float a2) {
  float diff;

  diff = a2 - a1;

  if (diff > 180) {
    diff -= 360;
  } else if (diff < -180) {
    diff += 360;
  }

  return(diff);
}

int VFH_Class::Read_Min_Turning_Radius_From_File(char *filename) {
  int temp, i;

  ifstream infile(filename);

  i = 0;
  while (infile >> temp) {
    Min_Turning_Radius[i++] = temp;
  }

  infile.close();
  return(1);
}

void VFH_Class::Print_Cells_Dir() {
  int x, y;

  printf("\nCell Directions:\n");
  printf("****************\n");
  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      printf("%1.1f\t", Cell_Direction[x][y]);
    }
    printf("\n");
  }
}

void VFH_Class::Print_Cells_Mag() {
  int x, y;

  printf("\nCell Magnitudes:\n");
  printf("****************\n");
  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      printf("%1.1f\t", Cell_Mag[x][y]);
    }
    printf("\n");
  }
}

void VFH_Class::Print_Cells_Dist() {
  int x, y;

  printf("\nCell Distances:\n");
  printf("****************\n");
  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      printf("%1.1f\t", Cell_Dist[x][y]);
    }
    printf("\n");
  }
}

void VFH_Class::Print_Cells_Sector() {
  int x, y, i;

  printf("\nCell Sectors:\n");
  printf("****************\n");
  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      for(i=0;i<Cell_Sector[x][y].size();i++) {
        if (i < (Cell_Sector[x][y].size() -1 )) {
          printf("%d,", Cell_Sector[x][y][i]);
        } else {
          printf("%d\t", Cell_Sector[x][y][i]);
        }
      }
    }
    printf("\n");
  }
}

void VFH_Class::Print_Cells_Enlargement_Angle() {
  int x, y;

  printf("\nEnlargement Angles:\n");
  printf("****************\n");
  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      printf("%1.1f\t", Cell_Enlarge[x][y]);
    }
    printf("\n");
  }
}

void VFH_Class::Print_Hist() {
  int x;
  printf("Histogram:\n");
  printf("****************\n");

  for(x=0;x<=(HIST_SIZE/2);x++) {
    printf("%d:\t%1.1f\n", (x * SECTOR_ANGLE), Hist[x]);
  }
  printf("\n\n");
}

int VFH_Class::Calculate_Cells_Mag() {
  int x, y;

/*
printf("Laser Ranges\n");
printf("************\n");
for(x=0;x<=360;x++) {
   printf("%d: %f\n", x, this->laser_ranges[x][0]);
}
*/

  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      if (Cell_Direction[x][y] >= 180) {	// behind the robot, so cant sense, assume free
        Cell_Mag[x][y] = 0.0;
      } else {
        if ((Cell_Dist[x][y] + CELL_WIDTH / 2.0) > this->laser_ranges[(int)rint(Cell_Direction[x][y] * 2.0)][0]) {
          Cell_Mag[x][y] = Cell_Base_Mag[x][y];
        } else {
          Cell_Mag[x][y] = 0.0;
        }
      }
    }
  }

  return(1);
}

int VFH_Class::Build_Primary_Polar_Histogram() {
  int x, y, i;

  for(x=0;x<HIST_SIZE;x++) {
    Hist[x] = 0;
  }

  Calculate_Cells_Mag();

//  Print_Cells_Dist();
//  Print_Cells_Dir();
//  Print_Cells_Mag();
//  Print_Cells_Sector();
//  Print_Cells_Enlargement_Angle();

  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      for(i=0;i<Cell_Sector[x][y].size();i++) {
        Hist[Cell_Sector[x][y][i]] += Cell_Mag[x][y];
      }
    }
  }

  return(1);
}

int VFH_Class::Build_Binary_Polar_Histogram() {
  int x;

  for(x=0;x<HIST_SIZE;x++) {
    if (Hist[x] > Binary_Hist_High) {
      Hist[x] = 1.0;
    } else if (Hist[x] < Binary_Hist_Low) {
      Hist[x] = 0.0;
    } else {
      Hist[x] = Last_Binary_Hist[x];
    }
  }

  for(x=0;x<HIST_SIZE;x++) {
    Last_Binary_Hist[x] = Hist[x];
  }

  return(1);
}

int VFH_Class::Build_Masked_Polar_Histogram(int speed) {
  int x, y;
  float center_x_right, center_x_left, center_y, dist_r, dist_l;
  float theta, phi_b, phi_l, phi_r, total_dist, angle;

  center_x_right = CENTER_X + (Min_Turning_Radius[speed] / (float)CELL_WIDTH);
  center_x_left = CENTER_X - (Min_Turning_Radius[speed] / (float)CELL_WIDTH);
  center_y = CENTER_Y;

  theta = 90;
  phi_b = theta + 180;
  phi_l = phi_b;
  phi_r = phi_b;

  phi_l = 180;
  phi_r = 0;

  if (speed > 0) {
    total_dist = Min_Turning_Radius[speed] + ROBOT_RADIUS + SAFETY_DIST;
  } else {
    total_dist = Min_Turning_Radius[speed] + ROBOT_RADIUS;
  }

  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      if (Cell_Mag[x][y] == 0) 
        continue;

      if ((Delta_Angle(Cell_Direction[x][y], theta) > 0) && 
		(Delta_Angle(Cell_Direction[x][y], phi_r) <= 0)) {
        dist_r = sqrt(powf((center_x_right - x), 2) + powf((center_y - y), 2)) * CELL_WIDTH;
        if (dist_r < total_dist) { 
          phi_r = Cell_Direction[x][y];
        }
      } else {
        if ((Delta_Angle(Cell_Direction[x][y], theta) <= 0) && 
		(Delta_Angle(Cell_Direction[x][y], phi_l) > 0)) {
          dist_l = sqrt(powf((center_x_left - x), 2) + powf((center_y - y), 2)) * CELL_WIDTH;
          if (dist_l < total_dist) { 
            phi_l = Cell_Direction[x][y];
          }
        }
      }
    }
  }

  for(x=0;x<HIST_SIZE;x++) {
    angle = x * SECTOR_ANGLE;
    if ((Hist[x] == 0) && (((Delta_Angle((float)angle, phi_r) <= 0) && 
	(Delta_Angle((float)angle, theta) >= 0)) || 
      	((Delta_Angle((float)angle, phi_l) >= 0) &&
	(Delta_Angle((float)angle, theta) <= 0)))) {
      Hist[x] = 0;
    } else {
      Hist[x] = 1;
    }
  }

  return(1);
}

int VFH_Class::Bisect_Angle(int angle1, int angle2) {
  float a;
  int angle;

  a = Delta_Angle((float)angle1, (float)angle2);

  angle = (int)rint(angle1 + (a / 2.0));
  if (angle < 0) {
    angle += 360;
  } else if (angle >= 360) {
    angle -= 360;
  }

  return(angle);
}

int VFH_Class::Select_Candidate_Angle() {
  int i;
  float weight, min_weight;

  if (Candidate_Angle.size() == 0) {
    Picked_Angle = -9999;
    return(1);
  }

  Picked_Angle = 90;
  min_weight = 10000000;
  for(i=0;i<Candidate_Angle.size();i++) {
//printf("CANDIDATE: %f\n", Candidate_Angle[i]);
    weight = U1 * fabs(Delta_Angle(Desired_Angle, Candidate_Angle[i])) +
    	U2 * fabs(Delta_Angle(Last_Picked_Angle, Candidate_Angle[i]));
    if (weight < min_weight) {
      min_weight = weight;
      Picked_Angle = Candidate_Angle[i];
    }
  }

  Last_Picked_Angle = Picked_Angle;

  return(1);
}

int VFH_Class::Select_Direction() {
  int start, i, left;
  float angle, new_angle;
  vector<pair<int,int> > border;
  pair<int,int> new_border;

  Candidate_Angle.clear();

  start = -1; 
  for(i=0;i<HIST_SIZE;i++) {
    if (Hist[i] == 1) {
      start = i;
      break;
    }
  }

  if (start == -1) {
    Candidate_Angle.push_back(Desired_Angle);
    return(1);
  }

  border.clear();

//printf("Start: %d\n", start);
  left = 1;
  for(i=start;i<=(start+HIST_SIZE);i++) {
    if ((Hist[i % HIST_SIZE] == 0) && (left)) {
      new_border.first = (i % HIST_SIZE) * SECTOR_ANGLE;
      left = 0;
    }

    if ((Hist[i % HIST_SIZE] == 1) && (!left)) {
      new_border.second = ((i % HIST_SIZE) - 1) * SECTOR_ANGLE;
      if (new_border.second < 0) {
        new_border.second += 360;
      }
      border.push_back(new_border);
      left = 1;
    }
  }

  for(i=0;i<border.size();i++) {
//printf("BORDER: %f %f\n", border[i].first, border[i].second);
    angle = Delta_Angle(border[i].first, border[i].second);

    if (fabs(angle) < 10) {
      continue;
    }

    if (fabs(angle) < 80) {
      new_angle = border[i].first + (border[i].second - border[i].first) / 2.0;

      Candidate_Angle.push_back(new_angle);
    } else {
      new_angle = border[i].first + (border[i].second - border[i].first) / 2.0;

      Candidate_Angle.push_back(new_angle);

      new_angle = (float)((border[i].first + 40) % 360);
      Candidate_Angle.push_back(new_angle);

      new_angle = (float)(border[i].second - 40);
      if (new_angle < 0) 
        new_angle += 360;
      Candidate_Angle.push_back(new_angle);

      // See if candidate dir is in this opening
      if ((Delta_Angle(Desired_Angle, Candidate_Angle[Candidate_Angle.size()-2]) < 0) && 
		(Delta_Angle(Desired_Angle, Candidate_Angle[Candidate_Angle.size()-1]) > 0)) {
        Candidate_Angle.push_back(Desired_Angle);
      }
    }
  }

  Select_Candidate_Angle();

  return(1);
}

int VFH_Class::Set_Motion() { 
  int i;

  // This happens if all directions blocked, so just spin in place
  if (speed <= 0) {
    turnrate = 40;
    speed = 0;
  } else {
//  printf("Picked %f\n", Picked_Angle);
    if ((Picked_Angle > 270) && (Picked_Angle < 360)) {
      turnrate = -1 * MAX_TURNRATE;
    } else if ((Picked_Angle < 270) && (Picked_Angle > 180)) {
      turnrate = MAX_TURNRATE;
    } else {
      turnrate = (int)rint(((float)(Picked_Angle - 90) / 75.0) * MAX_TURNRATE);

      if (turnrate > MAX_TURNRATE) {
        turnrate = MAX_TURNRATE;
      } else if (turnrate < (-1 * MAX_TURNRATE)) {
        turnrate = -1 * MAX_TURNRATE;
      }

//      if (abs(turnrate) > (0.9 * MAX_TURNRATE)) {
//        speed = 0;
//      }
    }
  }

  this->PutCommand();

  return(1);
}

////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void VFH_Class::PutPose()
{
  uint32_t timesec, timeusec;
  player_position_data_t data;

  this->GetOdom();

  data.xpos = (int32_t)rint(this->odom_pose[0]);
  data.ypos = (int32_t)rint(this->odom_pose[1]);
  data.yaw = (int32_t)rint(this->odom_pose[2]);

  data.xspeed = (int32_t)rint(this->odom_vel[0]);
  data.yspeed = (int32_t)rint(this->odom_vel[1]);
  data.yawspeed = (int32_t)rint(this->odom_vel[2]);


/*
  // Pose esimate
  data.xpos = (int32_t) (this->inc_pose.v[0] * 1000);
  data.ypos = (int32_t) (this->inc_pose.v[1] * 1000);
  data.yaw = (int32_t) (this->inc_pose.v[2] * 180 / M_PI);
  
  // Velocity estimate (use odometry device's pose esimate)
  data.xspeed = (int32_t) (this->inc_vel.v[0] * 1000);
  data.yspeed = (int32_t) (this->inc_vel.v[1] * 1000);
  data.yawspeed = (int32_t) (this->inc_vel.v[2] * 180 / M_PI);
*/
  
  // Byte swap
  data.xpos = htonl(data.xpos);
  data.ypos = htonl(data.ypos);
  data.yaw = htonl(data.yaw);
  data.xspeed = htonl(data.xspeed);
  data.yspeed = htonl(data.yspeed);
  data.yawspeed = htonl(data.yawspeed);

  // Compute time.  Use the laser device's time.
  timesec = (uint32_t) this->laser_time;
  timeusec = (uint32_t) (fmod(this->laser_time, 1.0) * 1e6);

  // Copy data to server.
  PutData((unsigned char*) &data, sizeof(data), timesec, timeusec);

  return;
}


