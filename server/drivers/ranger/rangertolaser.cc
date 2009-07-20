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
// Desc: Driver for converting ranger-interface devices to laser-interface
//       devices
// Author: Geoffrey Biggs
// Date: 06/05/2007
//
// Requires - Ranger device.
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_rangertolaser RangerToLaser
 * @brief Ranger-to-Laser converter

This driver translates data provided via the @ref interface_ranger interface into
the @ref interface_laser interface.

@par Compile-time dependencies

- None

@par Provides

- @ref interface_laser : Output laser interface

@par Requires

- @ref interface_ranger : Ranger interface to translate

@par Configuration requests

- PLAYER_LASER_REQ_GET_GEOM
- PLAYER_LASER_REQ_POWER
- PLAYER_LASER_REQ_GET_CONFIG
- PLAYER_LASER_REQ_SET_CONFIG

@par Configuration file options

 - None

@par Example

@verbatim
driver
(
  name "hokuyo_aist"
  provides ["ranger:0"]
)
driver
(
  name "rangertolaser"
  requires ["ranger:0"] # read from ranger:0
  provides ["laser:0"] # output results on laser:0
)
@endverbatim

@author Geoffrey Biggs

*/
/** @} */

#include <errno.h>
#include <string.h>

#include <libplayercore/playercore.h>

#include "fromranger.h"

class RangerToLaser : public FromRanger
{
	public:
		RangerToLaser (ConfigFile *cf, int section);

		int Setup (void);
		int Shutdown (void);

	protected:
		// Child message handler, for handling messages from the input device
		int ProcessMessage (QueuePointer &respQueue, player_msghdr *hdr, void *data);
		// Set power state
		int SetPower (QueuePointer &respQueue, player_msghdr *hdr, uint8_t state);
		// Set intensity data state
		int SetIntensity (QueuePointer &respQueue, player_msghdr *hdr, uint8_t state);
		// Convert ranger data to laser data
		int ConvertData (player_msghdr *hdr, void *data);
		// Convert geometry data
		bool HandleGeomRequest (player_laser_geom_t *dest, player_ranger_geom_t *data);
		// Handle config values
		void CopyConfig (player_ranger_config_t *data);

		bool receivedCfgResp, receivedIntnsResp, setConfigFailed;
		player_laser_config_t config;
		bool startup;
};

// Initialisation function
Driver* RangerToLaser_Init (ConfigFile* cf, int section)
{
	return reinterpret_cast<Driver*> (new RangerToLaser (cf, section));
}

