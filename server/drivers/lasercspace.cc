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
// Desc: Driver for computing the config space for a laser scan.
// Author: Andrew Howard
// Date: 1 Sep 2002
// CVS: $Id$
//
// Theory of operation - Shortens each range reading in the laser scan
// such that the new scan delimits the boundary of free configuration
// space (for a robot of some known radius).
//
// Requires - Laser device.
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


// Driver for computing the free c-space from a laser scan.
class LaserCSpace : public CDevice
{
  // Constructor
  public: LaserCSpace(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Client interface (this device has no thread).
  public: virtual size_t GetData(unsigned char *dest, size_t maxsize,
                                 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  // Client interface (this device has no thread).
  public: virtual int PutConfig(player_device_id_t* device, void *client, 
                                void *data, size_t len);

  // Process laser data.  Returns non-zero if the laser data has been
  // updated.
  private: int UpdateLaser();

  // Compute the maximum free-space range for sample n.
  private: double FreeRange(int n);

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Device pose relative to robot.
  private: double pose[3];
  
  // Laser stuff.
  private: int laser_index;
  private: CDevice *laser_device;
  private: player_laser_data_t laser_data;
  private: uint32_t laser_timesec, laser_timeusec;

  // Step size for subsampling the scan (saves CPU cycles)
  private: int sample_step;

  // Robot radius.
  private: double radius;

  // Fiducila stuff (the data we generate).
  private: player_laser_data_t data;
  private: uint32_t timesec, timeusec;
};


// Initialization function
CDevice* LaserCSpace_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_LASER_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"lasercspace\" does not support interface \"%s\"\n",
                  interface);
    return (NULL);
  }
  return ((CDevice*) (new LaserCSpace(interface, cf, section)));
}


