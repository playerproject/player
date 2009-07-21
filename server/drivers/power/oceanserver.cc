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
// Desc: Driver wrapper around the Gearbox gbxsmartbatteryacfr library.
// Author: Geoffrey Biggs
// Date: 24/06/2009
//
// Provides - Power device.
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_gbxsmartbatteryacfr gbxsmartbatteryacfr
 * @brief Gearbox SmartBattery driver for OceanServer devices.

This driver provides a @ref interface_power interface to the OceanServer
Smart Battery systems supported by the GbxSmartBatteryAcfr library.

@par Compile-time dependencies

- Gearbox library GbxSmartBatteryAcfr

@par Provides

- @ref interface_power: Output power interface

@par Supported configuration requests

- None.

@par Configuration file options

 - port (string)
   - Default: /dev/ttyS0
   - Serial port the laser is connected to.
 - debug (int)
   - Default: 0
   - Debugging level of the underlying library to get verbose output.
 - pose (float 6-tuple: (m, m, m, rad, rad, rad))
   - Default: [0.0 0.0 0.0 0.0 0.0 0.0]
   - Pose (x, y, z, roll, pitch, yaw) of the laser relative to its parent object (e.g. the robot).
 - size (float 3-tuple: (m, m, m))
   - Default: [0.0 0.0 0.0]
   - Size of the laser in metres.

@par Example

@verbatim
driver
(
  name "gbxsmartbatteryacfr"
  provides ["power:0"]
  port "/dev/ttyS0"
)
@endverbatim

@author Geoffrey Biggs

*/
/** @} */

#include <string>
#include <gbxutilacfr/trivialtracer.h>
#include <gbxsmartbatteryacfr/gbxsmartbatteryacfr.h>

#include <libplayercore/playercore.h>

class OceanServer : public ThreadedDriver
{
    public:
        OceanServer (ConfigFile* cf, int section);
        ~OceanServer (void);

        virtual int MainSetup (void);
        virtual void MainQuit (void);
        virtual int ProcessMessage (QueuePointer &resp_queue, player_msghdr *hdr, void *data);

    private:
        virtual void Main (void);
        bool ReadSensor (void);

        // Configuration parameters
        std::string _port;
        unsigned int _debug;
        // The hardware device itself
        std::auto_ptr<gbxsmartbatteryacfr::OceanServer> _device;
        // Objects to handle messages from the driver
        std::auto_ptr<gbxutilacfr::TrivialTracer> _tracer;
};

Driver*
OceanServer_Init (ConfigFile* cf, int section)
{
    return reinterpret_cast <Driver*> (new OceanServer (cf, section));
}

void oceanserver_Register(DriverTable* table)
{
    table->AddDriver ("oceanserver", OceanServer_Init);
}

OceanServer::OceanServer (ConfigFile* cf, int section)
    : ThreadedDriver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_RANGER_CODE)
{
    // Setup config object
    _port = cf->ReadString (section, "port", "/dev/ttyS0");
    _debug = cf->ReadBool (section, "debug", 0);
}

OceanServer::~OceanServer (void)
{
}

int OceanServer::MainSetup (void)
{
    // Create status tracker
    _tracer.reset (new gbxutilacfr::TrivialTracer (_debug));

    // Create the driver object
    try
    {
        _device.reset (new gbxsmartbatteryacfr::OceanServer (_port, *_tracer));
    }
    catch (const std::exception& e)
    {
        PLAYER_ERROR1 ("OceanServer: Failed to initialise device: %s\n", e.what ());
        return -1;
    }

    return 0;
}

void OceanServer::MainQuit (void)
{
    _device.reset (NULL);
    _tracer.reset (NULL);
}

int OceanServer::ProcessMessage (QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
    // Check for capability requests
    HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILTIES_REQ);
    return -1;
}

void OceanServer::Main (void)
{
    while (true)
    {
        pthread_testcancel ();
        ProcessMessages ();

        if (!ReadSensor ())
            break;
    }
}

bool OceanServer::ReadSensor (void)
{
    try
    {
        gbxsmartbatteryacfr::OceanServerSystem data = _device->getData ();
        player_power_data_t powerData;
        memset (&powerData, 0, sizeof (powerData));

        powerData.percent = data.percentCharge;
        powerData.valid |= PLAYER_POWER_MASK_PERCENT;

        float lowVoltage = -1;
        for (unsigned int ii = 0; ii < 8; ii++)
        {
            if (!data.availableBatteries[ii])
                continue;
            powerData.valid |= PLAYER_POWER_MASK_VOLTS;
            if (data.battery (ii).has (gbxsmartbatteryacfr::Voltage))
            {
                if (lowVoltage == -1)
                    lowVoltage = data.battery (ii).voltage ();
                else if (data.battery (ii).voltage () < lowVoltage)
                    lowVoltage = data.battery (ii).voltage ();
            }
        }
        powerData.volts = lowVoltage;

        // First check if any batteries are charging
        for (unsigned int ii = 0; ii < 8; ii++)
        {
            if (!data.availableBatteries[ii])
                continue;
            powerData.valid |= PLAYER_POWER_MASK_CHARGING;
            if (data.chargingStates[ii])
                powerData.charging = 1;
        }
        // Next check if any are discharing - because we have to squeeze
        // up to 8 batteries into one status, give discharging priority over
        // charging.
        for (unsigned int ii = 0; ii < 8; ii++)
        {
            if (!data.availableBatteries[ii])
                continue;
            powerData.valid |= PLAYER_POWER_MASK_CHARGING;
            if (data.supplyingPowerStates[ii])
                powerData.charging = -1;
        }

        Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_POWER_DATA_STATE, reinterpret_cast<void*> (&powerData), sizeof (powerData), NULL);
    }
    catch (const std::exception &e)
    {
        PLAYER_ERROR1 ("OceanServer: Failed to read data: %s\n", e.what ());
        return false;
    }

    return true;
}

