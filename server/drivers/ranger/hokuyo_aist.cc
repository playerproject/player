/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2008
 *     Geoffrey Biggs
 *
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version
 * 2 of the License, or (at your option) any later version.
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

 - PLAYER_RANGER_REQ_INTNS
 - PLAYER_RANGER_REQ_POWER
 - PLAYER_RANGER_REQ_GET_GEOM
 - PLAYER_RANGER_REQ_GET_CONFIG
 - PLAYER_RANGER_REQ_SET_CONFIG
 - Note: Only the min_angle and max_angle values can be configured using this request. To change
   the scanning frequency, use the speed_level property.

 @par Configuration file options

 - get_intensities (boolean)
   - Default: false
   - Set to non-zero to get intensity data with each range scan on models that support it. This can
     also be enabled/disabled using the PLAYER_RANGER_REQ_INTNS message. Note that the data
     retrieval mode used to get intensity data requires that the scan is performed *after* the
     command is received, so this will introduce a slight delay before the data is delivered.
 - portopts (string)
   - Default: "type=serial,device=/dev/ttyACM0,timeout=1"
   - Options to create the Flexiport port with. Note that any baud rate specified in this line
     should be the scanner's startup baud rate.
 - pose (float 6-tuple: (m, m, m, rad, rad, rad))
   - Default: [0.0 0.0 0.0 0.0 0.0 0.0]
   - Pose (x, y, z, roll, pitch, yaw) of the laser relative to its parent object (e.g. the robot).
 - size (float 3-tuple: (m, m, m))
   - Default: [0.0 0.0 0.0]
   - Size of the laser in metres.
 - min_angle (float, radians)
   - Default: -4.0 rad (Will use laser default)
   - Minimum scan angle to return. Will be adjusted if outside the laser's scannable range.
 - max_angle (float, radians)
   - Default: 4.0 rad (Will use laser default)
   - Maximum scan angle to return. Will be adjusted if outside the laser's scannable range.
 - power (boolean)
   - Default: true
   - If true, the sensor power will be switched on upon driver activation (i.e. when the first
     client connects). Otherwise a power request must be made to turn it on before data will be
     received.
 - verbose (boolean)
   - Default: false
   - Enable verbose debugging information in the underlying library.
 - ignoreunknowns (boolean)
   - Default: false
   - Ignore unknown lines sent by the laser in response to sensor info request commands.

 @par Properties

 - baud_rate (integer)
   - Default: 19200bps
   - Change the baud rate of the connection to the laser. See hokuyo_aist documentation for valid
     values. This is separate from the scanner's power-on default baud rate, which should be
     specified in portopts.
 - speed_level (integer, 0 to 10 or 99)
   - Default: 0
   - The speed at which the laser operates, as a level down from maximum speed. See the hokuyo_aist
     library documentation for suitable values.
 - high_sensitivity (integer)
   - Default: 0
   - Set to non-zero to enable high sensitivity mode on models that support it.
 - min_dist (float, metres)
   - Default: 0m
   - Minimum possible distance. Below that means there is an error (a scratch on the laser for
     instance). The reading is then adjusted to the average of the neighboring valid beams.
 - hw_timestamps (boolean)
   - Default: false
   - When false, the server will use server time stamps in data messages. When true, the time stamp
     in the laser data will be used.

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
const int DEFAULT_SPEED_LEVEL = 0;
const int DEFAULT_SENSITIVITY = 0;
const int DEFAULT_GET_INTENSITIES = 0;
const double DEFAULT_MIN_DIST = 0.0;
const bool DEFAULT_TIMESTAMPS = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver object
////////////////////////////////////////////////////////////////////////////////////////////////////

class HokuyoDriver : public ThreadedDriver
{
	public:
		HokuyoDriver (ConfigFile* cf, int section);
		~HokuyoDriver (void);

		virtual int MainSetup (void);
		virtual void MainQuit (void);
		virtual int ProcessMessage (QueuePointer &resp_queue, player_msghdr *hdr, void *data);

	private:
		virtual void Main (void);
		bool ReadLaser (void);
		bool AllocateDataSpace (void);