// a driver registration function
void LaserCSpace_Register(DriverTable* table)
{
  table->AddDriver("lasercspace", PLAYER_READ_MODE, LaserCSpace_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserCSpace::LaserCSpace(char* interface, ConfigFile* cf, int section)
    : CDevice(0, 0, 0, 1)
{
  // Device pose relative to robot.
  this->pose[0] = 0;
  this->pose[1] = 0;
  this->pose[2] = 0;

  // If laser_index is not overridden by an argument here, then we'll
  // use the device's own index, which we can get in Setup() below.
  this->laser_index = cf->ReadInt(section, "laser", -1);
  this->laser_device = NULL;
  this->laser_timesec = 0;
  this->laser_timeusec = 0;

  // Settings.
  this->radius = cf->ReadLength(section, "radius", 0.50);
  this->sample_step = cf->ReadInt(section, "step", 10);
  
  // Outgoing data
  this->timesec = 0;
  this->timeusec = 0;
  memset(&this->data, 0, sizeof(this->data));

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int LaserCSpace::Setup()
{
  player_device_id_t id;
  
  // Subscribe to the laser.
  id.code = PLAYER_LASER_CODE;
  id.index = (this->laser_index >= 0 ? this->laser_index : this->device_id.index);
  id.port = this->device_id.port;
  this->laser_device = deviceTable->GetDevice(id);
  if (!this->laser_device)
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return(-1);
  }
  if (this->laser_device->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return(-1);
  }

  // Get the laser geometry.
  // TODO: no support for this at the moment.
  //this->pose[0] = 0.10;
  //this->pose[1] = 0;
  //this->pose[2] = 0;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int LaserCSpace::Shutdown()
{
  // Unsubscribe from devices.
  this->laser_device->Unsubscribe(this);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by client thread)
size_t LaserCSpace::GetData(unsigned char *dest, size_t maxsize,
                            uint32_t* timesec, uint32_t* timeusec)
{
  // Get the current laser data.
  this->laser_device->GetData((uint8_t*) &this->laser_data, sizeof(this->laser_data),
                              &this->laser_timesec, &this->laser_timeusec);
  
  // If there is new laser data, update our data.  Otherwise, we will
  // just reuse the existing data.
  if (this->laser_timesec != this->timesec || this->laser_timeusec != this->timeusec)
    this->UpdateLaser();

  // Copy results
  ASSERT(maxsize >= sizeof(this->data));
  memcpy(dest, &this->data, sizeof(this->data));

  // Copy the laser timestamp
  this->timesec = this->laser_timesec;
  this->timeusec = this->laser_timeusec;
  *timesec = this->timesec;
  *timeusec = this->timeusec;

  return (sizeof(this->data));
}


////////////////////////////////////////////////////////////////////////////////
// Process laser data.
int LaserCSpace::UpdateLaser()
{
  int i;
  double r;
  
  // Do some byte swapping on the laser data.
  this->laser_data.resolution = ntohs(this->laser_data.resolution);
  this->laser_data.min_angle = ntohs(this->laser_data.min_angle);
  this->laser_data.max_angle = ntohs(this->laser_data.max_angle);
  this->laser_data.range_count = ntohs(this->laser_data.range_count);
  for (i = 0; i < this->laser_data.range_count; i++)
    this->laser_data.ranges[i] = ntohs(this->laser_data.ranges[i]);

  // Construct the outgoing laser packet
  this->data.resolution = this->laser_data.resolution;
  this->data.min_angle = this->laser_data.min_angle;
  this->data.max_angle = this->laser_data.max_angle;
  this->data.range_count = this->laser_data.range_count;
  
  for (i = 0; i < this->laser_data.range_count; i++)
  {
    r = FreeRange(i);
    this->data.ranges[i] = (int16_t) (r * 1000);
  }

    // Do some byte swapping on the outgoing data.
  this->data.resolution = htons(this->data.resolution);
  this->data.min_angle = htons(this->data.min_angle);
  this->data.max_angle = htons(this->data.max_angle);
  for (i = 0; i < this->data.range_count; i++)
    this->data.ranges[i] = htons(this->data.ranges[i]);
  this->data.range_count = htons(this->data.range_count);

  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Compute the maximum free-space range for sample n.
double LaserCSpace::FreeRange(int n)
{
  int i, step;
  double r, b, x, y;
  double r_, b_, x_, y_;
  double s, nr, nx, ny, dx, dy;
  double d, h;
  double max_r;

  // Step size for subsampling the scan (saves CPU cycles)
  step = this->sample_step;
  
  // Range and bearing of this reading.
  r = (double) (this->laser_data.ranges[n]) / 1000;
  b = (double) (this->laser_data.min_angle +
                this->laser_data.resolution * n) / 100.0 * M_PI / 180;
  x = r * cos(b);
  y = r * sin(b);

  max_r = r - this->radius;

  // Look for intersections with obstacles.
  for (i = 0; i < this->laser_data.range_count; i += step)
  {
    r_ = (double) (this->laser_data.ranges[i]) / 1000;
    b_ = (double) (this->laser_data.min_angle +
                   this->laser_data.resolution * i) / 100.0 * M_PI / 180;
    x_ = r_ * cos(b_);
    y_ = r_ * sin(b_);

    // Compute parametric point on ray that is nearest the obstacle.
    s = (x * x_ + y * y_) / (x * x + y * y);
    if (s < 0 || s > 1)
      continue;

    // Compute the nearest point.
    nr = s * r;
    nx = s * x;
    ny = s * y;

    // Compute distance from nearest point to obstacle.
    dx = nx - x_;
    dy = ny - y_;
    d = sqrt(dx * dx + dy * dy);
    
    if (d > this->radius)
      continue;
    
    // Compute the shortened range.
    h = nr - sqrt(this->radius * this->radius - d * d);
    if (h < max_r)
      max_r = h;

    //printf("%d %d %f %f %f %f\n", n, i, s, d, r, max_r);
  }

  // Clip negative ranges.
  if (max_r < 0)
    max_r = 0;

  return max_r;
}


////////////////////////////////////////////////////////////////////////////////
// Put configuration in buffer (called by client thread)
int LaserCSpace::PutConfig(player_device_id_t* device, void *client, void *data, size_t len) 
{
  uint8_t subtype;

  subtype = ((uint8_t*) data)[0];
  
  switch (subtype)
  {
    case PLAYER_LASER_GET_GEOM:
      HandleGetGeom(client, data, len);
      break;
    default:
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Handle geometry requests.
void LaserCSpace::HandleGetGeom(void *client, void *request, int len)
{
  player_laser_geom_t geom;

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
  geom.size[0] = htons((short) (0.15 * 1000)); // TODO
  geom.size[1] = htons((short) (0.15 * 1000));

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


