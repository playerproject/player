/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006 - Radu Bogdan Rusu (rusu@cs.tum.edu)
 *  Copyright (C) 2009 - Markus Eich (markus.eich@dfki.de)
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
/** @defgroup driver_laserptzcloud laserptzcloud
 * @brief Build a 3D point cloud from laser and ptz data

The laserptztcloud driver reads laser scans from a laser device and PTZ poses
from a ptz device, linearly interpolates to estimate the actual pan/tilt pose
from which the scan was taken, then outputs messages containing the cartesian
3D coordinates (X,Y,Z in [m]) via a pointcloud3d interface. No additional
thread is started. Based on Brian's laserposerinterpolator.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_pointcloud3d

@par Requires

- @ref interface_laser
- @ref interface_ptz

@par Configuration requests

- None (yet)

@par Configuration file options

@par Example

@verbatim
driver
(
  name "laserptzcloud"
  provides ["pointcloud3d:0"]
  requires ["laser:0" "ptz:0"]
)
@endverbatim

@author Radu Bogdan Rusu

 */
/** @} */

#include <config.h>

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <assert.h>

#include <vector>
#include <iostream>

#include <libplayercore/playercore.h>

#define DEFAULT_MAXDISTANCE 10
#define DEFAULT_MINDISTANCE 0.020


// PTZ defaults for tilt
#define PTZ_PAN  0
#define PTZ_TILT 1
#define DEFAULT_PTZ_PAN_OR_TILT PTZ_TILT


using namespace std;


struct ScanHelper{
        
    float min_angle;
    float resolution;
    uint32_t ranges_count;
    vector<float> ranges;
    double timestamp;
 };



// The laser device class.
class LaserPTZCloud : public Driver
{
    public:
        // Constructor
        LaserPTZCloud (ConfigFile* cf, int section);
        ~LaserPTZCloud ();

        int Setup();
        int Shutdown();

        // MessageHandler
        int ProcessMessage (QueuePointer &resp_queue,
                            player_msghdr * hdr,
                            void * data);
    private:

        // device bookkeeping
        player_devaddr_t laser_addr;
        player_devaddr_t ptz_addr;
        Device*          laser_device;
        Device*          ptz_device;

        // Laser scans
    vector<ScanHelper> scans;


        // Maximum distance that we should consider from the laser
        float maxdistance;
        float mindistance;

        // PTZ tilt parameters
        float ptz_pan_or_tilt;
		
		player_laser_data_t test_data;	

        // Timeouts, delays
        float delay;

        // First PTZ pose
        player_ptz_data_t lastpose;
        double            lastposetime;
};

////////////////////////////////////////////////////////////////////////////////
//Factory creation function. This functions is given as an argument when
// the driver is added to the driver table
Driver* LaserPTZCloud_Init (ConfigFile* cf, int section)
{
    return ((Driver*)(new LaserPTZCloud (cf, section)));
}

