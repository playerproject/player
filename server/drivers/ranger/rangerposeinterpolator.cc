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

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_rangerposeinterpolator rangerposeinterpolator
 * @brief Attach poses to ranger scans

The rangerposeinterpolator driver reads ranger scans from a ranger device
and poses from a position2d device, linearly interpolates to estimate
the actual pose from which the scan was taken, then outputs messages
containing both scan and pose.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_ranger : Pose-stamped ranger scans (subtype
PLAYER_RANGER_DATA_RANGESTAMPED) are published via this interface

@par Requires

- @ref interface_ranger : Raw ranger scans (subtype PLAYER_RANGER_DATA_RANGE) are read from this device
- @ref interface_position2d : Pose data (subtype PLAYER_POSITION2D_DATA_STATE) is read from this device

@par Configuration requests

- All configuration are forwarded to the underlying @ref interface_ranger device for handling.

@par Configuration file options

- interpolate (integer)
  - Default: 1
  - Linearly interpolate between poses for each scan (1), or just attach the
    most recent pose to each scan (0).
- max_scans (integer)
  - Default: 100
  - Maximum number of scans to buffer while waiting for a second pose in order
    to interpolate.
- update_thresh ([length angle] tuple)
  - Default: [-1.0 -1.0]
  - Minimum change in pose (translation or rotation) required before
    a new ranger scan will be published.  Use this option to choke the data rate.
    Set either value to -1.0 to indicate that no threshold should be applied in
    that dimension (i.e., every scan should be published).
- update_interval (float, seconds)
  - Default: -1.0
  - Interval after which a new scan will be published, regardless of how far
    the robot has moved.  Set to -1.0 to disable this threshold.
- send_all_scans (integer)
  - Default: 1
  - Whether to stamp and publish every ranger scan.  If set to 1, this option
    overrides update_thresh and update_interval.

@par Example

@verbatim
driver
(
  name "sicklms200"
  provides ["ranger:0"]
)
driver
(
  name "p2os"
  provides ["odometry::position:0"]
)
driver
(
  name "rangerposeinterpolator"
  provides ["ranger:1"]
  requires ["ranger:0" "position2d:0"]
)
@endverbatim

@author Brian Gerkey (modified from laser to ranger by Patrick Beeson)

*/
/** @} */


#include <config.h>

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <assert.h>

#include <libplayercore/playercore.h>
#include <libplayerinterface/functiontable.h>

#if defined (WIN32)
  #define hypot _hypot
#endif

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

// The ranger device class.
class RangerPoseInterp : public Driver
{
  public:

    // Constructor
    RangerPoseInterp(ConfigFile* cf, int section);
    ~RangerPoseInterp();

    int Setup();
    int Shutdown();

    // MessageHandler
    int ProcessMessage(QueuePointer & resp_queue,
		       player_msghdr * hdr,
		       void * data);
  private:

    // device bookkeeping
    player_devaddr_t ranger_addr;
    player_devaddr_t position_addr;
    Device* ranger_device;
    Device* position_device;

    // interpolation bookkeeping
    bool interpolate;
    int maxnumscans;
    int numscans;
    player_ranger_data_range_t* scans;
    double* scantimes;
    player_position2d_data_t lastpose;
    double lastposetime;
    player_pose3d_t lastpublishpose;
    double lastpublishposetime;
    double update_thresh[2];
    double update_interval;
    bool send_all_scans;
};

// a factory creation function
Driver* RangerPoseInterp_Init(ConfigFile* cf, int section)
{
  return((Driver*)(new RangerPoseInterp(cf, section)));
}

// a driver registration function
void rangerposeinterpolator_Register(DriverTable* table)
{
  table->AddDriver("rangerposeinterpolator", RangerPoseInterp_Init);
}