// Register function
void rangertolaser_Register (DriverTable* table)
{
	table->AddDriver ("rangertolaser", RangerToLaser_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Driver management
////////////////////////////////////////////////////////////////////////////////

// Constructor
// Sets up the input ranger interface
RangerToLaser::RangerToLaser (ConfigFile* cf, int section)
	: FromRanger (cf, section)
{
	// Need a ranger device as input
	if (cf->ReadDeviceAddr(&inputDeviceAddr, section, "requires", PLAYER_RANGER_CODE, -1, NULL) != 0)
	{
		SetError (-1);
		return;
	}
}

// Setup function
int RangerToLaser::Setup (void)
{
	// First call the base setup
	if (FromRanger::Setup () != 0)
		return -1;

	receivedCfgResp = receivedIntnsResp = setConfigFailed = false;
	memset (&config, 0, sizeof (config));
	startup = true;

	// Subscribe to the ranger.
	if ((inputDevice = deviceTable->GetDevice (inputDeviceAddr)) == NULL)
	{
		PLAYER_ERROR ("Could not find input ranger device");
		return -1;
	}

	if (inputDevice->Subscribe (InQueue) != 0)
	{
		PLAYER_ERROR ("Could not subscribe to ranger laser device");
		return -1;
	}

	// Request the ranger device's configuration
	inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_CONFIG, NULL, 0, NULL);

	return 0;
}

// Shutdown function
int RangerToLaser::Shutdown (void)
{
	// Unsubscribe from the ranger device
	inputDevice->Unsubscribe (InQueue);

	// Call the base shutdown function
	return FromRanger::Shutdown ();
}

////////////////////////////////////////////////////////////////////////////////
//	Message handling
////////////////////////////////////////////////////////////////////////////////

int RangerToLaser::ConvertData (player_msghdr *hdr, void *data)
{
	player_laser_data_scanpose_t scanData;
	double *rangeData = NULL, *intensityData = NULL;
	int rangeCount = 0, intensityCount = 0;
	player_ranger_geom_t *pose = NULL;

	memset (&scanData, 0, sizeof (scanData));

	switch (hdr->subtype)
	{
		case PLAYER_RANGER_DATA_RANGE:
			if (reinterpret_cast<player_ranger_data_range_t*> (data)->ranges_count > 0)
			{
				rangeData = reinterpret_cast<player_ranger_data_range_t*> (data)->ranges;
				rangeCount = reinterpret_cast<player_ranger_data_range_t*> (data)->ranges_count;
			}
			break;
		case PLAYER_RANGER_DATA_RANGESTAMPED:
			if (reinterpret_cast<player_ranger_data_rangestamped_t*> (data)->data.ranges_count > 0)
			{
				rangeData = reinterpret_cast<player_ranger_data_rangestamped_t*> (data)->data.ranges;
				rangeCount = reinterpret_cast<player_ranger_data_rangestamped_t*> (data)->data.ranges_count;
			}
			pose = &reinterpret_cast<player_ranger_data_rangestamped_t*> (data)->geom;
			break;
		case PLAYER_RANGER_DATA_INTNS:
			if (reinterpret_cast<player_ranger_data_intns_t*> (data)->intensities_count > 0)
			{
				intensityData = reinterpret_cast<player_ranger_data_intns_t*> (data)->intensities;
				intensityCount = reinterpret_cast<player_ranger_data_intns_t*> (data)->intensities_count;
			}
			break;
		case PLAYER_RANGER_DATA_INTNSSTAMPED:
			if (reinterpret_cast<player_ranger_data_intnsstamped_t*> (data)->data.intensities_count > 0)
			{
				intensityData = reinterpret_cast<player_ranger_data_intnsstamped_t*> (data)->data.intensities;
				intensityCount = reinterpret_cast<player_ranger_data_intnsstamped_t*> (data)->data.intensities_count;
			}
			pose = &reinterpret_cast<player_ranger_data_intnsstamped_t*> (data)->geom;
			break;
		case PLAYER_RANGER_DATA_GEOM:
		default:
			return 0;
	}

	// Copy the data into the laser message format
	if (rangeData != NULL)
	{
		scanData.scan.ranges_count = rangeCount;
		if ((scanData.scan.ranges = new float[rangeCount]) == NULL)
		{
			PLAYER_ERROR ("Failed to allocate memory for range data");
			return 0;
		}
		for (int ii = 0; ii < rangeCount; ii++)
			scanData.scan.ranges[ii] = static_cast<float> (rangeData[ii]);
	}
	if (intensityData != NULL)
	{
		scanData.scan.intensity_count = intensityCount;
		if ((scanData.scan.intensity = new uint8_t[intensityCount]) == NULL)
		{
			PLAYER_ERROR ("Failed to allocate memory for intensity data");
			return 0;
		}
		for (int ii = 0; ii < intensityCount; ii++)
			scanData.scan.intensity[ii] = static_cast<uint8_t> (intensityData[ii]);
	}

	// If we found a pose, copy it
	if (pose != NULL)
	{
		scanData.pose.px = pose->pose.px;
		scanData.pose.py = pose->pose.py;
	}

	// Need to copy across the latest configuration information
	scanData.scan.min_angle = config.min_angle;
	scanData.scan.max_angle = config.max_angle;
	scanData.scan.resolution = config.resolution;
	scanData.scan.max_range = config.max_range;
	scanData.scan.id = 0;

	if (pose != NULL)
		Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCANPOSE, reinterpret_cast<void*> (&scanData), sizeof (scanData), NULL);
	else
		Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, reinterpret_cast<void*> (&scanData.scan), sizeof (scanData.scan), NULL);

	if (scanData.scan.ranges != NULL)
		delete[] scanData.scan.ranges;
	if (scanData.scan.intensity != NULL)
		delete[] scanData.scan.intensity;

	return 0;
}

bool RangerToLaser::HandleGeomRequest (player_laser_geom_t *dest, player_ranger_geom_t *data)
{
	dest->pose.px = data->pose.px;
	dest->pose.py = data->pose.py;
	dest->pose.pz = data->pose.pz;
	dest->pose.proll = data->pose.proll;
	dest->pose.ppitch = data->pose.ppitch;
	dest->pose.pyaw = data->pose.pyaw;

	dest->size.sw = data->size.sw;
	dest->size.sl = data->size.sl;
	dest->size.sh = data->size.sh;

	return true;
}

void RangerToLaser::CopyConfig (player_ranger_config_t *data)
{
	config.min_angle = static_cast<float> (data->min_angle);
	config.max_angle = static_cast<float> (data->max_angle);
	config.resolution = static_cast<float> (data->angular_res);
	config.max_range = static_cast<float> (data->max_range);
	config.range_res = static_cast<float> (data->range_res);
	config.scanning_frequency = static_cast<float> (data->frequency);
}

