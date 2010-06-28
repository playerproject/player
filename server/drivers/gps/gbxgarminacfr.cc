/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003  Brian Gerkey gerkey@usc.edu
 *                           Andrew Howard ahoward@usc.edu
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
// Desc: Driver wrapper around the Gearbox gbxgarminacfr library.
// Author: Geoffrey Biggs
// Date: 24/06/2009
//
// Provides - GPS device.
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_gbxgarminacfr gbxgarminacfr
 * @brief Gearbox Garmin GPS driver

This driver provides a @ref interface_gps interface to Garmin GPS devices,
as well as any other GPS device supported by the GbxGarminAcfr library. This
should include most GPS devices that use NMEA to communicate.

@par Compile-time dependencies

- Gearbox library GbxGarminAcfr

@par Provides

- @ref interface_gps: Output GPS interface

@par Supported configuration requests

- None.

@par Configuration file options

 - read_gga (boolean)
   - Default: true
   - Read and parse GGA messages.
 - read_vtg (boolean)
   - Default: true
   - Read and parse VTG messages.
 - read_rme (boolean)
   - Default: true
   - Read and parse RME messages.
 - ignore_unknown (boolean)
   - Default: false
   - Silently ignore unknown messages.
 - port (string)
   - Default: /dev/ttyS0
   - Serial port the laser is connected to.
 - debug (int)
   - Default: 0
   - Debugging level of the underlying library to get verbose output.

@par Example

@verbatim
driver
(
  name "gbxgarminacfr"
  provides ["gps:0"]
  port "/dev/ttyS0"
)
@endverbatim

@author Geoffrey Biggs

*/
/** @} */

#include <gbxgarminacfr/driver.h>
#include <gbxutilacfr/trivialtracer.h>
#include <gbxutilacfr/trivialstatus.h>
#include <gbxutilacfr/mathdefs.h>

#include <libplayercore/playercore.h>

class GbxGarminAcfr : public ThreadedDriver
{
    public:
        GbxGarminAcfr (ConfigFile* cf, int section);
        ~GbxGarminAcfr (void);

        virtual int MainSetup (void);
        virtual void MainQuit (void);
        virtual int ProcessMessage (QueuePointer &resp_queue, player_msghdr *hdr, void *data);

    private:
        virtual void Main (void);
        bool ReadSensor (void);

        // Configuration parameters
        gbxgarminacfr::Config _config;
        unsigned int _debug;
        // Data storage
        player_gps_data_t _gpsData;
        // The hardware device itself
        std::auto_ptr<gbxgarminacfr::Driver> _device;
        // Objects to handle messages from the driver
        std::auto_ptr<gbxutilacfr::TrivialTracer> _tracer;
        std::auto_ptr<gbxutilacfr::TrivialStatus> _status;
};

Driver*
GbxGarminAcfr_Init (ConfigFile* cf, int section)
{
    return reinterpret_cast <Driver*> (new GbxGarminAcfr (cf, section));
}

void gbxgarminacfr_Register(DriverTable* table)
{
    table->AddDriver ("gbxgarminacfr", GbxGarminAcfr_Init);
}

GbxGarminAcfr::GbxGarminAcfr (ConfigFile* cf, int section)
    : ThreadedDriver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_RANGER_CODE)
{
    // Setup config object
    _config.readGga = cf->ReadFloat (section, "read_gga", true);
    _config.readVtg = cf->ReadFloat (section, "read_vtg", true);
    _config.readRme = cf->ReadFloat (section, "read_rme", true);
    _config.ignoreUnknown = cf->ReadInt (section, "ignore_unknown", false);
    _config.device = cf->ReadString (section, "port", "/dev/ttyS0");
    _debug = cf->ReadBool (section, "debug", 0);

    memset (&_gpsData, 0, sizeof (_gpsData));
}

GbxGarminAcfr::~GbxGarminAcfr (void)
{
}

int GbxGarminAcfr::MainSetup (void)
{
    // Validate the configuration
    //std::string reason;
    if (!_config.isValid ())
    {
        PLAYER_ERROR("GbxGarminAcfr: Invalid sensor configuration.\n");//, reason.c_str());
        return -1;
    }

    // Create status trackers
    _tracer.reset (new gbxutilacfr::TrivialTracer (_debug));
    _status.reset (new gbxutilacfr::TrivialStatus (*_tracer));

    // Create the driver object
    try
    {
        _device.reset (new gbxgarminacfr::Driver (_config, *_tracer, *_status));
    }
    catch (const std::exception& e)
    {
        PLAYER_ERROR1 ("GbxGarminAcfr: Failed to initialise GPS device: %s\n", e.what ());
        return -1;
    }

    return 0;
}

void GbxGarminAcfr::MainQuit (void)
{
    _device.reset (NULL);
    _status.reset (NULL);
    _tracer.reset (NULL);
}

int GbxGarminAcfr::ProcessMessage (QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
    // Check for capability requests
    HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILTIES_REQ);
    return -1;
}

void GbxGarminAcfr::Main (void)
{
    while (true)
    {
        pthread_testcancel ();
        ProcessMessages ();

        if (!ReadSensor ())
            break;
    }
}

bool GbxGarminAcfr::ReadSensor (void)
{
    std::auto_ptr<gbxgarminacfr::GenericData> data;

    try
    {
        data = _device->read ();

        // find out which data type it is
        switch (data->type ())
        {
            case gbxgarminacfr::GpGga:
            {
                gbxgarminacfr::GgaData *d = reinterpret_cast<gbxgarminacfr::GgaData*> (data.get ());
                _gpsData.time_sec = d->timeStampSec;
                _gpsData.time_usec = d->timeStampUsec;
                _gpsData.latitude = static_cast<int32_t> (d->latitude / 1e7);
                _gpsData.longitude = static_cast<int32_t> (d->longitude / 1e7);
                _gpsData.altitude = static_cast<int32_t> (d->altitude * 1000);
                _gpsData.num_sats = d->satellites;
                _gpsData.hdop = static_cast<uint32_t> (d->horizontalDilutionOfPosition * 10);
                if (d->fixType == gbxgarminacfr::Autonomous)
                    _gpsData.quality = 1;
                else if (d->fixType == gbxgarminacfr::Differential)
                    _gpsData.quality = 2;
                else
                    _gpsData.quality = 0;
                break;
            }
            case gbxgarminacfr::GpVtg:
            {
                gbxgarminacfr::VtgData *d = reinterpret_cast<gbxgarminacfr::VtgData*> (data.get ());
                _gpsData.time_sec = d->timeStampSec;
                _gpsData.time_usec = d->timeStampUsec;
                break;
            }
            case gbxgarminacfr::PgRme:
            {
                gbxgarminacfr::RmeData *d = reinterpret_cast<gbxgarminacfr::RmeData*> (data.get ());
                _gpsData.time_sec = d->timeStampSec;
                _gpsData.time_usec = d->timeStampUsec;
                _gpsData.err_horz = d->horizontalPositionError;
                _gpsData.err_vert = d->verticalPositionError;
                break;
            }
            default:
                PLAYER_WARN ("GbxGarminAcfr: Unknown message type received from GPS sensor.");
        }
        Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_GPS_DATA_STATE, reinterpret_cast<void*> (&_gpsData), sizeof (_gpsData), NULL);
    }
    catch (const std::exception &e)
    {
        PLAYER_ERROR1 ("GbxGarminAcfr: Failed to read data: %s\n", e.what ());
        return false;
    }

    return true;
}
