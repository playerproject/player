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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/*
  Desc: Driver for the Segway RMP
  Author: John Sweeney and Brian Gerkey
  Date:
  CVS: $Id$
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_segwayrmp segwayrmp
 * @brief Segway RMP mobile robot

@todo Enable advanced functionality (ramp speeds, etc)

The segwayrmp driver provides control of a Segway RMP (Robotic
Mobility Platform), which is an experimental robotic version of the
Segway HT (Human Transport), a kind of two-wheeled, self-balancing
electric scooter.  Four wheeled versions are also available, and consist
of two independent two-wheeled platforms bolted together.  Thus, two instances
of this driver should be able to control a four-wheeled version.

The driver has been updated to the 3.0 API to the point that it compiles
and runs.  Position2d commands and odometry have been tested and work,
and both batteries report voltages.  Testing has only been done over USB, 
with a Segway RMP400 platform.

This segwayrmp driver supports a usb interface for the newer
RMP UI protocol.  It also maintains compatibility for the older CAN interface.


@par Compile-time dependencies

- libcanlib (from Kvaser)

@par Notes

- Because of its power, weight, height, and dynamics, the Segway RMP is
a potentially dangerous machine.  Be @e very careful with it.

- Although the RMP driver does not actually support motor power control from
software, for safety you must explicitly enable the motors using a
@p PLAYER_POSITION2D_REQ_MOTOR_POWER or @p PLAYER_POSITION3D_REQ_MOTOR_POWER
(depending on which interface you are using).  You must @e also enable
the motors in the command packet, by setting the @p state field to 1.

- For safety, this driver will stop the RMP (i.e., send zero velocities)
if no new command has been received from a client in the previous 400ms or so.
Thus, even if you want to continue moving at a constant velocity, you must
continuously send your desired velocities.

- Most of the configuration requests have not been tested.

- Currently, the only supported type of CAN I/O is "kvaser", which uses
Kvaser, Inc.'s CANLIB interface library.  This library provides access
to CAN cards made by Kvaser, such as the LAPcan II.  However, the CAN
I/O subsystem within this driver is modular, so that it should be pretty
straightforward to add support for other CAN cards.

- This driver uses version 2.0 of the Segway RMP Interface Guide as a
reference.  It may break compatibility with the older CAN only models,
but none were available for testing to verify.


@par Provides

- @ref interface_position2d
  - This interface returns odometry data, and accepts velocity commands.

- @ref interface_position3d
  - This interface returns odometry data (x, y and yaw) from the wheel
  encoders, and attitude data (pitch and roll) from the IMU.  The
  driver accepts velocity commands (x vel and yaw vel).

- @ref interface_power
  - Returns the current battery voltage (72 V when fully charged).

- @ref interface_power
  - Returns the current UI battery voltage (12 V when fully charged).

@par Configuration requests

- position interface
  - PLAYER_POSITION2D_REQ_MOTOR_POWER

- position3d interface
  - PLAYER_POSITION3D_REQ_MOTOR_POWER

@par Requires

- none

@par Configuration file options

- bus (string)
  - Default: "canbus"
  - Communication Protocol: "usb" or "canbus"

- usb_device (string)
  - Default: "/dev/ttyUSB0"
  - Device port, when using bus option "usb"

- canio (string)
  - Default: "kvaser"
  - Type of CANbus driver, when using bus option "canbus"

- max_xspeed (length / sec)
  - Default: 0.5 m/s
  - Maximum linear speed

- max_yawspeed (angle / sec)
  - Default: 40 deg/sec
  - Maximum angular speed

@par Example

@verbatim
driver
(
  name "segwayrmp"
  provides ["position2d:0" "position3d:0" "power:0" "ui:::power:1"]
  bus "usb"
  usb_device "/dev/ttyUSB1"
  max_xspeed 1.5
  max_yawspeed 80
)
@endverbatim

@author John Sweeney, Brian Gerkey, Andrew Howard, Eric Grele, Goutham Mallapragda, Anthony Cascone, Rich Mattes
 */
/** @} */


#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#define PLAYER_ENABLE_MSG 0

#include <libplayercore/playercore.h>

#include "rmp_frame.h"
#include "segwayrmp.h"


// Number of RMP read cycles, without new speed commands from clients,
// after which we'll stop the robot (for safety).  The read loop
// seems to run at about 50Hz, or 20ms per cycle.
#define RMP_TIMEOUT_CYCLES 20 // about 400ms


////////////////////////////////////////////////////////////////////////////////
// A factory creation function
Driver* SegwayRMP_Init(ConfigFile* cf, int section)
{
	// Create and return a new instance of this driver
	return ((Driver*) (new SegwayRMP(cf, section)));
}