		// Configuration parameters
		bool _verbose, _powerOnStartup, _getIntensities, _ignoreUnknowns;
		double _minAngle, _maxAngle;
		IntProperty _baudRate, _speedLevel, _highSensitivity;
		DoubleProperty _minDist;
		BoolProperty _hwTimeStamps;
		std::string _portOpts;
		// Geometry
		player_ranger_geom_t _geom;
		player_pose3d_t _sensorPose;
		player_bbox3d_t _sensorSize;
		// The hardware device itself
		hokuyo_aist::HokuyoLaser _device;
		// Data storage
		hokuyo_aist::HokuyoData _data;
		double *_ranges;
		double *_intensities;
		int _numRanges;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor/destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

HokuyoDriver::HokuyoDriver (ConfigFile* cf, int section) :
	ThreadedDriver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_RANGER_CODE),
	_baudRate ("baud_rate", DEFAULT_BAUDRATE, false),
	_speedLevel ("speed_level", DEFAULT_SPEED_LEVEL, false),
	_highSensitivity ("high_sensitivity", DEFAULT_SENSITIVITY, false),
	_minDist ("min_dist", DEFAULT_MIN_DIST, false),
	_hwTimeStamps ("hw_timestamps", DEFAULT_TIMESTAMPS, false),
	_ranges (NULL), _intensities (NULL)
{
	// Get the baudrate, speed and sensitivity
	RegisterProperty ("baud_rate", &_baudRate, cf, section);
	RegisterProperty ("speed_level", &_speedLevel, cf, section);
	RegisterProperty ("high_sensitivity", &_highSensitivity, cf, section);
	RegisterProperty ("min_dist", &_minDist, cf, section);
	RegisterProperty ("hw_timestamps", &_hwTimeStamps, cf, section);

	// Get config
	_getIntensities = cf->ReadBool (section, "get_intensities", false);
	_minAngle = cf->ReadFloat (section, "min_angle", -4.0);
	_maxAngle = cf->ReadFloat (section, "max_angle", 4.0);
	_portOpts = cf->ReadString (section, "portopts", "type=serial,device=/dev/ttyACM0,timeout=1");
	_verbose = cf->ReadBool (section, "verbose", false);
	_ignoreUnknowns = cf->ReadBool (section, "ignoreunknowns", false);
	_powerOnStartup = cf->ReadBool (section, "power", true);

	// Set up geometry information
	_geom.pose.px = cf->ReadTupleLength (section, "pose", 0, 0.0);
	_geom.pose.py = cf->ReadTupleLength (section, "pose", 1, 0.0);
	_geom.pose.pz = cf->ReadTupleLength (section, "pose", 2, 0.0);
	_geom.pose.proll = cf->ReadTupleAngle (section, "pose", 3, 0.0);
	_geom.pose.ppitch = cf->ReadTupleAngle (section, "pose", 4, 0.0);
	_geom.pose.pyaw = cf->ReadTupleAngle (section, "pose", 5, 0.0);
	_geom.size.sw = cf->ReadTupleLength (section, "size", 0, 0.0);
	_geom.size.sl = cf->ReadTupleLength (section, "size", 1, 0.0);
	_geom.size.sh = cf->ReadTupleLength (section, "size", 2, 0.0);
	_geom.element_poses_count = 1;
	_geom.element_poses = &_sensorPose;
	memcpy(_geom.element_poses, &_geom.pose, sizeof (_geom.pose));
	_geom.element_sizes_count = 1;
	_geom.element_sizes = &_sensorSize;
	memcpy(_geom.element_sizes, &_geom.size, sizeof (_geom.size));

	// Turn on/off verbose mode
	_device.SetVerbose (_verbose);
}

