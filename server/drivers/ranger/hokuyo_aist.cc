/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2008
 *     Geoffrey Biggs
 *
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

/*
 Desc: Wrapper driver around the Gearbox hokuyo_aist library (see http://gearbox.sourceforge.net)
 Author: Geoffrey Biggs
 Date: 20 June 2008
 CVS: $Id$
*/

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_hokuyo_aist hokuyo_aist
 * @brief Gearbox hokuyo_aist Hokuyo laser scanner driver library

 This driver provides a @ref interface_ranger interface to the hokuyo_aist Hokuyo laser scanner
 driver provided by Gearbox. Communication with the laser is via the Gearbox Flexiport library. The
 driver supports the SCIP protocol versions 1 and 2.

 @par Compile-time dependencies

 - Gearbox library hokuyo_aist
 - Gearbox library flexiport

 @par Provides

 - @ref interface_ranger : Output ranger interface

 @par Configuration requests

 - PLAYER_RANGER_REQ_GET_GEOM
 - PLAYER_RANGER_REQ_GET_CONFIG
 - PLAYER_RANGER_REQ_SET_CONFIG
 - Note: Only the min_angle, max_angle and frequency values can be configured using this request.
   In addition, the frequency value must be equivalent to a suitable RPM value (see the hokuyo_aist
   library documentation for suitable values).

 @par Configuration file options

 - portopts (string)
   - Default: "type=serial,device=/dev/ttyACM0,timeout=1"
   - Options to create the Flexiport port with.
 - pose (float 6-tuple: (m, m, m, rad, rad, rad))
   - Default: [0.0 0.0 0.0 0.0 0.0 0.0]
   - Pose (x, y, z, roll, pitch, yaw) of the laser relative to its parent object (e.g. the robot).
 - size (float 3-tuple: (m, m, m))
   - Default: [0.0 0.0 0.0]
   - Size of the laser in metres.
 - min_angle (float, radians)
   - Default: -2.08 rad (-119.0 degrees)
   - Minimum scan angle to return. Will be adjusted if outside the laser's scannable range.
 - max_angle (float, radians)
   - Default: 2.08 rad (119.0 degrees)
   - Maximum scan angle to return. Will be adjusted if outside the laser's scannable range.
 - frequency (float, Hz)
   - Default: 10Hz
   - The frequency at which the laser operates. This must be equivalent to a suitable RPM value. See
   - the hokuyo_aist library documentation for suitable values.
 - power (boolean)
   - Default: true
   - If true, the sensor power will be switched on upon driver activation (i.e. when the first
   client connects). Otherwise a power request must be made to turn it on before data will be
   received.
 - verbose (boolean)
   - Default: false
   - Enable verbose debugging information in the underlying library.

 @par Properties

 - baudrate (integer)
   - Default: 19200bps
   - Change the baud rate of the connection to the laser. See hokuyo_aist documentation for valid
     values.

 @par Example

 @verbatim
 driver
 (
     name "hokuyo_aist"
     provides ["ranger:0"]
     portopts "type=serial,device=/dev/ttyS0,timeout=1"
 )
 @endverbatim

 @author Geoffrey Biggs

 */
/** @} */

#include <string>

#include <hokuyo_aist/hokuyo_aist.h>
#include <libplayercore/playercore.h>

const int DEFAULT_BAUDRATE = 19200;
const int DEFAULT_SPEED = 600;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver object
////////////////////////////////////////////////////////////////////////////////////////////////////

class HokuyoDriver : public Driver
{
	public:
		HokuyoDriver (ConfigFile* cf, int section);
		~HokuyoDriver (void);

		virtual int Setup (void);
		virtual int Shutdown (void);
		virtual int ProcessMessage (QueuePointer &resp_queue, player_msghdr *hdr, void *data);

	private:
		virtual void Main (void);
		bool ReadLaser (void);
		bool AllocateDataSpace (void);