////////////////////////////////////////////////////////////////////////////////
// A driver registration function.
void segwayrmp_Register(DriverTable* table)
{
	table->AddDriver("segwayrmp", SegwayRMP_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
SegwayRMP::SegwayRMP(ConfigFile* cf, int section)
: ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
	memset(&this->position_id, 0, sizeof(player_devaddr_t));
	memset(&this->position3d_id, 0, sizeof(player_devaddr_t));
	memset(&this->power_base_id, 0, sizeof(player_devaddr_t));
	memset(&this->power_ui_id, 0, sizeof(player_devaddr_t));

	// Do we create a position interface?
	if(cf->ReadDeviceAddr(&(this->position_id), section, "provides",
			PLAYER_POSITION2D_CODE, -1, NULL) == 0)
	{
		if(this->AddInterface(this->position_id) != 0)
		{
			this->SetError(-1);
			return;
		}
	}

	// Do we create a position3d interface?
	if(cf->ReadDeviceAddr(&(this->position3d_id), section, "provides",
			PLAYER_POSITION3D_CODE, -1, NULL) == 0)
	{
		if(this->AddInterface(this->position3d_id) != 0)
		{
			this->SetError(-1);
			return;
		}
	}

	// Do we create a power interface for powerbase?
	if(cf->ReadDeviceAddr(&(this->power_base_id), section, "provides",
			PLAYER_POWER_CODE, -1, NULL) == 0)
	{
		if(this->AddInterface(this->power_base_id) != 0)
		{
			this->SetError(-1);
			return;
		}
	}

	// Do we create a power interface for userinterface?
	if(cf->ReadDeviceAddr(&(this->power_ui_id), section, "provides",
			PLAYER_POWER_CODE, -1, "ui") == 0)
	{
		if(this->AddInterface(this->power_ui_id) != 0)
		{
			this->SetError(-1);
			return;
		}
	}

	// Determine if we're using the CAN or USB bus
	bus_type = UNKNOWN;
	const char *bus_setting = cf->ReadString(section,"bus","canbus");
	if( strcmp( bus_setting, "canbus" ) == 0 )
	{
		bus_type = CANBUS;
		PLAYER_MSG0(2, "Got CAN Bus");
#if !defined HAVE_CANLIB
    PLAYER_ERROR("CAN bus support not compiled into RMP driver.");
    PLAYER_ERROR("Please rebuild driver with canlib.h");
    this->SetError(-1);
#endif

	}
	else if(strcmp( bus_setting, "usb" ) == 0 )
	{
		PLAYER_MSG0(2, "Got USB Bus");
		bus_type = USB;
	}
	else
	{
		PLAYER_ERROR1("Unknown Bus Type \"%s\"", bus_setting);
		this->SetError(-1);
		return;
	}

	this->canio = NULL;
	this->usbio = 0;

	// Read in port settings
	this->caniotype = cf->ReadString(section, "canio", "kvaser");
	this->usb_device = cf->ReadString(section, "usb_device", "/dev/ttyUSB0" );

	// Read in maximum speeds
	this->max_xspeed = (int) (1000.0 * cf->ReadLength(section, "max_xspeed", 0.5));
	if(this->max_xspeed < 0)
		this->max_xspeed = -this->max_xspeed;
	this->max_yawspeed = (int) (RTOD(cf->ReadAngle(section, "max_yawspeed", DTOR(40))));
	if(this->max_yawspeed < 0)
		this->max_yawspeed = -this->max_yawspeed;

	return;
}


SegwayRMP::~SegwayRMP()
{
	return;
}

int
SegwayRMP::MainSetup()
{
	// Clear the command buffers
#if 0
	if (this->position_id.code)
		ClearCommand(this->position_id);
	if (this->position3d_id.code)
		ClearCommand(this->position3d_id);
#endif

	// Start the USB or CAN bus with a helper function
	if( bus_type == CANBUS )
	{
		if( CanBusSetup() != 0 )
		{
			return -1;
		}
	}
	else if ( bus_type == USB)
	{
		if (USBSetup() !=0)
		{
			return -1;
		}
	}
	else
	{
		PLAYER_ERROR("Unknown Bus type, please select CAN or USB");
		return -1;
	}

	// Initialize odometry
	this->odom_x = this->odom_y = this->odom_yaw = 0.0;

	this->curr_xspeed = this->curr_yawspeed = 0.0;
	this->motor_allow_enable = false;
	this->motor_enabled = false;
	this->firstread = true;
	this->timeout_counter = 0;

	return(0);
}

int SegwayRMP::CanBusSetup() {

#if defined HAVE_CANLIB
	PLAYER_MSG0(2, "CAN bus initializing");

	if(!strcmp(this->caniotype, "kvaser"))
		assert(this->canio = new CANIOKvaser);
	else
	{
		PLAYER_ERROR1("Unknown CAN I/O type: \"%s\"", this->caniotype);
		return(-1);
	}

	// start the CAN at 500 kpbs
	if(this->canio->Init(BAUD_500K) < 0)
	{
		PLAYER_ERROR("Error on CAN Init, could not start CAN bus");
		return(-1);
	}
	return 0;
#else
  PLAYER_ERROR("Error on CAN Init: CAN support not compiled into RMP driver");
  PLAYER_ERROR("Please verify canlib.h is present & rebuild RMP driver");
  return -1;
#endif
}

int SegwayRMP::USBSetup()
{
	usbio = new USBIO();
	PLAYER_MSG0(2,"Starting USB BUS");
	if( !usbio )
	{
		PLAYER_ERROR("Could not create USBIO device");
		return -1;
	}
	if( usbio->Init(usb_device) != 0 )
	{
		PLAYER_ERROR("Error or USB Init, could not start USB bus");
		return -1;
	}

	return 0;
}


void
SegwayRMP::MainQuit()
{
	PLAYER_MSG0(2, "Shutting down CAN bus");
	fflush(stdout);

	// send zero velocities, for some semblance of safety
	CanPacket pkt;

	MakeVelocityCommand(&pkt,0,0);
	Write(pkt);

	// shutdown the appropraite device
	if( bus_type == CANBUS ) {
		// shutdown the CAN
		PLAYER_MSG0(2, "Shutting down CAN bus");
		fflush(stdout);
		canio->Shutdown();
		delete canio;
		canio = NULL;
	}
	else if (bus_type == USB ) {
		// shutdown the USB
		PLAYER_MSG0(2, "Shutting down USB bus");
		fflush(stdout);
		usbio->Shutdown();
		delete usbio;
		usbio = 0;
	}
	return;
}

// Main function for device thread.
void
SegwayRMP::Main()
{
	//unsigned char buffer[256];
	//size_t buffer_len;
	//player_position2d_cmd_t position_cmd;
	//player_position3d_cmd_t position3d_cmd;
	//void *client;
	CanPacket pkt;
	//int32_t xspeed,yawspeed;
	//bool got_command;
	bool first;
	struct timeval now;
	double time_stamp, sent_command_time;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	first = true;
	sent_command_time = 0.0;
	PLAYER_MSG0(2, "starting main loop");

	for(;;)
	{
		pthread_testcancel();
		speed_change = false;

		ProcessMessages();
		// Read from the RMP

		if(Read() < 0)
		{
			PLAYER_ERROR("Read() errored; bailing");
			pthread_exit(NULL);
		}

		// Note that we got some data
		if (first)
		{
			first = false;
			PLAYER_MSG0(2, "got data from rmp");
		}

		///@ TODO
		/// report better timestamps, possibly using time info from the RMP

		// Send data to clients
		Publish(this->position_id, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
				&this->position_data, sizeof(this->position_data), NULL);
		Publish(this->position3d_id, PLAYER_MSGTYPE_DATA, PLAYER_POSITION3D_DATA_STATE,
				&this->position3d_data, sizeof(this->position3d_data), NULL);
		Publish(this->power_base_id, PLAYER_MSGTYPE_DATA, PLAYER_POWER_DATA_STATE,
				&this->power_base_data, sizeof(this->power_base_data), NULL);
		Publish(this->power_ui_id, PLAYER_MSGTYPE_DATA, PLAYER_POWER_DATA_STATE,
				&this->power_ui_data, sizeof(this->power_ui_data), NULL);

/*  // check for config requests from the position interface
    if((buffer_len = GetConfig(this->position_id, &client, buffer, sizeof(buffer),NULL)) > 0)
    {
      // if we write to the CAN bus as a result of the config, don't write
      // a velocity command (may need to make this smarter if we get slow
      // velocity control).
      if(HandlePositionConfig(client,buffer,buffer_len) > 0)
        continue;
    }

    // check for config requests from the position3d interface
    if((buffer_len = GetConfig(this->position3d_id, &client, buffer, sizeof(buffer),NULL)) > 0)
    {
      // if we write to the CAN bus as a result of the config, don't write
      // a velocity command (may need to make this smarter if we get slow
      // velocity control).
      if(HandlePosition3DConfig(client,buffer,buffer_len) > 0)
        continue;
    }

    // start with the last commanded values
    xspeed = this->last_xspeed;
    yawspeed = this->last_yawspeed;
    got_command = false;

    // Check for commands from the position interface
    if (this->position_id.code)
    {
      if(GetCommand(this->position_id, (void*) &position_cmd,
                    sizeof(position_cmd),NULL))
      {
        // zero the command buffer, so that we can timeout if a client doesn't
        // send commands for a while
        ClearCommand(this->position_id);

        // convert to host order; let MakeVelocityCommand do the rest
        xspeed = ntohl(position_cmd.xspeed);
        yawspeed = ntohl(position_cmd.yawspeed);
        motor_enabled = position_cmd.state && motor_allow_enable;
        timeout_counter=0;
        got_command = true;
      }
    }

    // Check for commands from the position3d interface
    if (this->position3d_id.code)
    {
      if(GetCommand(this->position3d_id, (void*) &position3d_cmd,
                    sizeof(position3d_cmd),NULL))
      {
        // zero the command buffer, so that we can timeout if a client doesn't
        // send commands for a while
        ClearCommand(this->position3d_id);

        // convert to host order; let MakeVelocityCommand do the rest
        // Position3d uses milliradians/sec, so convert here to
        // degrees/sec
        xspeed = ntohl(position3d_cmd.xspeed);
        yawspeed = (int32_t) (((double) (int32_t) ntohl(position3d_cmd.yawspeed)) / 1000 * 180 / M_PI);
        motor_enabled = position3d_cmd.state && motor_allow_enable;
        timeout_counter=0;
        got_command = true;
      }
    }
    // No commands, so we may timeout soon
    if (!got_command)*/
		// Increment counter, note, counter is reset when a valid command is received
		// in ProcessMessages
		timeout_counter++;

		if(timeout_counter >= RMP_TIMEOUT_CYCLES)
		{
			if(curr_xspeed || curr_yawspeed)
			{
				/// @todo
				/// We are getting intermittent timeouts. Does not break functionality, but should be checked.

				PLAYER_WARN("timeout exceeded without new commands; stopping robot");
				curr_xspeed = 0;
				curr_yawspeed = 0;
				speed_change = true;
			}
			// set it to the limit, to prevent overflow, but keep the robot
			// stopped until a new command comes in.
			timeout_counter = RMP_TIMEOUT_CYCLES;
		}

		if(!motor_enabled)
		{
			curr_xspeed = 0;
			curr_yawspeed = 0;
		}
		PLAYER_MSG2(2,"setting velocity to curr_xspeed %f curr_yawspeed %f\n",curr_xspeed,curr_yawspeed);

		// make a velocity command... could be zero
		// make sure we send commands at a rate of 20Hz, or on speed change
		// note this condition does not affect the timeout function above
		gettimeofday( &now, 0 );
		time_stamp = (1.0*now.tv_sec) + (0.000001*now.tv_usec);

		if( time_stamp - sent_command_time > 0.05 || speed_change )
		{
			MakeVelocityCommand(&pkt,static_cast<int> (curr_xspeed),static_cast<int> (curr_yawspeed));

			if(Write(pkt) < 0)
				PLAYER_ERROR("error on write");

			sent_command_time = time_stamp;
		}
	}
}

int
SegwayRMP::ProcessMessage(QueuePointer & resp_queue,
		player_msghdr * hdr,
		void * data)
{
	/// @todo
	/// Handle config requests
	HANDLE_CAPABILITY_REQUEST(position_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
	HANDLE_CAPABILITY_REQUEST(position3d_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);

	HANDLE_CAPABILITY_REQUEST(position_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL);
	HANDLE_CAPABILITY_REQUEST(position_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_MOTOR_POWER);
	HANDLE_CAPABILITY_REQUEST(position_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_RESET_ODOM);
	
	HANDLE_CAPABILITY_REQUEST(position3d_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_POSITION3D_CMD_SET_VEL);
	HANDLE_CAPABILITY_REQUEST(position3d_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_POSITION3D_REQ_MOTOR_POWER);

	// 2-D velocity command
	if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
			PLAYER_POSITION2D_CMD_VEL,
			this->position_id))
	{
		player_position2d_cmd_vel_t* cmd = (player_position2d_cmd_vel_t*)data;
		this->curr_xspeed = cmd->vel.px * 1000.0; //Added multiply by 1000.0, BMS.
		this->curr_yawspeed = cmd->vel.pa * 180.0/M_PI;  //Added multiply by 1000.0, BMS.
		this->motor_enabled = cmd->state & this->motor_allow_enable;
		this->timeout_counter = 0;
		return 0;
	}
	// 3-D velocity command
	if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
			PLAYER_POSITION3D_CMD_SET_VEL,
			this->position3d_id))
	{
		player_position3d_cmd_vel_t* cmd = (player_position3d_cmd_vel_t*)data;
		this->curr_xspeed = cmd->vel.px * 1000.0; //Added multiply by 1000.0, BMS.
		this->curr_yawspeed = cmd->vel.pyaw * 180.0/M_PI; ////Added multiply by 1000.0, BMS.
		this->motor_enabled = cmd->state & this->motor_allow_enable;
		this->timeout_counter = 0;
		return 0;
	}

	if (hdr->type == PLAYER_MSGTYPE_REQ && hdr->addr.interf == position_id.interf
			&& hdr->addr.index == position_id.index)
	{
		return HandlePositionConfig(resp_queue, hdr->subtype, data, hdr->size);
	}

	if (hdr->type == PLAYER_MSGTYPE_REQ && hdr->addr.interf == position3d_id.interf
			&& hdr->addr.index == position3d_id.index)
	{
		return HandlePosition3DConfig(resp_queue, hdr->subtype, data, hdr->size);
	}



	return(-1);
}

