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
// Desc: Driver for converting laser-interface devices to ranger-interface
//       devices
// Author: Geoffrey Biggs
// Date: 06/05/2007
//
// Requires - Laser device.
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_lasertoranger lasertoranger
 * @brief Laser-to-Ranger converter

This driver translates data provided via the @ref interface_laser interface into
the @ref interface_ranger interface.

@par Compile-time dependencies

- None

@par Provides

- @ref interface_ranger : Output ranger interface

@par Requires

- @ref interface_laser : Laser interface to translate

@par Configuration requests

- PLAYER_RANGER_REQ_GET_GEOM
- PLAYER_RANGER_REQ_POWER
- PLAYER_RANGER_REQ_INTNS

@par Configuration file options

 - None

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
  name "lasertoranger"
  requires ["laser:0"] # read from laser:0
  provides ["ranger:0"] # output results on ranger:0
)
@endverbatim

@author Geoffrey Biggs

*/
/** @} */

#include <errno.h>
#include <string.h>

#include <libplayercore/playercore.h>

#include "toranger.h"

// Driver for computing the free c-space from a laser scan.
class LaserToRanger : public ToRanger
{
	public:
		LaserToRanger (ConfigFile *cf, int section);

		int Setup (void);
		int Shutdown (void);

	protected:
		// Child message handler, for handling messages from the input device
		int ProcessMessage (MessageQueue *respQueue, player_msghdr *hdr, void *data);
		// Set power state
		int SetPower (MessageQueue *respQueue, player_msghdr *hdr, uint8_t state);
		// Set intensity data state
		int SetIntensity (MessageQueue *respQueue, player_msghdr *hdr, uint8_t state);
		// Convert laser data to ranger data
		int ConvertData (player_msghdr *hdr, void *data);
		// Convert geometry data
		bool HandleGeomRequest (player_laser_geom_t *geom);
		// Handle config values
		void HandleGetConfigResp (player_laser_config_t *data);

		player_laser_config_t laserConfig;	// Stored laser config

		uint8_t lastReqType;				// Last request type on the ranger interface
		bool startupComplete;				// True once this driver is ready to go
};

// Initialisation function
Driver* LaserToRanger_Init (ConfigFile* cf, int section)
{
	return reinterpret_cast<Driver*> (new LaserToRanger (cf, section));
}