HokuyoDriver::~HokuyoDriver (void)
{
	if (_ranges != NULL)
		delete[] _ranges;
	if (_intensities != NULL)
		delete[] _intensities;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

bool HokuyoDriver::AllocateDataSpace (void)
{
	if (_ranges != NULL)
		delete _ranges;

	_numRanges = _device.AngleToStep (_maxAngle) - _device.AngleToStep (_minAngle) + 1;
	if ((_ranges = new double[_numRanges]) == NULL)
	{
		PLAYER_ERROR1 ("hokuyo_aist: Failed to allocate space for %d range readings.", _numRanges);
		return false;
	}

	if (_getIntensities)
	{
		if ((_intensities = new double[_numRanges]) == NULL)
		{
			PLAYER_ERROR1 ("hokuyo_aist: Failed to allocate space for %d intensity readings.",
						_numRanges);
			return false;
		}
	}

	return true;
}

void HokuyoDriver::Main (void)
{
	while (true)
	{
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
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data,
			PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_INTNS);

	// Property handlers that need to be done manually due to calling into the hokuyo_aist library.
	if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_INTPROP_REQ, this->device_addr))
	{
		player_intprop_req_t *req = reinterpret_cast<player_intprop_req_t*> (data);
		// Change in the baud rate
		if (strncmp (req->key, "baud_rate", 9) == 0)
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
		else if (strncmp (req->key, "speed_level", 11) == 0)
		{
			try
			{
				_device.SetMotorSpeed (req->value);
			}
			catch (hokuyo_aist::HokuyoError &e)
			{
				PLAYER_ERROR2 ("hokuyo_aist: Error while changing motor speed: (%d) %s",
						e.Code (), e.what ());
				SetError (e.Code ());
				Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_SET_INTPROP_REQ,
						NULL, 0, NULL);
				return 0;
			}
			_speedLevel.SetValueFromMessage (data);
			Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SET_INTPROP_REQ, NULL,
					0, NULL);
			return 0;
		}
		else if (strncmp (req->key, "high_sensitivity", 16) == 0)
		{
			try
			{
				_device.SetHighSensitivity (req->value != 0);
			}
			catch (hokuyo_aist::HokuyoError &e)
			{
				PLAYER_ERROR2 ("hokuyo_aist: Error while changing sensitivity: (%d) %s",
						e.Code (), e.what ());
				SetError (e.Code ());
				Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_SET_INTPROP_REQ,
						NULL, 0, NULL);
				return 0;
			}
			_highSensitivity.SetValueFromMessage (data);
			Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SET_INTPROP_REQ, NULL,
					0, NULL);
			return 0;
		}
	}

	// Standard ranger messages
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_POWER, device_addr))
	{
		player_ranger_power_config_t *config =
			reinterpret_cast<player_ranger_power_config_t*> (data);
		try
		{
			if (config->state)
				_device.SetPower (true);
			else
				_device.SetPower (false);
		}
		catch (hokuyo_aist::HokuyoError &e)
		{
			PLAYER_ERROR2 ("hokuyo_aist: Error while setting power state: (%d) %s",
					e.Code (), e.what ());
			SetError (e.Code ());
			Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_POWER,
					NULL, 0, NULL);
		}
		Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_POWER, NULL,
				0, NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_INTNS, device_addr))
	{
		bool newValue = (reinterpret_cast<player_ranger_intns_config_t*> (data)->state != 0);
		if (newValue && !_getIntensities)
		{
			// State change, allocate space for intensity data
			if ((_intensities = new double[_numRanges]) == NULL)
			{
				PLAYER_ERROR1 ("hokuyo_aist: Failed to allocate space for %d intensity readings.",
							_numRanges);
				Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_RANGER_REQ_INTNS,
						NULL, 0, NULL);
				return 0;
			}
		}
		else if (!newValue && _getIntensities)
		{
			// State change, remove allocated space
			delete[] _intensities;
			_intensities = NULL;
		}
		_getIntensities = newValue;
		Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_INTNS, NULL,
				0, NULL);
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_GEOM,
			device_addr))
	{
		Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_RANGER_REQ_GET_GEOM,
				&_geom, sizeof (_geom), NULL);
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
		rangerConfig.angular_res = info.resolution;
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
	double time1, time2;
	if (_getIntensities)
	{
		player_ranger_data_range_t rangeData;
		player_ranger_data_intns_t intensityData;

		try
		{
			GlobalTime->GetTimeDouble (&time1);
			_device.GetNewRangesAndIntensitiesByAngle (&_data, _minAngle, _maxAngle);
			GlobalTime->GetTimeDouble (&time2);
			time1 = (time1 + time2) / 2.0;
		}
		catch (hokuyo_aist::HokuyoError &e)
		{
			PLAYER_ERROR2 ("hokuyo_aist: Failed to read scan: (%d) %s", e.Code (), e.what ());
			SetError (e.Code ());
			return false;
		}

		double lastValidValue = _minDist;
		for (unsigned int ii = 0; ii < _data.Length (); ii++)
		{
			_ranges[ii] = _data[ii] / 1000.0f;
			_intensities[ii] = _data.Intensities ()[ii];
			if (_minDist > 0)
			{
				if (_ranges[ii] < _minDist)
					_ranges[ii] = lastValidValue;
				else
					lastValidValue = _ranges[ii];
			}
		}

		rangeData.ranges = _ranges;
		rangeData.ranges_count = _data.Length ();
		if (_hwTimeStamps.GetValue ())
		{
			double ts = _data.TimeStamp () / 1000.0;
			Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE,
					reinterpret_cast<void*> (&rangeData), sizeof (rangeData), &ts);
		}
		else
		{
			Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE,
					reinterpret_cast<void*> (&rangeData), sizeof (rangeData), &time1);
		}

		intensityData.intensities = _intensities;
		intensityData.intensities_count = _data.Length ();
		if (_hwTimeStamps.GetValue ())
		{
			double ts = _data.TimeStamp () / 1000.0;
			Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_INTNS,
					reinterpret_cast<void*> (&intensityData), sizeof (intensityData), &ts);
		}
		else
		{
			Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_INTNS,
					reinterpret_cast<void*> (&intensityData), sizeof (intensityData), &time1);
		}
	}
	else
	{
		player_ranger_data_range_t rangeData;

		try
		{
			GlobalTime->GetTimeDouble (&time1);
			_device.GetRangesByAngle (&_data, _minAngle, _maxAngle);
			GlobalTime->GetTimeDouble (&time2);
			time1 = (time1 + time2) / 2.0;
		}
		catch (hokuyo_aist::HokuyoError &e)
		{
			PLAYER_ERROR2 ("hokuyo_aist: Failed to read scan: (%d) %s", e.Code (), e.what ());
			SetError (e.Code ());
			return false;
		}

		double lastValidValue = _minDist;
		for (unsigned int ii = 0; ii < _data.Length (); ii++)
		{
			_ranges[ii] = _data[ii] / 1000.0f;
			if (_minDist > 0)
			{
				if (_ranges[ii] < _minDist)
					_ranges[ii] = lastValidValue;
				else
					lastValidValue = _ranges[ii];
			}
		}
		rangeData.ranges = _ranges;
		rangeData.ranges_count = _data.Length ();
		if (_hwTimeStamps.GetValue ())
		{
			double ts = _data.TimeStamp () / 1000.0;
			Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE,
					reinterpret_cast<void*> (&rangeData), sizeof (rangeData), &ts);
		}
		else
		{
			Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE,
					reinterpret_cast<void*> (&rangeData), sizeof (rangeData), &time1);
		}
	}

	return true;
}

