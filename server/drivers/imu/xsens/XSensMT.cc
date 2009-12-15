/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006 Radu Bogdan Rusu (rusu@cs.tum.edu)
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
/*
 Desc: Driver for XSens MTx/MTi IMU. CMTComm class borrowed from XSens under GPL.
 Author: Radu Bogdan Rusu
 Date: 1 Aug 2006
  */
/** @ingroup drivers */
/** @{ */
/** @defgroup driver_xsensmt xsensmt
 * @brief XSens MTx/MTi Inertial Measurement Unit driver

The xsensmt driver controls the XSens MTx/MTi Inertial Measurement Unit. It
provides Kalman filtered orientation information (pitch, roll, yaw) via its
internal 3-axis accelerometer, 3-axis gyroscope and 3-axis magnetometer.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_imu

@par Requires

- none

@par Configuration requests

- PLAYER_IMU_REQ_SET_DATATYPE
- PLAYER_IMU_REQ_RESET_ORIENTATION

@par Configuration file options

- port (string)
  - Default: "/dev/ttyUSB0"
  - Serial port to which the XSens MTx/MTi sensor is attached.  If you are
    using a USB/232 or USB/422 converter, this will be "/dev/ttyUSBx".

- data_packet_type (integer)
  - Default: 4. Possible values: 1, 2, 3, 4, 5.
   (1 = 3D pose as X, Y, Z and + orientation/Euler angles as Roll, Pitch, Yaw;
    2 = calibrated IMU data: accel, gyro, magnetometer;
    3 = quaternions + calibrated IMU data;
    4 = Euler angles + calibrated IMU data.
	5 = Full state: 3D pose, velocity and acceleration as high presision values, only available for MtiG's)
  - Specify the type of data packet to send (can be set using
    PLAYER_IMU_REQ_SET_DATATYPE as well).
 
- mtig (integer)
  - Defalut: 0. Posible values: 0,1
  - Specifies whether the device is a Mtx/Mti (Default) or an Mti-G (1)

- gps_arm (double, double, double) (MTI-G ONLY)
 - Default (0,0,0)
 - Specifies the x, y and z offsets of the gps antenna with respect to the imu
 
- xkf (integer) - (ONLY TESTED ON MTI-G - remove this comment if you try and it works on an Mti or Mtx, the cenarios may be different)
 - Default: -1
 - Specifies which XKF scenario should be used:
	-2 = All of the possible xfk senario numbers and their names are printed out
    -1 = the xkf_scenario is left to whatever was previously written to flash
    0-5 = other scenarios
@par Example

@verbatim
driver
(
  name "xsensmt"
  provides ["imu:0"]
  port "/dev/ttyUSB0"
  # We need quaternions + calibrated accel/gyro/magnetometer data
  data_packet_type 3
  gps_arm [0.8 0 1.2]
  xkf 2
)
@endverbatim

@author Radu Bogdan Rusu, extended by Chris Chambers for the MTi-G

 */
/** @} */

#if !defined(WIN32)
#include <unistd.h>
#endif
#include <string.h>
#include <libplayercore/playercore.h>
#include "cmt3.h"

#define DEFAULT_PORT "/dev/ttyUSB0"
#include <iostream>

using namespace std;

////////////////////////////////////////////////////////////////////////////////
// The XSensMT device class.
class XSensMT : public ThreadedDriver
{
public:
	// Constructor
	XSensMT (ConfigFile* cf, int section);
	
	// Destructor
	~XSensMT ();

	// Implementations of virtual functions
	virtual int MainSetup ();
	virtual void MainQuit ();

	// This method will be invoked on each incoming message
	virtual int ProcessMessage (QueuePointer &resp_queue,
                                    player_msghdr * hdr,
                                    void * data);
private:

	// Cmt3 object
	xsens::Cmt3 cmt3;
	
	const char* portName;

