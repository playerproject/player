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
// Desc: Driver for detecting combined laser/visual barcodes.
// Author: Andrew Howard
// Date: 17 Aug 2002
// CVS: $Id$
//
// Theory of operation:
//   Parses a laser scan to find the retro-reflective patches (lines or
//   circles), then points the camera at the patch, zooms in, and
//   attempts to read the colored barcode.  Will not return sensible
//   orientations for circular patches.
//
// Requires:
//   SRF, PTZ and blobfinder devices.
//
///////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"


// Driver for detecting laser retro-reflectors.
class LaserVisualBarcode : public CDevice
{
  // Constructor
  public: LaserVisualBarcode(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

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

  // Update the PTZ to point at one of the laser reflectors.
  private: int UpdatePtz();

  // Select a target fiducial for the PTZ to inspect.
  private: void SelectPtzTarget(double time, player_ptz_data_t *data);

  // Servo the PTZ to a target fiducial.
  private: void ServoPtz(double time, player_ptz_data_t *data);

  // Process any new blobfinder data.
  private: int UpdateBlobfinder();

  // Find blobs with valid properties.
  private: void FindBlobs(double time, player_blobfinder_data_t *data);
    
  // Update the device data (the data going back to the client).
  private: void UpdateData();

  // Device pose relative to robot.
  private: double pose[3];
  
  // Fiducial properties.
  private: double width, height;

  // Laser stuff.
  private: int laser_index;
  private: CDevice *laser;
  private: double laser_time;

  // PTZ stuff
  private: int ptz_index;
  private: CDevice *ptz;
  private: double ptz_time;

  // Blobfinder stuff.
  private: int blobfinder_index;
  private: CDevice *blobfinder;
  private: double blobfinder_time;

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

    // Time at which fiducial was last seen by the ptz.
    double ptz_time;

    // Time at which ficuial was last seen by the blobfinder.
    double blobfind_time;
  };

  // List of currently tracked fiducials.
  private: int fiducial_count;
  private: fiducial_t fiducials[256];

  // The current selected fiducial for the ptz, and the time at which
  // we started paying attention to it.
  private: fiducial_t *ptz_fiducial;
  private: double ptz_fiducial_time;

  // Dimensions of the zoomed image for the target fiducial (m).
  private: double zoomwidth, zoomheight;

  // Info on valid blobs.
  private: struct blob_t
  {
    // Blob channel.
    int ch;

    // Blob position in image.
    int x, y;
  };

  // List of current valid blobs.
  private: int blob_count;
  private: blob_t blobs[256];
  
  // Local copy of the current fiducial data.
  private: player_fiducial_data_t fdata;
};


// Initialization function
CDevice* LaserVisualBarcode_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_FIDUCIAL_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"laserreflector\" does not support interface \"%s\"\n",
                  interface);
    return (NULL);
  }
  return ((CDevice*) (new LaserVisualBarcode(interface, cf, section)));
}