RangerPoseInterp::RangerPoseInterp(ConfigFile* cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN,
             PLAYER_RANGER_CODE)
{
  // Must have an input ranger
  if (cf->ReadDeviceAddr(&this->ranger_addr, section, "requires",
                         PLAYER_RANGER_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  this->ranger_device = NULL;

  // Must have an input position
  if (cf->ReadDeviceAddr(&this->position_addr, section, "requires",
                         PLAYER_POSITION2D_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  this->position_device = NULL;

  this->interpolate = cf->ReadInt(section, "interpolate", 1) != 0 ? true : false;
  this->maxnumscans = cf->ReadInt(section, "max_scans", DEFAULT_MAXSCANS);
  this->update_thresh[0] = cf->ReadTupleLength(section, "update_thresh",
                                               0, -1.0);
  this->update_thresh[1] = cf->ReadTupleAngle(section, "update_thresh",
                                              1, -1.0);
  this->update_interval = cf->ReadFloat(section, "update_interval", -1.0);
  this->send_all_scans = cf->ReadInt(section, "send_all_scans", 1) != 0 ? true : false;

  this->scans = (player_ranger_data_range_t*)calloc(this->maxnumscans,
						    sizeof(player_ranger_data_range_t));
  assert(this->scans);
  this->scantimes = (double*)calloc(this->maxnumscans, sizeof(double));
  assert(this->scantimes);

  return;
}

RangerPoseInterp::~RangerPoseInterp()
{
  for (int i=0; i< maxnumscans; i++)
    if (this->scans[i].ranges != NULL)
      delete [] this->scans[i].ranges;
  free(this->scans);
  free(this->scantimes);
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device
int RangerPoseInterp::Setup()
{
  // Subscribe to the ranger.
  if(Device::MatchDeviceAddress(this->ranger_addr, this->device_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return(-1);
  }
  if(!(this->ranger_device = deviceTable->GetDevice(this->ranger_addr)))
  {
    PLAYER_ERROR("unable to locate suitable ranger device");
    return(-1);
  }
  if(this->ranger_device->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to ranger device");
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
  this->lastpublishposetime = -1;

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int RangerPoseInterp::Shutdown()
{

  this->ranger_device->Unsubscribe(this->InQueue);
  this->position_device->Unsubscribe(this->InQueue);
  return(0);
}


int
RangerPoseInterp::ProcessMessage(QueuePointer & resp_queue,
                                player_msghdr * hdr,
                                void * data)
{
  // Is it a ranger scan?
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                           PLAYER_RANGER_DATA_RANGE,
                           this->ranger_addr))
  {
    // are we interpolating?
    if(!this->interpolate)
    {
      // make sure we've gotten at least one pose
      if(this->lastposetime < 0)
        return(0);

      // Tag this scan with the last received pose and push it out
      player_ranger_data_rangestamped_t scanpose;
      scanpose.have_config = false;
      scanpose.have_geom = true;
      scanpose.geom.pose.px = this->lastpose.pos.px;
      scanpose.geom.pose.py = this->lastpose.pos.py;
      scanpose.geom.pose.pyaw = this->lastpose.pos.pa;
      scanpose.geom.element_poses_count = 0;
      scanpose.geom.element_sizes_count = 0;
      scanpose.data.ranges_count = ((player_ranger_data_range_t*)data)->ranges_count;
      scanpose.data.ranges = ((player_ranger_data_range_t*)data)->ranges;

      this->Publish(this->device_addr,
		    PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGESTAMPED,
		    (void*)&scanpose, sizeof(scanpose), &hdr->timestamp);
      return(0);
    }
    else
    {
      // Buffer the scan to be pushed out later.

      // is there room?
      if(this->numscans >= this->maxnumscans)
      {
        PLAYER_WARN1("exceeded maximum number of scans to buffer (%d)",
                     this->maxnumscans);
        return(0);
      }
      // store the scan and timestamp, make sure we deep copy the data
	  player_copy_fn_t copyfunc;
      if (!(copyfunc = playerxdr_get_copyfunc (PLAYER_RANGER_CODE, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE)))
      {
        PLAYER_ERROR ("Couldn't fund copy function to copy ranger data");
        return(-1);
      }
      if (this->scans[numscans].ranges != NULL) {
	delete [] this->scans[numscans].ranges;
	this->scans[numscans].ranges = NULL;
      }
      
      this->scans[numscans].ranges = new double [ ((player_ranger_data_range_t*)data)->ranges_count ];

      (*copyfunc) (&this->scans[this->numscans],data);
      //this->scans[this->numscans] = *((player_ranger_data_t*)data);
      this->scantimes[this->numscans] = hdr->timestamp;
      this->numscans++;
      return(0);
    }
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
      // are we interpolating?
      if(this->interpolate)
      {
        // Interpolate pose for all buffered scans and send them out
        double t1 = hdr->timestamp - this->lastposetime;
        for(int i=0;i<this->numscans;i++)
        {
          double t0 = this->scantimes[i] - this->lastposetime;
          player_ranger_data_rangestamped_t scanpose;

          scanpose.geom.pose.px = this->lastpose.pos.px + t0 *
                  (newpose.pos.px - this->lastpose.pos.px) / t1;
          scanpose.geom.pose.py = this->lastpose.pos.py + t0 *
                  (newpose.pos.py - this->lastpose.pos.py) / t1;
          scanpose.geom.pose.pyaw = NORMALIZE(this->lastpose.pos.pa + t0 *
					      angle_diff(newpose.pos.pa,
							 this->lastpose.pos.pa) / t1);
	  scanpose.have_config = false;
	  scanpose.have_geom = true;
	  scanpose.geom.element_poses_count = 1;
	  scanpose.geom.element_poses = &(scanpose.geom.pose);
	  scanpose.geom.element_sizes_count = 0;
	  
	  scanpose.data.ranges_count = this->scans[i].ranges_count;
	  scanpose.data.ranges = this->scans[i].ranges;

	  //          scanpose.data = this->scans[i];

          // Should we publish this scan?  Take account of all the various
          // thresholds that the user can set.
          if((this->send_all_scans) ||
             (this->lastpublishposetime < 0.0) ||
             ((this->update_thresh[0] >= 0.0) &&
              (hypot(scanpose.geom.pose.px-this->lastpublishpose.px,
                     scanpose.geom.pose.py-this->lastpublishpose.py) >=
               this->update_thresh[0])) ||
             ((this->update_thresh[1] >= 0.0) &&
              (fabs(angle_diff(scanpose.geom.pose.pyaw,this->lastpublishpose.pyaw)) >=
               this->update_thresh[1])) ||
             ((this->update_interval >= 0.0) &&
              ((this->scantimes[i] - this->lastpublishposetime) >=
               this->update_interval)))
          {
	    this->Publish(this->device_addr,
			  PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGESTAMPED,
			  (void*)&scanpose, sizeof(scanpose),
			  this->scantimes + i);
	    
            this->lastpublishposetime = this->scantimes[i];
            this->lastpublishpose = scanpose.geom.pose;
          }
        }
        this->numscans = 0;
      }
      this->lastpose = newpose;
      this->lastposetime = hdr->timestamp;
    }
    return(0);
  }
  // Forward any request to the ranger
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->device_addr))
  {
    // Forward the message
    ranger_device->PutMsg(this->InQueue, hdr, data);
    // Store the return address for later use
    this->ret_queue = resp_queue;
    // Set the message filter to look for the response
    this->InQueue->SetFilter(this->ranger_addr.host,
                             this->ranger_addr.robot,
                             this->ranger_addr.interf,
                             this->ranger_addr.index,
                             -1,
                             hdr->subtype);
    // No response now; it will come later after we hear back from the
    // ranger
    return(0);
  }
  // Forward response (success or failure) from the ranger
  else if((Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK,
                            -1, this->ranger_addr)) ||
     (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_NACK,
                            -1, this->ranger_addr)))
  {
    // Copy in our address and forward the response
    hdr->addr = this->device_addr;
    this->Publish(this->ret_queue, hdr, data);
    // Clear the filter
    this->InQueue->ClearFilter();

    return(0);
  }
  // Don't know how to handle this message.
  return(-1);
}