// helper to handle config requests
// returns 1 to indicate we wrote to the CAN bus
// returns 0 to indicate we did NOT write to CAN bus
int
SegwayRMP::HandlePositionConfig(QueuePointer &client, uint32_t subtype, void* buffer, size_t len)
{
	//uint16_t rmp_cmd,rmp_val;
	//player_rmp_config_t *rmp;
	CanPacket pkt;

	switch(subtype)
	{
	case PLAYER_POSITION2D_REQ_MOTOR_POWER:
		// just set a flag telling us whether we should
		// act on motor commands
		// set the commands to 0... think it will automatically
		// do this for us.
		if(((char *) buffer)[0])
			this->motor_allow_enable = true;
		else
			this->motor_allow_enable = false;

		printf("SEGWAYRMP: motors state: %d\n", this->motor_allow_enable);

		Publish(position_id, client, PLAYER_MSGTYPE_RESP_ACK,subtype);
		return 0;

	case PLAYER_POSITION2D_REQ_GET_GEOM:
		player_position2d_geom_t geom;
		geom.pose.px = 0;
		geom.pose.py = 0;
		geom.pose.pz = 0;
		geom.pose.proll= 0;
		geom.pose.ppitch = 0;
		geom.pose.pyaw = 0;
		geom.size.sw = 0.508;
		geom.size.sl = 0.610;

		Publish(position_id, client, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_GET_GEOM, &geom, sizeof(geom),NULL);
		return 0;

	case PLAYER_POSITION2D_REQ_RESET_ODOM:
		// we'll reset all the integrators

		MakeStatusCommand(&pkt, (uint16_t)RMP_CAN_CMD_RST_INT,
				(uint16_t)RMP_CAN_RST_ALL);
		if(Write(pkt) < 0)
		{
			Publish(position_id, client, PLAYER_MSGTYPE_RESP_NACK,PLAYER_POSITION2D_REQ_RESET_ODOM);
		}
		else
		{

			if (Write(pkt) < 0) {
				Publish(position_id, client, PLAYER_MSGTYPE_RESP_NACK,PLAYER_POSITION2D_REQ_RESET_ODOM);
			} else {
				Publish(position_id, client, PLAYER_MSGTYPE_RESP_ACK,PLAYER_POSITION2D_REQ_RESET_ODOM);
			}
		}

		odom_x = odom_y = odom_yaw = 0.0;
		firstread = true;

		// return 1 to indicate that we wrote to the CAN bus this time
		return(0);

/*    case PLAYER_POSITION_RMP_VELOCITY_SCALE:
      rmp_cmd = RMP_CAN_CMD_MAX_VEL;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      printf("SEGWAYRMP: velocity scale %d\n", rmp_val);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_ACCEL_SCALE:
      rmp_cmd = RMP_CAN_CMD_MAX_ACCL;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_TURN_SCALE:
      rmp_cmd = RMP_CAN_CMD_MAX_TURN;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_GAIN_SCHEDULE:
      rmp_cmd = RMP_CAN_CMD_GAIN_SCHED;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_CURRENT_LIMIT:
      rmp_cmd = RMP_CAN_CMD_CURR_LIMIT;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_RST_INTEGRATORS:
      rmp_cmd = RMP_CAN_CMD_RST_INT;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if (Write(pkt) < 0) {
          if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
            PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
        } else {
          if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
            PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
        }
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_SHUTDOWN:
      MakeShutdownCommand(&pkt);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);
		 */
	default:
		printf("segwayrmp received unknown config request %d\n",subtype);
		break;
	}

	// return -1, to indicate that we did NOT handle the message
	return(-1);
}

