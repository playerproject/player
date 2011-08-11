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
 Desc: Wrapper driver around the HokuyoAIST library (see https://github.com/gbiggs/hokuyoaist)
 Author: Geoffrey Biggs
 Date: 20 June 2008
 CVS: $Id$
*/

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_hokuyoaist hokuyoaist
 * @brief HokuyoAIST Hokuyo laser scanner driver library

 This driver provides a @ref interface_ranger interface to the HokuyoAIST Hokuyo laser scanner
 driver. Communication with the laser is via the Flexiport library. The
 driver supports the SCIP protocol versions 1 and 2.

 @par Compile-time dependencies

 - HokuyoAIST 3.0
 - Flexiport 2.0

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
 - invert (boolean)
   - Default: false
   - If true, the reading will be inverted (i.e. read backwards). Useful if the laser scanner is
     mounted upside down.
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
   - Change the baud rate of the connection to the laser. See HokuyoAIST documentation for valid
     values. This is separate from the scanner's power-on default baud rate, which should be
     specified in portopts.
 - speed_level (integer, 0 to 10 or 99)
   - Default: 0
   - The speed at which the laser operates, as a level down from maximum speed. See the HokuyoAIST
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
     in the laser data will be used, offset to the current system time.

 @par Example

 @verbatim
 driver
 (
     name "hokuyoaist"
     provides ["ranger:0"]
     portopts "type=serial,device=/dev/ttyS0,timeout=1"
 )
 @endverbatim

 @author Geoffrey Biggs

 */
/** @} */

#include <string>

#include <hokuyoaist/hokuyoaist.h>
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
        HokuyoDriver(ConfigFile* cf, int section);
        ~HokuyoDriver(void);

        virtual int MainSetup(void);
        virtual void MainQuit(void);
        virtual int ProcessMessage(QueuePointer &resp_queue, player_msghdr
                *hdr, void *data);

    private:
        virtual void Main(void);
        bool ReadLaser(void);
        bool AllocateDataSpace(void);

        // Configuration parameters
        bool verbose_, invert_, powerOnStartup_, getIntensities_,
             ignoreUnknowns_;
        double minAngle_, maxAngle_;
        IntProperty baudRate_, speedLevel_, highSensitivity_;
        DoubleProperty minDist_;
        BoolProperty hwTimeStamps_;
        std::string portOpts_;
        // Geometry
        player_ranger_geom_t geom_;
        player_pose3d_t sensorPose_;
        player_bbox3d_t sensorSize_;
        // The hardware device itself
        hokuyoaist::Sensor device_;
        // Data storage
        hokuyoaist::ScanData data_;
        double *_ranges;
        double *_intensities;
        int _numRanges;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor/destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

HokuyoDriver::HokuyoDriver(ConfigFile* cf, int section) :
    ThreadedDriver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN,
            PLAYER_RANGER_CODE),
    baudRate_("baud_rate", DEFAULT_BAUDRATE, false),
    speedLevel_("speed_level", DEFAULT_SPEED_LEVEL, false),
    highSensitivity_("high_sensitivity", DEFAULT_SENSITIVITY, false),
    minDist_("min_dist", DEFAULT_MIN_DIST, false),
    hwTimeStamps_("hw_timestamps", DEFAULT_TIMESTAMPS, false),
    _ranges(NULL), _intensities(NULL)
{
    // Get the baudrate, speed and sensitivity
    RegisterProperty("baud_rate", &baudRate_, cf, section);
    RegisterProperty("speed_level", &speedLevel_, cf, section);
    RegisterProperty("high_sensitivity", &highSensitivity_, cf, section);
    RegisterProperty("min_dist", &minDist_, cf, section);
    RegisterProperty("hw_timestamps", &hwTimeStamps_, cf, section);

    // Get config
    getIntensities_ = cf->ReadBool(section, "get_intensities", false);
    minAngle_ = cf->ReadFloat(section, "min_angle", -4.0);
    maxAngle_ = cf->ReadFloat(section, "max_angle", 4.0);
    invert_ = cf->ReadBool(section, "invert", false);
    portOpts_ = cf->ReadString(section, "portopts",
            "type=serial,device=/dev/ttyACM0,timeout=1");
    verbose_ = cf->ReadBool(section, "verbose", false);
    ignoreUnknowns_ = cf->ReadBool(section, "ignoreunknowns", false);
    powerOnStartup_ = cf->ReadBool(section, "power", true);

    // Set up geometry information
    geom_.pose.px = cf->ReadTupleLength(section, "pose", 0, 0.0);
    geom_.pose.py = cf->ReadTupleLength(section, "pose", 1, 0.0);
    geom_.pose.pz = cf->ReadTupleLength(section, "pose", 2, 0.0);
    geom_.pose.proll = cf->ReadTupleAngle(section, "pose", 3, 0.0);
    geom_.pose.ppitch = cf->ReadTupleAngle(section, "pose", 4, 0.0);
    geom_.pose.pyaw = cf->ReadTupleAngle(section, "pose", 5, 0.0);
    geom_.size.sw = cf->ReadTupleLength(section, "size", 0, 0.0);
    geom_.size.sl = cf->ReadTupleLength(section, "size", 1, 0.0);
    geom_.size.sh = cf->ReadTupleLength(section, "size", 2, 0.0);
    geom_.element_poses_count = 1;
    geom_.element_poses = &sensorPose_;
    memcpy(geom_.element_poses, &geom_.pose, sizeof(geom_.pose));
    geom_.element_sizes_count = 1;
    geom_.element_sizes = &sensorSize_;
    memcpy(geom_.element_sizes, &geom_.size, sizeof(geom_.size));

    // Turn on/off verbose mode
    device_.set_verbose(verbose_);
}