		// Configuration parameters
		bool _verbose, _powerOnStartup;
		int _frequency;
		double _minAngle, _maxAngle;
		IntProperty _baudRate;
		std::string _portOpts;
		// Geometry
		player_ranger_geom_t geom;
		player_pose3d_t sensorPose;
		player_bbox3d_t sensorSize;
		// The hardware device itself
		hokuyo_aist::HokuyoLaser _device;
		// Data storage
		hokuyo_aist::HokuyoData _data;
		double *_ranges;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor/destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

HokuyoDriver::HokuyoDriver (ConfigFile* cf, int section) :
	Driver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_RANGER_CODE),
	_baudRate ("baudrate", DEFAULT_BAUDRATE, false), _ranges (NULL)
{
	// Get the baudrate and motor speed
	RegisterProperty ("baudrate", &_baudRate, cf, section);

	// Get config
	_minAngle = cf->ReadFloat (section, "min_angle", -2.08);
	_maxAngle = cf->ReadFloat (section, "max_angle", 2.08);
	_frequency = cf->ReadInt (section, "frequency", 10);
	_portOpts = cf->ReadString (section, "portopts", "type=serial,device=/dev/ttyACM0,timeout=1");
	_verbose = cf->ReadBool (section, "verbose", false);
	_powerOnStartup = cf->ReadBool (section, "power", true);

	// Set up geometry information
	geom.pose.px = cf->ReadTupleLength (section, "pose", 0, 0.0);
	geom.pose.py = cf->ReadTupleLength (section, "pose", 1, 0.0);
	geom.pose.pz = cf->ReadTupleLength (section, "pose", 2, 0.0);
	geom.pose.proll = cf->ReadTupleAngle (section, "pose", 3, 0.0);
	geom.pose.ppitch = cf->ReadTupleAngle (section, "pose", 4, 0.0);
	geom.pose.pyaw = cf->ReadTupleAngle (section, "pose", 5, 0.0);
	geom.size.sw = cf->ReadTupleLength (section, "size", 0, 0.0);
	geom.size.sl = cf->ReadTupleLength (section, "size", 1, 0.0);
	geom.size.sh = cf->ReadTupleLength (section, "size", 2, 0.0);
	geom.sensor_poses_count = 1;
	geom.sensor_poses = &sensorPose;
	memcpy(geom.sensor_poses, &geom.pose, sizeof (geom.pose));
	geom.sensor_sizes_count = 1;
	geom.sensor_sizes = &sensorSize;
	memcpy(geom.sensor_sizes, &geom.size, sizeof (geom.size));

	// Turn on/off verbose mode
	_device.SetVerbose (_verbose);
}

HokuyoDriver::~HokuyoDriver (void)
{
	if (_ranges != NULL)
		delete[] _ranges;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

bool HokuyoDriver::AllocateDataSpace (void)
{
	if (_ranges != NULL)
		delete _ranges;

	int numRanges = _device.AngleToStep (_maxAngle) - _device.AngleToStep (_minAngle) + 1;
	if ((_ranges = new double[numRanges]) == NULL)
	{
		PLAYER_ERROR1 ("hokuyo_aist: Failed to allocate space for %d range readings.", numRanges);
		return false;
	}

	return true;
}

void HokuyoDriver::Main (void)
{
	while (true)
	{
		pthread_testcancel ();
		ProcessMessages ();

		if (!ReadLaser ())
			break;
	}
}

int HokuyoDriver::ProcessMessage (QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
	// Check for capability requests
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data,
			PLAYER_MSGTYPE_REQ, PLAYER_CAPABILTIES_REQ);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data,
			PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_GEOM);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data,
			PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_CONFIG);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data,
			PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_SET_CONFIG);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data,
			PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_POWER);

	// Property handlers that need to be done manually due to calling into the hokuyo_aist library.
	if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_INTPROP_REQ, this->device_addr))
	{
		player_intprop_req_t *req = reinterpret_cast<player_intprop_req_t*> (data);
		// Change in the baud rate
		if (strncmp (req->key, "baudrate", 8) == 0)
		{
			try
			{
				// Change the baud rate
				_device.SetBaud (req->value);
			}
			catch (hokuyo_aist::HokuyoError &e)
			{
				if (e.Code () != hokuyo_aist::HOKUYO_ERR_NOTSERIAL)
				{
					PLAYER_ERROR2 ("hokuyo_aist: Error while changing baud rate: (%d) %s",
							e.Code (), e.what ());
					SetError (e.Code ());
				}
				else
				{
					PLAYER_WARN (
						"hokuyo_aist: Cannot change the baud rate of a non-serial connection.");
				}

				Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_SET_INTPROP_REQ,
						NULL, 0, NULL);
				return 0;
			}
			_baudRate.SetValueFromMessage (data);
			Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SET_INTPROP_REQ, NULL,
					0, NULL);
			return 0;
		}
	}

	// Standard ranger messages
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_GEOM,
			device_addr))
	{
		Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_GET_GEOM,
				&geom, sizeof (geom), NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_CONFIG,
			device_addr))
	{
		player_ranger_config_t rangerConfig;
		hokuyo_aist::HokuyoSensorInfo info;
		_device.GetSensorInfo (&info);

		rangerConfig.min_angle = _minAngle; // These two are user-configurable
		rangerConfig.max_angle = _maxAngle;
		rangerConfig.resolution = info.resolution;
		rangerConfig.max_range = info.maxRange / 1000.0;
		rangerConfig.range_res = 0.001; // 1mm
		rangerConfig.frequency = info.speed / 60.0;
		Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_GET_CONFIG,
				&rangerConfig, sizeof (rangerConfig), NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_SET_CONFIG,
			device_addr))
	{
		player_ranger_config_t *newParams = reinterpret_cast<player_ranger_config_t*> (data);

		_minAngle = newParams->min_angle;
		_maxAngle = newParams->max_angle;
		if (!AllocateDataSpace ())
		{
			PLAYER_ERROR ("hokuyo_aist: Failed to allocate space for storing range data.");
			Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_GET_CONFIG,
					NULL, 0, NULL);
			return 0;
		}

		_frequency = static_cast<int> (newParams->frequency);
		try
		{
			hokuyo_aist::HokuyoSensorInfo info;
			_device.GetSensorInfo (&info);
			if (_minAngle < info.minAngle)
			{
				_minAngle = info.minAngle;
				PLAYER_WARN1 ("hokuyo_aist: Adjusted min_angle to %lf", _minAngle);
			}
			if (_maxAngle> info.maxAngle)
			{
				_maxAngle = info.maxAngle;
				PLAYER_WARN1 ("hokuyo_aist: Adjusted max_angle to %lf", _maxAngle);
			}
			_device.SetMotorSpeed (_frequency * 60);
		}
		catch (hokuyo_aist::HokuyoError &e)
		{
			PLAYER_ERROR2 ("hokuyo_aist: Library error while changing settings: (%d) %s", e.Code (),
					e.what ());
			SetError (e.Code ());
			Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_GET_CONFIG,
					NULL, 0, NULL);
			return 0;
		}

		Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_GET_CONFIG,
				newParams, sizeof (*newParams), NULL);
		return 0;
	}

	return -1;
}

