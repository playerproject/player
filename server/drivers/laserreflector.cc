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
// Desc: Driver for detecting retro-reflective markers in a laser scan.
// Author: Andrew Howard
// Date: 16 Aug 2002
// CVS: $Id$
//
// Theory of operation
//
// Parses a laser scan to find the regions of constant depth that are
// also retro-reflective.  Will work with either flat or cylindrical
// markers with appropriate options, but will only return sensible
// orientation information on the flat markers.
//
///////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>

#define PLAYER_ENABLE_TRACE 0
#define PLAYER_ENABLE_MSG 0

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"


// Driver for detecting laser retro-reflectors.
class LaserReflector : public CDevice
{
  // Constructor
  public: LaserReflector(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Data configuration.
  public: virtual size_t GetData(unsigned char *, size_t maxsize,
                                 uint32_t* timestamp_sec,
                                 uint32_t* timestamp_usec);
  public: virtual int PutConfig(player_device_id_t* device, void *client, 
                                void *data, size_t len);

  // Analyze the laser data and pick out reflectors.
  private: void Find(const player_laser_data_t *laserdata,
                     player_fiducial_data_t *beacondata);

  // Test a patch to see if it has valid moments.
  private: bool TestMoments(double mn, double mr, double mb, double mrr, double mbb);

  // Find the line of best fit for the given segment of the laser
  // scan.  Fills in the pose and pose uncertainty of the reflector
  // (range, bearing, orientation).
  private: void FitCircle(const player_laser_data_t *laserdata,
                            int first, int last,
                            double *pr, double *pb, double *po,
                            double *ur, double *ub, double *uo);

  // Add a item into the beacon list.
  private: void Add(player_fiducial_data_t *beacondata,
                    double pr, double pb, double po,
                    double ur, double ub, double uo);

  // Device pose relative to robot.
  private: double pose[3];
  
  // Pointer to laser to get data from.
  private: int laser_index;
  private: CDevice *laser;

  // Reflector properties.
  private: double reflector_width;
  
  // Local copy of the current laser data.
  private: player_laser_data_t laserdata;

  // Local copy of the current beacon data.
  private: player_fiducial_data_t fiducialdata;
};


// Initialization function
CDevice* LaserReflector_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_FIDUCIAL_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"laserreflector\" does not support interface \"%s\"\n",
                  interface);
    return (NULL);
  }
  return ((CDevice*) (new LaserReflector(interface, cf, section)));
}