HokuyoDriver::~HokuyoDriver(void)
{
    if (_ranges != NULL)
        delete[] _ranges;
    if (_intensities != NULL)
        delete[] _intensities;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

bool HokuyoDriver::AllocateDataSpace(void)
{
    if (_ranges != NULL)
        delete _ranges;

    _numRanges = device_.angle_to_step(maxAngle_) -
        device_.angle_to_step(minAngle_) + 1;
    if ((_ranges = new double[_numRanges]) == NULL)
    {
        PLAYER_ERROR1("HokuyoAIST: Failed to allocate space for %d range readings.",
                _numRanges);
        return false;
    }

    if (getIntensities_)
    {
        if ((_intensities = new double[_numRanges]) == NULL)
        {
            PLAYER_ERROR1("HokuyoAIST: Failed to allocate space for %d intensity readings.",
                        _numRanges);
            return false;
        }
    }

    return true;
}

void HokuyoDriver::Main(void)
{
    while (true)
    {
        ProcessMessages();

        if (!ReadLaser())
            break;
    }
}

int HokuyoDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr,
        void *data)
{
    // Check for capability requests
    HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data,
            PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
    HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data,
            PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_GEOM);
    HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data,
            PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_GET_CONFIG);
    HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data,
            PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_SET_CONFIG);
    HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data,
            PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_POWER);
    HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data,
            PLAYER_MSGTYPE_REQ, PLAYER_RANGER_REQ_INTNS);

    // Property handlers that need to be done manually due to calling into the
    // HokuyoAIST library.
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_INTPROP_REQ,
                this->device_addr))
    {
        player_intprop_req_t *req =
            reinterpret_cast<player_intprop_req_t*> (data);
        // Change in the baud rate
        if (strncmp(req->key, "baud_rate", 9) == 0)
        {
            try
            {
                // Change the baud rate
                device_.set_baud(req->value);
            }
            catch(hokuyoaist::NotSerialError)
            {
                PLAYER_WARN(
                    "HokuyoAIST: Cannot change the baud rate of a non-serial connection.");
                Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                        PLAYER_SET_INTPROP_REQ, NULL, 0, NULL);
                return 0;
            }
            catch(hokuyoaist::BaseError &e)
            {
                PLAYER_ERROR1("HokuyoAIST: Error while changing baud rate: %s",
                        e.what());
                SetError(-1);

                Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                        PLAYER_SET_INTPROP_REQ, NULL, 0, NULL);
                return 0;
            }
            baudRate_.SetValueFromMessage(data);
            Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
                    PLAYER_SET_INTPROP_REQ, NULL, 0, NULL);
            return 0;
        }
        else if (strncmp(req->key, "speed_level", 11) == 0)
        {
            try
            {
                device_.set_motor_speed(req->value);
            }
            catch(hokuyoaist::BaseError &e)
            {
                PLAYER_ERROR1("HokuyoAIST: Error while changing motor speed: %s",
                        e.what());
                SetError(-1);
                Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                        PLAYER_SET_INTPROP_REQ, NULL, 0, NULL);
                return 0;
            }
            speedLevel_.SetValueFromMessage(data);
            Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SET_INTPROP_REQ, NULL,
                    0, NULL);
            return 0;
        }
        else if (strncmp(req->key, "high_sensitivity", 16) == 0)
        {
            try
            {
                device_.set_high_sensitivity(req->value != 0);
            }
            catch(hokuyoaist::BaseError &e)
            {
                PLAYER_ERROR1(
                        "HokuyoAIST: Error while changing sensitivity: %s",
                        e.what());
                SetError(-1);
                Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                        PLAYER_SET_INTPROP_REQ, NULL, 0, NULL);
                return 0;
            }
            highSensitivity_.SetValueFromMessage(data);
            Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
                    PLAYER_SET_INTPROP_REQ, NULL, 0, NULL);
            return 0;
        }
    }

    // Standard ranger messages
    else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                PLAYER_RANGER_REQ_POWER, device_addr))
    {
        player_ranger_power_config_t *config =
            reinterpret_cast<player_ranger_power_config_t*> (data);
        try
        {
            if (config->state)
                device_.set_power(true);
            else
                device_.set_power(false);
        }
        catch(hokuyoaist::BaseError &e)
        {
            PLAYER_ERROR1("HokuyoAIST: Error while setting power state: %s",
                    e.what());
            SetError(-1);
            Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                    PLAYER_RANGER_REQ_POWER, NULL, 0, NULL);
        }
        Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
                PLAYER_RANGER_REQ_POWER, NULL, 0, NULL);
        return 0;
    }
    else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                PLAYER_RANGER_REQ_INTNS, device_addr))
    {
        bool newValue =
            (reinterpret_cast<player_ranger_intns_config_t*>(data)->state != 0);
        if (newValue && !getIntensities_)
        {
            // State change, allocate space for intensity data
            if ((_intensities = new double[_numRanges]) == NULL)
            {
                PLAYER_ERROR1(
                    "HokuyoAIST: Failed to allocate space for %d intensity readings.",
                    _numRanges);
                Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                        PLAYER_RANGER_REQ_INTNS, NULL, 0, NULL);
                return 0;
            }
        }
        else if (!newValue && getIntensities_)
        {
            // State change, remove allocated space
            delete[] _intensities;
            _intensities = NULL;
        }
        getIntensities_ = newValue;
        Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
                PLAYER_RANGER_REQ_INTNS, NULL, 0, NULL);
        return 0;
    }
    else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                PLAYER_RANGER_REQ_GET_GEOM, device_addr))
    {
        Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
                PLAYER_RANGER_REQ_GET_GEOM, &geom_, sizeof(geom_), NULL);
        return 0;
    }
    else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                PLAYER_RANGER_REQ_GET_CONFIG, device_addr))
    {
        player_ranger_config_t rangerConfig;
        hokuyoaist::SensorInfo info;
        device_.get_sensor_info(info);

        if (!invert_)
        {
            rangerConfig.min_angle = minAngle_; // These two are user-configurable
            rangerConfig.max_angle = maxAngle_;
        }
        else
        {
            rangerConfig.min_angle = -maxAngle_;
            rangerConfig.max_angle = -minAngle_;
        }

        rangerConfig.angular_res = info.resolution;
        rangerConfig.min_range = info.min_range / 1000.0;
        rangerConfig.max_range = info.max_range / 1000.0;
        rangerConfig.range_res = 0.001; // 1mm
        rangerConfig.frequency = info.speed / 60.0;
        Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
                PLAYER_RANGER_REQ_GET_CONFIG, &rangerConfig,
                sizeof(rangerConfig), NULL);
        return 0;
    }
    else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                PLAYER_RANGER_REQ_SET_CONFIG, device_addr))
    {
        player_ranger_config_t *newParams =
            reinterpret_cast<player_ranger_config_t*> (data);

        if (!invert_)
        {
            minAngle_ = newParams->min_angle;
            maxAngle_ = newParams->max_angle;
        }
        else
        {
            minAngle_ = -newParams->max_angle;
            maxAngle_ = -newParams->min_angle;
        }

        if (!AllocateDataSpace())
        {
            PLAYER_ERROR(
                "HokuyoAIST: Failed to allocate space for storing range data.");
            Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                    PLAYER_RANGER_REQ_GET_CONFIG, NULL, 0, NULL);
            return 0;
        }

        try
        {
            hokuyoaist::SensorInfo info;
            device_.get_sensor_info(info);
            if (minAngle_ < info.min_angle)
            {
                minAngle_ = info.min_angle;
                PLAYER_WARN1("HokuyoAIST: Adjusted min_angle to %lf", minAngle_);
            }
            if (maxAngle_> info.max_angle)
            {
                maxAngle_ = info.max_angle;
                PLAYER_WARN1("HokuyoAIST: Adjusted max_angle to %lf", maxAngle_);
            }
        }
        catch(hokuyoaist::BaseError &e)
        {
            PLAYER_ERROR1(
                "HokuyoAIST: Library error while changing settings: %s",
                e.what());
            SetError(-1);
            Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                    PLAYER_RANGER_REQ_GET_CONFIG, NULL, 0, NULL);
            return 0;
        }

        Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
                PLAYER_RANGER_REQ_GET_CONFIG, newParams, sizeof(*newParams),
                NULL);
        return 0;
    }

    return -1;
}

