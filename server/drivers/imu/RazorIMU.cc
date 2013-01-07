/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/*
 * Author: Michael F Clarke <mfc5@aber.ac.uk>
 * Version 1.0
 * Intelligent Robotics Group
 * Department of Computer Science
 * Aberystwyth University, UK
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_razorimu razorimu
  * @brief Sparkfun Razor IMU driver

This driver provides support for the Razor IMU, sold by Sparkfun.
For this driver to be effective on the Razor IMU, the latest version of
the firmware from http://code.google.com/p/sf9domahrs/source/list should
be installed on the IMU.

When installing the firmware, the following settings should be configured
as true:
File: SF9DOF_AHRS.pde
#define PRINT_ANALOGS 1 <-- must be set to 1.

@par Compile-time dependencies

- None

@par Provides

- @ref interface_imu

@par Requires

- none

@par Configuration requests

- PLAYER_IMU_REQ_SET_DATATYPE
- PLAYER_IMU_REQ_RESET_EULER
- PLAYER_IMU_REQ_RESET_ORIENTATION

@par Configuration file options

- serial_port
  - Default: /dev/ttyUSB0
  - Serial port to connect to the IMU with

- baud_rate
  - Default: 57600
  - Baud rate for serial connection

- data_packet_type
  - Default: 4
  - Data packet format from IMU

@par Example
@verbatim
driver
(
	name "razorimu"
	provides ["imu:0"]
	serial_port "/dev/ttyUSB0"
	baud_rate 57600
	data_packet_type 5
)
@endverbatim

@author Michael F Clarke <mfc5@aber.ac.uk>

*/
/** @} */

#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <libplayercore/playercore.h>
#include <iostream>

#define DEFAULT_PORT "/dev/ttyUSB0"
#define DEFAULT_BAUD_RATE 57600
#define MAX_RESPONSE 100

using namespace std;

typedef struct imu_data imu_data_t;
struct imu_data {

	float roll;
	float pitch;
	float yaw;
	float acc_x;
	float acc_y;
	float acc_z;
	float gyr_x;
	float gyr_y;
	float gyr_z;
	float mag_x;
	float mag_y;
	float mag_z;
	
};

class RazorIMU : public ThreadedDriver {

	public:
		
		RazorIMU(ConfigFile *cf, int section);
		~RazorIMU();

		virtual int MainSetup();
		virtual void MainQuit();

		virtual int ProcessMessage(QueuePointer &resp_queue,
					   player_msghdr *hdr, void *data); 

	private:

		const char *serialPort;
		int imufd;
		int baudRate;

		player_imu_data_state_t     imu_data_state;
		player_imu_data_calib_t     imu_data_calib;
		player_imu_data_quat_t      imu_data_quat;
		player_imu_data_euler_t     imu_data_euler;
		player_imu_data_fullstate_t imu_data_fullstate;
		player_imu_reset_euler_config_t imu_euler_config;
		imu_data idata;

		int dataType;

		virtual void Main();
		virtual void RefreshData();

		player_imu_data_calib_t getCalibValues(imu_data_t data);

		void uart_init();
		unsigned char uart_rx();
		void uart_tx(char data);
		void uart_deinit();

};

/*
 * Factory creation function. This function is given as an argument when
 * the driver is added to the driver table.
 */
Driver *RazorIMU_Init(ConfigFile *cf, int section) {

	return ((Driver *)(new RazorIMU(cf, section)));

}

/*
 * Registers the driver in the driver table. Called from the player_driver_init
 * function that the loader looks for.
 */
void razorimu_Register(DriverTable *table) {
	
	table->AddDriver("razorimu", RazorIMU_Init);

}
/*
extern "C" {
	int player_driver_init(DriverTable *table) {
		razorimu_Register(table);
		return 0;
	}
}
*/
void RazorIMU::uart_init() {

	struct termios newtio;
	
	imufd = open(serialPort, O_RDWR | O_NOCTTY);
	memset(&newtio, 0, sizeof(newtio));

	if (baudRate == 4800)
		newtio.c_cflag = B4800 | CS8 | CLOCAL | CREAD;
	else if (baudRate == 9600)
		newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	else if (baudRate == 19200)
		newtio.c_cflag = B19200 | CS8 | CLOCAL | CREAD;
	else if (baudRate == 38400)
		newtio.c_cflag = B38400 | CS8 | CLOCAL | CREAD;
	else if (baudRate == 57600)
		newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
	else if (baudRate == 115200)
		newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;

	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* Set input mode (non-canonical, no echo, ...). */
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 0; /* inter-chartimer of 0.1 seconds. */
	newtio.c_cc[VMIN]  = 1; /* blocking read until 1 char received. */

	/* Flush out any old data. */
	tcflush(imufd, TCIFLUSH);

	/* Set baud rate, etc. */
	tcsetattr(imufd, TCSANOW, &newtio);

}