// a driver registration function
void LaserVisualBarcode_Register(DriverTable* table)
{
  table->AddDriver("laservisualbarcode", PLAYER_READ_MODE, LaserVisualBarcode_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserVisualBarcode::LaserVisualBarcode(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_fiducial_data_t), 0, 10, 10)
{
  // Device pose relative to robot.
  this->pose[0] = 0;
  this->pose[1] = 0;
  this->pose[2] = 0;

  // If laser_index is not overridden by an argument here, then we'll
  // use the device's own index, which we can get in Setup() below.
  this->laser_index = cf->ReadInt(section, "laser_index", -1);
  this->laser = NULL;
  this->laser_time = 0;

  this->ptz_index = cf->ReadInt(section, "ptz_index", -1);
  this->ptz = NULL;
  this->ptz_time = 0;

  this->blobfinder_index = cf->ReadInt(section, "blobfinder_index", -1);
  this->blobfinder = NULL;
  this->blobfinder_time = 0;

  // Default fiducial properties.
  this->width = cf->ReadLength(section, "width", 0.08);
  this->height = cf->ReadLength(section, "height", 0.02);

  // Reset fiducial list.
  this->fiducial_count = 0;

  // Reset PTZ target.
  this->ptz_fiducial = NULL;

  // Reset blob list.
  this->blob_count = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int LaserVisualBarcode::Setup()
{
  player_device_id_t id;
  
  // Subscribe to the laser.
  id.code = PLAYER_LASER_CODE;
  id.index = (this->laser_index > 0 ? this->laser_index : this->device_id.index);
  id.port = this->device_id.port;
  this->laser = deviceTable->GetDevice(id);
  if (!this->laser)
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return(-1);
  }
  if (this->laser->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return(-1);
  }

  // Get the laser geometry.
  // TODO: no support for this at the moment.
  this->pose[0] = 0.10;
  this->pose[1] = 0;
  this->pose[2] = 0;
  
  // Subscribe to the PTZ.
  id.code = PLAYER_PTZ_CODE;
  id.index = (this->ptz_index > 0 ? this->ptz_index : this->device_id.index);
  id.port = this->device_id.port;
  this->ptz = deviceTable->GetDevice(id);
  if (!this->ptz)
  {
    PLAYER_ERROR("unable to locate suitable PTZ device");
    return(-1);
  }
  if (this->ptz->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to PTZ device");
    return(-1);
  }

  // Subscribe to the blobfinder.
  id.code = PLAYER_BLOBFINDER_CODE;
  id.index = (this->blobfinder_index > 0 ? this->blobfinder_index : this->device_id.index);
  id.port = this->device_id.port;
  this->blobfinder = deviceTable->GetDevice(id);
  if (!this->blobfinder)
  {
    PLAYER_ERROR("unable to locate suitable blobfinder device");
    return(-1);
  }
  if (this->blobfinder->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to blobfinder device");
    return(-1);
  }

  // Start the driver thread.
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int LaserVisualBarcode::Shutdown()
{
  // Stop the driver thread.
  StopThread();
  
  // Unsubscribe from devices.
  this->blobfinder->Unsubscribe(this);
  this->ptz->Unsubscribe(this);
  this->laser->Unsubscribe(this);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void LaserVisualBarcode::Main() 
{
  while (true)
  {
    // Go to sleep for a while (this is a polling loop).
    usleep(10000);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    HandleRequests();

    // Process any new laser data.
    if (UpdateLaser())
    {
      // Update the device data (the data going back to the client).
      UpdateData();
    }

    // Process any new PTZ data.
    UpdatePtz();

    // Process any new blobfinder data.
    UpdateBlobfinder();
  }
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int LaserVisualBarcode::HandleRequests()
{
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  int len;
  
  while ((len = GetConfig(&client, &request, sizeof(request))) > 0)
  {
    switch (request[0])
    {
      case PLAYER_LASER_GET_GEOM:
        HandleGetGeom(client, request, len);
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
// Handle geometry requests.
void LaserVisualBarcode::HandleGetGeom(void *client, void *request, int len)
{
  player_fiducial_geom_t geom;

  if (len != 1)
  {
    PLAYER_ERROR2("geometry request len is invalid (%d != %d)", len, 1);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  geom.pose[0] = htons((short) (this->pose[0] * 1000));
  geom.pose[1] = htons((short) (this->pose[1] * 1000));
  geom.pose[2] = htons((short) (this->pose[2] * 180/M_PI));

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process laser data.
int LaserVisualBarcode::UpdateLaser()
{
  int i;
  player_laser_data_t data;
  size_t size;
  uint32_t timesec, timeusec;
  double time;
  
  // Get the laser data.
  size = this->laser->GetData((unsigned char*) &data, sizeof(data), &timesec, &timeusec);
  time = (double) timesec + ((double) timeusec) * 1e-6;
  
  // Dont do anything if this is old data.
  if (time == this->laser_time)
    return 0;
  this->laser_time = time;
  
  // Do some byte swapping on the laser data.
  data.resolution = ntohs(data.resolution);
  data.min_angle = ntohs(data.min_angle);
  data.max_angle = ntohs(data.max_angle);
  data.range_count = ntohs(data.range_count);
  for (i = 0; i < data.range_count; i++)
    data.ranges[i] = ntohs(data.ranges[i]);

  // Find possible fiducials in this scan.
  this->FindLaserFiducials(time, &data);

  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Analyze the laser data to find fidicuials (reflectors).
void LaserVisualBarcode::FindLaserFiducials(double time, player_laser_data_t *data)
{
  int i, h;
  int valid;
  double r, b;
  double db, dr;
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
    r = (double) (data->ranges[i]) / 1000;
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
      dr = this->width / 2;
      db = atan2(this->width / 2, mr);
      valid &= (mrr < (dr * dr));
      valid &= (mbb < (db * db));
      
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
void LaserVisualBarcode::FitLaserFiducial(player_laser_data_t *data,
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
    r = (double) (data->ranges[i]) / 1000;
    b = (double) (data->min_angle + i * data->resolution) / 100.0 * M_PI / 180;

    if (r < mr)
      mr = r;
    mn += 1.0;
    mb += b;
  }

  mr += this->width / 2;
  mb /= mn;

  pose[0] = mr * cos(mb);
  pose[1] = mr * sin(mb);
  pose[2] = mb;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Match a new laser fiducial against the ones we are already
// tracking.  The pose is relative to the laser.
void LaserVisualBarcode::MatchLaserFiducial(double time, double pose[3])
{
  int i;
  double k;
  double dx, dy, dr;
  double mindr;
  fiducial_t *fiducial;
  fiducial_t *minfiducial;

  // TODO get from config
  // Low-pass filter gain.
  k = 1.0;
  
  // Observations must be at least this close to the existing
  // fiducial.
  mindr = 0.20;  // TODO get from config or use pose uncertainty.
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
      minfiducial->ptz_time = 0.0;
      minfiducial->blobfind_time = 0.0;
    }
  }

  // Otherwise, update the existing fiducial.
  else
  {
    minfiducial->pose[0] = (1 - k) * minfiducial->pose[0] + k * pose[0];
    minfiducial->pose[1] = (1 - k) * minfiducial->pose[1] + k * pose[1];
    minfiducial->pose[2] = (1 - k) * minfiducial->pose[2] + k * pose[2];
    minfiducial->laser_time = time;
  }

  // Remove any old fiducials.
  for (i = 0; i < this->fiducial_count; i++)
  {
    fiducial = this->fiducials + i;

    if (time - this->laser_time > 2.0) // TODO
    {
      memmove(fiducial, fiducial + sizeof(fiducial_t),
              (this->fiducial_count - i - 1) * sizeof(fiducial_t));
      this->fiducial_count--;
    }
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the PTZ to point at one of the laser reflectors.
int LaserVisualBarcode::UpdatePtz()
{
  int i;
  player_ptz_data_t data;
  size_t size;
  uint32_t timesec, timeusec;
  double time;
  
  // Get the ptz data.
  size = this->ptz->GetData((unsigned char*) &data, sizeof(data), &timesec, &timeusec);
  time = (double) timesec + ((double) timeusec) * 1e-6;
  
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
}


////////////////////////////////////////////////////////////////////////////////
// Select a target fiducial for the PTZ to inspect.
// This algorithm picks the one that we havent looked at for a long time.
void LaserVisualBarcode::SelectPtzTarget(double time, player_ptz_data_t *data)
{
  int i, maxi;
  double t, maxt;
  fiducial_t *fiducial;

  // Consider the currently selected target.
  // Give the blobfinder time to identify it.
  if (this->ptz_fiducial != NULL)
  {
    if (this->ptz_fiducial->id < 0)
      if (time - this->ptz_fiducial_time < 4.0)
        return;
  }

  // Blobfinder has made the identification, or we have given up.
  if (this->ptz_fiducial != NULL)
    this->ptz_fiducial->blobfind_time = time;
      
  // Find one we havent looked at for while.
  maxt = 0.0;
  this->ptz_fiducial = NULL;
  
  for (i = 0; i < this->fiducial_count; i++)
  {
    fiducial = this->fiducials + i;

    if (fiducial->id >= 0)
      continue;
    t = time - fiducial->blobfind_time;
    if (t > maxt)
    {
      maxt = t;
      this->ptz_fiducial = fiducial;
      this->ptz_fiducial_time = time;
    }
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Servo the PTZ to a target fiducial.
void LaserVisualBarcode::ServoPtz(double time, player_ptz_data_t *data)
{
  double dx, dy, r, b, e;
  fiducial_t *fiducial;
  player_ptz_cmd_t cmd;
  double maxtime, maxtilt;

  // Max time to spend looking at one fiducial.
  maxtime = 4.0;

  // Max tilt value.
  maxtilt = 5 * M_PI / 180;
  
  fiducial = this->ptz_fiducial;
  if (fiducial == NULL)
  {
    r = 0;
    b = 0;
    e = 0;
  }
  else
  {
    // Compute range and bearing of fiducial relative to camera.
    // TODO: account for camera geometry.
    dx = fiducial->pose[0];
    dy = fiducial->pose[1];
    r = sqrt(dx * dx + dy * dy);
    b = atan2(dy, dx);
    e = maxtilt * sin((time - this->ptz_fiducial_time) / maxtime * 2 * M_PI);

    // TODO: compute zoom value.
  }

  // Compute the dimensions of the image at the range of the target fiducial.
  this->zoomwidth = 2 * r * tan(data->zoom * M_PI / 180 / 2);
  this->zoomheight = 3.0 / 4.0 * this->zoomwidth;

  // Compose the command packet to send to the PTZ device.
  cmd.pan = htons(((int16_t) (b * 180 / M_PI)));
  cmd.tilt = htons(((int16_t) (e * 180 / M_PI)));
  cmd.zoom = htons(20);
  //TESTING this->ptz->PutCommand((unsigned char*) &cmd, sizeof(cmd));

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process any new blobfinder data.
int LaserVisualBarcode::UpdateBlobfinder()
{
  int i, ch;
  player_blobfinder_data_t data;
  player_blobfinder_header_elt_t *channel;
  player_blobfinder_blob_elt_t *blob;
  size_t size;
  uint32_t timesec, timeusec;
  double time;
  
  // Get the blobfinder data.
  size = this->blobfinder->GetData((unsigned char*) &data, sizeof(data), &timesec, &timeusec);
  time = (double) timesec + ((double) timeusec) * 1e-6;
  
  // Dont do anything if this is old data.
  if (time == this->blobfinder_time)
    return 0;
  this->blobfinder_time = time;

  // Do some byte-swapping
  data.width = ntohs(data.width);
  data.height = ntohs(data.height);
  for (ch = 0; ch < PLAYER_BLOBFINDER_MAX_CHANNELS; ch++)
  {
    channel = data.header + ch;
    channel->index = ntohs(channel->index);
    channel->num = ntohs(channel->num);
    
    for (i = channel->index; i < channel->index + channel->num; i++)
    {
      blob = data.blobs + i;
      blob->x = ntohs(blob->x);
      blob->y = ntohs(blob->y);
      blob->left = ntohs(blob->left);
      blob->right = ntohs(blob->right);
      blob->top = ntohs(blob->top);
      blob->bottom = ntohs(blob->bottom);
      blob->area = ntohl(blob->area);
    }
  }

  // Extract valid blobs.
  this->FindBlobs(time, &data);

  printf("found blobs %d\n", this->blob_count);
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Find blobs with valid properties.
void LaserVisualBarcode::FindBlobs(double time, player_blobfinder_data_t *data)
{
  int i, ch;
  blob_t *nblob;
  player_blobfinder_header_elt_t *channel;
  player_blobfinder_blob_elt_t *blob;
  int width, height;
  int minx, maxx, miny, maxy;
  int minwidth, maxwidth, minheight, maxheight;
  int minarea, maxarea;
  double tol;

  // Allowable tolerance (fractional error).
  tol = 0.5;

  // Compute expect width and height at the current range and zoom.
  width = (int) (this->width / this->zoomwidth * data->width);
  height = (int) (this->height / this->zoomheight * data->height);
      
  // Set limits
  minx = (int) ((1 - tol) * data->width / 2);
  maxx = (int) ((1 + tol) * data->width / 2);
  miny = 0;
  maxy = data->height;
  minwidth = (int) ((1 - tol) * width);
  maxwidth = (int) ((1 + tol) * width);
  minheight = (int) ((1 - tol) * height);
  maxheight = (int) ((1 + tol) * height);
  minarea = 50;
  maxarea = maxwidth * maxheight;

  printf("zoom %.3f %.3f\n", this->zoomwidth, this->zoomheight);
  printf("image %d %d\n", data->width, data->height);
  printf("w, h %d %d\n", width, height);
  
  this->blob_count = 0;
  for (ch = 0; ch < PLAYER_BLOBFINDER_MAX_CHANNELS; ch++)
  {
    channel = data->header + ch;
    
    for (i = channel->index; i < channel->index + channel->num; i++)
    {
      blob = data->blobs + i;

      // Test the blob properties.
      if (blob->x < minx || blob->x > maxx)
        continue;
      if (blob->y < miny || blob->y > maxy)
        continue;
      if ((blob->right - blob->left) < minwidth || (blob->right - blob->left) > maxwidth)
        continue;
      if ((blob->bottom - blob->top) < minheight || (blob->bottom - blob->top) > maxheight)
        continue;
      if (blob->area < minarea || blob->area > maxarea)
        continue;

      printf("%d %d : %d %d : %d\n", blob->x, blob->y,
             blob->right - blob->left, blob->bottom - blob->top, blob->area);

      // Add to valid blob list.
      if (this->blob_count < ARRAYSIZE(this->blobs))
      {
        nblob = this->blobs + this->blob_count++;
        nblob->ch = ch;
        nblob->x = blob->x;
        nblob->y = blob->y;
      }
    }
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void LaserVisualBarcode::UpdateData()
{
  int i;
  int id;
  double r, b, o;
  uint32_t timesec, timeusec;
  fiducial_t *fiducial;
  player_fiducial_data_t data;
  
  data.count = htons(this->fiducial_count);
  for (i = 0; i < this->fiducial_count; i++)
  {
    fiducial = this->fiducials + i;

    r = sqrt(fiducial->pose[0] * fiducial->pose[0] + fiducial->pose[1] * fiducial->pose[1]);
    b = atan2(fiducial->pose[1], fiducial->pose[0]);
    o = fiducial->pose[2];
    
    /* REMOVE
    r = fiducial->pose[0];
    b = fiducial->pose[1];    
    o = fiducial->pose[2];
    */
    
    data.fiducials[i].id = htons(((int16_t) fiducial->id));
    data.fiducials[i].pose[0] = htons(((int16_t) (1000 * r)));
    data.fiducials[i].pose[1] = htons(((int16_t) (180 * b / M_PI)));
    data.fiducials[i].pose[2] = htons(((int16_t) (180 * o / M_PI))); 
  }

  // Compute the data timestamp (from laser).
  timesec = (int) this->laser_time;
  timeusec = (int) (fmod(this->laser_time, 1.0) * 1e6);
  
  // Copy data to server.
  PutData((unsigned char*) &data, sizeof(data), timesec, timeusec);
}
