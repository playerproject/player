/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
// Requires: position (odometry), laser, sonar
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
#define INCLUDE_OUTFILE 1
extern PlayerTime* GlobalTime;

#include "amcl.h"


////////////////////////////////////////////////////////////////////////////////
// Create an instance of the driver
CDevice* AdaptiveMCL_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_LOCALIZE_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"amcl\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new AdaptiveMCL(interface, cf, section)));
}


////////////////////////////////////////////////////////////////////////////////
// Register the driver
void AdaptiveMCL_Register(DriverTable* table)
{
  table->AddDriver("amcl", PLAYER_ALL_MODE, AdaptiveMCL_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
AdaptiveMCL::AdaptiveMCL(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_localize_data_t), 0, 100, 100)
{
  double u[3];

  // Get the map settings
  this->occ_filename = cf->ReadFilename(section, "map_file", NULL);
  this->map_scale = cf->ReadLength(section, "map_scale", 0.05);
  this->occ_map_negate = cf->ReadInt(section, "map_negate", 0);
  this->map = NULL;

  // Odometry stuff
  this->LoadOdom(cf, section);
  
  // Sonar stuff
  this->LoadSonar(cf, section);
  
  // Laser stuff
  this->LoadLaser(cf, section);
  
  // WiFi stuff
  this->LoadWifi(cf, section);

  // GPS stuff
  this->LoadGps(cf, section);
  
  // Particle filter settings
  this->pf = NULL;
  this->pf_min_samples = cf->ReadInt(section, "pf_min_samples", 100);
  this->pf_max_samples = cf->ReadInt(section, "pf_max_samples", 10000);

  // Adaptive filter parameters
  this->pf_err = cf->ReadFloat(section, "pf_err", 0.01);
  this->pf_z = cf->ReadFloat(section, "pf_z", 3);

  // Which sensor will we use to initialize the pf?
  this->pf_init_odom = cf->ReadInt(section, "pf_init_odom", 1);
  this->pf_init_gps = cf->ReadInt(section, "pf_init_gps", 0);
  
  // Initial pose estimate (odometry-based initialization)
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

  // Initial hypothesis list
  this->hyp_count = 0;

  // Create the sensor queue
  this->q_len = 0;
  this->q_size = 2000;
  this->q_data = new amcl_sensor_data_t[this->q_size];

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
  delete[] this->q_data;
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int AdaptiveMCL::Setup(void)
{
  int i;
  amcl_sensor_data_t sdata;
  amcl_wifi_beacon_t *beacon;
    
  PLAYER_TRACE0("setup");

  assert(this->map == NULL);
  this->map = map_alloc(this->map_scale);
  
  // Load the occupancy map
  if (this->occ_filename != NULL)
  {
    PLAYER_MSG1("loading occupancy map file [%s]", this->occ_filename);
    if (map_load_occ(this->map, this->occ_filename, this->occ_map_negate) != 0)
      return -1;
  }

  // Compute the c-space
  PLAYER_MSG0("computing cspace");
  map_update_cspace(this->map, 2 * this->robot_radius);

  // Load the wifi maps
  for (i = 0; i < this->wifi_beacon_count; i++)
  {
    beacon = this->wifi_beacons + i;
    if (beacon->filename != NULL)
    {
      PLAYER_MSG1("loading wifi map file [%s]", beacon->filename);
      if (map_load_wifi(this->map, beacon->filename, i) != 0)
        return -1;
    }
  }

  // Create the particle filter
  assert(this->pf == NULL);
  this->pf = pf_alloc(this->pf_min_samples, this->pf_max_samples);
  this->pf->pop_err = this->pf_err;
  this->pf->pop_z = this->pf_z;
  
  // Initialise the odometry device
  if (this->SetupOdom() != 0)
    return -1;
  
  // Initialise the sonar device
  if (this->SetupSonar() != 0)
    return -1;
  
  // Initialise the laser device
  if (this->SetupLaser() != 0)
    return -1;

  // Initialise wifi
  if (this->SetupWifi() != 0)
    return -1;

  // Initialise GPS
  if (this->SetupGps() != 0)
    return -1;

  // Set the initial odometric poses
  this->GetOdomData(&sdata);
  this->odom_pose = sdata.odom_pose;
  this->pf_odom_pose = sdata.odom_pose;
  this->pf_odom_time_sec = sdata.odom_time_sec;
  this->pf_odom_time_usec = sdata.odom_time_usec;

  // No data has yet been pushed, and the
  // filter has not yet been initialized
  this->push_init = false;
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
  PLAYER_TRACE0("shutting down");
    
  // Stop the driver thread.
  this->StopThread();

  // Stop the gps device
  this->ShutdownGps();
  
  // Stop the wifi device
  this->ShutdownWifi();

  // Stop the laser
  this->ShutdownLaser();

  // Stop the sonar
  this->ShutdownSonar();

  // Stop the odometry device
  this->ShutdownOdom();

  // Delete the particle filter
  pf_free(this->pf);
  this->pf = NULL;

  // Delete the map
  map_free(this->map);
  this->map = NULL;

  PLAYER_TRACE0("shutdown done");
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get the current pose.  This function is called by the server thread.
size_t AdaptiveMCL::GetData(void* client, unsigned char* dest, size_t len,
                            uint32_t* time_sec, uint32_t* time_usec)
{
  int i, j, k;
  int datalen;
  player_localize_data_t data;
  pf_vector_t odom_pose, odom_diff;
  pf_vector_t pose;
  pf_matrix_t pose_cov;
  amcl_sensor_data_t sdata;
  amcl_hyp_t *hyp;
  double scale[3];
  
  this->Lock();

  // See if there is new odometry data.  If there is, push it and all
  // the rest of the sensor data onto the sensor queue.
  this->GetOdomData(&sdata);

  // See how far the robot has moved
  odom_pose = sdata.odom_pose;
  odom_diff = pf_vector_coord_sub(odom_pose, this->odom_pose);

  // Make sure we have moved a reasonable distance
  if (this->push_init == false ||
      fabs(odom_diff.v[0]) > 0.20 ||
      fabs(odom_diff.v[1]) > 0.20 ||
      fabs(odom_diff.v[2]) > M_PI / 6)
  {
    this->odom_pose = sdata.odom_pose;

    // Get the current sonar data; we assume it is new data
    this->GetSonarData(&sdata);

    // Get the current laser data; we assume it is new data
    this->GetLaserData(&sdata);

    // Get the current wifi data; we assume it is new data
    this->GetWifiData(&sdata);

    // Get the current GPS data; we assume it is new data
    this->GetGpsData(&sdata);

    // Push the data
    this->Push(&sdata);
    this->push_init = true;
  }
  
  // Compute the change in odometric pose
  odom_diff = pf_vector_coord_sub(odom_pose, this->pf_odom_pose);

  // Record the number of pending observations
  data.pending_count = this->q_len;
  data.pending_time_sec = this->pf_odom_time_sec;
  data.pending_time_usec = this->pf_odom_time_usec;
  
  // Encode the hypotheses
  data.hypoth_count = this->hyp_count;
  for (i = 0; i < this->hyp_count; i++)
  {
    hyp = this->hyps + i;

    // Get the current estimate
    pose = hyp->pf_pose_mean;
    pose_cov = hyp->pf_pose_cov;

    // Translate/rotate the hypotheses to take account of latency in filter
    pose = pf_vector_coord_add(odom_diff, pose);

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

  this->Unlock();
  
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

  // Copy data to server
  assert(datalen > 0);
  assert(len >= (size_t) datalen);
  assert(dest != NULL);
  memcpy(dest, &data, datalen);

  // Set the timestamp
  if (time_sec)
    *time_sec = sdata.odom_time_sec;
  if (time_usec)
    *time_usec = sdata.odom_time_usec;

  return datalen;
}


////////////////////////////////////////////////////////////////////////////////
// Process configuration requests
int AdaptiveMCL::PutConfig(player_device_id_t* device, void* client,
                           void* data, size_t len)
{
  // Discard bogus empty packets
  if (len < 1)
  {
    PLAYER_WARN("got zero length configuration request; ignoring");
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return 0;
  }

  // Process some of the requests immediately
  switch (((char*) data)[0])
  {
    case PLAYER_LOCALIZE_GET_MAP_INFO_REQ:
      HandleGetMapInfo(client, data, len);
      return 0;
    case PLAYER_LOCALIZE_GET_MAP_DATA_REQ:
      HandleGetMapData(client, data, len);
      return 0;
  }

  // Let the device thread get the rest
  return CDevice::PutConfig(device, client, data, len);
}


////////////////////////////////////////////////////////////////////////////////
// Handle map info request
void AdaptiveMCL::HandleGetMapInfo(void *client, void *request, int len)
{
  int reqlen;
  player_localize_map_info_t info;

  // Expected length of request
  reqlen = sizeof(info.subtype);
  
  // check if the config request is valid
  if (len != reqlen)
  {
    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, reqlen);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  //PLAYER_TRACE4("%d %d %f %d\n", this->map->size_x, this->map->size_y,
  //              this->map->scale, ntohl(info.scale));
  
  info.scale = htonl((int32_t) (1000.0 / this->map->scale + 0.5));
  info.width = htonl((int32_t) (this->map->size_x));
  info.height = htonl((int32_t) (this->map->size_y));

  // Send map info to the client
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &info, sizeof(info)) != 0)
    PLAYER_ERROR("PutReply() failed");
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle map data request
void AdaptiveMCL::HandleGetMapData(void *client, void *request, int len)
{
  int i, j;
  int oi, oj, si, sj;
  int reqlen;
  map_cell_t *cell;
  player_localize_map_data_t data;

  // Expected length of request
  reqlen = sizeof(data) - sizeof(data.data);

  // check if the config request is valid
  if (len != reqlen)
  {
    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, reqlen);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  // Construct reply
  memcpy(&data, request, len);

  oi = ntohl(data.col);
  oj = ntohl(data.row);
  si = ntohl(data.width);
  sj = ntohl(data.height);

  //PLAYER_TRACE4("%d %d %d %d\n", oi, oj, si, sj);

  // Grab the pixels from the map
  for (j = 0; j < sj; j++)
  {
    for (i = 0; i < si; i++)
    {
      if (MAP_VALID(this->map, i + oi, j + oj))
      {
        cell = this->map->cells + MAP_INDEX(this->map, i + oi, j + oj);
        data.data[i + j * si] = cell->occ_state;
      }
      else
        data.data[i + j * si] = 0;
    }
  }
    
  // Send map info to the client
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &data, sizeof(data)) != 0)
    PLAYER_ERROR("PutReply() failed");
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Push data onto the filter queue.
void AdaptiveMCL::Push(amcl_sensor_data_t *data)
{
  int i;
  
  if (this->q_len >= this->q_size)
  {
    PLAYER_ERROR("queue overflow");
    return;
  }

  i = (this->q_start + this->q_len++) % this->q_size;
  this->q_data[i] = *data;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Pop data from the filter queue
int AdaptiveMCL::Pop(amcl_sensor_data_t *data)
{
  int i;
  
  if (this->q_len == 0)
    return 0;

  //PLAYER_TRACE1("q len %d\n", this->q_len);
  
  i = this->q_start++ % this->q_size;
  this->q_len--;
  *data = this->q_data[i];
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void AdaptiveMCL::Main(void) 
{  
  struct timespec sleeptime;
  amcl_sensor_data_t data;
  
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
  
  // Initialize the filter
  // REMOVE this->InitFilter(this->pf_init_pose_mean, this->pf_init_pose_cov);
  
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

    // Process any queued data
    if (this->Pop(&data))
    {
      this->UpdateFilter(&data);

#ifdef INCLUDE_OUTFILE
      // Save the error values
      pf_vector_t mean;
      double var;

      pf_get_cep_stats(pf, &mean, &var);
      
      fprintf(this->outfile, "%d.%03d unknown 6665 localize 01 %d.%03d ",
              0, 0, data.odom_time_sec, data.odom_time_usec / 1000);
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

/* REMOVE
////////////////////////////////////////////////////////////////////////////////
// Initialize the filter
void AdaptiveMCL::InitFilter(pf_vector_t pose_mean, pf_matrix_t pose_cov)
{
  int i;
  double weight;
  amcl_hyp_t *hyp;
  
  // Initialize the odometric model
  odometry_init_init(this->odom_model, pose_mean, pose_cov);
  
  // Draw samples from the odometric distribution
  pf_init(this->pf, (pf_init_model_fn_t) odometry_init_model, this->odom_model);
  
  odometry_init_term(this->odom_model);

  this->Lock();

  // Read out the current hypotheses
  this->hyp_count = 0;
  for (i = 0; (size_t) i < sizeof(this->hyps) / sizeof(this->hyps[0]); i++)
  {
    if (!pf_get_cluster_stats(this->pf, i, &weight, &pose_mean, &pose_cov))
      break;

    hyp = this->hyps + this->hyp_count++;
    hyp->weight = weight;
    hyp->pf_pose_mean = pose_mean;
    hyp->pf_pose_cov = pose_cov;
  }

  this->Unlock();
  
#ifdef INCLUDE_RTKGUI
  // Draw the samples
  if (this->enable_gui)
  {
    DrawPoseEst();
    
    rtk_fig_clear(this->pf_fig);
    rtk_fig_color(this->pf_fig, 0, 0, 1);
    pf_draw_samples(this->pf, this->pf_fig, 1000);
    pf_draw_cep_stats(this->pf, this->pf_fig);
    //rtk_fig_color(this->pf_fig, 0, 1, 0);
    //pf_draw_hist(this->pf, this->pf_fig);
  }
#endif

  return;
}
*/

////////////////////////////////////////////////////////////////////////////////
// Update the filter with new sensor data
void AdaptiveMCL::UpdateFilter(amcl_sensor_data_t *data)
{
  int i;
  double weight;
  pf_vector_t pose_mean;
  pf_matrix_t pose_cov;
  amcl_hyp_t *hyp;

  // If the distribution has not been initialized...
  if (this->pf_init == false)
  {
    // Initialize distribution from odometric model
    if (this->pf_init_odom)
      this->InitOdomModel(data);

    // Initialize distribution from GPS model
    if (this->pf_init_gps)
      this->InitGpsModel(data);
  }
  
  // Apply odometry action and sensor models
  this->UpdateOdomModel(data);
  
  // Apply sonar sensor model
  this->UpdateSonarModel(data);

  // Apply laser sensor model
  this->UpdateLaserModel(data);
  
  // Apply the wifi sensor model
  this->UpdateWifiModel(data);

  // Apply the GPS sensor model
  this->UpdateGpsModel(data);

  // Resample
  pf_update_resample(this->pf);

  this->Lock();

  this->pf_odom_pose = data->odom_pose;
  this->pf_odom_time_sec = data->odom_time_sec;
  this->pf_odom_time_usec = data->odom_time_usec;

  // Read out the current hypotheses
  this->hyp_count = 0;
  for (i = 0; (size_t) i < sizeof(this->hyps) / sizeof(this->hyps[0]); i++)
  {
    if (!pf_get_cluster_stats(this->pf, i, &weight, &pose_mean, &pose_cov))
      break;

    hyp = this->hyps + this->hyp_count++;
    hyp->weight = weight;
    hyp->pf_pose_mean = pose_mean;
    hyp->pf_pose_cov = pose_cov;
  }

  this->Unlock();
  
#ifdef INCLUDE_RTKGUI
  // Draw the samples
  if (this->enable_gui)
  {
    DrawPoseEst();
    
    this->DrawSonarData(data);
    this->DrawLaserData(data);
    this->DrawWifiData(data);
    this->DrawGpsData(data);
    
    rtk_fig_clear(this->pf_fig);
    rtk_fig_color(this->pf_fig, 0, 0, 1);
    pf_draw_samples(this->pf, this->pf_fig, 1000);
    pf_draw_cep_stats(this->pf, this->pf_fig);
    //rtk_fig_color(this->pf_fig, 0, 1, 0);
    //pf_draw_stats(this->pf, this->pf_fig);
    //pf_draw_hist(this->pf, this->pf_fig);
  }
#endif

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int AdaptiveMCL::HandleRequests(void)
{
  int len;
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  
  while ((len = GetConfig(&client, &request, sizeof(request))) > 0)
  {
    switch (request[0])
    {
      case PLAYER_LOCALIZE_SET_POSE_REQ:
        HandleSetPose(client, request, len);
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
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
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

  // Initialize the filter
  this->InitFilter(pose, cov);

  // Give them an ack
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Set up the GUI
int AdaptiveMCL::SetupGUI(void)
{
  // Initialize RTK
  rtk_init(NULL, NULL);

  this->app = rtk_app_create();

  this->canvas = rtk_canvas_create(this->app);
  rtk_canvas_title(this->canvas, "AdaptiveMCL");
  rtk_canvas_size(this->canvas, this->map->size_x, this->map->size_y);
  rtk_canvas_scale(this->canvas, this->map->scale, this->map->scale);

  this->map_fig = rtk_fig_create(this->canvas, NULL, -1);
  this->pf_fig = rtk_fig_create(this->canvas, this->map_fig, 5);

  // Draw the map
  map_draw_occ(this->map, this->map_fig);
  //map_draw_cspace(this->map, this->map_fig);

  // HACK; testing
  map_draw_wifi(this->map, this->map_fig, 0);

  this->robot_fig = rtk_fig_create(this->canvas, NULL, 9);
  this->sonar_fig = rtk_fig_create(this->canvas, this->robot_fig, 15);
  this->laser_fig = rtk_fig_create(this->canvas, this->robot_fig, 10);
  this->wifi_fig = rtk_fig_create(this->canvas, this->robot_fig, 16);
  this->gps_fig = rtk_fig_create(this->canvas, NULL, 17);
  this->imu_fig = rtk_fig_create(this->canvas, NULL, 17);

  // Draw the robot
  rtk_fig_color(this->robot_fig, 0.7, 0, 0);
  rtk_fig_rectangle(this->robot_fig, 0, 0, 0, 0.40, 0.20, 0);

  // TESTING
  //rtk_fig_movemask(this->robot_fig, RTK_MOVE_TRANS | RTK_MOVE_ROT);

  rtk_app_main_init(this->app);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the GUI
int AdaptiveMCL::ShutdownGUI(void)
{
  rtk_fig_destroy(this->wifi_fig);
  rtk_fig_destroy(this->laser_fig);
  rtk_fig_destroy(this->sonar_fig);
  rtk_fig_destroy(this->robot_fig);
  rtk_fig_destroy(this->map_fig);
  rtk_fig_destroy(this->pf_fig);  
  rtk_canvas_destroy(this->canvas);
  rtk_app_destroy(this->app);

  rtk_app_main_term(this->app);
  
  return 0;
}
  
////////////////////////////////////////////////////////////////////////////////
// Draw the current best pose estimate
void AdaptiveMCL::DrawPoseEst()
{
  int i;
  double max_weight;
  amcl_hyp_t *hyp;
  
  this->Lock();

  max_weight = 0;
  for (i = 0; i < this->hyp_count; i++)
  {
    hyp = this->hyps + i;

    if (hyp->weight > max_weight)
    {
      max_weight = hyp->weight;
      rtk_fig_origin(this->robot_fig, hyp->pf_pose_mean.v[0],
                     hyp->pf_pose_mean.v[1], hyp->pf_pose_mean.v[2]);
    }
  }
  
  this->Unlock();
  
  return;
}

#endif