unsigned char RazorIMU::uart_rx() {

	char rec[1];
	read(imufd, rec, 1);

	return rec[0];

}

void RazorIMU::uart_tx(char data) {
	write(imufd, &data, 1);
}

void RazorIMU::uart_deinit() {
	close(imufd);
}
	

/*
 * Constructor. Retrieve options from the configuration file and do any
 * pre-Setup() setup.
 */
RazorIMU::RazorIMU(ConfigFile *cf, int section) : ThreadedDriver(cf, 
	section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_IMU_CODE) {

	serialPort = cf->ReadString(section, "serial_port", DEFAULT_PORT);
	baudRate = cf->ReadInt(section, "baud_rate", DEFAULT_BAUD_RATE);
	dataType = cf->ReadInt(section, "data_packet_type", 4);

	if (!(baudRate == 4800 || baudRate == 9600 || baudRate == 19200 ||
	      baudRate == 38400 || baudRate == 57600 || baudRate == 115200))
	      baudRate = DEFAULT_BAUD_RATE;

	imu_euler_config.orientation.proll = 0.0;
	imu_euler_config.orientation.ppitch = 0.0;
	imu_euler_config.orientation.pyaw = 0.0;

}

/*
 * Destructor.
 */
RazorIMU::~RazorIMU() {

}

/*
 * Setup the device. Return 0 if things go well, and -1 otherwise.
 */
int RazorIMU::MainSetup() {

	uart_init();

	if (imufd < 0) {
		PLAYER_ERROR("Error starting RazorIMU.");
		return -1;
	}

	imu_euler_config.orientation.proll = 0.0;
	imu_euler_config.orientation.ppitch = 0.0;
	imu_euler_config.orientation.pyaw = 0.0;

	PLAYER_MSG0 (1, "> RazorIMU starting up... [done]");
	return 0;

}

/*
 * Shutdown the device.
 */
void RazorIMU::MainQuit() {
	
	uart_deinit();
	PLAYER_MSG0(1, "> RazorIMU driver shutting down... [done]");

}

/*
 * Main thread.
 */
void RazorIMU::Main() {

	while (1) {

		pthread_testcancel();
		ProcessMessages();
		RefreshData();
		usleep(1000);
	
	}

}

int RazorIMU::ProcessMessage(QueuePointer &resp_queue, 
				player_msghdr *hdr, void *data) {

	assert(hdr);
    HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
    HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_IMU_REQ_SET_DATATYPE);
    HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_IMU_REQ_RESET_EULER);
    HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_IMU_REQ_RESET_ORIENTATION);

	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, 
			PLAYER_IMU_REQ_SET_DATATYPE, device_addr)) {

		player_imu_datatype_config *datatype =
			(player_imu_datatype_config *)data;

		if ((datatype->value > 0) && (datatype->value < 6)) {

			dataType = datatype->value;

			Publish(device_addr, resp_queue,
				     PLAYER_MSGTYPE_RESP_ACK, hdr->subtype);

		} else {

			Publish(device_addr, resp_queue, 
				     PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);
	
		}

		return 0;

	} else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
			PLAYER_IMU_REQ_RESET_EULER, device_addr)) {

		player_imu_reset_euler_config_t *conf =
			(player_imu_reset_euler_config_t *)data;


		if (conf->orientation.proll > 90 || 
				conf->orientation.proll < -90) {

			PLAYER_WARN("The new roll value should be between "
								"-90 and 90.");

			Publish(device_addr, resp_queue,
					PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);

			return -1;
	
		}

		if (conf->orientation.ppitch > 90 || 
				conf->orientation.ppitch < -90) {

			PLAYER_WARN("The new pitch value should be between "
								"-90 and 90.");

			Publish(device_addr, resp_queue,
					PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);

			return -1;	

		}

		if (conf->orientation.pyaw > 180 || 
				conf->orientation.pyaw < -180) {

			PLAYER_WARN("The new yaw value should be between "
							     "-180 and 180.");

			Publish(device_addr, resp_queue,
					PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);

			return -1;	

		}

		/* Calculate the new offset values based on the last reading 
                 * from the IMU and the new desired value.
 	         */
		imu_euler_config.orientation.proll = 
				conf->orientation.proll - idata.roll;
		imu_euler_config.orientation.ppitch = 
				conf->orientation.ppitch - idata.pitch;
		imu_euler_config.orientation.pyaw = 
				conf->orientation.pyaw - idata.yaw;

		Publish(device_addr, resp_queue,
				PLAYER_MSGTYPE_RESP_ACK, hdr->subtype);

		return 0;
		
	} else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
			PLAYER_IMU_REQ_RESET_ORIENTATION, device_addr)) {

		PLAYER_WARN("The RazorIMU cannot reset its orientation.");

		Publish(device_addr, resp_queue,
				PLAYER_MSGTYPE_RESP_ACK, hdr->subtype);
	
		return 0;	

	} 

	return -1;

}

