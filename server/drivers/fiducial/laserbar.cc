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
// Parses a laser scan to find retro-reflective markers.  Currently only
// cylindrical markers are supported.
//
///////////////////////////////////////////////////////////////////////////

#include "player.h"

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>

#define PLAYER_ENABLE_TRACE 0
#define PLAYER_ENABLE_MSG 0

#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"


// Driver for detecting laser retro-reflectors.
class LaserBar : public Driver
{
  // Constructor
  public: LaserBar( ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Data configuration.
  public: virtual size_t GetData(player_device_id_t id,
                                 void* dest, size_t len,
                                 struct timeval* timestamp);
  public: virtual int PutConfig(player_device_id_t id, void *client, 
                                void *src, size_t len,
                                struct timeval* timestamp);

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *request, int len);

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
  
  // Pointer to laser to get data from.
  private: int laser_index;
  private: player_device_id_t laser_id;
  private: Driver *laser_driver;

  // Reflector properties.
  private: double reflector_width;
  private: double reflector_tol;
  
  // Local copy of the current laser data.
  private: player_laser_data_t ldata;

  // Local copy of the current fiducial data.
  private: struct timeval ftimestamp;
  private: player_fiducial_data_t fdata;
};


// Initialization function
Driver* LaserBar_Init( ConfigFile* cf, int section)
{
  return ((Driver*) (new LaserBar( cf, section)));
}


// a driver registration function
void LaserBar_Register(DriverTable* table)
{
  table->AddDriver("laserbar", LaserBar_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserBar::LaserBar( ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_FIDUCIAL_CODE, PLAYER_READ_MODE,
             0, 0, 0, 1)
{
  // The default laser device to use
  this->laser_index = cf->ReadInt(section, "laser_index", 0);

  // Default reflector properties.
  this->reflector_width = cf->ReadLength(section, "width", 0.08);
  this->reflector_tol = cf->ReadLength(section, "tol", 0.50);
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int LaserBar::Setup()
{
  // Get the pointer to the laser.  If index was not overridden by an
  // argument in the constructor, then we use this driver's index.
  this->laser_id.port = this->device_id.port;
  this->laser_id.code = PLAYER_LASER_CODE;
  this->laser_id.index = this->laser_index;
  
  if (!(this->laser_driver = deviceTable->GetDriver(this->laser_id)))
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return(-1);
  }
    
  // Subscribe to the laser device, but fail if it fails
  if (this->laser_driver->Subscribe(this->laser_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return(-1);
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int LaserBar::Shutdown()
{
  // Unsubscribe from the laser device
  this->laser_driver->Unsubscribe(this->laser_id);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by server thread).
size_t 
LaserBar::GetData(player_device_id_t id,
                  void* dest, size_t len,
                  struct timeval* timestamp)
{
  int i;
  size_t laser_size;
  struct timeval laser_timestamp;
  
  // Get the laser data.
  laser_size = this->laser_driver->GetData(this->laser_id, 
                                           (void*) &this->ldata, 
                                           sizeof(this->ldata),
                                           &laser_timestamp);
  assert(laser_size <= sizeof(this->ldata));
  
  // If the laser doesnt have new data, just return a copy of our old
  // data.
  if ((laser_timestamp.tv_sec != this->ftimestamp.tv_sec) || 
      (laser_timestamp.tv_usec != this->ftimestamp.tv_usec))
  {
    // Do some byte swapping
    this->ldata.resolution = ntohs(this->ldata.resolution);
    this->ldata.range_res = ntohs(this->ldata.range_res);
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
      this->fdata.fiducials[i].pos[0] = htonl(this->fdata.fiducials[i].pos[0]);
      this->fdata.fiducials[i].pos[1] = htonl(this->fdata.fiducials[i].pos[1]);
      this->fdata.fiducials[i].rot[2] = htonl(this->fdata.fiducials[i].rot[2]);
    }
    this->fdata.count = htons(this->fdata.count);
  }

  // Copy results
  assert(len >= sizeof(this->fdata));
  memcpy(dest, &this->fdata, sizeof(this->fdata));

  // Copy the laser timestamp
  *timestamp = laser_timestamp;

  this->ftimestamp = laser_timestamp;
  
  return (sizeof(this->fdata));
}


////////////////////////////////////////////////////////////////////////////////
// Put configuration in buffer (called by server thread)
int 
LaserBar::PutConfig(player_device_id_t id, void *client, 
                    void *src, size_t len,
                    struct timeval* timestamp)
{
 int subtype;

  if (len < 1)
  {
    PLAYER_ERROR("empty requestion; ignoring");
    return 0;
  }
  
  subtype = ((uint8_t*) src)[0];
  switch (subtype)
  {
    case PLAYER_FIDUCIAL_GET_GEOM:
    {
      HandleGetGeom(client, src, len);
      break;
    }
    default:
    {
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
  }
  
  return (0);
}


////////////////////////////////////////////////////////////////////////////////
// Handle geometry requests.
void LaserBar::HandleGetGeom(void *client, void *request, int len)
{
  unsigned short reptype;
  struct timeval ts;
  int replen;
  player_laser_geom_t lgeom;
  player_fiducial_geom_t fgeom;
    
  // Get the geometry from the laser
  replen = this->laser_driver->Request(this->laser_id, this, 
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
  fgeom.fiducial_size[0] = ntohs((int) (this->reflector_width * 1000));
  fgeom.fiducial_size[1] = ntohs((int) (this->reflector_width * 1000));
    
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &fgeom, sizeof(fgeom),&ts) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Analyze the laser data to find reflectors.
void LaserBar::Find()
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
    r = (double) (this->ldata.ranges[i] * this->ldata.range_res) / 1000;
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
bool LaserBar::TestMoments(double mn, double mr, double mb, double mrr, double mbb)
{
  double dr, db;
  
  //printf("Testing moments %.0f %f %f %f %f\n", mn, mr, mb, mrr, mbb);

  if (mn < 2.0)
    return false;

  // These are tests for a cylindrical reflector.
  dr = (1 + this->reflector_tol) * this->reflector_width / 2;
  db = (1 + this->reflector_tol) * atan2(this->reflector_width / 2, mr);
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
void LaserBar::FitCircle(int first, int last,
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
    r = (double) (this->ldata.ranges[i] * this->ldata.range_res) / 1000;
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
void LaserBar::Add(double pr, double pb, double po,
                         double ur, double ub, double uo)

{
  player_fiducial_item_t *fiducial;

  //printf("adding %f %f %f\n", pr, pb, po);
  
  assert(this->fdata.count < ARRAYSIZE(this->fdata.fiducials));
  fiducial = this->fdata.fiducials + this->fdata.count++;
  fiducial->id = (int16_t) -1;
  fiducial->pos[0] = (int32_t) (int) (pr * cos(pb) * 1000);
  fiducial->pos[1] = (int32_t) (int) (pr * sin(pb) * 1000);
  fiducial->rot[2] = (int32_t) (int) (po * 1000);

  return;
}


