/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2005 -
 *     Brian Gerkey
 *                      
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

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_laserposeinterpolator laserposeinterpolator


The laserposeinterpolator driver reads laser scans from a laser device
and poses from a position2d device, linearly interpolates to estimate
the actual pose from which the scan was taken, then outputs messages
containing both scan and pose.

@par Compile-time dependencies

- none

@par Provides

- @ref player_interface_laser

@par Requires

- @ref player_interface_laser
- @ref player_interface_position2d

@par Configuration requests

- PLAYER_LASER_REQ_GET_GEOM
- PLAYER_LASER_REQ_GET_CONFIG
- PLAYER_LASER_REQ_SET_CONFIG
  
@par Configuration file options

      
@par Example 

@verbatim
driver
(
  name "sicklms200"
  provides ["laser:0"]
)
driver
(
  name "p2os"
  provides ["odometry::position:0"]
)
driver
(
  name "laserposeinterpolator"
  provides ["laser:1"]
  requires ["laser:0" "position2d:0"]
)
@endverbatim

@par Authors

Brian Gerkey

*/
/** @} */
  
  
#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include <libplayercore/playercore.h>
#include <libplayercore/error.h>

#define DEFAULT_MAXSCANS 100

// computes the signed minimum difference between the two angles.
static double
angle_diff(double a, double b)
{
  double d1, d2; 
  a = NORMALIZE(a);
  b = NORMALIZE(b);
  d1 = a-b;
  d2 = 2*M_PI - fabs(d1);
  if(d1 > 0)
    d2 *= -1.0;
  if(fabs(d1) < fabs(d2))
    return(d1);
  else
    return(d2);
}

// The laser device class.
class LaserPoseInterp : public Driver
{
  public:
    
    // Constructor
    LaserPoseInterp(ConfigFile* cf, int section);

    int Setup();
    int Shutdown();

    // MessageHandler
    int ProcessMessage(MessageQueue * resp_queue, 
		       player_msghdr * hdr, 
		       void * data, 
		       void ** resp_data, 
		       size_t * resp_len);
  private:

    // device bookkeeping
    player_devaddr_t laser_addr;
    player_devaddr_t position_addr;
    Device* laser_device;
    Device* position_device;

    // interpolation bookkeeping
    int maxnumscans;
    int numscans;
    player_laser_data_t* scans;
    double* scantimes;
    player_position2d_data_t lastpose;
    double lastposetime;
};

// a factory creation function
Driver* LaserPoseInterp_Init(ConfigFile* cf, int section)
{
  return((Driver*)(new LaserPoseInterp(cf, section)));
}

// a driver registration function
void LaserPoseInterp_Register(DriverTable* table)
{
  table->AddDriver("laserposeinterpolator", LaserPoseInterp_Init);
}

LaserPoseInterp::LaserPoseInterp(ConfigFile* cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, 
             PLAYER_LASER_CODE)
{
  // Must have an input laser
  if (cf->ReadDeviceAddr(&this->laser_addr, section, "requires",
                         PLAYER_LASER_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);    
    return;
  }
  this->laser_device = NULL;

  // Must have an input position
  if (cf->ReadDeviceAddr(&this->position_addr, section, "requires",
                         PLAYER_POSITION2D_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);    
    return;
  }
  this->position_device = NULL;

  this->maxnumscans = cf->ReadInt(section, "max_scans", DEFAULT_MAXSCANS);

  this->scans = (player_laser_data_t*)calloc(this->maxnumscans, 
                                             sizeof(player_laser_data_t));
  assert(this->scans);
  this->scantimes = (double*)calloc(this->maxnumscans, sizeof(double));
  assert(this->scantimes);

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device
int LaserPoseInterp::Setup()
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

  // Subscribe to the position.
  if(!(this->position_device = deviceTable->GetDevice(this->position_addr)))
  {
    PLAYER_ERROR("unable to locate suitable position device");
    return(-1);
  }
  if(this->position_device->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to position device");
    return(-1);
  }

  this->numscans = 0;
  this->lastposetime = -1;

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int LaserPoseInterp::Shutdown()
{
  
  this->laser_device->Unsubscribe(this->InQueue);
  this->position_device->Unsubscribe(this->InQueue);
  StopThread();
  return(0);
}


int 
LaserPoseInterp::ProcessMessage(MessageQueue * resp_queue, 
                                player_msghdr * hdr,
                                void * data, void ** resp_data,
                                size_t * resp_len)
{
  *resp_len = 0;

  // Is it a laser scan?
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 
                           PLAYER_LASER_DATA_SCAN, 
                           this->laser_addr))
  {
    // is there room?
    if(this->numscans >= this->maxnumscans)
    {
      PLAYER_WARN1("exceeded maximum number of scans to buffer (%d)",
                   this->maxnumscans);
      return(0);
    }
    // store the scan and timestamp
    this->scans[this->numscans] = *((player_laser_data_t*)data);
    this->scantimes[this->numscans] = hdr->timestamp;
    this->numscans++;
    return(0);
  }
  // Is it a new pose?
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 
                                PLAYER_POSITION2D_DATA_STATE, 
                                this->position_addr))
  {
    player_position2d_data_t newpose = *((player_position2d_data_t*)data);
    // Is it the first pose?
    if(this->lastposetime < 0)
    {
      // Just store it.
      this->lastpose = newpose;
      this->lastposetime = hdr->timestamp;
    }
    else
    {
      // Interpolate pose for all buffered scans and send them out
      double t1 = hdr->timestamp - this->lastposetime;
      for(int i=0;i<this->numscans;i++)
      {
        double t0 = this->scantimes[i] - this->lastposetime;
        player_laser_data_scanpose_t scanpose;

        scanpose.pose.px = this->lastpose.pos.px + t0 *
                (newpose.pos.px - this->lastpose.pos.px) / t1;
        scanpose.pose.py = this->lastpose.pos.py + t0 *
                (newpose.pos.py - this->lastpose.pos.py) / t1;
        scanpose.pose.pa = NORMALIZE(this->lastpose.pos.pa + t0 *
                                     angle_diff(newpose.pos.pa,
                                                this->lastpose.pos.pa) / t1);
        scanpose.scan = this->scans[i];

        this->Publish(this->device_addr, NULL,
                      PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCANPOSE,
                      (void*)&scanpose, sizeof(scanpose), 
                      this->scantimes + i);
      }
      this->numscans = 0;
      this->lastpose = newpose;
      this->lastposetime = hdr->timestamp;
    }
    return(0);
  }
  // Don't know how to handle this message.
  return(-1);
}

