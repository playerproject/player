/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  John Sweeney & Brian Gerkey
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

#include "canio.h"
#include "canio_kvaser.h"
#include "usb_packet.h"
#include <libplayercore/playercore.h>

#ifndef BusType
typedef enum { UNKNOWN, RS232, CANBUS, USB } BusType;
#endif

#define SPEEDTHRESH 0.01

// Forward declarations
class rmp_frame_t;


// Driver for robotic Segway
class SegwayRMP : public ThreadedDriver
{
public:
	// Constructors etc
	SegwayRMP(ConfigFile* cf, int section);
	~SegwayRMP();

	// Setup/shutdown routines.
	virtual int MainSetup();
	virtual void MainQuit();
	virtual int ProcessMessage(QueuePointer & resp_queue,
			player_msghdr * hdr,
			void * data);
	/// Sets up the CAN bus for communication
	int CanBusSetup();

	/// Sets up the USB bus for communication
	int USBSetup();

protected:

	// Supported interfaces
	// Position2d device
	player_devaddr_t position_id;
	player_position2d_data_t position_data;

	// Position3d device
	player_devaddr_t position3d_id;
	player_position3d_data_t position3d_data;

	// Main Battery device
	player_devaddr_t power_base_id;
	player_power_data_t power_base_data;

	// UI Battery device
	player_devaddr_t power_ui_id;
	player_power_data_t power_ui_data;

private:

	BusType bus_type;
	const char* portname;
	const char* caniotype;
	const char* usb_device;

	int timeout_counter;

	float max_xspeed, max_yawspeed;

	bool firstread;

	DualCANIO *canio;
	USBIO *usbio;

	float curr_xspeed, curr_yawspeed;
	float last_xspeed, last_yawspeed;

	// Flag to override the default update of 20Hz
	bool speed_change;

	// Flag set if motors can be enabled (i.e., enable them to be
	// enabled).  Set by a config request.
	bool motor_allow_enable;

	// Flag set if motors are currently enabled
	bool motor_enabled;

	// For handling rollover
	uint32_t last_raw_yaw, last_raw_left, last_raw_right, last_raw_foreaft;

	// Odometry calculation
	double odom_x, odom_y, odom_yaw;

	// helper to handle config requests
	int HandlePositionConfig(QueuePointer &resp_queue, uint32_t subtype, void* data, size_t len);

	// helper to handle config requests
	int HandlePosition3DConfig(QueuePointer &resp_queue, uint32_t subtype, void* data, size_t len);

	// helper to read a cycle of data from the RMP
	int Read();
	int CanBusRead();
	int USBRead();

	// Calculate the difference between two raw counter values, taking care
	// of rollover.
	int Diff(uint32_t from, uint32_t to, bool first);

	// helper to write a packet
	int Write(CanPacket& pkt);

	// Main function for device thread.
	virtual void Main();

	// helper to create a status command packet from the given args
	void MakeStatusCommand(CanPacket* pkt, uint16_t cmd, uint16_t val);

	// helper to take a player command and turn it into a CAN command packet
	void MakeVelocityCommand(CanPacket* pkt, int32_t xspeed, int32_t yawspeed);

	void MakeShutdownCommand(CanPacket* pkt);

	void UpdateData(rmp_frame_t *);
};