/**
 * This method is used to get the latest data from the RazorIMU. 
 */
void RazorIMU::RefreshData() {

	int i;
	imu_data_t data;

	char str[MAX_RESPONSE + 1];

	while(1) {
	
		while(1) {

			/* Wait for the correct start sequence. */
			while(1) {

				if (uart_rx() == '\n')
					if (uart_rx() == '!')
						if (uart_rx() == 'A')
							break;
			}

			/* Copy the data to the \n from the serial line. */
			for (i = 0; i < MAX_RESPONSE + 1; i++) {
				str[i] = uart_rx();
				if (str[i] == '\n') break;
			}

			/* We may have been interrupted and so we may have 
	 	         * rubbish data in the buffer. Ensure that the last
			 * character is indeed a \n.
			 */
			 if (str[i] == '\n') break;

		}

		/* NULL terinate the string and parse the resulting string. */
		str[MAX_RESPONSE] = '\0';

		i = sscanf(str, "NG:%f,%f,%f,AN:%f,%f,%f,%f,%f,%f,%f,%f,%f\n", 
					&data.roll, &data.pitch, &data.yaw,
					&data.gyr_x, &data.gyr_y, &data.gyr_z,
					&data.acc_x, &data.acc_y, &data.acc_z,
					&data.mag_x, &data.mag_y, &data.mag_z);

		idata = data;

		/* Adjust euler values. */
		data.roll += imu_euler_config.orientation.proll;
		data.pitch += imu_euler_config.orientation.ppitch;
		data.yaw += imu_euler_config.orientation.pyaw;

		/* Correct for wrap around. */
		if (data.roll > 90) data.roll -= 180;
		if (data.roll < -90) data.roll += 180;
		if (data.pitch > 90) data.pitch -= 180;
		if (data.pitch < -90) data.pitch += 180;
		if (data.yaw > 180) data.yaw -= 360;
		if (data.yaw < -180) data.yaw += 360;

		/* And finally convert from degrees to radians. */
		data.roll = DTOR(data.roll);
		data.pitch = DTOR(data.pitch);
		data.yaw = DTOR(data.yaw);

		/* Finally, ensure that scanf managed to match all the data 
		 * fields correctly, and if it did, return. 
		 */
		if (i == 12) break;

	}

	if (dataType == PLAYER_IMU_DATA_STATE) {

		imu_data_state.pose.px = data.mag_x;
		imu_data_state.pose.py = data.mag_y;
		imu_data_state.pose.pz = data.mag_z;
	
		imu_data_state.pose.proll = data.roll;
		imu_data_state.pose.ppitch = data.pitch;
		imu_data_state.pose.pyaw = data.yaw;

		Publish(device_addr, PLAYER_MSGTYPE_DATA,
				PLAYER_IMU_DATA_STATE, &imu_data_state,
					sizeof(player_imu_data_state_t), NULL);


	} else if (dataType == PLAYER_IMU_DATA_CALIB) {

		imu_data_calib = getCalibValues(data);

		Publish(device_addr, PLAYER_MSGTYPE_DATA,
				PLAYER_IMU_DATA_CALIB, &imu_data_calib,
					sizeof(player_imu_data_calib_t), NULL);

	} else if (dataType == PLAYER_IMU_DATA_QUAT) {

		imu_data_quat.calib_data = getCalibValues(data);

		/* We have to calculate these, unfortunately.
	 	 * Calculations are based on the Wikipedia page at:
	 	 * http://en.wikipedia.org/wiki/
		 *         Conversion_between_quaternions_and_Euler_angles
		 *
	 	 * See sections 'Conversion' and 'Relationship with Tait-Bryan
 		 * angles' for definitions of roll, pitch and yaw.
	 	 */
		imu_data_quat.q0 = cos(data.roll/2)  * cos(data.pitch/2) *
                                   cos(data.yaw/2)   + sin(data.roll/2)  *
                                   sin(data.pitch/2) * sin(data.yaw/2);

		imu_data_quat.q1 = sin(data.roll/2)  * cos(data.pitch/2) *
                                   cos(data.yaw/2)   - cos(data.roll/2)  *
                                   sin(data.pitch/2) * sin(data.yaw/2);

		imu_data_quat.q2 = cos(data.roll/2)  * sin(data.pitch/2) *
                                   cos(data.yaw/2)   + sin(data.roll/2)  *
                                   cos(data.pitch/2) * sin(data.yaw/2);

		imu_data_quat.q3 = cos(data.roll/2)  * cos(data.pitch/2) *
                                   sin(data.yaw/2)   - sin(data.roll/2)  *
                                   sin(data.pitch/2) * cos(data.yaw/2);

		Publish(device_addr, PLAYER_MSGTYPE_DATA,
				PLAYER_IMU_DATA_QUAT, &imu_data_quat,
					sizeof(player_imu_data_quat_t), NULL);

	} else if (dataType == PLAYER_IMU_DATA_EULER) {

		imu_data_euler.calib_data = getCalibValues(data);

		imu_data_euler.orientation.proll = data.roll;
		imu_data_euler.orientation.ppitch = data.pitch;
		imu_data_euler.orientation.pyaw = data.yaw;

		Publish(device_addr, PLAYER_MSGTYPE_DATA,
				PLAYER_IMU_DATA_EULER, &imu_data_euler,
					sizeof(player_imu_data_euler_t), NULL);

	} else {

		imu_data_fullstate.pose.px = data.mag_x;
		imu_data_fullstate.pose.py = data.mag_y;
		imu_data_fullstate.pose.pz = data.mag_z;
		imu_data_fullstate.pose.proll = data.roll;
		imu_data_fullstate.pose.ppitch = data.pitch;
		imu_data_fullstate.pose.pyaw = data.yaw;

		imu_data_fullstate.vel.px = 0;
		imu_data_fullstate.vel.py = 0;
		imu_data_fullstate.vel.pz = 0;
		imu_data_fullstate.vel.proll = 0;
		imu_data_fullstate.vel.ppitch = 0;
		imu_data_fullstate.vel.pyaw = 0;

		imu_data_fullstate.acc.px = data.acc_x;
		imu_data_fullstate.acc.py = data.acc_y;
		imu_data_fullstate.acc.pz = data.acc_z;
                imu_data_fullstate.acc.ppitch = data.gyr_x;
                imu_data_fullstate.acc.pyaw = data.gyr_y;
                imu_data_fullstate.acc.proll = data.gyr_z;

		Publish(device_addr, PLAYER_MSGTYPE_DATA, 
			PLAYER_IMU_DATA_FULLSTATE, &imu_data_fullstate, 
				sizeof(player_imu_data_fullstate_t), NULL);

	}

}

/**
 * A useful method; used simply for converting a imu_data_t (directly from the
 * IMU) into a player_imu_data_calib_t.
 */
player_imu_data_calib_t RazorIMU::getCalibValues(imu_data_t data) {

	player_imu_data_calib_t calib_data;

	calib_data.accel_x = data.acc_x;
	calib_data.accel_y = data.acc_y;
	calib_data.accel_z = data.acc_z;
	calib_data.gyro_x  = data.gyr_x;
	calib_data.gyro_y  = data.gyr_y;
	calib_data.gyro_z  = data.gyr_z;
	calib_data.magn_x  = data.mag_x;
	calib_data.magn_y  = data.mag_y;
	calib_data.magn_z  = data.mag_z;

	return calib_data;

}