// helper to handle config requests
// returns 1 to indicate we wrote to the CAN bus
// returns 0 to indicate we did NOT write to CAN bus
int
SegwayRMP::HandlePosition3DConfig(QueuePointer &client, uint32_t subtype, void* buffer, size_t len)
{
	switch(subtype)
	{
	case PLAYER_POSITION3D_REQ_MOTOR_POWER:
		// just set a flag telling us whether we should
		// act on motor commands
		// set the commands to 0... think it will automatically
		// do this for us.
		if(((char*)buffer)[0])
			this->motor_allow_enable = true;
		else
			this->motor_allow_enable = false;

		printf("SEGWAYRMP: motors state: %d\n", this->motor_allow_enable);

		Publish(this->position3d_id, client, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION3D_REQ_MOTOR_POWER);
		return 0;

	default:
		printf("segwayrmp received unknown config request %d\n",
				subtype);
	}

	// return -1, to indicate that we did process the message
	return(-1);
}


int
SegwayRMP::Read()
{
	if( bus_type == CANBUS )
		return CanBusRead();
	else if( bus_type == USB )
		return USBRead();
	else
		return -1;
}

int SegwayRMP::CanBusRead()
{
	CanPacket pkt;
	int channel;
	int ret;
	rmp_frame_t data_frame[2];

	//static struct timeval last;
	//struct timeval curr;

	data_frame[0].ready = 0;
	data_frame[1].ready = 0;

	// read one cycle of data from each channel
	for(channel = 0; channel < DUALCAN_NR_CHANNELS; channel++)
	{
		ret=0;
		// read until we've gotten all 5 packets for this cycle on this channel
		while((ret = canio->ReadPacket(&pkt, channel)) >= 0)
		{
			// then we got a new packet...
			//printf("SEGWAYIO: pkt: %s\n", pkt.toString());

			data_frame[channel].AddPacket(pkt);

			// if data has been filled in, then let's make it the latest data
			// available to player...
			if(data_frame[channel].IsReady())
			{
				// Only bother doing the data conversions for one channel.
				// It appears that the data on channel 0 is garbarge, so we'll read
				// from channel 1.
				if(channel == 1)
				{
					UpdateData(&data_frame[channel]);
				}

				data_frame[channel].ready = 0;
				break;
			}
		}

		if (ret < 0)
		{
			PLAYER_ERROR2("error (%d) reading packet on channel %d", ret, channel);
		}
	}

	return(0);
}