// Register function
void LaserToRanger_Register (DriverTable* table)
{
	table->AddDriver ("lasertoranger", LaserToRanger_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Driver management
////////////////////////////////////////////////////////////////////////////////

// Constructor
// Sets up the input laser interface
LaserToRanger::LaserToRanger( ConfigFile* cf, int section)
	: ToRanger (cf, section)
{
	// Need a laser device as input
	if (cf->ReadDeviceAddr(&inputDeviceAddr, section, "requires", PLAYER_LASER_CODE, -1, NULL) != 0)
	{
		SetError (-1);
		return;
	}
}

// Setup function
int LaserToRanger::Setup (void)
{
	// First call the base setup
	if (ToRanger::Setup () != 0)
		return -1;

	lastReqType = 0;
	startupComplete = false;

	// Subscribe to the laser.
	if ((inputDevice = deviceTable->GetDevice (inputDeviceAddr)) == NULL)
	{
		PLAYER_ERROR ("Could not find input laser device");
		return -1;
	}

	if (inputDevice->Subscribe (InQueue) != 0)
	{
		PLAYER_ERROR ("Could not subscribe to input laser device");
		return -1;
	}

	// Request the config from the laser to fill in the config values
	inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_CONFIG, NULL, 0, NULL);

	// Prepare some space for storing geometry data - the parent class will clean this up when necessary
	deviceGeom.sensor_poses_count = 1;
	if ((deviceGeom.sensor_poses = new player_pose3d_t) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for sensor poses");
		return -1;
	}
	deviceGeom.sensor_sizes_count = 1;
	if ((deviceGeom.sensor_sizes = new player_bbox3d_t) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for sensor sizes");
		delete deviceGeom.sensor_sizes;
		deviceGeom.sensor_sizes = NULL;
		return -1;
	}

	return 0;
}

// Shutdown function
int LaserToRanger::Shutdown (void)
{
	// Unsubscribe from the laser device
	inputDevice->Unsubscribe (InQueue);

	// Call the base shutdown function
	return ToRanger::Shutdown ();
}

////////////////////////////////////////////////////////////////////////////////
//	Message handling
////////////////////////////////////////////////////////////////////////////////

int LaserToRanger::ConvertData (player_msghdr *hdr, void *data)
{
	player_laser_data_t *scanData = NULL;
	player_pose_t *pose = NULL;
	player_ranger_data_rangepose_t rangeData;
	player_ranger_data_intnspose_t intensityData;

	memset (&rangeData, 0, sizeof (rangeData));
	memset (&intensityData, 0, sizeof (intensityData));

	// Copy the data out
	if (hdr->subtype == PLAYER_LASER_DATA_SCAN)
		scanData = reinterpret_cast<player_laser_data_t*> (data);
	else if (hdr->subtype == PLAYER_LASER_DATA_SCANPOSE)
	{
		scanData = &(reinterpret_cast<player_laser_data_scanpose_t*> (data)->scan);
		pose = &(reinterpret_cast<player_laser_data_scanpose_t*> (data)->pose);
	}
	else
		return -1;

	// If we found a pose, handle that first (we will need to publish it)
	if (pose != NULL)
	{
		deviceGeom.pose.px = pose->px;
		deviceGeom.pose.py = pose->py;
		deviceGeom.pose.pz = 0.0f;
		deviceGeom.pose.proll = 0.0f;
		deviceGeom.pose.ppitch = 0.0f;
		deviceGeom.pose.pyaw = pose->pa;
		*(deviceGeom.sensor_poses) = deviceGeom.pose;
	}
	// Handle any data we got data
	if (data != NULL)
	{
		// Update the properties from the data
		laserConfig.min_angle = scanData->min_angle;
		laserConfig.max_angle = scanData->max_angle;
		laserConfig.resolution = scanData->resolution;
		laserConfig.max_range = scanData->max_range;

		// Copy out the range data
		if (scanData->ranges_count > 0)
		{
			rangeData.data.ranges_count = scanData->ranges_count;
			if ((rangeData.data.ranges = new double[scanData->ranges_count]) == NULL)
			{
				PLAYER_ERROR ("Failed to allocate memory for range data");
				return 0;
			}
			for (uint32_t ii = 0; ii < scanData->ranges_count; ii++)
				rangeData.data.ranges[ii] = scanData->ranges[ii];
			// Send off this chunk of data, with pose if necessary
			if (pose == NULL)
				Publish (device_addr, NULL, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE, reinterpret_cast<void*> (&rangeData.data), sizeof (rangeData.data), NULL);
			else
			{
				rangeData.geom = deviceGeom;
				Publish (device_addr, NULL, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGEPOSE, reinterpret_cast<void*> (&rangeData), sizeof (rangeData), NULL);
			}
			// Delete the space we allocated for the data
			delete[] rangeData.data.ranges;
		}

		// Do the same for intensity data, if there is any
		if (scanData->intensity_count > 0)
		{
			intensityData.data.intensities_count = scanData->intensity_count;
			if ((intensityData.data.intensities = new double[scanData->intensity_count]) == NULL)
			{
				PLAYER_ERROR ("Failed to allocate memory for range data");
				return 0;
			}
			for (uint32_t ii = 0; ii < scanData->intensity_count; ii++)
				intensityData.data.intensities[ii] = scanData->intensity[ii];
			// Send off this chunk of data, with pose if necessary
			if (pose == NULL)
				Publish (device_addr, NULL, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_INTNS, reinterpret_cast<void*> (&intensityData.data), sizeof (intensityData.data), NULL);
			else
			{
				rangeData.geom = deviceGeom;
				Publish (device_addr, NULL, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_INTNSPOSE, reinterpret_cast<void*> (&intensityData), sizeof (intensityData), NULL);
			}
			// Delete the space we allocated for the data
			delete[] intensityData.data.intensities;
		}
	}

	return 0;
}

bool LaserToRanger::HandleGeomRequest (player_laser_geom_t *data)
{
	deviceGeom.pose.px = data->pose.px;
	deviceGeom.pose.py = data->pose.py;
	deviceGeom.pose.pz = 0.0f;
	deviceGeom.pose.proll = 0.0f;
	deviceGeom.pose.ppitch = 0.0f;
	deviceGeom.pose.pyaw = data->pose.pa;
	deviceGeom.size.sw = data->size.sw;
	deviceGeom.size.sl = data->size.sl;
	deviceGeom.size.sh = 0.0f;

	*(deviceGeom.sensor_poses) = deviceGeom.pose;
	*(deviceGeom.sensor_sizes) = deviceGeom.size;

	return true;
}

int LaserToRanger::ProcessMessage (MessageQueue *respQueue, player_msghdr *hdr, void *data)
{
	// Check the parent message handler
	if (ToRanger::ProcessMessage (respQueue, hdr, data) == 0)
		return 0;

	// Check capabilities requests
	HANDLE_CAPABILITY_REQUEST (device_addr, respQueue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_POWER);
	HANDLE_CAPABILITY_REQUEST (device_addr, respQueue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_INTNS);
	HANDLE_CAPABILITY_REQUEST (device_addr, respQueue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_GEOM);

	// Messages from the ranger interface
	// Power config request
	if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_POWER, device_addr))
	{
		// Tell the laser device to switch the power state
		player_laser_power_config_t req;
		req.state = reinterpret_cast<player_ranger_power_config_t*> (data)->state;
		inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_POWER, &req, sizeof (req), 0);
		// Store the return queue
		ret_queue = respQueue;
		return 0;
	}
	// Intensity data config request
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_INTNS, device_addr))
	{
		// Tell the laser device to give intensity data
		laserConfig.intensity = reinterpret_cast<player_ranger_intns_config_t*> (data)->state;
		inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_SET_CONFIG, &laserConfig, sizeof (laserConfig), 0);
		lastReqType = PLAYER_RANGER_REQ_INTNS;
		return 0;
	}
	// Geometry request
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_GEOM, device_addr))
	{
		// Get geometry from the laser device
		inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_GEOM, NULL, 0, NULL);
		ret_queue = respQueue;
		return 0;
	}
	// Config set request
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_SET_CONFIG, device_addr))
	{
		// Translate and forward the request to the laser device
		player_ranger_config_t *req = reinterpret_cast<player_ranger_config_t*> (data);
		laserConfig.min_angle = req->min_angle;
		laserConfig.max_angle = req->max_angle;
		laserConfig.resolution = req->resolution;
		laserConfig.max_range = req->max_range;
		laserConfig.range_res = req->range_res;
		laserConfig.scanning_frequency = req->frequency;
		inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_SET_CONFIG, &laserConfig, sizeof (laserConfig), 0);
		lastReqType = PLAYER_RANGER_REQ_SET_CONFIG;
		return 0;
	}
	// Config get request
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_CONFIG, device_addr))
	{
		// Forward the request onto the laser device, will handle the response when it comes
		inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_CONFIG, NULL, 0, NULL);
		lastReqType = PLAYER_RANGER_REQ_GET_CONFIG;
		return 0;
	}


	// Messages from the laser interface
	// Reqest ACKs
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LASER_REQ_POWER, inputDeviceAddr))
	{
		// Power request
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_POWER, NULL, 0, NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LASER_REQ_SET_CONFIG, inputDeviceAddr))
	{
		// Set config may have been triggered by either a ranger set config request or a ranger set intensity request
		memcpy (&laserConfig, data, sizeof (player_laser_config_t));
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_ACK, lastReqType, &laserConfig, sizeof (laserConfig), NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LASER_REQ_GET_CONFIG, inputDeviceAddr))
	{
		// Only way to get a get config response from the laser is via a ranger get config request, or in startup
		memcpy (&laserConfig, data, sizeof (player_laser_config_t));
		if (lastReqType == PLAYER_RANGER_REQ_GET_CONFIG && startupComplete)
			Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_GET_CONFIG, &laserConfig, sizeof (laserConfig), NULL);
		else if (!startupComplete)
		{
			PLAYER_MSG0 (1, "LaserToRanger startup complete");
			startupComplete = true;
		}
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LASER_REQ_GET_GEOM, inputDeviceAddr))
	{
		// Geometry request - need to manage the info we just got
		if (HandleGeomRequest (reinterpret_cast<player_laser_geom_t*> (data)))
			Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_GET_GEOM, &deviceGeom, sizeof (deviceGeom), NULL);
		else
			Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_GET_GEOM, NULL, 0, NULL);
	}

	// Request NACKs
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_NACK, PLAYER_LASER_REQ_POWER, inputDeviceAddr))
	{
		// Power request
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_POWER, NULL, 0, NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_NACK, PLAYER_LASER_REQ_SET_CONFIG, inputDeviceAddr))
	{
		// Set config may have been triggered by either a ranger set config request or a ranger set intensity request
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, lastReqType, NULL, 0, NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_NACK, PLAYER_LASER_REQ_GET_CONFIG, inputDeviceAddr))
	{
		// Only way to get a get config response from the laser is via a ranger get config request, or in startup
		if (lastReqType == PLAYER_RANGER_REQ_GET_CONFIG && startupComplete)
			Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_GET_CONFIG, NULL, 0, NULL);
		else if (!startupComplete)
		{
			PLAYER_MSG0 (1, "LaserToRanger startup failed to get config from device");
			startupComplete = true;
		}
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_NACK, PLAYER_LASER_REQ_GET_GEOM, inputDeviceAddr))
	{
		// Geometry request
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_GET_GEOM, NULL, 0, NULL);
		return 0;
	}

	// Data
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_DATA, -1, inputDeviceAddr))
	{
		return ConvertData (hdr, data);
	}

	return -1;
}
