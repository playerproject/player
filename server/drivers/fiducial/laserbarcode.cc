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
// Desc: Driver for detecting laser barcodes.
// Author: Andrew Howard
// Date: 29 Jan 2001
// CVS: $Id$
//
// Theory of operation:
//  Will detect binary coded beacons (i.e. bar-codes) in laser data.
//  Reflectors represent '1' bits, non-reflectors represent '0' bits.
//  The first and last bits of the beacon must be '1'.
//
// Requires: laser
// Provides: fiducial
//
///////////////////////////////////////////////////////////////////////////

#define PLAYER_ENABLE_TRACE 0
#define PLAYER_ENABLE_MSG 0

#include "player.h"

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>  // for atoi(3)
#include <netinet/in.h>  /* for htons(3) */
#include <unistd.h>

#include "drivertable.h"
#include "devicetable.h"
#include "device.h"


// The laser barcode detector.
class LaserBarcode : public CDevice
{
  // Constructor
  public: LaserBarcode(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines
  //
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Main function for device thread
  private: virtual void Main(void);
  
  // Get the laser data
  private: int ReadLaser();

  // Analyze the laser data and return beacon data
  private: void FindBeacons(const player_laser_data_t *laser_data,
                            player_fiducial_data_t *beacon_data);

  // Analyze the candidate beacon and return its id (0 == none)
  private: int IdentBeacon(int a, int b, double ox, double oy, double oth,
                           const player_laser_data_t *laser_data);

  // Write fidicual data 
  private: void WriteFiducial();

  // Process configuration requests
  private: int HandleConfig();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *request, size_t len);

  // Pointer to laser to get data from
  private: int laser_index;
  private: CDevice *laser_device;
  
  // Magic numbers
  private: int bit_count;
  private: double bit_width;
  private: double max_depth;
  private: double accept_thresh, zero_thresh, one_thresh;

  // Current laser data
  private: player_laser_data_t laser_data;
  private: uint32_t laser_tsec, laser_tusec;
  
  // Current fiducial data
  private: player_fiducial_data_t data;
};