// a driver registration function
void LaserReflector_Register(DriverTable* table)
{
  table->AddDriver("laserreflector", PLAYER_READ_MODE, LaserReflector_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserReflector::LaserReflector(char* interface, ConfigFile* cf, int section)
    : CDevice(0, 0, 0, 1)
{
  // TODO: how can we get the geometry from the laser?
  // Device pose relative to robot.
  this->pose[0] = 0.10;
  this->pose[1] = 0;
  this->pose[2] = 0;

  // If laser_index is not overridden by an argument here, then we'll
  // use the device's own index, which we can get in Setup() below.
  this->laser_index = cf->ReadInt(section, "laser_index", -1);

  // Default reflector properties.
  this->reflector_width = 0.08;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int LaserReflector::Setup()
{
  // Get the pointer to the laser.  If index was not overridden by an
  // argument in the constructor, then we use this driver's index.
  player_device_id_t id;
  id.port = this->device_id.port;
  id.code = PLAYER_LASER_CODE;
  
  if(this->laser_index >= 0)
    id.index = this->laser_index;
  else
    id.index = this->device_id.index;
  
  printf("LaserReflector:Setup(%d:%d:%d)\n", id.code,id.index,id.port);
  
  if(!(this->laser = deviceTable->GetDevice(id)))
  {
    fputs("LaserReflector:Setup(): couldn't find laser device\n",stderr);
    return(-1);
  }
    
  // Subscribe to the laser device, but fail if it fails
  if(this->laser->Subscribe(this) != 0)
  {
    fputs("LaserReflector:Setup(): couldn't setup laser device\n",stderr);
    return(-1);
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int LaserReflector::Shutdown()
{
  // Unsubscribe from the laser device
  this->laser->Unsubscribe(this);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by server thread).
size_t LaserReflector::GetData(unsigned char *dest, size_t maxsize,
                               uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  int i;
  size_t laser_size;
  uint32_t laser_time_sec, laser_time_usec;
  
  // Get the laser data.
  laser_size = this->laser->GetData((unsigned char*) &this->laserdata, sizeof(this->laserdata),
                                    &laser_time_sec, &laser_time_usec);
  assert(laser_size <= sizeof(this->laserdata));
  
  // If the laser doesnt have new data, just return a copy of our old
  // data.
  if (laser_time_sec == this->data_timestamp_sec &&
      laser_time_usec == this->data_timestamp_usec)
  {
    assert(maxsize >= sizeof(this->fiducialdata));
    memcpy(dest, &this->fiducialdata, sizeof(this->fiducialdata));
    return (sizeof(this->fiducialdata));
  }

  // Do some byte swapping
  this->laserdata.resolution = ntohs(this->laserdata.resolution);
  this->laserdata.min_angle = ntohs(this->laserdata.min_angle);
  this->laserdata.max_angle = ntohs(this->laserdata.max_angle);
  this->laserdata.range_count = ntohs(this->laserdata.range_count);
  for (i = 0; i < this->laserdata.range_count; i++)
    this->laserdata.ranges[i] = ntohs(this->laserdata.ranges[i]);

  // Analyse the laser data
  this->Find(&this->laserdata, &this->fiducialdata);

  // Do some byte-swapping on the beacon data.
  for (i = 0; i < this->fiducialdata.count; i++)
  {
    this->fiducialdata.fiducials[i].pose[0] = htons(this->fiducialdata.fiducials[i].pose[0]);
    this->fiducialdata.fiducials[i].pose[1] = htons(this->fiducialdata.fiducials[i].pose[1]);
    this->fiducialdata.fiducials[i].pose[2] = htons(this->fiducialdata.fiducials[i].pose[2]);
  }
  this->fiducialdata.count = htons(this->fiducialdata.count);

  // Copy results
  assert(maxsize >= sizeof(this->fiducialdata));
  memcpy(dest, &this->fiducialdata, sizeof(this->fiducialdata));

  // Copy the laser timestamp
  *timestamp_sec = this->laser->data_timestamp_sec;
  *timestamp_usec = this->laser->data_timestamp_usec;
  
  return (sizeof(this->fiducialdata));
}


////////////////////////////////////////////////////////////////////////////////
// Put configuration in buffer (called by server thread)
int LaserReflector::PutConfig(player_device_id_t* device, void *client, 
                              void *data, size_t len) 
{
  int subtype;
  player_fiducial_geom_t geom;

  if (len < 1)
  {
    PLAYER_ERROR("empty requestion; ignoring");
    return 0;
  }
  
  subtype = ((uint8_t*) data)[0];
  switch (subtype)
  {
    case PLAYER_FIDUCIAL_GET_GEOM:
    {
      if (len != 1)
      {
        PLAYER_ERROR2("request len is invalid (%d != %d)", len, 1);
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }

      geom.pose[0] = htons((short) (this->pose[0] * 1000));
      geom.pose[1] = htons((short) (this->pose[1] * 1000));
      geom.pose[2] = htons((short) (this->pose[2] * 180/M_PI));
              
      if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
    default:
    {
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
  }
  
  return (0);
}


////////////////////////////////////////////////////////////////////////////////
// Analyze the laser data to find reflectors.
void LaserReflector::Find(const player_laser_data_t *laserdata,
                          player_fiducial_data_t *fiducialdata)
{
  int i;
  int h;
  double r, b;
  double mn, mr, mb, mrr, mbb;
  double pr, pb, po;
  double ur, ub, uo;

  // Empty the beacon list.
  fiducialdata->count = 0;
  
  // Initialise patch statistics.
  mn = 0.0;
  mr = 0.0;
  mb = 0.0;
  mrr = 0.0;
  mbb = 0.0;
    
  // Look for a candidate patch in scan.
  for (i = 0; i < laserdata->range_count; i++)
  {
    r = (double) (laserdata->ranges[i] & 0x1FFF) / 1000;
    b = (double) (laserdata->min_angle + i * laserdata->resolution) / 100.0 * M_PI / 180;
    h = (int) (laserdata->ranges[i] >> 13);

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
      
      // Apply tests to see if this is a sensible looking patch.
      if (this->TestMoments(mn, mr, mb, mrr, mbb))
      {
        // Do a best fit to determine the pose of the reflector.
        this->FitCircle(laserdata, i - (int) mn, i - 1, &pr, &pb, &po, &ur, &ub, &uo);

        // Fill in the beacon data structure.
        this->Add(fiducialdata, pr, pb, po, ur, ub, uo);
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
// Test a patch to see if it has valid moments.
bool LaserReflector::TestMoments(double mn, double mr, double mb, double mrr, double mbb)
{
  double dr, db;
  
  printf("Testing moments %.0f %f %f %f %f\n", mn, mr, mb, mrr, mbb);

  if (mn < 2.0)
    return false;

  // These are tests for a cylindrical reflector.
  dr = this->reflector_width / 2;
  db = atan2(this->reflector_width / 2, mr);
  if (mrr > dr * dr)
    return false;
  if (mbb > db * db)
    return false;
  
  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Find the line of best fit for the given segment of the laser scan.
// Fills in the pose and pose uncertainty of the reflector (range,
// bearing, orientation).  This one works for cylindrical beacons.
void LaserReflector::FitCircle(const player_laser_data_t *laserdata,
                               int first, int last,
                               double *pr, double *pb, double *po,
                               double *ur, double *ub, double *uo)
{
  int i;
  double r, b;
  double mn, mr, mb;

  mn = 0.0;
  mr = 1e6;
  mb = 0.0;

  for (i = first; i <= last; i++)
  {
    r = (double) (laserdata->ranges[i] & 0x1FFF) / 1000;
    b = (double) (laserdata->min_angle + i * laserdata->resolution) / 100.0 * M_PI / 180;

    if (r < mr)
      mr = r;
    mn += 1.0;
    mb += b;
  }

  mr += this->reflector_width / 2;
  mb /= mn;

  *pr = mr;
  *pb = mb;
  *po = 0.0;

  // TODO: put in proper uncertainty estimates.
  *ur = 0.02;  
  *ub = laserdata->resolution / 100.0 * M_PI / 180;
  *uo = 1e6;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Add a item into the beacon list.
void LaserReflector::Add(player_fiducial_data_t *fiducialdata,
                         double pr, double pb, double po,
                         double ur, double ub, double uo)

{
  player_fiducial_item_t *beacon;

  printf("adding %f %f %f\n", pr, pb, po);
  
  assert(fiducialdata->count < ARRAYSIZE(fiducialdata->fiducials));
  beacon = fiducialdata->fiducials + fiducialdata->count++;
  beacon->id = 0;
  beacon->pose[0] = (uint16_t) (int) (pr * 1000);
  beacon->pose[1] = (int16_t) (int) (pb * 180 / M_PI);
  beacon->pose[2] = (int16_t) (int) (po * 180 / M_PI);

  return;
}


