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

  // Write laser data to a file (testing).
  private: int WriteLaser();

  // Read laser data from a file (testing).
  private: int ReadLaser();

  // Segment the scan into straight-line segments.
  private: void SegmentLaser();

  // Update the line filter.  Returns an error signal.
  private: double UpdateFilter(double x[2], double P[2][2], double Q[2][2],
                               double R, double z, double res);

  // Fit lines to the segments.
  private: void FitSegments();

  // Merge overlapping segments with similar properties.
  private: void MergeSegments();

  // Update the device data (the data going back to the client).
  private: void UpdateData();

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

  // Fiducial data.
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
  {
    this->UpdateLaser();
    this->UpdateData();
  }
  
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
  
  // Do some byte swapping on the laser data.
  this->laser_data.resolution = ntohs(this->laser_data.resolution);
  this->laser_data.min_angle = ntohs(this->laser_data.min_angle);
  this->laser_data.max_angle = ntohs(this->laser_data.max_angle);
  this->laser_data.range_count = ntohs(this->laser_data.range_count);
  for (i = 0; i < this->laser_data.range_count; i++)
    this->laser_data.ranges[i] = ntohs(this->laser_data.ranges[i]);

  /*
  for (i = 0; i < this->laser_data.range_count; i++)
  {
    r = (double) (this->laser_data.ranges[i]) / 1000;
    b = (double) (this->laser_data.min_angle) / 100.0 * M_PI / 180 + i * res;
  }
  */

  return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void LaserCSpace::UpdateData()
{

  // TODO
  
  return;
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

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