// Initialization function
CDevice* LaserBarcode_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_FIDUCIAL_STRING))
  {
    PLAYER_ERROR1("driver \"laserbarcode\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new LaserBarcode(interface, cf, section)));
}

// a driver registration function
void LaserBarcode_Register(DriverTable* table)
{
  table->AddDriver("laserbarcode", PLAYER_READ_MODE, LaserBarcode_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserBarcode::LaserBarcode(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_fiducial_data_t), 0, 10, 10)
{
  // The default laser device to use
  this->laser_index = cf->ReadInt(section, "laser", 0);

  // Get beacon settings.
  this->bit_count = cf->ReadInt(section, "bit_count", 8);
  this->bit_width = cf->ReadLength(section, "bit_width", 0.05);
  
  // Maximum variance in the flatness of the beacon
  this->max_depth = cf->ReadLength(section, "max_depth", 0.05);

  // Default thresholds
  this->accept_thresh = cf->ReadFloat(section, "accept_thresh", 1.0);
  this->zero_thresh = cf->ReadFloat(section, "zero_thresh", 0.60);
  this->one_thresh = cf->ReadFloat(section, "one_thresh", 0.60);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
int LaserBarcode::Setup()
{
  // get the pointer to the laser
  player_device_id_t id;
  id.port = device_id.port;
  id.code = PLAYER_LASER_CODE;
  id.index = this->laser_index;
  this->laser_device = deviceTable->GetDevice(id);
  if(!this->laser_device)
  {
    PLAYER_ERROR("unable to find laser device");
    return(-1);
  }
    
  // Subscribe to the laser device, but fail if it fails
  if(this->laser_device->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return(-1);
  }

  // Start our own thread
  this->StartThread();
  
  PLAYER_MSG2("laserbarcode device: bitcount [%d] bitwidth [%fm]",
              this->bit_count, this->bit_width);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int LaserBarcode::Shutdown()
{
  // Stop the driver thread.
  this->StopThread();

  // Unsubscribe from the laser device
  this->laser_device->Unsubscribe(this);

  PLAYER_MSG0("laserbarcode device: shutdown");
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Device thread
void LaserBarcode::Main(void)
{
  while (true)
  {
    // Wait for new data from the laser
    this->laser_device->Wait();

    // Read the new laser data
    this->ReadLaser();
    
    // Analyse the laser data
    this->FindBeacons(&this->laser_data, &this->data);

    // Write out the fiducials
    this->WriteFiducial();

    // Process any pending requests
    this->HandleConfig();
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Get the laser data
int LaserBarcode::ReadLaser()
{
  size_t size;
  
  // Get the laser data.
  size = this->laser_device->GetData(this, (uint8_t*) &this->laser_data, sizeof(this->laser_data),
                                     &this->laser_tsec, &this->laser_tusec);
  assert(size <= sizeof(this->laser_data));
  
  // Do some byte swapping
  this->laser_data.resolution = ntohs(this->laser_data.resolution);
  this->laser_data.range_res = ntohs(this->laser_data.range_res);
  this->laser_data.min_angle = ntohs(this->laser_data.min_angle);
  this->laser_data.max_angle = ntohs(this->laser_data.max_angle);
  this->laser_data.range_count = ntohs(this->laser_data.range_count);
  for (int i = 0; i < this->laser_data.range_count; i++)
    this->laser_data.ranges[i] = ntohs(this->laser_data.ranges[i]);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Analyze the laser data and return beacon data
void LaserBarcode::FindBeacons(const player_laser_data_t *laser_data,
                               player_fiducial_data_t *data)
{
  data->count = 0;

  int ai = -1;
  int bi = -1;
  double ax, ay;
  double bx, by;

  // Expected width of beacon
  //
  double min_width = (this->bit_count - 1) * this->bit_width;
  double max_width = (this->bit_count + 1) * this->bit_width;

  ax = ay = 0;
  bx = by = 0;
    
  // Find the beacons in this scan
  for (int i = 0; i < laser_data->range_count; i++)
  {
    double range = (double) (laser_data->ranges[i] * laser_data->range_res) / 1000;
    double bearing = (double) (laser_data->min_angle + i * laser_data->resolution)
      / 100.0 * M_PI / 180;
    int intensity = (int) (laser_data->intensity[i]);

    double px = range * cos(bearing);
    double py = range * sin(bearing);
        
    if (intensity > 0)
    {
      if (ai < 0)
      {
        ai = i;
        ax = px;
        ay = py;
      }
      bi = i;
      bx = px;
      by = py;
    }
    if (ai < 0)
      continue;

    double width = sqrt((px - ax) * (px - ax) + (py - ay) * (py - ay));
    if (width < max_width)
      continue;

    width = sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay));
    if (width < min_width)
      continue;
    if (width > max_width)
    {
      ai = -1;
      continue;
    }

    // Assign an id to the beacon
    double orient = atan2(by - ay, bx - ax);
    int id = IdentBeacon(ai, bi, ax, ay, orient, laser_data);

    // Reset counters so we can find new beacons
    //
    ai = -1;
    bi = -1;

    // Ignore invalid ids
    //
    if (id < 0)
      continue;
                
    // Check for array overflow.
    if (data->count >= ARRAYSIZE(data->fiducials))
      continue;
    
    double ox = (bx + ax) / 2;
    double oy = (by + ay) / 2;
    range = sqrt(ox * ox + oy * oy);
    bearing = atan2(oy, ox);

    // Create an entry for this beacon.
    // Note that we return the surface normal for the beacon orientation.
    data->fiducials[data->count].id = (id > 0 ? id : -1);
    data->fiducials[data->count].pose[0] = (int) (range * 1000);
    data->fiducials[data->count].pose[1] = (int) (bearing * 180 / M_PI);
    data->fiducials[data->count].pose[2] = (int) (NORMALIZE(orient + M_PI/2) * 180 / M_PI);
    data->count++;
  }
}


////////////////////////////////////////////////////////////////////////////////
// Analyze the candidate beacon and return its id.
// Will return -1 if this is not a beacon.
// Will return  0 if this is a beacon, but it cannot be identified.
// Will return beacon id otherwise.
int LaserBarcode::IdentBeacon(int a, int b, double ox, double oy, double oth,
                              const player_laser_data_t *laser_data)
{
  // Compute pose of laser relative to beacon
  double lx = -ox * cos(-oth) + oy * sin(-oth);
  double ly = -ox * sin(-oth) - oy * cos(-oth);
  double la = -oth;

  // Initialise our probability distribution.
  // We determine the probability that each bit is set using Bayes law.
  double prob[8][2];
  for (int bit = 0; bit < ARRAYSIZE(prob); bit++)
    prob[bit][0] = prob[bit][1] = 0;

  // Scan through the readings that make up the candidate.
  for (int i = a; i <= b; i++)
  {
    double range = (double) (laser_data->ranges[i] * laser_data->range_res) / 1000;
    double bearing = (double) (laser_data->min_angle + i * laser_data->resolution)
      / 100.0 * M_PI / 180;
    int intensity = (int) (laser_data->intensity[i]);
    double res = (double) laser_data->resolution / 100.0 * M_PI / 180;

    // Compute point relative to beacon
    double py = ly + range * sin(la + bearing);

    // Discard candidate points are not close to x-axis (ie candidate
    // is not flat).
    if (fabs(py) > this->max_depth)
      return -1;

    // Compute intercept with beacon
    //double cx = lx + ly * tan(la + bearing + M_PI/2);
    double ax = lx + ly * tan(la + bearing - res/2 + M_PI/2);
    double bx = lx + ly * tan(la + bearing + res/2 + M_PI/2);

    // Update our probability distribution
    // Use Bayes law
    for (int bit = 0; bit < this->bit_count; bit++)
    {
      // Use a rectangular distribution
      double a = (bit + 0.0) * this->bit_width;
      double b = (bit + 1.0) * this->bit_width;

      double p = 0;
      if (ax < a && bx > b)
        p = 1;
      else if (ax > a && bx < b)
        p = 1;
      else if (ax > b || bx < a)
        p = 0;
      else if (ax < a && bx < b)
        p = 1 - (a - ax) / (bx - ax);
      else if (ax > a && bx > b)
        p = 1 - (bx - b) / (bx - ax);
      else
        assert(0);

      //printf("prob : %f %f %f %f %f\n", ax, bx, a, b, p);
            
      if (intensity == 0)
      {
        assert(bit >= 0 && bit < ARRAYSIZE(prob));            
        prob[bit][0] += p;
        prob[bit][1] += 0;
      }
      else
      {
        assert(bit >= 0 && bit < ARRAYSIZE(prob));            
        prob[bit][0] += 0;
        prob[bit][1] += p;
      }
    }
  }

  // Now assign the id
  int id = 0;
  for (int bit = 0; bit < this->bit_count; bit++)
  {
    double pn = prob[bit][0] + prob[bit][1];
    double p0 = prob[bit][0] / pn;
    double p1 = prob[bit][1] / pn;
       
    if (pn < this->accept_thresh)
      id = -1;
    else if (p0 > this->zero_thresh)
      id |= (0 << bit);
    else if (p1 > this->one_thresh)
      id |= (1 << bit);
    else
      id = -1;

    //printf("%d %f %f : %f %f %f\n", bit, prob[bit][0], prob[bit][1], p0, p1, pn);
  }
  //printf("\n");

  if (id < 0)
    id = 0;
    
  return id;
}


////////////////////////////////////////////////////////////////////////////////
// Write fidicual data 
void LaserBarcode::WriteFiducial()
{
  int i;
  
  // Do some byte-swapping
  for (i = 0; i < this->data.count; i++)
  {
    this->data.fiducials[i].id = htons(this->data.fiducials[i].id);
    this->data.fiducials[i].pose[0] = htons(this->data.fiducials[i].pose[0]);
    this->data.fiducials[i].pose[1] = htons(this->data.fiducials[i].pose[1]);
    this->data.fiducials[i].pose[2] = htons(this->data.fiducials[i].pose[2]);
  }
  this->data.count = htons(this->data.count);

  // Write the data with the laser timestamp
  this->PutData(&this->data, sizeof(this->data), this->laser_tsec, this->laser_tusec);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process configuration requests
int LaserBarcode::HandleConfig() 
{
  int subtype;
  size_t len;
  void *client;
  char req[PLAYER_MAX_REQREP_SIZE];

  while ((len = this->GetConfig(&client, req, sizeof(req))) > 0)
  {
    subtype = ((uint8_t*) req)[0];
    
    switch (subtype)
    {
      case PLAYER_FIDUCIAL_GET_GEOM:
      {
        HandleGetGeom(client, req, len);
        break;
      }
      default:
      {
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
    }
  }
    
  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Handle geometry requests.
void LaserBarcode::HandleGetGeom(void *client, void *request, size_t len)
{
  unsigned short reptype;
  struct timeval ts;
  int replen;
  player_laser_geom_t lgeom;
  player_fiducial_geom_t fgeom;
    
  // Get the geometry from the laser
  replen = this->laser_device->Request(&this->laser_device->device_id, this, request, len,
                                       &reptype, &ts, &lgeom, sizeof(lgeom));
  if (replen <= 0 || replen != sizeof(lgeom))
  {
    PLAYER_ERROR("unable to get geometry from laser device");
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  fgeom.pose[0] = lgeom.pose[0];
  fgeom.pose[1] = lgeom.pose[1];
  fgeom.pose[2] = lgeom.pose[2];
  fgeom.size[0] = lgeom.size[0];
  fgeom.size[1] = lgeom.size[1];
  fgeom.fiducial_size[0] = ntohs((int) (0.05 * 1000));
  fgeom.fiducial_size[1] = ntohs((int) (this->bit_count * this->bit_width * 1000));
    
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &ts, &fgeom, sizeof(fgeom)) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}