////////////////////////////////////////////////////////////////////////////////
//Registers the driver in the driver table. Called from the
// player_driver_init function that the loader looks for
void laserptzcloud_Register (DriverTable* table)
{
    table->AddDriver ("laserptzcloud", LaserPTZCloud_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
LaserPTZCloud::LaserPTZCloud (ConfigFile* cf, int section)
    : Driver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN,
             PLAYER_POINTCLOUD3D_CODE)
{
    // Must have an input laser
    if (cf->ReadDeviceAddr (&this->laser_addr, section, "requires",
        PLAYER_LASER_CODE, -1, NULL) != 0)
    {
        PLAYER_ERROR ("must have an input laser");
        this->SetError (-1);
        return;
    }
    this->laser_device = NULL;

    // Must have an input ptz
    if (cf->ReadDeviceAddr (&this->ptz_addr, section, "requires",
        PLAYER_PTZ_CODE, -1, NULL) != 0)
    {
        PLAYER_ERROR ("must have an input ptz");
        this->SetError (-1);
        return;
    }
    this->ptz_device = NULL;

    // ---[ PTZ parameters ]---
    this->ptz_pan_or_tilt = static_cast<float> (cf->ReadFloat
            (section, "ptz_pan_or_tilt", DEFAULT_PTZ_PAN_OR_TILT));

    // Maximum allowed distance
    this->maxdistance = static_cast<float> (cf->ReadFloat (section, "max_distance", DEFAULT_MAXDISTANCE));
    this->mindistance = static_cast<float> (cf->ReadFloat (section, "min_distance", DEFAULT_MINDISTANCE));       

    return;
}

////////////////////////////////////////////////////////////////////////////////
// Destructor.
LaserPTZCloud::~LaserPTZCloud()
{
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int LaserPTZCloud::Setup()
{
    // Subscribe to the laser
    if (!(this->laser_device = deviceTable->GetDevice (this->laser_addr)))
    {
        PLAYER_ERROR ("unable to locate suitable laser device");
        return (-1);
    }
    if (this->laser_device->Subscribe (this->InQueue) != 0)
    {
        PLAYER_ERROR ("unable to subscribe to laser device");
        return (-1);
    }

    // Subscribe to the ptz.
    if (!(this->ptz_device = deviceTable->GetDevice (this->ptz_addr)))
    {
        PLAYER_ERROR ("unable to locate suitable ptz device");
        return (-1);
    }
    if (this->ptz_device->Subscribe (this->InQueue) != 0)
    {
        PLAYER_ERROR ("unable to subscribe to ptz device");
        return (-1);
    }

    this->lastposetime = -1;
    return (0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int LaserPTZCloud::Shutdown()
{
    this->laser_device->Unsubscribe (this->InQueue);
    this->ptz_device->Unsubscribe   (this->InQueue);
    return (0);
}

////////////////////////////////////////////////////////////////////////////////
// ProcessMessage
int LaserPTZCloud::ProcessMessage (QueuePointer &resp_queue,
                                          player_msghdr * hdr,
                                          void * data)
{
    // Is it a laser scan?
    if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_DATA,
	PLAYER_LASER_DATA_SCAN,
        this->laser_addr))
    {

	player_laser_data_t laser;

	laser=*((player_laser_data_t*)data);

	ScanHelper storage;
	vector<float>::iterator iter;

	storage.min_angle=laser.min_angle;
	storage.resolution=laser.resolution;
	storage.ranges_count=laser.ranges_count;
	storage.timestamp=hdr->timestamp;

	for (unsigned  i=0;i<storage.ranges_count;i++)
	    storage.ranges.push_back(laser.ranges[i]);

	scans.push_back(storage);

            return (0);
	
    }
    // Is it a ptz pose?
    else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_DATA,
             PLAYER_PTZ_DATA_STATE, this->ptz_addr))
    {
        player_ptz_data_t newpose = *((player_ptz_data_t*)data);

	ScanHelper laserdata;
	player_pointcloud3d_data_t cloud_data;
	double t1,t0;
	double angle_x,angle_y;
	vector <player_pointcloud3d_element_t> pointlist;

        // Is it the first pose?
        if (this->lastposetime < 0)
        {
            // Just store it.
            this->lastpose     = newpose;
            this->lastposetime = hdr->timestamp;
        }
        else
        {
            // Interpolate pose for all buffered scans and send them out
            t1 = hdr->timestamp - this->lastposetime;

            if (newpose.tilt != lastpose.tilt)
        	{

		while (!scans.empty()){	 
		    //take first scan from vector
		    laserdata=scans.front();	

		    //process it
		    t0 = laserdata.timestamp - this->lastposetime;
		    
		    angle_y = this->lastpose.tilt + t0 * (newpose.tilt - this->lastpose.tilt) / t1;

                // Calculate the horizontal angles and the cartesian coordinates
		    angle_x    = laserdata.min_angle;

		    //process all points in a scan 
		    for (unsigned i = 0; i < laserdata.ranges_count; i++)
		    {

			if (laserdata.ranges[i] < maxdistance && laserdata.ranges[i] > mindistance)
                {
			    player_pointcloud3d_element_t element;			    

			    element.point.px = laserdata.ranges[i] * cos (angle_x) * sin (angle_y);
			    element.point.py = laserdata.ranges[i] * cos (angle_x) * cos (angle_y);
			    element.point.pz = laserdata.ranges[i] * sin (angle_x);
			    
			    pointlist.push_back(element);
                }

			angle_x += laserdata.resolution;
                }


		    //publish pointcloud
		    cloud_data.points_count = pointlist.size();
		    cloud_data.points = new player_pointcloud3d_element_t[cloud_data.points_count];
		    int i=0;
		    while (!pointlist.empty())
		    {
			cloud_data.points[i] = pointlist.front();
			pointlist.erase(pointlist.begin());
			i++;			
		    }			

                Publish (this->device_addr, PLAYER_MSGTYPE_DATA,
                     PLAYER_POINTCLOUD3D_DATA_STATE, &cloud_data,
                     sizeof (player_pointcloud3d_data_t), NULL);
		    delete []cloud_data.points;

		    laserdata.ranges.erase(laserdata.ranges.begin(),laserdata.ranges.end());
		    scans.erase(scans.begin());

        	}
	    }
	    pointlist.erase(pointlist.begin(),pointlist.end());
            this->lastpose     = newpose;
            this->lastposetime = hdr->timestamp;
        }
        return(0);
    }
    // Don't know how to handle this message.
    return (-1);
}