int RangerToLaser::ProcessMessage (QueuePointer &respQueue, player_msghdr *hdr, void *data)
{
	// Check the parent message handler
	if (FromRanger::ProcessMessage (respQueue, hdr, data) == 0)
		return 0;

	// Check capabilities requests
	HANDLE_CAPABILITY_REQUEST (device_addr, respQueue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_POWER);
	HANDLE_CAPABILITY_REQUEST (device_addr, respQueue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_GEOM);
	HANDLE_CAPABILITY_REQUEST (device_addr, respQueue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_SET_CONFIG);
	HANDLE_CAPABILITY_REQUEST (device_addr, respQueue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_CONFIG);

	// Messages from the laser interface
	// Power config request
	if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_POWER, device_addr))
	{
		// Tell the ranger device to switch the power state
		player_ranger_power_config_t req;
		req.state = reinterpret_cast<player_laser_power_config_t*> (data)->state;
		inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_POWER, &req, sizeof (req), 0);
		// Store the return queue
		ret_queue = respQueue;
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_GEOM, device_addr))
	{
		// Get geometry from the ranger device
		inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_GEOM, NULL, 0, NULL);
		ret_queue = respQueue;
		return 0;
	}
	// Config set request
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_SET_CONFIG, device_addr))
	{
		// Translate and forward the request to the ranger device
		player_laser_config_t *req = reinterpret_cast<player_laser_config_t*> (data);
		player_ranger_config_t translation;
		translation.min_angle = req->min_angle;
		translation.max_angle = req->max_angle;
		translation.angular_res = req->resolution;
		translation.max_range = req->max_range;
		translation.range_res = req->range_res;
		translation.frequency = req->scanning_frequency;
		inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_SET_CONFIG, &translation, sizeof (translation), 0);
		receivedCfgResp = false;
		// Also need to send the intensity configuration request
		player_ranger_intns_config_t intns;
		intns.state = req->intensity;
		config.intensity = req->intensity;
		inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_INTNS, &intns, sizeof (intns), 0);
		receivedIntnsResp = false;
		setConfigFailed = false;
		ret_queue = respQueue;
		return 0;
	}
	// Config get request
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_CONFIG, device_addr))
	{
		// Forward the request onto the ranger device, will handle the response when it comes
		inputDevice->PutMsg (InQueue, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_CONFIG, NULL, 0, NULL);
		ret_queue = respQueue;
		return 0;
	}

	// Messages from the ranger interface
	// Reqest ACKs
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_POWER, inputDeviceAddr))
	{
		// Power request
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LASER_REQ_POWER, NULL, 0, NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_SET_CONFIG, inputDeviceAddr))
	{
		if (setConfigFailed)
		{
			// Ignore this message if the intensity message failed
			receivedCfgResp = false;
			receivedIntnsResp = false;
			return 0;
		}

		CopyConfig (reinterpret_cast<player_ranger_config_t*> (data));
		if (receivedIntnsResp)
		{
			Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LASER_REQ_SET_CONFIG, &config, sizeof (config), NULL);
			receivedIntnsResp = false;
		}
		else
			receivedCfgResp = true;
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_GET_CONFIG, inputDeviceAddr))
	{
		CopyConfig (reinterpret_cast<player_ranger_config_t*> (data));
		// No need to send an ACK in startup mode, as this is our request
		if (startup)
			startup = false;
		else
			Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LASER_REQ_GET_CONFIG, &config, sizeof (config), NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_INTNS, inputDeviceAddr))
	{
		if (setConfigFailed)
		{
			// Ignore this message if the config message failed
			receivedCfgResp = false;
			receivedIntnsResp = false;
			return 0;
		}

		config.intensity = reinterpret_cast<player_ranger_intns_config_t*> (data)->state;
		if (receivedCfgResp)
		{
			Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LASER_REQ_SET_CONFIG, &config, sizeof (config), NULL);
			receivedCfgResp = false;
		}
		else
			receivedIntnsResp = true;
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_GET_GEOM, inputDeviceAddr))
	{
		// Geometry request - need to manage the info we just got
		player_laser_geom_t geom;
		if (HandleGeomRequest (&geom, reinterpret_cast<player_ranger_geom_t*> (data)))
			Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LASER_REQ_GET_GEOM, &geom, sizeof (geom), NULL);
		else
			Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_LASER_REQ_GET_GEOM, NULL, 0, NULL);
		return 0;
	}

	// Request NACKs
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_POWER, inputDeviceAddr))
	{
		// Power request
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_LASER_REQ_POWER, NULL, 0, NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_SET_CONFIG, inputDeviceAddr))
	{
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_LASER_REQ_SET_CONFIG, NULL, 0, NULL);
		setConfigFailed = true;
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_GET_CONFIG, inputDeviceAddr))
	{
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_LASER_REQ_GET_CONFIG, NULL, 0, NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_GET_GEOM, inputDeviceAddr))
	{
		// Geometry request
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_LASER_REQ_GET_GEOM, NULL, 0, NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_INTNS, inputDeviceAddr))
	{
		Publish (device_addr, ret_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_LASER_REQ_SET_CONFIG, NULL, 0, NULL);
		setConfigFailed = true;
		return 0;
	}

	// Data
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_DATA, -1, inputDeviceAddr))
	{
		return ConvertData (hdr, data);
	}

	return -1;
}

