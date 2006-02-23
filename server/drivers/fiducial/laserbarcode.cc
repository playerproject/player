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

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_laserbarcode laserbarcode
 * @brief Laser barcode detector

@todo This driver is currently disabled because it needs to be updated to
the Player 2.0 API.

The laser barcode detector searches for specially constructed barcodes in
the laser range finder data.  An example laser barcode is shown below.
The barcode is constructed using strips of retro-reflective paper.
Each retro-reflective strip represents a `1' bit; each non-reflective
strip represents a `0' bit.  By default, the laserbarcode driver
searches for barcodes containing 8 bits, each of which is exactly 50mm
wide (the total barcode width is thus 400mm).  The first and last bits
are used as start and end markers, and the remaining bits are used to
determine the identity of the barcode; with an 8-bit barcode there are
64 unique IDs.  The number of bits and the width of each bit can be set
in the configuration file.

The range at which barcodes can be detected identified is dependent on the
bit width and the angular resolution of the laser.  With 50mm bits and an
angular resolution of 0.5 deg, barcodes can be detected and identified
at a range of about 2.5m.  With the laser resolution set to  0.25 deg,
this distance is roughly doubled to about 5m.

See also the @ref driver_laserbar and
@ref driver_laservisualbarcode drivers.

@image html beacon.jpg "A sample laser barcode.  This barcode has 8 bits, each of which is 50mm wide."

@par Compile-time dependencies

- none

@par Provides

- This driver provides detected target information through a @ref
  interface_fiducial device.

@par Requires

- This driver finds targets in scans from a @ref interface_laser
  device.

@par Configuration requests

- PLAYER_FIDUCIAL_GET_GEOM

@par Configuration file options

- bit_count (integer)
  - Default: 8
  - Number of bits in each barcode.
- bit_width (length)
  - Default: 0.05 m
  - Width of each bit.
- max_depth (length)
  - Default: 0.05 m
  - Maximum variance in the flatness of the beacon.
- accept_thresh (float)
  - Default: 1.0
  - Acceptance threshold
- zero_thresh (float)
  - Default: 0.6
  - Zero threshold
- one_thresh (float)
  - Default: 0.6
  - One threshold

@par Example

@verbatim
driver
(
  name "laserbarcode"
  requires ["laser:0"]
  provides ["fiducial:0"]
  bit_count 5
  bit_width 0.1
)
@endverbatim

@author Andrew Howard
*/
/** @} */

#define PLAYER_ENABLE_TRACE 0
#define PLAYER_ENABLE_MSG 0

#include "player.h"

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>  // for atoi(3)
#include <netinet/in.h>  /* for htons(3) */
#include <unistd.h>

#include "error.h"
#include "drivertable.h"
#include "devicetable.h"
#include "driver.h"
#include "clientdata.h"
#include "clientmanager.h"

extern ClientManager* clientmanager;

// The laser barcode detector.
class LaserBarcode : public Driver
{
  // Constructor
  public: LaserBarcode( ConfigFile* cf, int section);

  // Setup/shutdown routines
  //
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Process incoming messages from clients 
  int ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, size_t * resp_len);

  // Main function for device thread
  private: virtual void Main(void);
  
  // Get the laser data
  //private: int ReadLaser();

  // Analyze the laser data and return beacon data
  private: void FindBeacons(const player_laser_data_t *laser_data,
                            player_fiducial_data_t *beacon_data);

  // Analyze the candidate beacon and return its id (0 == none)
  private: int IdentBeacon(int a, int b, double ox, double oy, double oth,
                           const player_laser_data_t *laser_data);

  // Write fidicual data 
  private: void WriteFiducial();

  // Process configuration requests
  //private: int HandleConfig();

  // Handle geometry requests.
  //private: void HandleGetGeom(void *client, void *request, size_t len);

  // Pointer to laser to get data from
  private: player_device_id_t laser_id;
  private: Driver *laser_driver;
  
  // Magic numbers
  private: int bit_count;
  private: double bit_width;
  private: double max_depth;
  private: double accept_thresh, zero_thresh, one_thresh;

  // Current laser data
  private: player_laser_data_t laser_data;
  private: struct timeval laser_timestamp;
  
  // Current fiducial data
  private: player_fiducial_data_t data;
};


// Initialization function
Driver* LaserBarcode_Init( ConfigFile* cf, int section)
{
  return((Driver*)(new LaserBarcode( cf, section)));
}

