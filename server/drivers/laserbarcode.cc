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

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>  // for atoi(3)
#include <netinet/in.h>  /* for htons(3) */
#include <unistd.h>

#include "drivertable.h"
#include "devicetable.h"
#include "device.h"
#include "player.h"


// The laser barcode detector.
class LaserBarcode : public CDevice
{
  // Constructor
  public: LaserBarcode(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines
  //
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Client interface
  public: virtual size_t GetData(unsigned char *, size_t maxsize,
                                 uint32_t* timestamp_sec,
                                 uint32_t* timestamp_usec);
  public: virtual int PutConfig(player_device_id_t* device, void *client, 
                                void *data, size_t len);

  // Analyze the laser data and return beacon data
  private: void FindBeacons(const player_laser_data_t *laser_data,
                            player_fiducial_data_t *beacon_data);

  // Analyze the candidate beacon and return its id (0 == none)
  private: int IdentBeacon(int a, int b, double ox, double oy, double oth,
                           const player_laser_data_t *laser_data);

  // Pointer to laser to get data from
  private: int index;
  private: CDevice *laser;

  // Defaults
  private: int default_bitcount;
  private: double default_bitwidth;
  
  // Magic numbers
  // See setup for definitions
  private: int max_bits;
  private: double bit_width;
  private: double max_depth;
  private: double accept_thresh, zero_thresh, one_thresh;

  // The current data
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
void 
LaserBarcode_Register(DriverTable* table)
{
  table->AddDriver("laserbarcode", PLAYER_READ_MODE, LaserBarcode_Init);
}

extern int global_playerport; // used to get at devices

////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserBarcode::LaserBarcode(char* interface, ConfigFile* cf, int section) :
  CDevice(0,0,0,1)
{
  // if index is not overridden by an argument here, then we'll use the
  // device's own index, which we can get in Setup() below.
  this->index = cf->ReadInt(section, "index", -1);

  // Get beacon settings.
  this->default_bitcount = cf->ReadInt(section, "bitcount", 8);
  this->default_bitwidth = cf->ReadLength(section, "bitwidth", 0.05);

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
  // if index was not overridden by an argument in the constructor, then we
  // use the device's own index
  if(this->index >= 0)
    id.index = index;
  else
    id.index = device_id.index;
  printf("LaserBarcode:Setup(%d:%d:%d)\n", id.code,id.index,id.port);
  if(!(this->laser = deviceTable->GetDevice(id)))
  {
    fputs("LaserBarcode:Setup(): couldn't find laser device\n",stderr);
    return(-1);
  }
    
  // Subscribe to the laser device, but fail if it fails
  if(this->laser->Subscribe(this) != 0)
  {
    fputs("LaserBarcode:Setup(): couldn't setup laser device\n",stderr);
    return(-1);
  }

  // Maximum variance in the flatness of the beacon
  this->max_depth = 0.05;

  // Number of bits and size of each bit
  this->max_bits = this->default_bitcount;
  this->bit_width = this->default_bitwidth;

  // Default thresholds
  this->accept_thresh = 1.0;
  this->zero_thresh = 0.60;
  this->one_thresh = 0.60;
    
  PLAYER_MSG2("laser beacon device: bitcount [%d] bitwidth [%fm]",
              this->max_bits, this->bit_width);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
//
int LaserBarcode::Shutdown()
{
  // Unsubscribe from the laser device
  this->laser->Unsubscribe(this);

  PLAYER_MSG0("laser beacon device: shutdown");
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by client thread)
//
size_t LaserBarcode::GetData(unsigned char *dest, size_t maxsize,
                                   uint32_t* timestamp_sec,
                                   uint32_t* timestamp_usec)
{
  Lock();

  // If the laser doesnt have new data,
  // just return a copy of our old data.
  if (this->laser->data_timestamp_sec == this->data_timestamp_sec &&
      this->laser->data_timestamp_usec == this->data_timestamp_usec)
  {
    ASSERT(maxsize >= sizeof(this->data));
    memcpy(dest, &this->data, sizeof(this->data));
    Unlock();
    return(sizeof(this->data));
  }

  // Get the laser data
  player_laser_data_t laser_data;
  this->laser->GetData((unsigned char*) &laser_data, sizeof(laser_data), NULL, NULL);

  // Do some byte swapping
  laser_data.resolution = ntohs(laser_data.resolution);
  laser_data.min_angle = ntohs(laser_data.min_angle);
  laser_data.max_angle = ntohs(laser_data.max_angle);
  laser_data.range_count = ntohs(laser_data.range_count);
  for (int i = 0; i < laser_data.range_count; i++)
    laser_data.ranges[i] = ntohs(laser_data.ranges[i]);

  // Analyse the laser data
  FindBeacons(&laser_data, &this->data);

  // Do some byte-swapping
  for (int i = 0; i < this->data.count; i++)
  {
    this->data.fiducials[i].id = htons(this->data.fiducials[i].id);
    this->data.fiducials[i].pose[0] = htons(this->data.fiducials[i].pose[0]);
    this->data.fiducials[i].pose[1] = htons(this->data.fiducials[i].pose[1]);
    this->data.fiducials[i].pose[2] = htons(this->data.fiducials[i].pose[2]);
  }
  PLAYER_TRACE1("setting beacon count: %u",this->data.count);
  this->data.count = htons(this->data.count);

  // Copy results
  ASSERT(maxsize >= sizeof(this->data));
  memcpy(dest, &this->data, sizeof(this->data));

  // Copy the laser timestamp
  *timestamp_sec = this->data_timestamp_sec = this->laser->data_timestamp_sec;
  *timestamp_usec = this->data_timestamp_usec  = this->laser->data_timestamp_usec;

  Unlock();

  return (sizeof(this->data));
}


////////////////////////////////////////////////////////////////////////////////
// Put configuration in buffer (called by client thread)
int LaserBarcode::PutConfig(player_device_id_t* device, void *client, 
                                  void *data, size_t len) 
{
  player_laserbarcode_config_t config;

  memcpy(&config, data, min(sizeof(config),len));

  switch (config.subtype)
  {
    case PLAYER_LASERBARCODE_SET_CONFIG:
    {
      // Check the message length
      if (len != sizeof(config))
      {
        PLAYER_ERROR2("config request len is invalid (%d != %d)", 
                      len, sizeof(config));
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        return 0;
      }
      Lock();
      this->max_bits = config.bit_count;
      this->max_bits = max(this->max_bits, 3);
      this->max_bits = min(this->max_bits, 8);
      this->bit_width = ntohs(config.bit_size) / 1000.0;
      this->zero_thresh = ntohs(config.zero_thresh) / 100.0;
      this->one_thresh = ntohs(config.one_thresh) / 100.0;
      Unlock();
      
      if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }

    case PLAYER_LASERBARCODE_GET_CONFIG:
    {
      // Check the message length
      if (len != sizeof(config.subtype))
      {
        PLAYER_ERROR2("config request len is invalid (%d != %d)", 
                      len, sizeof(config.subtype));
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        return 0;
      }
      Lock();
      config.bit_count = this->max_bits;
      config.bit_size = htons((short) (this->bit_width * 1000));
      config.one_thresh = htons((short) (this->one_thresh * 100));
      config.zero_thresh = htons((short) (this->zero_thresh * 100));
      Unlock();
      
      if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &config, 
                   sizeof(config)) != 0)
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
    
  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Analyze the laser data and return beacon data
//
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
  double min_width = (this->max_bits - 1) * this->bit_width;
  double max_width = (this->max_bits + 1) * this->bit_width;

  ax = ay = 0;
  bx = by = 0;
    
  // Find the beacons in this scan
  for (int i = 0; i < laser_data->range_count; i++)
  {
    double range = (double) (laser_data->ranges[i]) / 1000;
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
    double range = (double) (laser_data->ranges[i] & 0x1FFF) / 1000;
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
    for (int bit = 0; bit < this->max_bits; bit++)
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
  for (int bit = 0; bit < this->max_bits; bit++)
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