bool HokuyoDriver::ReadLaser(void)
{
    double time1, time2;
    if (getIntensities_)
    {
        player_ranger_data_range_t rangeData;
        player_ranger_data_intns_t intensityData;

        try
        {
            GlobalTime->GetTimeDouble(&time1);
            device_.get_new_ranges_intensities_by_angle(data_, minAngle_,
                    maxAngle_);
            GlobalTime->GetTimeDouble(&time2);
            time1 = (time1 + time2) / 2.0;
        }
        catch(hokuyoaist::BaseError &e)
        {
            PLAYER_ERROR1("HokuyoAIST: Failed to read scan: %s", e.what());
            SetError(-1);
            return false;
        }

        double lastValidValue = minDist_;
        if (!invert_)
        {
            for (unsigned int ii = 0; ii < data_.ranges_length(); ii++)
            {
                _ranges[ii] = data_[ii] / 1000.0f;
                _intensities[ii] = data_.intensities()[ii];
                if (minDist_ > 0)
                {
                    if (_ranges[ii] < minDist_)
                        _ranges[ii] = lastValidValue;
                    else
                        lastValidValue = _ranges[ii];
                }
            }
        }
        else  // Invert
        {
            for (unsigned int ii = 0; ii < data_.ranges_length(); ii++)
            {
                unsigned int jj = data_.ranges_length() - 1 - ii;
                _ranges[ii] = data_[jj] / 1000.0f;
                _intensities[ii] = data_.intensities()[jj];
                if (minDist_ > 0)
                {
                    if (_ranges[ii] < minDist_)
                        _ranges[ii] = lastValidValue;
                    else
                        lastValidValue = _ranges[ii];
                }
            }
        }

        rangeData.ranges = _ranges;
        rangeData.ranges_count = data_.ranges_length();
        if (hwTimeStamps_.GetValue())
        {
            double ts = data_.system_time_stamp() / 1000000000.0;
            Publish(device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE,
                    reinterpret_cast<void*>(&rangeData), sizeof(rangeData),
                    &ts);
        }
        else
        {
            Publish(device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE,
                    reinterpret_cast<void*>(&rangeData), sizeof(rangeData),
                    &time1);
        }

        intensityData.intensities = _intensities;
        intensityData.intensities_count = data_.intensities_length();
        if (hwTimeStamps_.GetValue())
        {
            double ts = data_.system_time_stamp() / 1000000000.0;
            Publish(device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_INTNS,
                    reinterpret_cast<void*>(&intensityData),
                    sizeof(intensityData), &ts);
        }
        else
        {
            Publish(device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_INTNS,
                    reinterpret_cast<void*>(&intensityData),
                    sizeof(intensityData), &time1);
        }
    }
    else
    {
        player_ranger_data_range_t rangeData;

        try
        {
            GlobalTime->GetTimeDouble(&time1);
            device_.get_ranges_by_angle(data_, minAngle_, maxAngle_);
            GlobalTime->GetTimeDouble(&time2);
            time1 = (time1 + time2) / 2.0;
        }
        catch(hokuyoaist::BaseError &e)
        {
            PLAYER_ERROR1("HokuyoAIST: Failed to read scan: %s", e.what());
            SetError(-1);
            return false;
        }

        double lastValidValue = minDist_;
        if (!invert_) {
            for (unsigned int ii = 0; ii < data_.ranges_length(); ii++)
            {
                _ranges[ii] = data_[ii] / 1000.0f;
                if (minDist_ > 0)
                {
                    if (_ranges[ii] < minDist_)
                        _ranges[ii] = lastValidValue;
                    else
                        lastValidValue = _ranges[ii];
                }
            }
        }
        else  // Invert
        {
            for (unsigned int ii = 0; ii < data_.ranges_length(); ii++)
            {
                _ranges[ii] = data_[data_.ranges_length() - 1 - ii] / 1000.0f;
                if (minDist_ > 0)
                {
                    if (_ranges[ii] < minDist_)
                        _ranges[ii] = lastValidValue;
                    else
                        lastValidValue = _ranges[ii];
                }
            }
        }
        rangeData.ranges = _ranges;
        rangeData.ranges_count = data_.ranges_length();
        if (hwTimeStamps_.GetValue())
        {
            double ts = data_.system_time_stamp() / 1000000000.0;
            Publish(device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE,
                    reinterpret_cast<void*> (&rangeData), sizeof(rangeData),
                    &ts);
        }
        else
        {
            Publish(device_addr, PLAYER_MSGTYPE_DATA, PLAYER_RANGER_DATA_RANGE,
                    reinterpret_cast<void*> (&rangeData), sizeof(rangeData),
                    &time1);
        }
    }

    return true;
}