int HokuyoDriver::MainSetup (void)
{
	try
	{
		_device.IgnoreUnknowns (_ignoreUnknowns);
		// Open the laser
		_device.OpenWithProbing (_portOpts);
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
			if (e.Code () == hokuyo_aist::HOKUYO_ERR_NOTSERIAL)
				PLAYER_WARN ("hokuyo_aist: Cannot change the baud rate of a non-serial connection.");
			else
				PLAYER_WARN2 ("hokuyo_aist: Error changing baud rate: (%d) %s", e.Code (), e.what ());
		}
		try
		{
			// Catch any errors here as this is an optional setting not supported by all models
			_device.SetMotorSpeed (_speedLevel.GetValue ());
		}
		catch (hokuyo_aist::HokuyoError &e)
		{
			PLAYER_WARN2 ("hokuyo_aist: Unable to set motor speed: (%d) %s", e.Code (), e.what ());
		}
		try
		{
			// Optional setting
			_device.SetHighSensitivity (_highSensitivity.GetValue () != 0);
		}
		catch (hokuyo_aist::HokuyoError &e)
		{
			PLAYER_WARN2 ("hokuyo_aist: Unable to set sensitivity: (%d) %s", e.Code (), e.what ());
		}
	}
	catch (hokuyo_aist::HokuyoError &e)
	{
		PLAYER_ERROR2 ("hokuyo_aist: Failed to setup laser driver: (%d) %s", e.Code (), e.what ());
		SetError (e.Code ());
		return -1;
	}
	return 0;
}

void HokuyoDriver::MainQuit (void)
{
	_device.Close ();
	_data.CleanUp ();
	if (_ranges != NULL)
	{
		delete[] _ranges;
		_ranges = NULL;
	}
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