int SegwayRMP::USBRead() {
	CanPacket pkt;
	int ret;
	rmp_frame_t data_frame;

	//static struct timeval last;
	//struct timeval curr;

	data_frame.ready = 0;

	ret=0;
	// read until we've gotten all 8 packets for this cycle on this channel
	while((ret = usbio->ReadPacket(&pkt)) >= 0)
	{
		// then we got a new packet...
		/*printf("SEGWAYIO: READ : pkt: %s\n", pkt.toString());
    fflush(stdout);*/

		data_frame.AddPacket(pkt);

		// if data has been filled in, then let's make it the latest data
		// available to player...

		if(data_frame.IsReady()) {
			UpdateData(&data_frame);
			data_frame.ready = 0;
			break;
		}
	}

	if (ret < 0) {
		PLAYER_ERROR1("error (%d) reading packet on usb", ret);
	}

	return(0);
}


void
SegwayRMP::UpdateData(rmp_frame_t * data_frame)
{
	int delta_lin_raw, delta_ang_raw;
	double delta_lin, delta_ang;
	double tmp;

	// Get the new linear and angular encoder values and compute
	// odometry.  Note that we do the same thing here, regardless of
	// whether we're presenting 2D or 3D position info.
	delta_lin_raw = Diff(this->last_raw_foreaft,
			data_frame->foreaft,
			this->firstread);
	this->last_raw_foreaft = data_frame->foreaft;

	delta_ang_raw = Diff(this->last_raw_yaw,
			data_frame->yaw,
			this->firstread);
	this->last_raw_yaw = data_frame->yaw;

	delta_lin = (double)delta_lin_raw / (double)RMP_COUNT_PER_M;
	delta_ang = DTOR((double)delta_ang_raw /
			(double)RMP_COUNT_PER_REV * 360.0);

	// First-order odometry integration
	this->odom_x += delta_lin * cos(this->odom_yaw);
	this->odom_y += delta_lin * sin(this->odom_yaw);
	this->odom_yaw += delta_ang;

	// Normalize yaw in [0, 360]
	this->odom_yaw = atan2(sin(this->odom_yaw), cos(this->odom_yaw));
	if (this->odom_yaw < 0)
		this->odom_yaw += 2 * M_PI;

	// first, do 2D info.
	this->position_data.pos.px = this->odom_x;
	this->position_data.pos.py = this->odom_y;
	this->position_data.pos.pa = this->odom_yaw;

	// combine left and right wheel velocity to get foreward velocity
	// change from counts/s into mm/s
	this->position_data.vel.px = ((double)data_frame->left_dot +
			(double)data_frame->right_dot) /
			(double)RMP_COUNT_PER_M_PER_S
			/ 2.0;

	// no side speeds for this bot
	this->position_data.vel.py = 0;

	// from counts/sec into deg/sec.  also, take the additive
	// inverse, since the RMP reports clockwise angular velocity as
	// positive.
	this->position_data.vel.pa =
		DTOR(-(double)data_frame->yaw_dot / (double)RMP_COUNT_PER_DEG_PER_S);

	this->position_data.stall = 0;

	// now, do 3D info.
	this->position3d_data.pos.px = this->odom_x;
	this->position3d_data.pos.py = this->odom_y;
	// this robot doesn't fly
	this->position3d_data.pos.pz = 0;

	/// TODO left off here

	// normalize angles to [0,360]
	tmp = NORMALIZE(DTOR((double)data_frame->roll /
			(double)RMP_COUNT_PER_DEG));
	if(tmp < 0)
		tmp += 2*M_PI;
	this->position3d_data.pos.proll = tmp;//htonl((int32_t)rint(tmp * 1000.0));

	// normalize angles to [0,360]
	tmp = NORMALIZE(DTOR((double)data_frame->pitch /
			(double)RMP_COUNT_PER_DEG));
	if(tmp < 0)
		tmp += 2*M_PI;
	this->position3d_data.pos.ppitch = tmp;//htonl((int32_t)rint(tmp * 1000.0));

	this->position3d_data.pos.pyaw = tmp;//htonl(((int32_t)(this->odom_yaw * 1000.0)));

	// combine left and right wheel velocity to get foreward velocity
	// change from counts/s into m/s
	this->position3d_data.vel.px =
		((double)data_frame->left_dot +
				(double)data_frame->right_dot) /
				(double)RMP_COUNT_PER_M_PER_S
				/ 2.0;
	// no side or vertical speeds for this bot
	this->position3d_data.vel.py = 0;
	this->position3d_data.vel.pz = 0;

	this->position3d_data.vel.proll =
		(double)data_frame->roll_dot /
		(double)RMP_COUNT_PER_DEG_PER_S * M_PI / 180;
	this->position3d_data.vel.ppitch =
		(double)data_frame->pitch_dot /
		(double)RMP_COUNT_PER_DEG_PER_S * M_PI / 180;
	// from counts/sec into millirad/sec.  also, take the additive
	// inverse, since the RMP reports clockwise angular velocity as
	// positive.

	// This one uses left_dot and right_dot, which comes from odometry
	this->position3d_data.vel.pyaw =
		(double)(data_frame->right_dot - data_frame->left_dot) /
		(RMP_COUNT_PER_M_PER_S * RMP_GEOM_WHEEL_SEP * M_PI);
	// This one uses yaw_dot, which comes from the IMU
	//data.position3d_data.yawspeed =
	//  htonl((int32_t)(-rint((double)data_frame->yaw_dot /
	//                        (double)RMP_COUNT_PER_DEG_PER_S * M_PI / 180 * 1000)));

	this->position3d_data.stall = 0;


	// Get Battery voltage from base (72v pack)
	// Manual says the segway returns 4 counts per volt
	this->power_base_data.volts =
		data_frame->powerbase_battery /4;
	this->power_base_data.valid = PLAYER_POWER_MASK_VOLTS;

	// Manual says the segway returns (1.4 + counts * .0125) volts
	this->power_ui_data.volts =
		(data_frame->ui_battery * .0125) + 1.4;
	this->power_ui_data.valid = PLAYER_POWER_MASK_VOLTS;
	firstread = false;
}


