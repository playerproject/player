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
// Desc: Driver for detecting combined laser reflectors with B&W barcodes
// Author: Andrew Howard
// Date: 9 Jan 2004
// CVS: $Id$
//
// Theory of operation:
//   Parses a laser scan to find the retro-reflective patches (lines or
//   circles), then points the camera at the patch, zooms in, and
//   attempts to read the B&W barcode.  Will not return sensible
//   orientations for circular patches.
//
// Requires:
//   Laser, PTZ and camera devices.
//
///////////////////////////////////////////////////////////////////////////

#include "player.h"

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>

#include "error.h"
#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"


// Driver for detecting laser retro-reflectors.
class LaserVisualBW : public Driver
{
  // Constructor
  public: LaserVisualBW( ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Info on potential fiducials.
  private: struct fiducial_t
  {
    // Id (-1) if undetermined.
    int id;

    // Pose of fiducial.
    double pose[3];

    // Uncertainty in pose.
    double upose[3];

    // Time at which fiducial was last seen by the laser.
    double laser_time;

    // Time at which ptz selected this fiducial,
    // and the time at which the ptz locked on to this fiducial.
    double ptz_select_time;
    double ptz_lockon_time;

    // Time at which the fiducial was identified.
    double id_time;
  };

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);
  
  // Process laser data.
  // Returns non-zero if the laser data has been updated.
  private: int UpdateLaser();

  // Analyze the laser data to find fidicuials (reflectors).
  private: void FindLaserFiducials(double time, player_laser_data_t *data);
    
  // Find the line of best fit for the given segment of the laser
  // scan.  Fills in the pose of the reflector relative to the laser.
  private: void FitLaserFiducial(player_laser_data_t *data, int first, int last, double pose[3]);

  // Match a new laser fiducial against the ones we are already
  // tracking.  The pose is relative to the laser.
  private: void MatchLaserFiducial(double time, double pose[3]);

  // Retire fiducials we havent seen for a while.
  private: void RetireLaserFiducials(double time, player_laser_data_t *data);

  // Update the PTZ to point at one of the laser reflectors.
  private: int UpdatePtz();

  // Select a target fiducial for the PTZ to inspect.
  private: void SelectPtzTarget(double time, player_ptz_data_t *data);

  // Servo the PTZ to a target fiducial.
  private: void ServoPtz(double time, player_ptz_data_t *data);

  // Process any new camera data.
  private: int UpdateCamera();

  // Extract a bit string from the image.  
  private: int ExtractSymbols(int x, int symbol_max_count, int symbols[]);

  // Extract a code from a symbol string.
  private: int ExtractCode(int symbol_count, int symbols[]);
  
  // Write the device data (the data going back to the client).
  private: void WriteData();

  // Image processing
  private: double edge_thresh;
  
  // Barcode tolerances
  private: int barcount;
  private: double barwidth;
  private: double guard_min, guard_tol;
  private: double err_first, err_second;

  // Max time to spend looking at a fiducial.
  private: double max_ptz_attention;

  // Retirement age for fiducials that havent been seen for a while.
  private: double retire_time;

  // Max distance between fiducials in successive laser scans.
  private: double max_dist;

  // Laser stuff
  private: int laser_index;
  private: Driver *laser;
  private: player_device_id_t laser_id;
  private: double laser_time;

  // PTZ stuff
  private: int ptz_index;
  private: Driver *ptz;
  private: player_device_id_t ptz_id;
  private: double ptz_time;

  // Camera stuff
  private: int camera_index;
  private: Driver *camera;
  private: player_device_id_t camera_id;
  private: double camera_time;
  private: player_camera_data_t camera_data;

  // List of currently tracked fiducials.
  private: int fiducial_count;
  private: fiducial_t fiducials[256];

  // The current selected fiducial for the ptz, the time at which we
  // selected it, and the time at which we first locked on to it.
  private: fiducial_t *ptz_fiducial;

  // Dimensions of the zoomed image for the target fiducial (m).
  private: double zoomwidth, zoomheight;
  
  // Local copy of the current fiducial data.
  private: player_fiducial_data_t fdata;
};


// Initialization function
Driver* LaserVisualBW_Init( ConfigFile* cf, int section)
{
  return ((Driver*) (new LaserVisualBW( cf, section)));
}


