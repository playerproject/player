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
  private: void Find();

  // Test a patch to see if it has valid moments.
  private: bool TestMoments(double mn, double mr, double mb, double mrr, double mbb);

  // Find the line of best fit for the given segment of the laser
  // scan.  Fills in the pose and pose uncertainty of the reflector
  // (range, bearing, orientation).
  private: void FitCircle(int first, int last,
                          double *pr, double *pb, double *po,
                          double *ur, double *ub, double *uo);

  // Add a item into the fiducial list.
  private: void Add(double pr, double pb, double po,
                    double ur, double ub, double uo);

  // Device pose relative to robot.
  private: double pose[3];
  
  // Pointer to laser to get data from.
  private: int laser_index;
  private: CDevice *laser;

  // Reflector properties.
  private: double reflector_width;
  
  // Local copy of the current laser data.
  private: player_laser_data_t ldata;

  // Local copy of the current fiducial data.
  private: player_fiducial_data_t fdata;
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
  // Device pose relative to robot.
  this->pose[0] = 0;
  this->pose[1] = 0;
  this->pose[2] = 0;

  // If laser_index is not overridden by an argument here, then we'll
  // use the device's own index, which we can get in Setup() below.
  this->laser_index = cf->ReadInt(section, "laser_index", -1);

  // Default reflector properties.
  this->reflector_width = cf->ReadLength(section, "width", 0.08);
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
  
  if (this->laser_index >= 0)
    id.index = this->laser_index;
  else
    id.index = this->device_id.index;
  
  if (!(this->laser = deviceTable->GetDevice(id)))
  {
    PLAYER_ERROR("unable to locate suitable SRF device");
    return(-1);
  }
    
  // Subscribe to the laser device, but fail if it fails
  if (this->laser->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to SRF device");
    return(-1);
  }

  // Get the laser geometry.
  // TODO: no support for this at the moment.
  this->pose[0] = 0.10;
  this->pose[1] = 0;
  this->pose[2] = 0;

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
  laser_size = this->laser->GetData((unsigned char*) &this->ldata, sizeof(this->ldata),
                                    &laser_time_sec, &laser_time_usec);
  assert(laser_size <= sizeof(this->ldata));
  
  // If the laser doesnt have new data, just return a copy of our old
  // data.
  if (laser_time_sec == this->data_timestamp_sec &&
      laser_time_usec == this->data_timestamp_usec)
  {
    assert(maxsize >= sizeof(this->fdata));
    memcpy(dest, &this->fdata, sizeof(this->fdata));
    return (sizeof(this->fdata));
  }

  // Do some byte swapping
  this->ldata.resolution = ntohs(this->ldata.resolution);
  this->ldata.min_angle = ntohs(this->ldata.min_angle);
  this->ldata.max_angle = ntohs(this->ldata.max_angle);
  this->ldata.range_count = ntohs(this->ldata.range_count);
  for (i = 0; i < this->ldata.range_count; i++)
    this->ldata.ranges[i] = ntohs(this->ldata.ranges[i]);

  // Analyse the laser data
  this->Find();

  // Do some byte-swapping on the fiducial data.
  for (i = 0; i < this->fdata.count; i++)
  {
    this->fdata.fiducials[i].pose[0] = htons(this->fdata.fiducials[i].pose[0]);
    this->fdata.fiducials[i].pose[1] = htons(this->fdata.fiducials[i].pose[1]);
    this->fdata.fiducials[i].pose[2] = htons(this->fdata.fiducials[i].pose[2]);
  }
  this->fdata.count = htons(this->fdata.count);

  // Copy results
  assert(maxsize >= sizeof(this->fdata));
  memcpy(dest, &this->fdata, sizeof(this->fdata));

  // Copy the laser timestamp
  *timestamp_sec = this->laser->data_timestamp_sec;
  *timestamp_usec = this->laser->data_timestamp_usec;
  
  return (sizeof(this->fdata));
}


////////////////////////////////////////////////////////////////////////////////
// Put configuration in buffer (called by server thread)
int LaserReflector::PutConfig(player_device_id_t* device, void *client, void *data, size_t len) 
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
void LaserReflector::Find()
{
  int i;
  int h;
  double r, b;
  double mn, mr, mb, mrr, mbb;
  double pr, pb, po;
  double ur, ub, uo;

  // Empty the fiducial list.
  this->fdata.count = 0;
  
  // Initialise patch statistics.
  mn = 0.0;
  mr = 0.0;
  mb = 0.0;
  mrr = 0.0;
  mbb = 0.0;
    
  // Look for a candidate patch in scan.
  for (i = 0; i < this->ldata.range_count; i++)
  {
    r = (double) (this->ldata.ranges[i]) / 1000;
    b = (double) (this->ldata.min_angle + i * this->ldata.resolution) / 100.0 * M_PI / 180;
    h = (int) (this->ldata.intensity[i]);

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
        this->FitCircle(i - (int) mn, i - 1, &pr, &pb, &po, &ur, &ub, &uo);

        // Fill in the fiducial data structure.
        this->Add(pr, pb, po, ur, ub, uo);
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
  
  //printf("Testing moments %.0f %f %f %f %f\n", mn, mr, mb, mrr, mbb);

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
// bearing, orientation).  This one works for cylindrical fiducials.
void LaserReflector::FitCircle(int first, int last,
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
    r = (double) (this->ldata.ranges[i]) / 1000;
    b = (double) (this->ldata.min_angle + i * this->ldata.resolution) / 100.0 * M_PI / 180;

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
  *ub = this->ldata.resolution / 100.0 * M_PI / 180;
  *uo = 1e6;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Add a item into the fiducial list.
void LaserReflector::Add(double pr, double pb, double po,
                         double ur, double ub, double uo)

{
  player_fiducial_item_t *fiducial;

  //printf("adding %f %f %f\n", pr, pb, po);
  
  assert(this->fdata.count < ARRAYSIZE(this->fdata.fiducials));
  fiducial = this->fdata.fiducials + this->fdata.count++;
  fiducial->id = (int16_t) -1;
  fiducial->pose[0] = (int16_t) (int) (pr * 1000);
  fiducial->pose[1] = (int16_t) (int) (pb * 180 / M_PI);
  fiducial->pose[2] = (int16_t) (int) (po * 180 / M_PI);

  return;
}