int
SegwayRMP::Write(CanPacket& pkt)
{
	if( bus_type == CANBUS )
		return(canio->WritePacket(pkt));
	else if( bus_type == USB )
		return usbio->WritePacket(pkt);
	else
		return -1;
}

/* Creates a status CAN packet from the given arguments
 */
void
SegwayRMP::MakeStatusCommand(CanPacket* pkt, uint16_t cmd, uint16_t val)
{
	int16_t trans,rot;

	pkt->id = RMP_CAN_ID_COMMAND;
	pkt->PutSlot(2, cmd);

	// it was noted in the windows demo code that they
	// copied the 8-bit value into both bytes like this
	pkt->PutByte(6, val);
	pkt->PutByte(7, val);

	trans = (int16_t) rint((double)this->curr_xspeed *
			(double)RMP_COUNT_PER_MM_PER_S);

	if(trans > RMP_MAX_TRANS_VEL_COUNT)
		trans = RMP_MAX_TRANS_VEL_COUNT;
	else if(trans < -RMP_MAX_TRANS_VEL_COUNT)
		trans = -RMP_MAX_TRANS_VEL_COUNT;

	rot = (int16_t) rint((double)this->curr_yawspeed *
			(double)RMP_COUNT_PER_DEG_PER_SS);

	if(rot > RMP_MAX_ROT_VEL_COUNT)
		rot = RMP_MAX_ROT_VEL_COUNT;
	else if(rot < -RMP_MAX_ROT_VEL_COUNT)
		rot = -RMP_MAX_ROT_VEL_COUNT;

	// put in the last speed commands as well
	pkt->PutSlot(0,(uint16_t)trans);
	pkt->PutSlot(1,(uint16_t)rot);

	if(cmd)
	{
		printf("SEGWAYIO: STATUS: cmd: %02x val: %02x pkt: %s\n",
				cmd, val, pkt->toString());
	}
}

