/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000 Brian Gerkey and others
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
///////////////////////////////////////////////////////////////////////////
//
// Desc: Adaptive Monte-Carlo localization
// Author: Andrew Howard
// Date: 6 Feb 2003
// CVS: $Id$
//
// Theory of operation:
//  TODO
//
// Requires: position (odometry)
// Provides: localization
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <sys/types.h>
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>
#include <sys/time.h>

#define PLAYER_ENABLE_TRACE 1
#define PLAYER_ENABLE_MSG 1

// TESTING
#include "playertime.h"
//#define INCLUDE_OUTFILE 1
extern PlayerTime* GlobalTime;

#include "deviceregistry.h"
#include "amcl.h"

// Sensors
#include "amcl_odom.h"
#include "amcl_laser.h"
//#include "amcl_gps.h"
//#include "amcl_imu.h"


////////////////////////////////////////////////////////////////////////////////
// Create an instance of the driver
Driver* AdaptiveMCL_Init( ConfigFile* cf, int section)
{
  return ((Driver*) (new AdaptiveMCL(cf, section)));
}


////////////////////////////////////////////////////////////////////////////////
// Register the driver
void AdaptiveMCL_Register(DriverTable* table)
{
  table->AddDriver("amcl", AdaptiveMCL_Init);
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Useful macros
#define AMCL_DATASIZE MAX(sizeof(player_localize_data_t), sizeof(player_position_data_t))

  
////////////////////////////////////////////////////////////////////////////////
// Constructor
AdaptiveMCL::AdaptiveMCL( ConfigFile* cf, int section)
    : Driver(cf, section)
{
  int i;
  double u[3];
  AMCLSensor *sensor;

  // Work out what interface we should use
  if (cf->ReadDeviceId(section, 0, -1, &this->device_id) != 0)
  {
    this->SetError(-1);
    return;
  }

  // Create an interface 
  if (this->AddInterface(this->device_id, PLAYER_READ_MODE, 
                         AMCL_DATASIZE, 0, 100, 100) != 0)
  {
    this->SetError(-1);    
    return;
  }

  // Check for compatible codes
  if (!(this->device_id.code == PLAYER_LOCALIZE_CODE ||
        this->device_id.code == PLAYER_POSITION_CODE))
  {
    PLAYER_ERROR1("driver \"amcl\" does not support interface \"%s\"\n",
                  ::lookup_interface_name(0, this->device_id.code));
    this->SetError(-1);
    return;
  }

  // HACK; fix this later
  // Remember which interface we go opened for
  this->interface = strdup(::lookup_interface_name(0, this->device_id.code));
  
  
  this->init_sensor = -1;
  this->action_sensor = -1;
  this->sensor_count = 0;

  // Create odometry sensor
  if (cf->ReadInt(section, "odom_index", -1) >= 0)
  {
    this->action_sensor = this->sensor_count;
    if (cf->ReadInt(section, "odom_init", 1))
      this->init_sensor = this->sensor_count;
    sensor = new AMCLOdom();
    sensor->is_action = 1;
    this->sensors[this->sensor_count++] = sensor;
  }

  // Create laser sensor
  if (cf->ReadInt(section, "laser_index", -1) >= 0)
  {
    sensor = new AMCLLaser();
    sensor->is_action = 0;
    this->sensors[this->sensor_count++] = sensor;
  }

  /*
  // Create GPS sensor
  if (cf->ReadInt(section, "gps_index", -1) >= 0)
  {
    if (cf->ReadInt(section, "gps_init", 1))
      this->init_sensor = this->sensor_count;
    this->sensors[this->sensor_count++] = new AMCLGps();
  }

  // Create IMU sensor
  if (cf->ReadInt(section, "imu_index", -1) >= 0)
    this->sensors[this->sensor_count++] = new AMCLImu();
  */

  // Load sensor settings
  for (i = 0; i < this->sensor_count; i++)
    this->sensors[i]->Load(cf, section);  
  
  // Create the sensor data queue
  this->q_len = 0;
  this->q_size = 20000;
  this->q_data = new AMCLSensorData*[this->q_size];

  // Particle filter settings
  this->pf = NULL;
  this->pf_min_samples = cf->ReadInt(section, "pf_min_samples", 100);
  this->pf_max_samples = cf->ReadInt(section, "pf_max_samples", 10000);

  // Adaptive filter parameters
  this->pf_err = cf->ReadFloat(section, "pf_err", 0.01);
  this->pf_z = cf->ReadFloat(section, "pf_z", 3);

  // Initial pose estimate
  this->pf_init_pose_mean = pf_vector_zero();
  this->pf_init_pose_mean.v[0] = cf->ReadTupleLength(section, "init_pose", 0, 0);
  this->pf_init_pose_mean.v[1] = cf->ReadTupleLength(section, "init_pose", 1, 0);
  this->pf_init_pose_mean.v[2] = cf->ReadTupleAngle(section, "init_pose", 2, 0);

  // Initial pose covariance
  u[0] = cf->ReadTupleLength(section, "init_pose_var", 0, 1e3);
  u[1] = cf->ReadTupleLength(section, "init_pose_var", 1, 1e3);
  u[2] = cf->ReadTupleAngle(section, "init_pose_var", 2, 1e2);
  this->pf_init_pose_cov = pf_matrix_zero();
  this->pf_init_pose_cov.m[0][0] = u[0] * u[0];
  this->pf_init_pose_cov.m[1][1] = u[1] * u[1];
  this->pf_init_pose_cov.m[2][2] = u[2] * u[2];

  // Update distances
  this->min_dr = cf->ReadTupleLength(section, "update_thresh", 0, 0.20);
  this->min_da = cf->ReadTupleAngle(section, "update_thresh", 0, 30 * M_PI / 180);

  // Initial hypothesis list
  this->hyp_count = 0;

#ifdef INCLUDE_RTKGUI
  // Enable debug gui
  this->enable_gui = cf->ReadInt(section, "enable_gui", 0);
#endif

  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
AdaptiveMCL::~AdaptiveMCL(void)
{
  int i;

  // Delete sensor data queue
  delete[] this->q_data;

  // Delete sensors
  for (i = 0; i < this->sensor_count; i++)
  {
    this->sensors[i]->Unload();
    delete this->sensors[i];
  }
  this->sensor_count = 0;

  free(this->interface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int AdaptiveMCL::Setup(void)
{
  int i;
    
  PLAYER_TRACE0("setup");

  // Create the particle filter
  assert(this->pf == NULL);
  this->pf = pf_alloc(this->pf_min_samples, this->pf_max_samples);
  this->pf->pop_err = this->pf_err;
  this->pf->pop_z = this->pf_z;

  // Start sensors
  for (i = 0; i < this->sensor_count; i++)
    if (this->sensors[i]->Setup() < 0)
      return -1;

  this->q_len = 0;

  // No data has yet been pushed, and the
  // filter has not yet been initialized
  this->pf_init = false;

  // Initial hypothesis list
  this->hyp_count = 0;
  
  // Start the driver thread.
  PLAYER_MSG0("running");
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int AdaptiveMCL::Shutdown(void)
{
  int i;
  
  PLAYER_TRACE0("shutting down");
    
  // Stop the driver thread.
  this->StopThread();

  // Stop sensors
  for (i = 0; i < this->sensor_count; i++)
    this->sensors[i]->Shutdown();

  // Delete the particle filter
  pf_free(this->pf);
  this->pf = NULL;

  PLAYER_TRACE0("shutdown done");
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for updated sensor data
void AdaptiveMCL::Update(void)
{
  int i;
  AMCLSensorData *data;

  assert(this->action_sensor >= 0 && this->action_sensor < this->sensor_count);
  
  // Check the action sensor for new data
  data = this->sensors[this->action_sensor]->GetData();
  if (data == NULL)
    return;

  //printf("push odom pos\n");
  //pf_vector_fprintf(((AMCLOdomData*) data)->pose, stdout, "%.3f");

  // Flag this as action data and push onto queue
  this->Push(data);

  // Check the remaining sensors for new data
  for (i = 0; i < this->sensor_count; i++)
  {
    data = this->sensors[i]->GetData();
    if (data != NULL)
      this->Push(data);
  }

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Push data onto the filter queue.
void AdaptiveMCL::Push(AMCLSensorData *data)
{
  int i;

  this->Lock();

  if (this->q_len >= this->q_size)
  {
    this->Unlock();
    PLAYER_ERROR("queue overflow");
    return;
  }
  i = (this->q_start + this->q_len++) % this->q_size;

  this->q_data[i] = data;

  this->Unlock();  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Take a peek at the queue 
AMCLSensorData *AdaptiveMCL::Peek(void)
{
  int i;

  this->Lock();
  
  if (this->q_len == 0)
  {
    this->Unlock();  
    return NULL;
  }
  i = this->q_start % this->q_size;

  this->Unlock();  
  return this->q_data[i];
}


////////////////////////////////////////////////////////////////////////////////
// Pop data from the filter queue
AMCLSensorData *AdaptiveMCL::Pop(void)
{
  int i;

  this->Lock();
  
  if (this->q_len == 0)
  {
    this->Unlock();  
    return NULL;
  }
  i = this->q_start++ % this->q_size;
  this->q_len--;

  this->Unlock();  
  return this->q_data[i];
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void AdaptiveMCL::Main(void) 
{  
  struct timespec sleeptime;
  
  // WARNING: this only works for Linux
  // Run at a lower priority
  nice(10);

#ifdef INCLUDE_RTKGUI
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  
  // Start the GUI
  if (this->enable_gui)
    this->SetupGUI();
#endif

#ifdef INCLUDE_OUTFILE
  // Open file for logging results
  this->outfile = fopen("amcl.out", "w+");
#endif
    
  while (true)
  {
#ifdef INCLUDE_RTKGUI
    if (this->enable_gui)
    {
      rtk_canvas_render(this->canvas);
      rtk_app_main_loop(this->app);
    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
#endif

    // Sleep for 1ms (will actually take longer than this).
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 1000000L;
    nanosleep(&sleeptime, NULL);

    // Test if we are supposed to cancel this thread.  This is the
    // only place we can cancel from if we are running the GUI.
    pthread_testcancel();

#ifdef INCLUDE_RTKGUI
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
#endif

    // Process any pending requests.
    this->HandleRequests();

    // Initialize the filter if we havent already done so
    if (!this->pf_init)
      this->InitFilter();
    
    // Update the filter
    if (this->UpdateFilter())
    {
#ifdef INCLUDE_OUTFILE
      // Save the error values
      pf_vector_t mean;
      double var;

      pf_get_cep_stats(pf, &mean, &var);

      printf("amcl %.3f %.3f %f\n", mean.v[0], mean.v[1], mean.v[2] * 180 / M_PI);

      fprintf(this->outfile, "%d.%03d unknown 6665 localize 01 %d.%03d ",
              0, 0, 0, 0);
      fprintf(this->outfile, "1.0 %e %e %e %e 0 0 0 0 0 0 0 0 \n",
              mean.v[0], mean.v[1], mean.v[2], var);
      fflush(this->outfile);
#endif
    }
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Thread finalization
void AdaptiveMCL::MainQuit()
{
#ifdef INCLUDE_RTKGUI
  // Stop the GUI
  if (this->enable_gui)
    this->ShutdownGUI();
#endif

#ifdef INCLUDE_OUTFILE
  // Close the log file
  fclose(this->outfile);
#endif
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Initialize the filter
void AdaptiveMCL::InitFilter(void)
{
  ::pf_init(this->pf, this->pf_init_pose_mean, this->pf_init_pose_cov);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the filter with new sensor data
bool AdaptiveMCL::UpdateFilter(void)
{
  int i;
  uint32_t tsec, tusec;
  double weight;
  pf_vector_t pose, delta;
  pf_vector_t pose_mean;
  pf_matrix_t pose_cov;
  amcl_hyp_t *hyp;
  AMCLSensorData *data;
  bool update;

  //printf("q len %d\n", this->q_len);

  // Get the action data
  data = this->Pop();
  if (data == NULL)
    return false;
  if (!data->sensor->is_action)
    return false;
  
  // Use the action timestamp 
  tsec = data->tsec;
  tusec = data->tusec;

  // HACK
  pose = ((AMCLOdomData*) data)->pose;
  delta = pf_vector_zero();
  update = false;

  //printf("odom pose\n");
  //pf_vector_fprintf(pose, stdout, "%.3f");

  // Compute change in pose
  if (this->pf_init)
  {
    // Compute change in pose
    delta = pf_vector_coord_sub(pose, this->pf_odom_pose);

    // See if we should update the filter
    update = fabs(delta.v[0]) > this->min_dr ||
      fabs(delta.v[1]) > this->min_dr ||   
      fabs(delta.v[2]) > this->min_da;
  }

  // If the filter has not been initialized
  if (!this->pf_init)
  {
    // Discard this action data
    delete data; data = NULL;

    // Pose at last filter update
    this->pf_odom_pose = pose;

    // Filter is now initialized
    this->pf_init = true;

    // Should update sensor data
    update = true;

    //printf("init\n");
    //pf_vector_fprintf(pose, stdout, "%.3f");
  }

  // If the robot has moved, update the filter
  else if (this->pf_init && update)
  {
    //printf("pose\n");
    //pf_vector_fprintf(pose, stdout, "%.3f");

    // HACK
    // Modify the delta in the action data so the filter gets
    // updated correctly
    ((AMCLOdomData*) data)->delta = delta;
    
    // Use the action data to update the filter
    data->sensor->UpdateAction(this->pf, data);
    delete data; data = NULL;

    // Pose at last filter update
    this->pf_odom_pose = pose;
  }
  else
  {
    // Discard this action data
    delete data; data = NULL;
  }

  // If the robot has moved, update the filter
  if (update)
  {
    // Process the remaining sensor data up to the next action data
    while (1)
    {
      data = this->Peek();
      if (data == NULL)
        break;
      if (data->sensor->is_action)
        break;
      data = this->Pop();
      assert(data);

      // Use the data to update the filter
      data->sensor->UpdateSensor(this->pf, data);

#ifdef INCLUDE_RTKGUI
      // Draw the current sensor data
      if (this->enable_gui)
        data->sensor->UpdateGUI(this->canvas, this->robot_fig, data);
#endif

      // Discard data
      delete data; data = NULL;
    }
    
    // Resample the particles
    pf_update_resample(this->pf);

    // Read out the current hypotheses
    this->hyp_count = 0;
    for (i = 0; (size_t) i < sizeof(this->hyps) / sizeof(this->hyps[0]); i++)
    {
      if (!pf_get_cluster_stats(this->pf, i, &weight, &pose_mean, &pose_cov))
        break;

      //pf_vector_fprintf(pose_mean, stdout, "%.3f");

      hyp = this->hyps + this->hyp_count++;
      hyp->weight = weight;
      hyp->pf_pose_mean = pose_mean;
      hyp->pf_pose_cov = pose_cov;
    }

#ifdef INCLUDE_RTKGUI
    // Update the GUI
    if (this->enable_gui)
      this->UpdateGUI();
#endif
    
    // Encode data to send to server
    if (strcmp(this->interface, PLAYER_LOCALIZE_STRING) == 0)
      this->PutDataLocalize(tsec, tusec);
    else if (strcmp(this->interface, PLAYER_POSITION_STRING) == 0)
      this->PutDataPosition(tsec, tusec, delta);

    return true;
  }
  else
  {
    // Process the remaining sensor data up to the next action data
    while (1)
    {
      data = this->Peek();
      if (data == NULL)
        break;
      if (data->sensor->is_action)
        break;
      data = this->Pop();
      assert(data);
      delete data; data = NULL;
    }
    
    // Encode data to send to server; only the position interface
    // gets updates every cycle
    if (strcmp(this->interface, PLAYER_POSITION_STRING) == 0)
      this->PutDataPosition(tsec, tusec, delta);

    return false;
  }
}

// this function will be passed to qsort(3) to sort the hypoths before
// sending them out.  the idea is to sort them in descending order of
// weight.
int
hypoth_compare(const void* h1, const void* h2)
{
  const player_localize_hypoth_t* hyp1 = (const player_localize_hypoth_t*)h1;
  const player_localize_hypoth_t* hyp2 = (const player_localize_hypoth_t*)h2;
  if(hyp1->alpha < hyp2->alpha)
    return(1);
  else if(hyp1->alpha == hyp2->alpha)
    return(0);
  else
    return(-1);
}


////////////////////////////////////////////////////////////////////////////////
// Output data on the localize interface
void AdaptiveMCL::PutDataLocalize(uint32_t tsec, uint32_t tusec)
{
  int i, j, k;
  amcl_hyp_t *hyp;
  double scale[3];
  pf_vector_t pose;
  pf_matrix_t pose_cov;
  size_t datalen;
  player_localize_data_t data;

  // Record the number of pending observations
  data.pending_count = 0;
  data.pending_time_sec = 0;
  data.pending_time_usec = 0;
  
  // Encode the hypotheses
  data.hypoth_count = this->hyp_count;
  for (i = 0; i < this->hyp_count; i++)
  {
    hyp = this->hyps + i;

    // Get the current estimate
    pose = hyp->pf_pose_mean;
    pose_cov = hyp->pf_pose_cov;

    // Check for bad values
    if (!pf_vector_finite(pose))
    {
      pf_vector_fprintf(pose, stderr, "%e");
      assert(0);
    }
    if (!pf_matrix_finite(pose_cov))
    {
      pf_matrix_fprintf(pose_cov, stderr, "%e");
      assert(0);
    }

    scale[0] = 1000;
    scale[1] = 1000;
    scale[2] = 3600 * 180 / M_PI;
    
    data.hypoths[i].alpha = (uint32_t) (hyp->weight * 1e6);
        
    data.hypoths[i].mean[0] = (int32_t) (pose.v[0] * scale[0]);
    data.hypoths[i].mean[1] = (int32_t) (pose.v[1] * scale[1]);
    data.hypoths[i].mean[2] = (int32_t) (pose.v[2] * scale[2]);
  
    data.hypoths[i].cov[0][0] = (int64_t) (pose_cov.m[0][0] * scale[0] * scale[0]);
    data.hypoths[i].cov[0][1] = (int64_t) (pose_cov.m[0][1] * scale[1] * scale[1]);
    data.hypoths[i].cov[0][2] = 0;
  
    data.hypoths[i].cov[1][0] = (int64_t) (pose_cov.m[1][0] * scale[0] * scale[0]);
    data.hypoths[i].cov[1][1] = (int64_t) (pose_cov.m[1][1] * scale[1] * scale[1]);
    data.hypoths[i].cov[1][2] = 0;

    data.hypoths[i].cov[2][0] = 0;
    data.hypoths[i].cov[2][1] = 0;
    data.hypoths[i].cov[2][2] = (int64_t) (pose_cov.m[2][2] * scale[2] * scale[2]);
  }
  
  // sort according to weight
  qsort((void*)data.hypoths,data.hypoth_count,
        sizeof(player_localize_hypoth_t),&hypoth_compare);

  // Compute the length of the data packet
  datalen = sizeof(data) - sizeof(data.hypoths) + data.hypoth_count * sizeof(data.hypoths[0]);

  // Byte-swap
  data.pending_count = htons(data.pending_count);
  data.pending_time_sec = htonl(data.pending_time_sec);
  data.pending_time_usec = htonl(data.pending_time_usec);

  // Byte-swap
  for (i = 0; (size_t) i < data.hypoth_count; i++)
  {
    for (j = 0; j < 3; j++)
    {
      data.hypoths[i].mean[j] = htonl(data.hypoths[i].mean[j]);
      for (k = 0; k < 3; k++)
        data.hypoths[i].cov[j][k] = htonll(data.hypoths[i].cov[j][k]);
    }
    data.hypoths[i].alpha = htonl(data.hypoths[i].alpha);
  }
  data.hypoth_count = htonl(data.hypoth_count);

  struct timeval timestamp;
  timestamp.tv_sec = tsec;
  timestamp.tv_usec = tusec;
  // Copy data to server
  ((Driver*) this)->PutData((char*) &data, datalen, &timestamp);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Output data on the position interface
void AdaptiveMCL::PutDataPosition(uint32_t tsec, uint32_t tusec, pf_vector_t delta)
{
  int i;
  amcl_hyp_t *hyp;
  pf_vector_t pose;
  pf_matrix_t pose_cov;
  player_position_data_t data;
  double max_weight;

  data.xpos = data.ypos = data.yaw = 0;
  data.xspeed = data.yspeed = data.yawspeed = 0;
  data.stall = 0;

  max_weight = 0.0;
  
  // Encode the hypotheses
  for (i = 0; i < this->hyp_count; i++)
  {
    hyp = this->hyps + i;

    // Get the current estimate
    pose = hyp->pf_pose_mean;
    pose_cov = hyp->pf_pose_cov;

    // Add the accumulated odometric change
    pose = pf_vector_coord_add(delta, pose);

    // Check for bad values
    if (!pf_vector_finite(pose))
    {
      pf_vector_fprintf(pose, stderr, "%e");
      assert(0);
    }
    if (!pf_matrix_finite(pose_cov))
    {
      pf_matrix_fprintf(pose_cov, stderr, "%e");
      assert(0);
    }

    if (hyp->weight > max_weight)
    {
      max_weight = hyp->weight;
      data.xpos = (int) (1000 * pose.v[0]);
      data.ypos = (int) (1000 * pose.v[1]);
      data.yaw = (int) (180 / M_PI * pose.v[2]);
    }
  }

  // Byte-swap
  data.xpos = htonl(data.xpos);
  data.ypos = htonl(data.ypos);
  data.yaw = htonl(data.yaw);
  
  struct timeval timestamp;
  timestamp.tv_sec = tsec;
  timestamp.tv_usec = tusec;
  // Copy data to server
  ((Driver*) this)->PutData((char*) &data, sizeof(data), &timestamp);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int AdaptiveMCL::HandleRequests(void)
{
  int len;
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  
  while ((len = GetConfig(&client, &request, sizeof(request),NULL)) > 0)
  {
    switch (request[0])
    {
      case PLAYER_LOCALIZE_SET_POSE_REQ:
        HandleSetPose(client, request, len);
        break;
      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Handle the set pose request
void AdaptiveMCL::HandleSetPose(void *client, void *request, int len)
{
  int reqlen;
  player_localize_set_pose_t req;
  pf_vector_t pose;
  pf_matrix_t cov;

  // Expected length of request
  reqlen = sizeof(req);
  
  // check if the config request is valid
  if (len != reqlen)
  {
    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, reqlen);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  req = *((player_localize_set_pose_t*) request);

  pose.v[0] = ((int32_t) ntohl(req.mean[0])) / 1000.0;
  pose.v[1] = ((int32_t) ntohl(req.mean[1])) / 1000.0;
  pose.v[2] = ((int32_t) ntohl(req.mean[2])) / 3600.0 * M_PI / 180;
    
  cov = pf_matrix_zero();
  cov.m[0][0] = ((int64_t) ntohll(req.cov[0][0])) / 1e6;
  cov.m[0][1] = ((int64_t) ntohll(req.cov[0][1])) / 1e6;
  cov.m[1][0] = ((int64_t) ntohll(req.cov[1][0])) / 1e6;
  cov.m[1][1] = ((int64_t) ntohll(req.cov[1][1])) / 1e6;
  cov.m[2][2] = ((int64_t) ntohll(req.cov[2][2])) / (3600.0 * 3600.0) * (M_PI / 180 * M_PI / 180);

  // Re-initialize the filter
  this->pf_init_pose_mean = pose;
  this->pf_init_pose_cov = cov;
  this->pf_init = false;

  // Give them an ack
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Set up the GUI
int AdaptiveMCL::SetupGUI(void)
{
  int i;
  
  // Initialize RTK
  rtk_init(NULL, NULL);

  this->app = rtk_app_create();

  this->canvas = rtk_canvas_create(this->app);
  rtk_canvas_title(this->canvas, "AdaptiveMCL");

  /* TODO: fix
  if (this->map != NULL)
  {
    rtk_canvas_size(this->canvas, this->map->size_x, this->map->size_y);
    rtk_canvas_scale(this->canvas, this->map->scale, this->map->scale);
  }
  else
  */
  {
    rtk_canvas_size(this->canvas, 400, 400);
    rtk_canvas_scale(this->canvas, 0.1, 0.1);
  }

  this->map_fig = rtk_fig_create(this->canvas, NULL, -1);
  this->pf_fig = rtk_fig_create(this->canvas, this->map_fig, 5);

  /* FIX
  // Draw the map
  if (this->map != NULL)
  {
    map_draw_occ(this->map, this->map_fig);
    //map_draw_cspace(this->map, this->map_fig);

    // HACK; testing
    map_draw_wifi(this->map, this->map_fig, 0);
  }
  */

  rtk_fig_line(this->map_fig, 0, -100, 0, +100);
  rtk_fig_line(this->map_fig, -100, 0, +100, 0);

  // Best guess figure
  this->robot_fig = rtk_fig_create(this->canvas, NULL, 9);

  // Draw the robot
  rtk_fig_color(this->robot_fig, 0.7, 0, 0);
  rtk_fig_rectangle(this->robot_fig, 0, 0, 0, 0.40, 0.20, 0);

  // TESTING
  //rtk_fig_movemask(this->robot_fig, RTK_MOVE_TRANS | RTK_MOVE_ROT);

  // Initialize the sensor GUI's
  for (i = 0; i < this->sensor_count; i++)
    this->sensors[i]->SetupGUI(this->canvas, this->robot_fig);

  rtk_app_main_init(this->app);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the GUI
int AdaptiveMCL::ShutdownGUI(void)
{
  int i;
  
  // Finalize the sensor GUI's
  for (i = 0; i < this->sensor_count; i++)
    this->sensors[i]->ShutdownGUI(this->canvas, this->robot_fig);

  rtk_fig_destroy(this->robot_fig);
  rtk_fig_destroy(this->pf_fig);
  rtk_fig_destroy(this->map_fig);
  
  rtk_canvas_destroy(this->canvas);
  rtk_app_destroy(this->app);

  rtk_app_main_term(this->app);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Update the GUI
void AdaptiveMCL::UpdateGUI(void)
{
  rtk_fig_clear(this->pf_fig);
  rtk_fig_color(this->pf_fig, 0, 0, 1);
  pf_draw_samples(this->pf, this->pf_fig, 1000);
  pf_draw_cep_stats(this->pf, this->pf_fig);
  //rtk_fig_color(this->pf_fig, 0, 1, 0);
  //pf_draw_stats(this->pf, this->pf_fig);
  //pf_draw_hist(this->pf, this->pf_fig);

  // Draw the best pose estimate
  this->DrawPoseEst();

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Draw the current best pose estimate
void AdaptiveMCL::DrawPoseEst()
{
  int i;
  double max_weight;
  amcl_hyp_t *hyp;
  pf_vector_t pose;
  
  this->Lock();

  max_weight = -1;
  for (i = 0; i < this->hyp_count; i++)
  {
    hyp = this->hyps + i;

    if (hyp->weight > max_weight)
    {
      max_weight = hyp->weight;
      pose = hyp->pf_pose_mean;
    }
  }
  
  this->Unlock();

  if (max_weight < 0.0)
    return;
    
  // Shift the robot figure
  rtk_fig_origin(this->robot_fig, pose.v[0], pose.v[1], pose.v[2]);
 
  return;
}

#endif


