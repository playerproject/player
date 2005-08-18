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

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_lasercspace lasercspace


The lasercspace driver processes a laser scan to compute the
configuration space (`C-space') boundary.  That is, it shortens the
range of each laser scan such that the resultant scan delimits the
obstacle-free portion of the robot's configuration space.  This driver
is particular useful for writing obstacle avoidance algorithms, since the
robot may safely move to any point in the obstacle-free portion of the 
configuration space.

Note that driver computes the configuration space for a robot of some
fixed radius; this radius may be set in the configuration file.

@image html lasercspace-1.jpg "Standard laser scan"
@image html lasercspace-2.jpg "Corresponding C-space scan for a robot of 0.5 m"

@par Compile-time dependencies

- none

@par Provides

- @ref player_interface_laser : output of the C-space scan

@par Requires

- @ref player_interface_laser : raw laser data from which to make C-space scan

@par Configuration requests

- PLAYER_LASER_REQ_GET_GEOM
  
@par Configuration file options

- radius (length)
  - Default: 0.5 m
  - Radius of robot for which to make C-space scan
- step (integer)
  - Default: 1
  - Step size for subsampling the scan (saves CPU cycles)
      
@par Example 

@verbatim
driver
(
  name "sicklms200"
  provides ["laser:0"]
  port "/dev/ttyS0"
)
driver
(
  name "lasercspace"
  requires ["laser:0"] # read from laser:0
  provides ["laser:1"] # output results on laser:1
  radius 0.5
)
@endverbatim

@par Authors

Andrew Howard

*/
/** @} */

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <unistd.h>

#include <libplayercore/playercore.h>

// Driver for computing the free c-space from a laser scan.
class LaserCSpace : public Driver
{
  // Constructor
  public: LaserCSpace( ConfigFile* cf, int section);

    // MessageHandler
  public: int ProcessMessage(MessageQueue * resp_queue, 
                              player_msghdr * hdr, 
                              void * data, 
                              void ** resp_data, 
                              size_t * resp_len);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Process laser data.  Returns non-zero if the laser data has been
  // updated.
  private: int UpdateLaser(player_laser_data_t * data);

  // Pre-compute a bunch of stuff
  private: void Precompute(player_laser_data_t* data);

  // Compute the maximum free-space range for sample n.
  private: double FreeRange(player_laser_data_t* data, int n);

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Laser stuff.
  private: Device *laser_device;
  private: player_devaddr_t laser_addr;
  private: struct timeval laser_timestamp;

  // Step size for subsampling the scan (saves CPU cycles)
  private: int sample_step;

  // Robot radius.
  private: double radius;

  // Lookup table for precomputations
  private: double lu[PLAYER_LASER_MAX_SAMPLES][4];

  // Fiducila stuff (the data we generate).
  private: player_laser_data_t data;
  private: struct timeval time;
};


// Initialization function
Driver* LaserCSpace_Init( ConfigFile* cf, int section)
{
  return ((Driver*) (new LaserCSpace( cf, section)));
}


// a driver registration function
void LaserCSpace_Register(DriverTable* table)
{
  table->AddDriver("lasercspace", LaserCSpace_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserCSpace::LaserCSpace( ConfigFile* cf, int section)
  : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_LASER_CODE)
{
  // Must have an input laser
  if (cf->ReadDeviceAddr(&this->laser_addr, section, "requires",
                         PLAYER_LASER_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);    
    return;
  }
  this->laser_device = NULL;
  this->laser_timestamp.tv_sec = 0;
  this->laser_timestamp.tv_usec = 0;

  // Settings.
  this->radius = cf->ReadLength(section, "radius", 0.50);
  this->sample_step = cf->ReadInt(section, "step", 1);
  
  // Outgoing data
  this->time.tv_sec = 0;
  this->time.tv_usec = 0;
  memset(&this->data, 0, sizeof(this->data));

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int LaserCSpace::Setup()
{
  // Subscribe to the laser.
  if(Device::MatchDeviceAddress(this->laser_addr, this->device_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return(-1);
  }
  if(!(this->laser_device = deviceTable->GetDevice(this->laser_addr)))
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return(-1);
  }
  if(this->laser_device->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return(-1);
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int LaserCSpace::Shutdown()
{
  // Unsubscribe from devices.
  this->laser_device->Unsubscribe(this->InQueue);
  
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int LaserCSpace::ProcessMessage(MessageQueue * resp_queue, 
                                player_msghdr * hdr, 
                                void * data, 
                                void ** resp_data, 
                                size_t * resp_len)
{
  *resp_len = 0;

  // Handle new data from the laser
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 0, this->laser_addr))
  {
    assert(hdr->size == sizeof(player_laser_data_t));
    player_laser_data_t * l_data = reinterpret_cast<player_laser_data_t * > (data);
    this->UpdateLaser(l_data);
    return(0);
  }

  // Forward geometry request to the laser
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_GEOM,
                           this->device_addr))
  {
    // Forward the message
    laser_device->PutMsg(this->InQueue, hdr, data);
    // Store the return address for later use
    this->ret_queue = resp_queue;
    // Set the message filter to look for the response
    this->InQueue->SetFilter(this->laser_addr.host,
                             this->laser_addr.robot,
                             this->laser_addr.interf,
                             this->laser_addr.index,
                             -1,
                             PLAYER_LASER_REQ_GET_GEOM);
    // No response now; it will come later after we hear back from the
    // laser
    return(0);
  }

  // Forward geometry response (success or failure) from the laser
  if((Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK,
                            PLAYER_LASER_REQ_GET_GEOM, this->laser_addr)) ||
     (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_NACK,
                            PLAYER_LASER_REQ_GET_GEOM, this->laser_addr)))
  {
    // Copy in our address and forward the response
    hdr->addr = this->device_addr;
    this->Publish(this->ret_queue, hdr, data);
    // Clear the filter
    this->InQueue->ClearFilter();
    // No response to send; we just sent it ourselves
    return(0);
  }

  return(-1);
}

////////////////////////////////////////////////////////////////////////////////
// Process laser data.
int LaserCSpace::UpdateLaser(player_laser_data_t * data)
{
  unsigned int i;
  
  // Construct the outgoing laser packet
  this->data.resolution = data->resolution;
  this->data.min_angle = data->min_angle;
  this->data.max_angle = data->max_angle;
  this->data.ranges_count = data->ranges_count;

  // Do some precomputations to save time
  this->Precompute(data);

  // Generate the range estimate for each bearing.
  for (i = 0; i < data->ranges_count; i++)
    this->data.ranges[i]  = this->FreeRange(data,i);

  this->Publish(this->device_addr, NULL, 
                PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN,
                (uint8_t *)&this->data, sizeof(this->data), NULL);

  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Pre-compute a bunch of stuff
void LaserCSpace::Precompute(player_laser_data_t* data)
{
  unsigned int i;
  double r, b, x, y;
  
  for (i = 0; i < data->ranges_count; i++)
  {
    r = data->ranges[i];
    b = data->min_angle + data->resolution * i;
    x = r * cos(b);
    y = r * sin(b);

    this->lu[i][0] = r;
    this->lu[i][1] = b;
    this->lu[i][2] = x;
    this->lu[i][3] = y;
  }
  return;
}
  

////////////////////////////////////////////////////////////////////////////////
// Compute the maximum free-space range for sample n.
double LaserCSpace::FreeRange(player_laser_data_t* data, int n)
{
  unsigned int i; 
  int step;
  double r, b, x, y;
  double r_, b_, x_, y_;
  double s, nr, nx, ny, dx, dy;
  double d, h;
  double max_r;

  // Step size for subsampling the scan (saves CPU cycles)
  step = this->sample_step;
  
  // Range and bearing of this reading.
  r = this->lu[n][0];
  b = this->lu[n][1];
  x = this->lu[n][2];
  y = this->lu[n][3];  

  max_r = r - this->radius;

  // Look for intersections with obstacles.
  for (i = 0; i < data->ranges_count; i += step)
  {
    r_ = this->lu[i][0];
    if (r_ - this->radius > max_r)
      continue;
    b_ = this->lu[i][1];
    x_ = this->lu[i][2];
    y_ = this->lu[i][3];  

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
  }

  // Clip negative ranges.
  if (max_r < 0)
    max_r = 0;

  return max_r;
}