/* takes a player command (in host byte order) and turns it into CAN packets
 * for the RMP
 */
void
SegwayRMP::MakeVelocityCommand(CanPacket* pkt,
		int32_t xspeed,
		int32_t yawspeed)
{
	pkt->id = RMP_CAN_ID_COMMAND;
	pkt->PutSlot(2, (uint16_t)RMP_CAN_CMD_NONE);

	// we only care about cmd.xspeed and cmd.yawspeed
	// translational velocity is given to RMP in counts
	// [-1176,1176] ([-8mph,8mph])

	// player is mm/s
	// 8mph is 3576.32 mm/s
	// so then mm/s -> counts = (1176/3576.32) = 0.32882963

	if(xspeed > this->max_xspeed)
	{
		PLAYER_WARN2("xspeed thresholded! (%d > %d)", xspeed, this->max_xspeed);
		xspeed = this->max_xspeed;
	}
	else if(xspeed < -this->max_xspeed)
	{
		PLAYER_WARN2("xspeed thresholded! (%d < %d)", xspeed, -this->max_xspeed);
		xspeed = -this->max_xspeed;
	}

	this->curr_xspeed = xspeed;

	int16_t trans = (int16_t) rint((double)xspeed *
			(double)RMP_COUNT_PER_MM_PER_S);

	if(trans > RMP_MAX_TRANS_VEL_COUNT)
		trans = RMP_MAX_TRANS_VEL_COUNT;
	else if(trans < -RMP_MAX_TRANS_VEL_COUNT)
		trans = -RMP_MAX_TRANS_VEL_COUNT;

	if(yawspeed > this->max_yawspeed)
	{
		PLAYER_WARN2("yawspeed thresholded! (%d > %d)",
				yawspeed, this->max_yawspeed);
		yawspeed = this->max_yawspeed;
	}
	else if(yawspeed < -this->max_yawspeed)
	{
		PLAYER_WARN2("yawspeed thresholded! (%d < %d)",
				yawspeed, -this->max_yawspeed);
		yawspeed = -this->max_yawspeed;
	}
	this->curr_yawspeed = yawspeed;

	// rotational RMP command \in [-1024, 1024]
	// this is ripped from rmi_demo... to go from deg/s to counts
	// deg/s -> count = 1/0.013805056
	int16_t rot = (int16_t) rint((double)yawspeed *
			(double)RMP_COUNT_PER_DEG_PER_S);

	if(rot > RMP_MAX_ROT_VEL_COUNT)
		rot = RMP_MAX_ROT_VEL_COUNT;
	else if(rot < -RMP_MAX_ROT_VEL_COUNT)
		rot = -RMP_MAX_ROT_VEL_COUNT;

	pkt->PutSlot(0, (uint16_t)trans);
	pkt->PutSlot(1, (uint16_t)rot);
}

void
SegwayRMP::MakeShutdownCommand(CanPacket* pkt)
{
	pkt->id = RMP_CAN_ID_SHUTDOWN;

	printf("SEGWAYIO: SHUTDOWN: pkt: %s\n",
			pkt->toString());
}

// Calculate the difference between two raw counter values, taking care
// of rollover.
int
SegwayRMP::Diff(uint32_t from, uint32_t to, bool first)
{
	int diff1, diff2;
	static uint32_t max = (uint32_t)pow(2.0,32)-1;

	// if this is the first time, report no change
	if(first)
		return(0);

	diff1 = to - from;

	/* find difference in two directions and pick shortest */
	if(to > from)
		diff2 = -(from + max - to);
	else
		diff2 = max - from + to;

	if(abs(diff1) < abs(diff2))
		return(diff1);
	else
		return(diff2);
}