int HokuyoDriver::MainSetup(void)
{
    try
    {
        device_.ignore_unknowns(ignoreUnknowns_);
        // Open the laser
        device_.open_with_probing(portOpts_);
        // Get the sensor information and check minAngle_ and maxAngle_ are OK
        hokuyoaist::SensorInfo info;
        device_.get_sensor_info(info);
        if (minAngle_ < info.min_angle)
        {
            minAngle_ = info.min_angle;
            PLAYER_WARN1("HokuyoAIST: Adjusted min_angle to %lf", minAngle_);
        }
        if (maxAngle_> info.max_angle)
        {
            maxAngle_ = info.max_angle;
            PLAYER_WARN1("HokuyoAIST: Adjusted max_angle to %lf", maxAngle_);
        }
        if (!AllocateDataSpace())
            return -1;

        try
        {
            device_.set_baud(baudRate_.GetValue());
        }
        catch(hokuyoaist::NotSerialError)
        {
            PLAYER_WARN(
                "HokuyoAIST: Cannot change the baud rate of a non-serial connection.");
        }
        catch(hokuyoaist::BaseError &e)
        {
            PLAYER_WARN1("HokuyoAIST: Error while changing baud rate: %s",
                    e.what());
        }
        try
        {
            // Catch any errors here as this is an optional setting not supported by all models
            device_.set_motor_speed(speedLevel_.GetValue());
        }
        catch(hokuyoaist::BaseError &e)
        {
            PLAYER_WARN1("HokuyoAIST: Unable to set motor speed: %s",
                    e.what());
        }
        try
        {
            // Optional setting
            device_.set_high_sensitivity(highSensitivity_.GetValue() != 0);
        }
        catch(hokuyoaist::BaseError &e)
        {
            PLAYER_WARN1("HokuyoAIST: Unable to set sensitivity: %s",
                    e.what());
        }
	if (hwTimeStamps_.GetValue()) {
		device_.calibrate_time();
	}

        if (powerOnStartup_)
            device_.set_power(true);

    }
    catch(hokuyoaist::BaseError &e)
    {
        PLAYER_ERROR1("HokuyoAIST: Failed to setup laser driver: %s",
                e.what());
        SetError(-1);
        return -1;
    }
    return 0;
}

void HokuyoDriver::MainQuit(void)
{
    device_.close();
    data_.clean_up();
    if (_ranges != NULL)
    {
        delete[] _ranges;
        _ranges = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver management functions
////////////////////////////////////////////////////////////////////////////////////////////////////

Driver* HokuyoDriver_Init(ConfigFile* cf, int section)
{
    return reinterpret_cast <Driver*> (new HokuyoDriver(cf, section));
}

void hokuyoaist_Register(DriverTable* table)
{
    table->AddDriver("hokuyoaist", HokuyoDriver_Init);
}