// a driver registration function
void LaserBarcode_Register(DriverTable* table)
{
  table->AddDriver("laserbarcode", LaserBarcode_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserBarcode::LaserBarcode( ConfigFile* cf, int section)
  : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_FIDUCIAL_CODE, PLAYER_READ_MODE)
{
  // Must have an input laser
  if (cf->ReadDeviceId(&this->laser_id, section, "requires",
                       PLAYER_LASER_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);    
    return;
  }

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
  BaseClient = new ClientDataInternal(this);
  clientmanager->AddClient(BaseClient);

  this->laser_driver = SubscribeInternal(laser_id);
  if(!this->laser_driver)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return(-1);
  }
    
  // Start our own thread
  this->StartThread();
  
  PLAYER_MSG2(2, "laserbarcode device: bitcount [%d] bitwidth [%fm]",
              this->bit_count, this->bit_width);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int LaserBarcode::Shutdown()
{
  // Stop the driver thread.
  this->StopThread();

  // Unsubscribe from devices.
  UnsubscribeInternal(this->laser_id);

  PLAYER_MSG0(2, "laserbarcode device: shutdown");
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Device thread
void LaserBarcode::Main(void)
{
  while (true)
  {
    // Wait for new data from the laser
    this->laser_driver->Wait();

	ProcessMessages();
	
	pthread_testcancel();

    // Read the new laser data
    //this->ReadLaser();
    
    // Analyse the laser data
    //this->FindBeacons(&this->laser_data, &this->data);

    // Write out the fiducials
    //this->WriteFiducial();

    // Process any pending requests
    //this->HandleConfig();
  }
  
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int LaserBarcode::ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, size_t * resp_len)
{
  assert(hdr);
  assert(data);
  assert(resp_data);
  assert(resp_len);
  assert(*resp_len==PLAYER_MAX_MESSAGE_SIZE);
  
  if (MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 0, laser_id))
  {
  	assert(hdr->size == sizeof(player_laser_data_t));
  	player_laser_data_t * l_data = reinterpret_cast<player_laser_data_t * > (data);
  	Lock();

    // Do some byte swapping
    this->laser_data.resolution = ntohs(l_data->resolution);
    this->laser_data.range_res = ntohs(l_data->range_res);
    this->laser_data.min_angle = ntohs(l_data->min_angle);
    this->laser_data.max_angle = ntohs(l_data->max_angle);
    this->laser_data.range_count = ntohs(l_data->range_count);
    for (int i = 0; i < this->laser_data.range_count; i++)
      this->laser_data.ranges[i] = ntohs(l_data->ranges[i]);

    // Analyse the laser data
    this->FindBeacons(&this->laser_data, &this->data);

    // Write out the fiducials
    this->WriteFiducial();

  	Unlock();
    *resp_len = 0;
  	return 0;
  }
 
  if (MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_FIDUCIAL_GET_GEOM, device_id))
  {
    hdr->device_index = laser_id.index;
    hdr->subtype = PLAYER_LASER_GET_GEOM;
    int ret = laser_driver->ProcessMessage(BaseClient, hdr, data, resp_data, resp_len);
    hdr->subtype = PLAYER_FIDUCIAL_GET_GEOM;
    hdr->device_index = device_id.index;
    
  	assert(*resp_len == sizeof(player_laser_geom_t));
  	player_laser_geom_t lgeom = * reinterpret_cast<player_laser_geom_t * > (resp_data);
  	player_fiducial_geom_t * fgeom = reinterpret_cast<player_fiducial_geom_t * > (resp_data);

    fgeom->pose[0] = lgeom.pose[0];
    fgeom->pose[1] = lgeom.pose[1];
    fgeom->pose[2] = lgeom.pose[2];
    fgeom->size[0] = lgeom.size[0];
    fgeom->size[1] = lgeom.size[1];
    fgeom->fiducial_size[0] = ntohs((int) (0.05 * 1000));
    fgeom->fiducial_size[1] = ntohs((int) (this->bit_count * this->bit_width * 1000));
  
  	*resp_len=sizeof(player_fiducial_geom_t);
  
    return ret;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Get the laser data
/*int LaserBarcode::ReadLaser()
{
  size_t size;
  
  // Get the laser data.
  size = this->laser_driver->GetData(this->laser_id, 
                                     (uint8_t*)&this->laser_data, 
                                     sizeof(this->laser_data),
                                     &this->laser_timestamp);
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
}*/


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
    data->fiducials[data->count].pos[0] = (int) (range * cos(bearing) * 1000);
    data->fiducials[data->count].pos[1] = (int) (range * sin(bearing) * 1000);
    data->fiducials[data->count].rot[2] = (int) (NORMALIZE(orient + M_PI/2) * 1000);
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
    this->data.fiducials[i].pos[0] = htonl(this->data.fiducials[i].pos[0]);
    this->data.fiducials[i].pos[1] = htonl(this->data.fiducials[i].pos[1]);
    this->data.fiducials[i].rot[2] = htonl(this->data.fiducials[i].rot[2]);
  }
  this->data.count = htons(this->data.count);

  // Write the data with the laser timestamp
  this->PutMsg(device_id, NULL, PLAYER_MSGTYPE_DATA, 0, &this->data, sizeof(this->data), &this->laser_timestamp);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process configuration requests
/*int LaserBarcode::HandleConfig() 
{
  int subtype;
  size_t len;
  void *client;
  char req[PLAYER_MAX_REQREP_SIZE];

  while ((len = this->GetConfig(&client, req, sizeof(req),NULL)) > 0)
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
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
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
  fgeom.fiducial_size[0] = ntohs((int) (0.05 * 1000));
  fgeom.fiducial_size[1] = ntohs((int) (this->bit_count * this->bit_width * 1000));
    
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &fgeom, sizeof(fgeom), &ts) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}
*/