bool HokuyoDriver::ReadLaser (void)
{
	player_ranger_data_range_t rangeData;

	try
	{
		_device.GetRanges (&_data, _minAngle, _maxAngle);
	}
	catch (hokuyo_aist::HokuyoError &e)
	{
		PLAYER_ERROR2 ("hokuyo_aist: Failed to read scan: (%d) %s", e.Code (), e.what ());
		SetError (e.Code ());
		return false;
	}

	for (unsigned int ii = 0; ii < _data.Length (); ii++)
		_ranges[ii] = _data[ii] / 1000.0f;
	rangeData.ranges = _ranges;
	rangeData.ranges_count = _data.Length ();
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE,
			reinterpret_cast<void*> (&rangeData), sizeof (rangeData), NULL);

	return true;
}

int HokuyoDriver::Setup (void)
{
	try
	{
		// Open the laser
		_device.Open (_portOpts);
		// Get the sensor information and check _minAngle and _maxAngle are OK
		hokuyo_aist::HokuyoSensorInfo info;
		_device.GetSensorInfo (&info);
		if (_minAngle < info.minAngle)
		{
			_minAngle = info.minAngle;
			PLAYER_WARN1 ("hokuyo_aist: Adjusted min_angle to %lf", _minAngle);
		}
		if (_maxAngle> info.maxAngle)
		{
			_maxAngle = info.maxAngle;
			PLAYER_WARN1 ("hokuyo_aist: Adjusted max_angle to %lf", _maxAngle);
		}
		if (!AllocateDataSpace ())
			return -1;

		if (_powerOnStartup)
			_device.SetPower (true);

		try
		{
			_device.SetBaud (_baudRate.GetValue ());
		}
		catch (hokuyo_aist::HokuyoError &e)
		{
			if (e.Code () != hokuyo_aist::HOKUYO_ERR_NOTSERIAL)
				throw;
			PLAYER_WARN ("hokuyo_aist: Cannot change the baud rate of a non-serial connection.");
		}
	}
	catch (hokuyo_aist::HokuyoError &e)
	{
		PLAYER_ERROR2 ("hokuyo_aist: Failed to setup laser driver: (%d) %s", e.Code (), e.what ());
		SetError (e.Code ());
		return -1;
	}

	StartThread ();
	return 0;
}

int HokuyoDriver::Shutdown (void)
{
	StopThread ();

	_device.Close ();
	_data.CleanUp ();
	if (_ranges != NULL)
		delete[] _ranges;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver management functions
////////////////////////////////////////////////////////////////////////////////////////////////////

Driver* HokuyoDriver_Init (ConfigFile* cf, int section)
{
	return reinterpret_cast <Driver*> (new HokuyoDriver (cf, section));
}

void hokuyo_aist_Register (DriverTable* table)
{
	table->AddDriver ("hokuyo_aist", HokuyoDriver_Init);
}