	// Data
	player_imu_data_state_t  imu_data_state;
	player_imu_data_calib_t  imu_data_calib;
	player_imu_data_quat_t   imu_data_quat;
	player_imu_data_euler_t  imu_data_euler;
	player_imu_data_fullstate_t imu_data_fullstate;

	xsens::Packet* packet;
	int received;
	bool mtig;

	// The state the the IMU was in the last iteration
	uint8_t last_status;
	// Desired data packet type
	int dataType;

	// Main function for device thread.
	virtual void Main ();
	virtual void RefreshData  ();
	
	CmtVector gps_arm;
	
	int xkf_scenario;

	player_imu_data_calib_t GetCalibValues ();
};

////////////////////////////////////////////////////////////////////////////////
// Factory creation function. This functions is given as an argument when
// the driver is added to the driver table
Driver* XSensMT_Init (ConfigFile* cf, int section)
{
    // Create and return a new instance of this driver
    return ((Driver*)(new XSensMT (cf, section)));
}

////////////////////////////////////////////////////////////////////////////////
// Registers the driver in the driver table. Called from the
// player_driver_init function that the loader looks for
void xsensmt_Register (DriverTable* table)
{
    table->AddDriver ("xsensmt", XSensMT_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
XSensMT::XSensMT (ConfigFile* cf, int section)
    : ThreadedDriver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN,
              PLAYER_IMU_CODE)
{
	this->packet = new xsens::Packet(0,false);
	assert(packet);
	
    portName = cf->ReadString (section, "port", DEFAULT_PORT);

    // Euler + Calibrated values
    dataType = cf->ReadInt    (section, "data_packet_type", 4);
	
	// GPS lever arm, only used for MTi-G
	this->gps_arm.m_data[0] = cf->ReadTupleLength(section, "gps_arm", 0, 0.0);
	this->gps_arm.m_data[1] = cf->ReadTupleLength(section, "gps_arm", 1, 0.0);
	this->gps_arm.m_data[2] = cf->ReadTupleLength(section, "gps_arm", 2, 0.0);
	
	//TODO: Read xkf here
	xkf_scenario = cf->ReadInt(section, "xkf", -1);
	
	mtig = bool(cf->ReadInt(section, "mtig", 0));
	if (!mtig && dataType == 5)
	{
		PLAYER_WARN("Only Mtig can use data mode 5");
		dataType = 4;
	}

    return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor.
XSensMT::~XSensMT()
{
	if (this->packet != NULL)
		delete this->packet;
    return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int XSensMT::MainSetup ()
{
	//if (strncmp(portName, "/dev/ttyUSB", 11) != 0)
	//	PLAYER_ERROR("Only USB devices are currently supported by the Xsens API");
	//long temp = strtol(&portName[11],(char**)NULL, 10);
    // Open the device
    if (cmt3.openPort (&portName[0]) != XRV_OK)
	{
		PLAYER_ERROR("Failed to open the port");
		return (-1);
	}
        
	printf("***Opened port\n");

    // Put MTi/MTx in Config State
    if (cmt3.gotoConfig() != XRV_OK){
        PLAYER_ERROR ("No device connected!");
        return false;
    }

	printf("***Switched to config mode\n");
	
    CmtDeviceMode tmpdeviceMode;
    // Get current settings and check if Xbus Master is connected
    if (cmt3.getDeviceMode (tmpdeviceMode) != XRV_OK) {
        if (cmt3.getDeviceCount() == 1)
            PLAYER_ERROR ("MTi / MTx has not been detected\nCould not get device mode!");
        else
            PLAYER_ERROR ("Not just MTi / MTx connected to Xbus\nCould not get all device modes!");
        return false;
    }

    // Check if Xbus Master is connected
    if (cmt3.isXm())
    {
        // If Xbus Master is connected, attached Motion Trackers should not send sample counter
        PLAYER_ERROR ("Sorry, this driver only talks to one MTx/MTi device.");
        return false;
    }
	
	if (this->mtig)
	{
		if (cmt3.setGpsLeverArm(this->gps_arm) != XRV_OK)
			PLAYER_ERROR("Failed to set GPS lever arm for MTi-G");
	}
	
	if (this->xkf_scenario >= 0)
	{
		if (cmt3.setScenario(char(this->xkf_scenario)) != XRV_OK)
			PLAYER_ERROR("Failed to set the scenario of the MTi-G");
		else
		{
			unsigned char type, version;
			cmt3.getScenario(type, version);
			printf("Successfully set scenario to: type: %d; version: %d\n", uint8_t(type), uint8_t(version));
		}
	}
	
	if (this->xkf_scenario == -2)
	{
		// TODO: Get this to prin out correctly, currently nly printing out the lable
		cout << "Possible XKF scenarios:\nType\tVersion\tFilter type\tLabel" << endl;
		CmtScenario scenarios[CMT_MAX_SCENARIOS_IN_MT + 1];
		cmt3.getAvailableScenarios(scenarios);
		for (int i = 0; i < CMT_MAX_SCENARIOS_IN_MT; i++)
		{
			if (scenarios[i].m_type == 0)
				break;
			else
			{
				printf("%d\t%d\t%d\t%s\n", uint8_t(scenarios[i].m_type) , uint8_t(scenarios[i].m_version), uint8_t(scenarios[i].m_filterType), scenarios[i].m_label);
			}
		}
		unsigned char type, version;
		cmt3.getScenario(type, version);
		printf("Current scenario bing used - type: %d; version: %d\n", uint8_t(type), uint8_t(version));
	}

	unsigned long outputSettings = CMT_OUTPUTSETTINGS_ORIENTMODE_EULER;
    unsigned long outputMode = CMT_OUTPUTMODE_CALIB + CMT_OUTPUTMODE_ORIENT + CMT_OUTPUTMODE_STATUS;
	switch (dataType)
	{
		case 1:
		case 2:
		{
		    break;
		}
		case 3:
		{
		    outputSettings = CMT_OUTPUTSETTINGS_ORIENTMODE_QUATERNION;
		    break;
		}
		case 4:
		{
		    break;
		}
		case 5:
		{
			if (!mtig)
			{
				PLAYER_WARN("Only MTi-G can use datmode 5");
				dataType = 4;
				break;
			}
			// TODO: switch to high precision here
			// TODO: make this so that it gets the data that we want here
			outputSettings = CMT_OUTPUTSETTINGS_ORIENTMODE_EULER + CMT_OUTPUTSETTINGS_DATAFORMAT_FP1632;
			outputMode += CMT_OUTPUTMODE_VELOCITY + CMT_OUTPUTMODE_POSITION;
		    break;
		}
		default:
		{
		    outputSettings = CMT_OUTPUTSETTINGS_ORIENTMODE_EULER;
		}
	}
			
	CmtDeviceMode deviceMode(outputMode, outputSettings);
    // Set output mode and output settings for the MTi/MTx
    if (cmt3.setDeviceMode (deviceMode, false) != XRV_OK) {
        PLAYER_ERROR ("Could not set device mode(s)!");
        return false;
    }

    // Put MTi/MTx in Measurement State
	if (cmt3.gotoMeasurement() != XRV_OK){
        PLAYER_ERROR ("Error going to measurement mode");
        return false;
    }
    PLAYER_MSG0 (1, "> XSensMT starting up... [done]");
	
	// Assumes that everything failed before starting
	this->last_status = 0;

    return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void XSensMT::MainQuit ()
{
    // Close the MTx device
    if (cmt3.closePort () != XRV_OK)
        PLAYER_ERROR ("Could not close device!");

    PLAYER_MSG0 (1, "> XSensMT driver shutting down... [done]");
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void XSensMT::Main ()
{
    //timespec sleepTime = {0, 0};

    // modify the scheduling priority
//    nice (10);

    // The main loop; interact with the device here
    while (true)
    {
        // test if we are supposed to cancel
        pthread_testcancel ();

        // Process any pending messages
        ProcessMessages ();

        // Refresh data
        this->RefreshData ();

		usleep(1000);
        //nanosleep (&sleepTime, NULL);
    }
}

////////////////////////////////////////////////////////////////////////////////
// ProcessMessage function
int XSensMT::ProcessMessage (QueuePointer &resp_queue,
                             player_msghdr * hdr,
                             void * data)
{
    assert (hdr);

    // this holds possible error messages returned by mtcomm.writeMessage
    int err;

	// TODO: Change this so that is is an intprop? more sensible as you need a simplier interface?
    if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ,
        PLAYER_IMU_REQ_SET_DATATYPE, device_addr))
    {
        // Change the data type according to the user's preferences
        player_imu_datatype_config *datatype =
                (player_imu_datatype_config*)data;

        if ((datatype->value > 0) && (datatype->value < 6))
        {
            dataType = datatype->value;

	    unsigned long outputSettings = CMT_OUTPUTSETTINGS_ORIENTMODE_EULER;
		unsigned long outputMode = CMT_OUTPUTMODE_CALIB + CMT_OUTPUTMODE_ORIENT;
	    switch (dataType)
	    {
		case 1:
		case 2:
		{
		    break;
		}
		case 3:
		{
		    outputSettings = CMT_OUTPUTSETTINGS_ORIENTMODE_QUATERNION;
		    break;
		}
		case 4:
		{
		    break;
		}
		case 5:
		{
			if (!mtig)
			{
				PLAYER_WARN("Only MTi-G can use datmode 5");
				dataType = 4;
				break;
			}
			// TODO: switch to high precision here
			// TODO: make this so that it gets the data that we want here
			outputSettings = CMT_OUTPUTSETTINGS_ORIENTMODE_EULER + CMT_OUTPUTSETTINGS_DATAFORMAT_FP1632;
			outputMode += CMT_OUTPUTMODE_VELOCITY + CMT_OUTPUTMODE_POSITION;
		    break;
		}
		default:
		{
		    outputSettings = CMT_OUTPUTSETTINGS_ORIENTMODE_EULER;
		}

	    }
	    // Put MTi/MTx in Config State
	    if (cmt3.gotoConfig() != XRV_OK)
	    {
    		PLAYER_ERROR ("No device connected!");
        	Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                     hdr->subtype);
		return (-1);
	    }

		// TODO: make it so that it gets back velocity here
		CmtDeviceMode deviceMode(outputMode, outputSettings);
	    // Set output mode and output settings for the MTi/MTx
	    if (cmt3.setDeviceMode (deviceMode, false) != XRV_OK) {
    		PLAYER_ERROR ("Could not set device mode(s)!");
    		return false;
	    }

	    // Put MTi/MTx in Measurement State
	    cmt3.gotoMeasurement();

	    Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
                     hdr->subtype);
        }
        else
            Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                     hdr->subtype);

        return 0;
    }
    else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ,
        PLAYER_IMU_REQ_RESET_ORIENTATION, device_addr))
    {
		if (mtig)
		{
			PLAYER_WARN("The Mtig cannot reset its orientation");
			Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
                     hdr->subtype);
			return 0;
		}
        // Change the data type according to the user's preferences
        player_imu_reset_orientation_config *rconfig =
                (player_imu_reset_orientation_config*)data;

	// 0 = store current settings
	// 1 = heading reset
	// 2 = global reset
	// 3 = object reset
	// 4 = align reset
        if ((rconfig->value >= 0) && (rconfig->value <= 4))
        {
	    // Force <global reset> until further tests.
            rconfig->value = 2;

	    if ((err = cmt3.resetOrientation(CMT_RESETORIENTATION_GLOBAL)) != XRV_OK)
	    {
    		PLAYER_ERROR1 ("Could not put reset orientation on device! Error 0x%x\n", err);
        	Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                     hdr->subtype);
		return (-1);
	    }

            Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
                     hdr->subtype);
        }
        else
            Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
                     hdr->subtype);

        return 0;
    }
    else
    {
        return -1;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// RefreshData function
void XSensMT::RefreshData ()
{
	//printf("***Refreshing data\n");
    // Get data from the MTx device
    received = cmt3.waitForDataMessage (this->packet); //should this be read data packet instead?

	//received = cmt3.requestData		(this->packet); //should this be read data packet instead?

    // Need to test this!
    if (received != XRV_OK)
	{
		PLAYER_MSG1(9,"***Failed to read packet %d", received);
		return;
	}
//	PLAYER_MSG0(9,"***Sucessfully got new data");

	//TODO: This might work for not mtig sensrs too, I'm not sure
	// Gets the status of the IMU, bit 0 = self check, 1 = XFK Valid,  2 = GPS Fix
	if (mtig)
	{
		if (!this->packet->containsStatus())
			PLAYER_ERROR("Packet dosen't contain the data we want - status");
		uint8_t status = packet->getStatus();
		if (!(status&0x01))
			PLAYER_ERROR("MTi-g Self check failed");
		if (last_status^status)
		{
			// Something changed
			if ((last_status^status)&0x02)
			{
				if (status&0x02)
					PLAYER_MSG0(0, "XKF Scenario now valid for MTi-G");
				else
				{
					//not player_warn as you might not care if this happens, though if you care it is a warning
					PLAYER_MSG0(0, "Warning: XKF Scenario now NOT valid");
				}
			}
			if ((last_status^status)&0x04)
			{
				if (status&0x04)
					PLAYER_MSG0(0, "GPS Fix attained");
				else
					PLAYER_MSG0(0, "GPS Fix lost");
			}
			last_status = status;
		}
	}
	
    // Check which type of data does the user want to receive
    switch (dataType)
    {
        case PLAYER_IMU_DATA_STATE:
        {
            // Parse and get value (EULER orientation)
			if (!this->packet->containsOriEuler())
				PLAYER_ERROR("Packet dosen't contain the data we want - Ori Euler");
            CmtEuler euler_data = packet->getOriEuler();
            imu_data_state.pose.px = -1;
            imu_data_state.pose.py = -1;
            imu_data_state.pose.pz = -1;

            imu_data_state.pose.proll  = euler_data.m_roll;
            imu_data_state.pose.ppitch = euler_data.m_pitch;
            imu_data_state.pose.pyaw   = euler_data.m_yaw;

            Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_IMU_DATA_STATE,
                     &imu_data_state, sizeof (player_imu_data_state_t), NULL);
            break;
        }
        case PLAYER_IMU_DATA_CALIB:
        {
            imu_data_calib = GetCalibValues ();

            Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_IMU_DATA_CALIB,
                     &imu_data_calib, sizeof (player_imu_data_calib_t), NULL);
            break;
        }
        case PLAYER_IMU_DATA_QUAT:
        {
            // Parse and get value (quaternion orientation)
            imu_data_quat.calib_data = GetCalibValues ();
			if (!this->packet->containsOriQuat())
				PLAYER_ERROR("Packet dosen't contain the data we want - Ori Quat");
			CmtQuat quaternion_data = packet->getOriQuat();
			imu_data_quat.q0 = quaternion_data.m_data[0];
            imu_data_quat.q1 = quaternion_data.m_data[1];
            imu_data_quat.q2 = quaternion_data.m_data[2];
            imu_data_quat.q3 = quaternion_data.m_data[3];

            Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_IMU_DATA_QUAT,
                     &imu_data_quat, sizeof (player_imu_data_quat_t), NULL);
            break;
        }
        case PLAYER_IMU_DATA_EULER:
        {
            // Parse and get value (Euler orientation)
			if (!this->packet->containsOriEuler())
				PLAYER_ERROR("Packet dosen't contain the data we want - Ori Euler");
			CmtEuler euler_data = packet->getOriEuler();
            
			imu_data_euler.calib_data = GetCalibValues ();
            imu_data_euler.orientation.proll  = euler_data.m_roll;
            imu_data_euler.orientation.ppitch = euler_data.m_pitch;
            imu_data_euler.orientation.pyaw   = euler_data.m_yaw;

            Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_IMU_DATA_EULER,
                     &imu_data_euler, sizeof (player_imu_data_euler_t), NULL);
            break;
        }
		case PLAYER_IMU_DATA_FULLSTATE:
		{
			if (!this->packet->containsOriEuler())
				PLAYER_ERROR("Packet dosen't contain the data we want - Ori Euler");
			CmtEuler euler_data = packet->getOriEuler();
			if (!this->packet->containsCalData())
				PLAYER_ERROR("Packet dosen't contain the data we want - Cal");
			CmtCalData cal_data = packet->getCalData();
			if (!this->packet->containsVelocity())
				PLAYER_ERROR("Packet dosen't contain the data we want - Velocity");
			CmtVector vel_data = packet->getVelocity();
			if (!this->packet->containsPositionLLA())
				PLAYER_ERROR("Packet dosen't contain the data we want - PosLLA");
			CmtVector pos_data = packet->getPositionLLA();
			imu_data_fullstate.pose.px = pos_data.m_data[0];
			imu_data_fullstate.pose.py = pos_data.m_data[1];
			imu_data_fullstate.pose.pz = pos_data.m_data[2];
			imu_data_fullstate.pose.proll = euler_data.m_roll;
			imu_data_fullstate.pose.ppitch = euler_data.m_pitch;
			imu_data_fullstate.pose.pyaw = euler_data.m_yaw;
			imu_data_fullstate.vel.px = vel_data.m_data[0];
			imu_data_fullstate.vel.py = vel_data.m_data[1];
			imu_data_fullstate.vel.pz = vel_data.m_data[2];
			imu_data_fullstate.vel.proll = cal_data.m_gyr.m_data[0];
			imu_data_fullstate.vel.ppitch = cal_data.m_gyr.m_data[1];
			imu_data_fullstate.vel.pyaw = cal_data.m_gyr.m_data[2];
			imu_data_fullstate.acc.px = cal_data.m_acc.m_data[0];
			imu_data_fullstate.acc.py = cal_data.m_acc.m_data[1];
			imu_data_fullstate.acc.pz = cal_data.m_acc.m_data[2];
			imu_data_fullstate.acc.ppitch = 0;
			imu_data_fullstate.acc.pyaw = 0;
			imu_data_fullstate.acc.proll = 0;
			
			Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_IMU_DATA_FULLSTATE,
                     &imu_data_fullstate, sizeof (player_imu_data_fullstate_t), NULL);
			
			break;
		}
    }
}


////////////////////////////////////////////////////////////////////////////////
// GetCalibValues function
player_imu_data_calib_t XSensMT::GetCalibValues () {
    player_imu_data_calib_t calib_data;
	if (!this->packet->containsCalData())
		PLAYER_ERROR("Packet dosen't contain the data we want - Cal");
	CmtCalData cal_data = this->packet->getCalData();
	
    calib_data.accel_x = cal_data.m_acc.m_data [0];
    calib_data.accel_y = cal_data.m_acc.m_data [1];
    calib_data.accel_z = cal_data.m_acc.m_data [2];
    calib_data.gyro_x  = cal_data.m_gyr.m_data  [0];
    calib_data.gyro_y  = cal_data.m_gyr.m_data  [1];
    calib_data.gyro_z  = cal_data.m_gyr.m_data  [2];
    calib_data.magn_x  = cal_data.m_mag.m_data  [0];
    calib_data.magn_y  = cal_data.m_mag.m_data  [1];
    calib_data.magn_z  = cal_data.m_mag.m_data  [2];
	//PLAYER_WARN3("Fullstate %f %f %f", cal_data.m_acc.m_data[0], cal_data.m_acc.m_data[1], cal_data.m_acc.m_data[2]);

    return calib_data;
}