// a driver registration function
void LaserVisualBW_Register(DriverTable* table)
{
  table->AddDriver("laservisualbw", LaserVisualBW_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserVisualBW::LaserVisualBW( ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_FIDUCIAL_CODE, PLAYER_READ_MODE,
             sizeof(player_fiducial_data_t), 0, 10, 10)
{
  this->laser_index = cf->ReadInt(section, "laser", 0);
  this->laser = NULL;
  this->laser_time = 0;

  this->ptz_index = cf->ReadInt(section, "ptz", 0);
  this->ptz = NULL;
  this->ptz_time = 0;

  this->camera_index = cf->ReadInt(section, "camera", 0);
  this->camera = NULL;
  this->camera_time = 0;

  this->max_ptz_attention = cf->ReadFloat(section, "max_ptz_attention", 6.0);
  this->retire_time = cf->ReadFloat(section, "retire_time", 1.0);
  this->max_dist = cf->ReadFloat(section, "max_dist", 0.2);

  // Image processing
  this->edge_thresh = cf->ReadFloat(section, "edge_thresh", 20);
  
  // Default fiducial properties.
  this->barwidth = cf->ReadLength(section, "bit_width", 0.08);
  this->barcount = cf->ReadInt(section, "bit_count", 3);

  // Barcode properties: minimum height (pixels), height tolerance (ratio).
  this->guard_min = cf->ReadInt(section, "guard_min", 4);
  this->guard_tol = cf->ReadLength(section, "guard_tol", 0.20);

  // Error threshold on the first and second best digits
  this->err_first = cf->ReadFloat(section, "digit_err_first", 0.5);
  this->err_second = cf->ReadFloat(section, "digit_err_second", 1.0);

  // Reset fiducial list.
  this->fiducial_count = 0;

  // Reset PTZ target.
  this->ptz_fiducial = NULL;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int LaserVisualBW::Setup()
{
  // Subscribe to the laser.
  this->laser_id.code = PLAYER_LASER_CODE;
  this->laser_id.index = this->laser_index;
  this->laser_id.port = this->device_id.port;
  this->laser = deviceTable->GetDriver(this->laser_id);
  if (!this->laser)
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return(-1);
  }
  if (this->laser->Subscribe(this->laser_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return(-1);
  }

  // Subscribe to the PTZ.
  this->ptz_id.code = PLAYER_PTZ_CODE;
  this->ptz_id.index = this->ptz_index;
  this->ptz_id.port = this->device_id.port;
  this->ptz = deviceTable->GetDriver(this->ptz_id);
  if (!this->ptz)
  {
    PLAYER_ERROR("unable to locate suitable PTZ device");
    return(-1);
  }
  if (this->ptz->Subscribe(this->ptz_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to PTZ device");
    return(-1);
  }

  // Subscribe to the camera.
  this->camera_id.code = PLAYER_CAMERA_CODE;
  this->camera_id.index = this->camera_index;
  this->camera_id.port = this->device_id.port;
  this->camera = deviceTable->GetDriver(this->camera_id);
  if (!this->camera)
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return(-1);
  }
  if (this->camera->Subscribe(this->camera_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    return(-1);
  }

  // Start the driver thread.
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int LaserVisualBW::Shutdown()
{
  // Stop the driver thread.
  StopThread();
  
  // Unsubscribe from devices.
  this->camera->Unsubscribe(this->camera_id);
  this->ptz->Unsubscribe(this->ptz_id);
  this->laser->Unsubscribe(this->laser_id);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void LaserVisualBW::Main() 
{
  while (true)
  {
    // Let the camera drive update rate
    this->camera->Wait();

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    HandleRequests();

    // Process any new laser data.
    if (UpdateLaser())
    {
      // Update the device data (the data going back to the client).
      WriteData();
    }

    // Process any new PTZ data.
    UpdatePtz();

    // Process any new camera data.
    UpdateCamera();
  }
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int LaserVisualBW::HandleRequests()
{
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  int len;
  
  while ((len = GetConfig(&client, &request, sizeof(request),NULL)) > 0)
  {
    switch (request[0])
    {
      case PLAYER_FIDUCIAL_GET_GEOM:
        HandleGetGeom(client, request, len);
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
// Handle geometry requests.
void LaserVisualBW::HandleGetGeom(void *client, void *request, int len)
{
  unsigned short reptype;
  struct timeval ts;
  int replen;
  player_laser_geom_t lgeom;
  player_fiducial_geom_t fgeom;
    
  // Get the geometry from the laser
  replen = this->laser->Request(this->laser_id, this, 
                                request, len, NULL,
                                &reptype, &lgeom, sizeof(lgeom), &ts);
  if (replen <= 0 || replen != sizeof(lgeom))
  {
    PLAYER_ERROR("unable to get geometry from laser device");
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  fgeom.pose[0] = lgeom.pose[0];
  fgeom.pose[1] = lgeom.pose[1];
  fgeom.pose[2] = lgeom.pose[2];
  fgeom.size[0] = lgeom.size[0];
  fgeom.size[1] = lgeom.size[1];
  fgeom.fiducial_size[0] = ntohs((int) (this->barwidth * 1000));
  fgeom.fiducial_size[1] = ntohs((int) (this->barwidth * 1000));
    
  if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &fgeom, sizeof(fgeom), &ts) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process laser data.
int LaserVisualBW::UpdateLaser()
{
  int i;
  player_laser_data_t data;
  size_t size;
  struct timeval timestamp;
  double time;
  
  // Get the laser data.
  size = this->laser->GetData(this->laser_id,(void*)&data, 
                              sizeof(data), &timestamp);
  time = (double) timestamp.tv_sec + ((double) timestamp.tv_usec) * 1e-6;
  
  // Dont do anything if this is old data.
  if (time == this->laser_time)
    return 0;
  this->laser_time = time;
  
  // Do some byte swapping on the laser data.
  data.resolution = ntohs(data.resolution);
  data.range_res = ntohs(data.range_res);
  data.min_angle = ntohs(data.min_angle);
  data.max_angle = ntohs(data.max_angle);
  data.range_count = ntohs(data.range_count);
  for (i = 0; i < data.range_count; i++)
    data.ranges[i] = ntohs(data.ranges[i]);

  // Find possible fiducials in this scan.
  this->FindLaserFiducials(time, &data);

  // Retire fiducials we havent seen for a while.
  this->RetireLaserFiducials(time, &data);

  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Analyze the laser data to find fidicuials (reflectors).
void LaserVisualBW::FindLaserFiducials(double time, player_laser_data_t *data)
{
  int i, h;
  int valid;
  double r, b;
  //double db, dr;
  double mn, mr, mb, mrr, mbb;
  double pose[3];

  // Empty the fiducial list.
  this->fdata.count = 0;
  
  // Initialise patch statistics.
  mn = 0.0;
  mr = 0.0;
  mb = 0.0;
  mrr = 0.0;
  mbb = 0.0;
    
  // Look for a candidate patch in scan.
  for (i = 0; i < data->range_count; i++)
  {
    r = (double) ((int) (uint32_t) data->ranges[i] * data->range_res) / 1000;
    b = (double) (data->min_angle + i * data->resolution) / 100.0 * M_PI / 180;
    h = (int) (data->intensity[i]);

    // If there is a reflection...
    if (h > 0)
    {
      mn += 1;
      mr += r;
      mb += b;
      mrr += r * r;
      mbb += b * b;
    }

    // If there is no reflection and we have a patch...
    else if (mn > 0)
    {
      // Compute the moments of the patch.
      mr /= mn;
      mb /= mn;
      mrr = mrr / mn - mr * mr;
      mbb = mbb / mn - mb * mb;

      // Test moments to see if they are valid.
      valid = 1;
      valid &= (mn >= 1.0);

      // TODO: fix or remove
      /*
      dr = this->barwidth / 2;
      db = atan2(this->barwidth / 2, mr);
      valid &= (mrr < (dr * dr));
      valid &= (mbb < (db * db));
      */
      
      if (valid)
      {
        // Do a best fit to determine the pose of the reflector.
        this->FitLaserFiducial(data, i - (int) mn, i - 1, pose);

        // Match this fiducial against the ones we are already tracking.
        this->MatchLaserFiducial(time, pose);
      }
      
      mn = 0.0;
      mr = 0.0;
      mb = 0.0;
      mrr = 0.0;
      mbb = 0.0;
    }
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Find the line of best fit for the given segment of the laser scan.
// Fills in the pose and pose of the reflector relative to the laser.
void LaserVisualBW::FitLaserFiducial(player_laser_data_t *data,
                                          int first, int last, double pose[3])
{
  int i;
  double r, b;
  double mn, mr, mb;

  mn = 0.0;
  mr = 1e6;
  mb = 0.0;

  for (i = first; i <= last; i++)
  {
    r = (double) ((int) (uint32_t) data->ranges[i] * data->range_res) / 1000;
    b = (double) (data->min_angle + i * data->resolution) / 100.0 * M_PI / 180;

    if (r < mr)
      mr = r;
    mn += 1.0;
    mb += b;
  }

  mr += this->barwidth / 2;
  mb /= mn;

  pose[0] = mr * cos(mb);
  pose[1] = mr * sin(mb);
  pose[2] = mb;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Match a new laser fiducial against the ones we are already
// tracking.  The pose is relative to the laser.
void LaserVisualBW::MatchLaserFiducial(double time, double pose[3])
{
  int i;
  double dx, dy, dr;
  double mindr;
  fiducial_t *fiducial;
  fiducial_t *minfiducial;
  
  // Observations must be at least this close to the existing
  // fiducial.
  mindr = this->max_dist;
  minfiducial = NULL;

  // Find the existing fiducial which is closest to the new
  // observation.
  for (i = 0; i < this->fiducial_count; i++)
  {
    fiducial = this->fiducials + i;

    dx = pose[0] - fiducial->pose[0];
    dy = pose[1] - fiducial->pose[1];
    dr = sqrt(dx * dx + dy * dy);
    if (dr < mindr)
    {
      mindr = dr;
      minfiducial = fiducial;
    }
  }

  // If we didnt find a matching fiducial, add a new one.
  if (minfiducial == NULL)
  {
    if (this->fiducial_count < ARRAYSIZE(this->fiducials))
    {
      minfiducial = this->fiducials + this->fiducial_count++;
      minfiducial->id = -1;
      minfiducial->pose[0] = pose[0];
      minfiducial->pose[1] = pose[1];
      minfiducial->pose[2] = pose[2];
      minfiducial->laser_time = time;
      minfiducial->ptz_select_time = -1;
      minfiducial->ptz_lockon_time = -1;
      minfiducial->id_time = -1;
    }
  }

  // Otherwise, update the existing fiducial.
  else
  {
    minfiducial->pose[0] = pose[0];
    minfiducial->pose[1] = pose[1];
    minfiducial->pose[2] = pose[2];
    minfiducial->laser_time = time;
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Retire fiducials we havent seen for a while.
void LaserVisualBW::RetireLaserFiducials(double time, player_laser_data_t *data)
{
  int i;
  fiducial_t *fiducial;
  
  // Remove any old fiducials.
  for (i = 0; i < this->fiducial_count; i++)
  {
    fiducial = this->fiducials + i;

    if (time - fiducial->laser_time > this->retire_time)
    {
      if (this->ptz_fiducial == fiducial)
        this->ptz_fiducial = NULL;
      memmove(fiducial, fiducial + 1, (this->fiducial_count - i - 1) * sizeof(fiducial_t));
      this->fiducial_count--;
      i--;
    }
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the PTZ to point at one of the laser reflectors.
int LaserVisualBW::UpdatePtz()
{
  player_ptz_data_t data;
  size_t size;
  struct timeval timestamp;
  double time;
  
  // Get the ptz data.
  size = this->ptz->GetData(this->ptz_id,(void*) &data,
                            sizeof(data), &timestamp);
  time = (double) timestamp.tv_sec + ((double) timestamp.tv_usec) * 1e-6;
  
  // Dont do anything if this is old data.
  if (time == this->ptz_time)
    return 0;
  this->ptz_time = time;
  
  // Do some byte swapping on the ptz data.
  data.pan = ntohs(data.pan);
  data.tilt = ntohs(data.tilt);
  data.zoom = ntohs(data.zoom);

  // Pick a fiducial to look at.
  this->SelectPtzTarget(time, &data);

  // Point the fiducial
  this->ServoPtz(time, &data);

  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Select a target fiducial for the PTZ to inspect.
// This algorithm picks the one that we havent looked at for a long time.
void LaserVisualBW::SelectPtzTarget(double time, player_ptz_data_t *data)
{
  int i;
  double t, maxt;
  fiducial_t *fiducial;

  // Consider the currently selected target for a while to
  // give the camera time to identify it.  If the target has
  // been identified, move on to another one.
  if (this->ptz_fiducial != NULL)
  {
    if (this->ptz_fiducial->id_time < 0)
      if (time - this->ptz_fiducial->ptz_select_time < this->max_ptz_attention)
        return;
  }

  // Find one we havent looked at for while.
  this->ptz_fiducial = NULL;
  maxt = -1;
  
  for (i = 0; i < this->fiducial_count; i++)
  {
    fiducial = this->fiducials + i;

    t = time - fiducial->ptz_select_time;
    if (t > maxt)
    {
      maxt = t;
      this->ptz_fiducial = fiducial;
    }
  }

  if (this->ptz_fiducial)
  {
    this->ptz_fiducial->ptz_select_time = time;
    this->ptz_fiducial->ptz_lockon_time = -1;
    this->ptz_fiducial->id_time = -1;
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Servo the PTZ to a target fiducial.
void LaserVisualBW::ServoPtz(double time, player_ptz_data_t *data)
{
  int i;
  double dx, dy, r, pan, tilt, zoom;
  fiducial_t *fiducial;
  player_ptz_cmd_t cmd;
  double maxtilt;
  double deadpan, deadzoom;

  // Tilt pattern
  double pattern[] = {0, 0.5, 1.0, 0.5, 0, -0.5, -1.0, -0.5};
  
  // Max tilt value.
  maxtilt = 5 * M_PI / 180;

  // Deadband values.
  deadpan = 2;
  deadzoom = 2;
  
  fiducial = this->ptz_fiducial;
  if (fiducial == NULL)
  {
    r = 0;
    pan = 0;
    tilt = 0;
    zoom = M_PI;
  }
  else
  {
    // Compute range and bearing of fiducial relative to camera.
    // TODO: account for camera geometry.
    dx = fiducial->pose[0];
    dy = fiducial->pose[1];
    r = sqrt(dx * dx + dy * dy);
    pan = atan2(dy, dx);
    zoom = 8 * atan2(this->barwidth / 2, r);

    // See if we have locked on yet.
    if (fiducial->ptz_lockon_time < 0)
    {
      if (fabs(pan * 180 / M_PI - data->pan) < deadpan &&
          fabs(zoom * 180 / M_PI - data->zoom) < deadzoom)
        fiducial->ptz_lockon_time = time;
    }

    // If we havent locked on yet...
    if (fiducial->ptz_lockon_time < 0)
      tilt = 0;
    else
    {
      i = (int) floor((time - fiducial->ptz_lockon_time) / this->max_ptz_attention * 8);
      tilt = maxtilt * pattern[i];        
      //tilt = sin((time - fiducial->ptz_lockon_time) / this->max_ptz_attention * 2 * M_PI);
    }
  }
  
  // Compose the command packet to send to the PTZ device.
  cmd.pan = htons(((int16_t) (pan * 180 / M_PI)));
  cmd.tilt = htons(((int16_t) (tilt * 180 / M_PI)));
  cmd.zoom = htons(((int16_t) (zoom * 180 / M_PI)));
  this->ptz->PutCommand(this->ptz_id,(void*)&cmd, sizeof(cmd), NULL);

  // Compute the dimensions of the image at the range of the target fiducial.
  this->zoomwidth = 2 * r * tan(data->zoom * M_PI / 180 / 2);
  this->zoomheight = 3.0 / 4.0 * this->zoomwidth;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process any new camera data.
int LaserVisualBW::UpdateCamera()
{
  size_t size;
  struct timeval timestamp;
  double time;
  int id, best_id;
  int x;
  int symbol_count;
  int symbols[480];

  // Get the camera data.
  size = this->camera->GetData(this->camera_id, (void*) &this->camera_data,
                               sizeof(this->camera_data), &timestamp);
  time = (double) timestamp.tv_sec + ((double) timestamp.tv_usec) * 1e-6;

  // Dont do anything if this is old data.
  if (fabs(time - this->camera_time) < 0.001)
    return 0;
  this->camera_time = time;
  
  // Do some byte swapping
  this->camera_data.width = ntohs(this->camera_data.width);
  this->camera_data.height = ntohs(this->camera_data.height); 
  this->camera_data.bpp = this->camera_data.bpp;

  best_id = -1;
  
  // Barcode may not be centered, so look across entire image
  for (x = 0; x < this->camera_data.width; x += 16)
  {
    // Extract raw symbols
    symbol_count = this->ExtractSymbols(x, sizeof(symbols) / sizeof(symbols[0]), symbols);

    // Identify barcode
    id = this->ExtractCode(symbol_count, symbols);

    // If we see multiple barcodes, we dont know which is the correct one
    if (id >= 0)
    {
      if (best_id < 0)
      {
        best_id = id;
      }
      else if (best_id != id)
      {
        best_id = -1;
        break;
      }
    }
  }

  // Assign id to fiducial we are currently looking at.
  // TODO: check for possible aliasing (not looking at the correct barcode).
  if (best_id >= 0 && this->ptz_fiducial)
  {
    if (this->ptz_fiducial->ptz_lockon_time >= 0)
    {
      this->ptz_fiducial->id = best_id;
      this->ptz_fiducial->id_time = time;
    }
  }
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Extract a bit string from the image.  Takes a vertical column in
// the image and thresholds it.
int LaserVisualBW::ExtractSymbols(int x, int symbol_max_count, int symbols[])
{
  int i, j, off, inc, pix;
  double fn, fv;
  int state, start, symbol_count;
  double kernel[] = {+1, +2, 0, -2, -1};

  // GREY
  if (this->camera_data.bpp == 8)
  {
    off = x * this->camera_data.bpp / 8;
    inc = this->camera_data.width * this->camera_data.bpp / 8;
  }

  // RGB24, use G channel
  else if (this->camera_data.bpp == 24)
  {
    off = x * this->camera_data.bpp / 8 + 1;
    inc = this->camera_data.width * this->camera_data.bpp / 8;
  }

  // RGB32, use G channel
  else if (this->camera_data.bpp == 32)
  {
    off = x * this->camera_data.bpp / 8 + 1;
    inc = this->camera_data.width * this->camera_data.bpp / 8;
  }

  else
  {
    PLAYER_ERROR1("no support for image depth %d", this->camera_data.bpp);
    return 0;
  }

  //FILE *file = fopen("edge.out", "a+");

  assert(symbol_max_count >= this->camera_data.height);

  state = -1;
  start = -1;
  symbol_count = 0;

  for (i = 2, pix = off + 2 * inc; i < this->camera_data.height - 2; i++, pix += inc)
  {
   // Run an edge detector
    fn = fv = 0.0;
    for (j = -2; j <= 2; j++)
    {
      fv += kernel[j + 2] * this->camera_data.image[pix + j * inc];
      fn += fabs(kernel[j + 2]);
    }
    fv /= fn;
    
    // Pick the transitions
    if (state == -1)
    {
      if (fv > +this->edge_thresh)
      {
        state = 1;
        start = i;
      }
      else if (fv < -this->edge_thresh)
      {
        state = 0;
        start = i;
      }
    }
    else if (state == 0)
    {
      if (fv > +this->edge_thresh)
      {
        symbols[symbol_count++] = -(i - start);
        state = 1;
        start = i;
      }
    }
    else if (state == 1)
    {
      if (fv < -this->edge_thresh)
      {
        symbols[symbol_count++] = +(i - start);
        state = 0;
        start = i;
      }
    }

    //fprintf(file, "%d %f %f %d\n", i, fv, fn, state);
  }

  if (state == 0)
    symbols[symbol_count++] = -(i - start);
  else if (state == 1)
    symbols[symbol_count++] = +(i - start);

  //fprintf(file, "\n\n");
  //fclose(file);

  return symbol_count;
}


////////////////////////////////////////////////////////////////////////////////
// Extract a code from a symbol string.
int LaserVisualBW::ExtractCode(int symbol_count, int symbols[])
{
  int i, j, k;
  double a, b, c;
  double mean, min, max, wm, wo;
  int best_digit;
  double best_err;
  double err[10];

  // These are UPC the mark-space patterns for digits.  From:
  // http://www.ee.washington.edu/conselec/Sp96/projects/ajohnson/proposal/project.htm
  double digits[][4] =
    {
      {-3,+2,-1,+1}, // 0
      {-2,+2,-2,+1}, // 1
      {-2,+1,-2,+2}, // 2
      {-1,+4,-1,+1}, // 3
      {-1,+1,-3,+2}, // 4
      {-1,+2,-3,+1}, // 5
      {-1,+1,-1,+4}, // 6
      {-1,+3,-1,+2}, // 7
      {-1,+2,-1,+3}, // 8
      {-3,+1,-1,+2}, // 9
    };

  best_digit = -1;
  
  // Note that each code has seven symbols in it, not counting the
  // initial space.
  for (i = 0; i < symbol_count - 7; i++)
  {
    /*
    for (j = 0; j < 7; j++)
      printf("%+d", symbols[i + j]);
    printf("\n");
    */

    a = symbols[i];
    b = symbols[i + 1];
    c = symbols[i + 2];

    // Look for a start guard: +N-N+N
    if (a > this->guard_min && b < -this->guard_min && c > this->guard_min)
    {
      mean = (a - b + c) / 3.0;
      min = MIN(a, MIN(-b, c));
      max = MAX(a, MAX(-b, c));
      assert(mean > 0);

      if ((mean - min) / mean > this->guard_tol)
        continue;
      if ((max - mean) / mean > this->guard_tol)
        continue;

      //printf("guard %d %.2f\n", i, mean);

      best_err = this->err_first;
      best_digit = -1;
      
      // Read the code digit (4 symbols) and compare against the known
      // digit patterns
      for (k = 0; k < (int) (sizeof(digits) / sizeof(digits[0])); k++)
      {
        err[k] = 0;        
        for (j = 0; j < 4; j++)
        {
          wm = digits[k][j];
          wo = symbols[i + 3 + j] / mean;
          err[k] += fabs(wo - wm);
          //printf("digit %d = %.3f %.3f\n", k, wm, wo);
        }
        //printf("digit %d = %.3f\n", k, err[k]);

        if (err[k] < best_err)
        {
          best_err = err[k];
          best_digit = k;
        }
      }

      // Id is good if it fits on and *only* one pattern.  So find the
      // second best digit and make sure it has a much higher error.
      for (k = 0; k < (int) (sizeof(digits) / sizeof(digits[0])); k++)
      {
        if (k == best_digit)
          continue;
        if (err[k] < this->err_second)
        {
          best_digit = -1;
          break;
        }
      }

      // Stop if we found a valid digit
      if (best_digit >= 0)
        break;
    }    
  }

  //if (best_digit >= 0)
  //  printf("best = %d\n", best_digit);

  return best_digit;
}


////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void LaserVisualBW::WriteData()
{
  int i;
  double r, b, o;
  struct timeval timestamp;
  fiducial_t *fiducial;
  player_fiducial_data_t data;

  data.count = 0;
  for (i = 0; i < this->fiducial_count; i++)
  {
    fiducial = this->fiducials + i;

    // Only report fiducials that where seen in the most recent laser
    // scan.
    if (fiducial->laser_time != this->laser_time)
      continue;
    
    r = sqrt(fiducial->pose[0] * fiducial->pose[0] +
             fiducial->pose[1] * fiducial->pose[1]);
    b = atan2(fiducial->pose[1], fiducial->pose[0]);
    o = fiducial->pose[2];

    data.fiducials[data.count].id = htons(((int16_t) fiducial->id));
    data.fiducials[data.count].pos[0] = htonl(((int32_t) (1000 * r * cos(b))));
    data.fiducials[data.count].pos[1] = htonl(((int32_t) (1000 * r * sin(b))));
    data.fiducials[data.count].rot[2] = htonl(((int32_t) (1000 * o)));
    data.count++;
  }
  data.count = htons(data.count);
  
  // Compute the data timestamp (from laser).
  timestamp.tv_sec = (uint32_t) this->laser_time;
  timestamp.tv_usec = (uint32_t) (fmod(this->laser_time, 1.0) * 1e6);
  
  // Copy data to server.
  PutData((void*) &data, sizeof(data), &timestamp);

  return;
}


