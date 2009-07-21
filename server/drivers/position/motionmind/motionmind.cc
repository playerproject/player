/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 *  mmdriver.cpp
 *  motionminddriver
 *
 *  Created by Chris Chambers on 31/03/08.
 *
 */

/** @ingroup drivers Drivers */
/** @{ */
/** @defgroup driver_motionmind motionmind
 * @brief Solutions Cubed motor controller

The motionmind driver is for communicating with a Solutions Cubed "Motion Mind" PID motor controller while it is in serial PID mode.
Multiple boards can be daisy chained together and different drivers used for each one by simply giving each driver the address
of the board that it is commanding. This driver allows for simple absolute position commands and publishes the position of the device as well.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_position1d : publishes the position of the motor and allows for absolute position commands to be sent

@par Requires

- @ref interface_opaque : used to get the serial information form the motion mind control boards

@par Configuration requests

- none

@par Supported commands

- PLAYER_POSITION_1D_CMD_POS: The absolute position to send the actuator to
- @todo : add support for PLAYER_POSITON_1D_CMD_VEL

@par Configuration file options

- address (int)
  - Default 1
  - Address of the motionmind board that you wish to control
- cpr (int)
  - Default 500
  - counts per motor rotation
- gear_ratio
  - Default 1.0
  - n:1 gear_ratio - robot position in m or rad:motor rotation
	- divide the gear_ratio by 2*PI for rotational actuators

@par Example

@verbatim
# Board number 1
driver(
  name "motionmind"
  provides ["position1d:0"]
  requires ["opaque:0"]
  address 1
)

#Board Number 2
driver(
  name "motionmind"
  provides ["position1d:1"]
  requires ["opaque:0"]
  address 2
  cpr 500
  gear_ratio 2.0
)

driver(
  name "serialstream"
  port "/dev/ttyS0"
  transfer_rate 19200
  parity "none"
  provides ["opaque:0"]
  alwayson 1
  # IF ATTACHED TO MORE THAN ONE MOTIONMIND BOARD YOU MUST HAVE A WAIT TIME OR THINGS TIMEOUT
  wait_time 40000
)
@endverbatim

@author Chris Chambers

*/

/** @} */

#if !defined (WIN32)
  #include <unistd.h>
#endif
#include <string.h>
#include <time.h>

#include <libplayercore/playercore.h>

#define DEFAULT_RX_BUFFER_SIZE 128
#define DEFAULT_ADDRESS 1
#define MESSAGE_LENGTH 7
#define MM_WRITE_MESSAGE_LENGTH 8
#define MM_MSG_WAIT 20000  //microseconds to wait before sending another command 
                           //  s/b 1250 according documentation but had comm errors below 20000
#define MSG_TIMEOUT 250000 //microseconds before it sends another read request if it hasn't yet got a reply
#define MM_DATA_WAIT 2500  //microseconds to wait before checking for response data
#define MM_CPU_WAIT 10000  //microseconds to wait before checking for missing response data and to prevent CPU overloading
#define MM_DEFAULT_CPR 500
#define MM_DEFAULT_GEAR_RATIO 1.0
#define MM_READ_POSITION 0x01 // Data0
#define MM_READ_STATUS 0x01 // Data2
#define MM_WRITE_REG 0x18

// Status register bit packing
#define MM_STATUS_NEGLIMIT     0x0001
#define MM_STATUS_POSLIMIT     0x0002
#define MM_STATUS_BRAKE        0x0004
#define MM_STATUS_INDEX        0x0008
#define MM_STATUS_BADRC        0x0010
#define MM_STATUS_VNLIMIT      0x0020
#define MM_STATUS_VPLIMIT      0x0040
#define MM_STATUS_CURRENTLIMIT 0x0080
#define MM_STATUS_PWMLIMIT     0x0100
#define MM_STATUS_INPOSITION   0x0200

// Register indexes for WRITE and WRITE STORE commands
#define MM_REG_POSITION        0x00

///////////////////////////////////////////////////////////////////////////////
// The class for the driver
class MotionMind : public ThreadedDriver
{
  public:
    // Constructor; need that
    MotionMind(ConfigFile* cf, int section);
	~MotionMind();

    // Must implement the following methods.
    virtual int MainSetup();
    virtual void MainQuit();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr* hdr,
                               void* data);

  private:

	// Main function for device thread.
	virtual void Main();

	//Requests the pos of the robot if it hasnt been already
	void FindCurrentPos();
	//Requests the status register of the robot if it hasn't been already
	void FindCurrentStatus();

	// Checks whether or not you have sent a request for the position
	bool pos_request_sent;
	bool status_request_sent;
	struct timeval msg_sent;
	struct timeval time_sent_pos;
	struct timeval time_sent_status;

	int MsgWait();
	//Makes a command to be sent to the opaque driver
	void makeAbsolutePositionCommand(uint8_t* buffer, unsigned int address, float position);

	//Makes a command which requests the current osition bback from the motor ocntroller
	void makeReadPositionCommand(uint8_t* buffer, unsigned int address);

	//Makes a command which requests the current status back from the motor controller
	void makeReadStatusCommand(uint8_t* buffer, unsigned int address);

	//Makes a command which sets the odometry to the given value
	void makeSetOdomReq(uint8_t* buffer, unsigned int address, const float position);

	//Converts robot positions to absolute motionmind positions
	int robot2abspos(const float position);

	// The function to do stuff here
	void DoStuff();

	// Opaque Driver info
    Device *opaque;
    player_devaddr_t opaque_id;

	// The address of the board being addressed
	uint8_t address;

	// rx buffer
    uint8_t * rx_buffer;
    unsigned int rx_buffer_size;
    unsigned int rx_count;

	// player position1 data odometric pose, velocity and motor stall info
		player_position1d_data_t pos_data;

	// counts per rotation
		int cpr;

	// gear ratio robot:motor
		double gear_ratio;
};



////////////////////////////////////////////////////////////////////////////////
// Now the driver

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver*
MotionMind_Init(ConfigFile* cf, int section)
{
	// Create and return a new instance of this driver
	return ((Driver*)(new MotionMind(cf, section)));

}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void motionmind_Register(DriverTable* table)
{
	table->AddDriver("motionmind", MotionMind_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
MotionMind::MotionMind(ConfigFile* cf, int section)
    : ThreadedDriver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_POSITION1D_CODE)
{
	this->opaque = NULL;
	// Must have an opaque device
	if (cf->ReadDeviceAddr(&this->opaque_id, section, "requires",
                       PLAYER_OPAQUE_CODE, -1, NULL) != 0)
	{
		puts ("No Opaque driver specified");
		this->SetError(-1);
		return;
	}

	// Read options from the configuration file
	address = cf->ReadInt(section, "address", DEFAULT_ADDRESS);

	rx_count = 0;
	rx_buffer_size = cf->ReadInt(section, "buffer_size", DEFAULT_RX_BUFFER_SIZE);
	rx_buffer = new uint8_t[rx_buffer_size];
	assert(rx_buffer);

	cpr = cf->ReadInt(section, "cpr", MM_DEFAULT_CPR);

	this->gear_ratio = cf->ReadFloat(section, "gear_ratio", MM_DEFAULT_GEAR_RATIO);
	if (this->gear_ratio == 0.0) fprintf (stderr,"gear_ratio cannot be 0.0: adjust your gear_ratio value");
	assert(this->gear_ratio != 0.0);

	this->pos_request_sent = false;
	this->status_request_sent = false;
	GlobalTime->GetTime(&(this->msg_sent));
	return;
}

MotionMind::~MotionMind()
{
	delete [] rx_buffer;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int MotionMind::MainSetup()
{
	puts("Setting up MotionMind driver");

	if(Device::MatchDeviceAddress(this->opaque_id, this->device_addr))
	{
		PLAYER_ERROR("attempt to subscribe to self");
		return(-1);
	}

	if(!(this->opaque = deviceTable->GetDevice(this->opaque_id)))
	{
		PLAYER_ERROR("unable to locate suitable opaque device");
		return(-1);
	}

	if(this->opaque->Subscribe(this->InQueue) != 0)
	{
		PLAYER_ERROR("unable to subscribe to opaque device");
		return(-1);
	}

	puts("MotionMind driver ready");

	return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void MotionMind::MainQuit()
{
	puts("MotionMind driver down");

	opaque->Unsubscribe(InQueue);

	puts("MotionMind driver has been shutdown");

}



// Process an incoming message
int MotionMind::ProcessMessage(QueuePointer & resp_queue,
                                 player_msghdr* hdr,
                                 void* data)
{
	assert(hdr);
	assert(data);

	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_OPAQUE_DATA_STATE, opaque_id))
	{
		player_opaque_data_t * recv = reinterpret_cast<player_opaque_data_t * > (data);
		unsigned int messageOffset = rx_count;
		rx_count += recv->data_count;
		if (rx_count > rx_buffer_size)
		{
			PLAYER_WARN("MotionMind driver Buffer Full");
			rx_count = 0;
		}
		else
		{
			memcpy(&rx_buffer[messageOffset], recv->data, recv->data_count);
		}
		return 0;
	}
	if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
							 PLAYER_POSITION1D_CMD_POS,
							 this->device_addr))
	{
		assert(hdr->size == sizeof(player_position1d_cmd_pos_t));
		player_position1d_cmd_pos_t * recv = reinterpret_cast<player_position1d_cmd_pos_t *> (data);
		uint8_t * buffer = new uint8_t[MESSAGE_LENGTH];
		this->makeAbsolutePositionCommand(buffer, this->address, recv->pos);
		player_opaque_data_t mData;
		mData.data_count = MESSAGE_LENGTH;
		mData.data = buffer;
		this->MsgWait();
		opaque->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, reinterpret_cast<void*>(&mData),0,NULL);
		GlobalTime->GetTime(&(this->msg_sent));
		delete [] buffer;
		return(0);
	}
	if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, 
							PLAYER_POSITION1D_REQ_SET_ODOM,
							this->device_addr))
	{
		assert(hdr->size == sizeof(player_position1d_set_odom_req_t));
		player_position1d_set_odom_req_t* req = reinterpret_cast<player_position1d_set_odom_req_t*> (data);
		uint8_t* buffer = new uint8_t[MM_WRITE_MESSAGE_LENGTH];
		this->makeSetOdomReq(buffer, this->address, req->pos);
		player_opaque_data_t mData;
		mData.data_count = MM_WRITE_MESSAGE_LENGTH;
		mData.data = buffer;
		this->MsgWait();
		opaque->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, reinterpret_cast<void*>(&mData),0,NULL);
		GlobalTime->GetTime(&(this->msg_sent));
		delete [] buffer;
		return(0);
	}

	return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void MotionMind::Main()
{

	// The main loop; interact with the device here
	for(;;)
	{
		// test if we are supposed to cancel
		pthread_testcancel();

		// Process incoming messages
		ProcessMessages();

		// Ask for the current position
		FindCurrentPos();
		// Ask for the current position status
		FindCurrentStatus();
		// Publish position data
		this->Publish(this->device_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION1D_DATA_STATE, (void*)&pos_data);

		usleep(MM_CPU_WAIT);
	}
	return;
}

// Makes the required serial packet to command a position or request the position
void MotionMind::makeAbsolutePositionCommand(uint8_t* buffer, unsigned int address, float position)
{
	unsigned int checksum = 0;
	int posInt = robot2abspos(position);
	char* pos = (char*)&posInt;
	buffer[0] = 0x15;
	checksum += buffer[0];
	buffer[1] = address;
	checksum += buffer[1];
	buffer[2] = pos[0];
	checksum += buffer[2];
	buffer[3] = pos[1];
	checksum += buffer[3];
	buffer[4] = pos[2];
	checksum += buffer[4];
	buffer[5] = pos[3];
	checksum += buffer[5];
	buffer[6] = checksum;
}

void MotionMind::makeReadPositionCommand(uint8_t* buffer, unsigned int address)
{
	unsigned int checksum = 0;
	// Command
	buffer[0] = 0x1A;
	checksum += buffer[0];
	// Address
	buffer[1] = address;
	checksum += buffer[1];
	// Data0
	buffer[2] = MM_READ_POSITION;
	checksum += buffer[2];
	// Data1
	buffer[3] = 0x00;
	checksum += buffer[3];
	// Data2
	buffer[4] = 0x00;
	checksum += buffer[4];
	// Data3
	buffer[5] = 0x00;
	checksum += buffer[5];
	//Checksum
	buffer[6] = checksum;
}

void MotionMind::makeReadStatusCommand(uint8_t* buffer, unsigned int address)
{
	unsigned int checksum = 0;
	// Command
	buffer[0] = 0x1A;
	checksum += buffer[0];
	// Address
	buffer[1] = address;
	checksum += buffer[1];
	// Data0
	buffer[2] = 0x00;
	checksum += buffer[2];
	// Data1
	buffer[3] = 0x00;
	checksum += buffer[3];
	// Data2
	buffer[4] = MM_READ_STATUS;
	checksum += buffer[4];
	// Data3
	buffer[5] = 0x00;
	checksum += buffer[5];
	//Checksum
	buffer[6] = checksum;
}

void MotionMind::makeSetOdomReq(uint8_t* buffer, unsigned int address, float position)
{
	unsigned int checksum = 0;
	uint32_t posInt = robot2abspos(position);
	printf("Setting position register to %0.6f : %d\n",position,posInt);
	char* pos = (char*)&posInt;
	// Command
	buffer[0] = MM_WRITE_REG;
	checksum += buffer[0];
	// Address
	buffer[1] = address;
	checksum += buffer[1];
	// Index
	buffer[2] = MM_REG_POSITION;
	checksum += buffer[2];
	// Data0
	buffer[3] = pos[0];
	checksum += buffer[3];
	// Data1
	buffer[4] = pos[1];
	checksum += buffer[4];
	// Data2
	buffer[5] = pos[2];
	checksum += buffer[5];
	// Data3
	buffer[6] = pos[3];
	checksum += buffer[6];
	//Checksum
	buffer[7] = checksum;
}

int MotionMind::robot2abspos(const float position)
{
	assert(this->gear_ratio != 0.0);
	int abspos = static_cast<int> ((double)position*(double)this->cpr*this->gear_ratio);
	return abspos;
}

// Currently this alternates between reading the pedal position and reading the steering position
void MotionMind::FindCurrentPos()
{
	long elapsed;
	struct timeval curr;
	if (!this->pos_request_sent)
	{
		uint8_t * buffer = new uint8_t[MESSAGE_LENGTH];
		makeReadPositionCommand(buffer, this->address);
		player_opaque_data_t mData;
		mData.data_count = MESSAGE_LENGTH;
		mData.data = buffer;
		this->MsgWait();
		opaque->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, reinterpret_cast<void*>(&mData),0,NULL);
		GlobalTime->GetTime(&(this->msg_sent));
		delete [] buffer;
		this->pos_request_sent = true;
		GlobalTime->GetTime(&(this->time_sent_pos));
		usleep(MM_DATA_WAIT);
	}
	while (this->pos_request_sent)
	{
		ProcessMessages();
		GlobalTime->GetTime(&curr);
		elapsed = (curr.tv_sec - this->time_sent_pos.tv_sec)*1e6 + (curr.tv_usec - this->time_sent_pos.tv_usec);
		if ((curr.tv_sec - this->time_sent_pos.tv_sec)*1e6 + (curr.tv_usec - this->time_sent_pos.tv_usec) > MSG_TIMEOUT)
		{
			printf("%d %ld mm pos request message timeout triggered\n",opaque_id.index,elapsed);
			this->pos_request_sent = false;
			break;
		}
		// Ensures that you are reading from the right point in the stream
		while( (rx_count > 0) && (rx_buffer[0] != this->address))
		{
			memmove(rx_buffer, rx_buffer+1, rx_count - 1);
			rx_count--;
		}
		if (rx_count >= 6)
		{
			assert (rx_buffer[0] == this->address);
			int position;
			unsigned char* pos = (unsigned char*)&position;
			unsigned int checksum = rx_buffer[0];
			unsigned char* csum = (unsigned char*)&checksum;
			pos[0] = rx_buffer[1];
			checksum += rx_buffer[1];
			pos[1] = rx_buffer[2];
			checksum += rx_buffer[2];
			pos[2] = rx_buffer[3];
			checksum += rx_buffer[3];
			pos[3] = rx_buffer[4];
			checksum += rx_buffer[4];
			if (csum[0] != rx_buffer[5])
			{
				// Checksums don't match - not necessarily bad as it can be due to the address header being in the middle of a responce
				// to another driver with a different address e.g. the responce is 02 01 xx xx xx xx, the data being sent to device 2
				// with the info 01 xx xx xx.
				memmove(rx_buffer, rx_buffer+1, rx_count - 1);
				rx_count--;
			}
			else
			{
				GlobalTime->GetTime(&curr);
				float robot_pos = (float)((double)position/(this->gear_ratio*((double)cpr)));
				this->pos_data.pos = robot_pos;
				rx_count -= 6;
				memmove(rx_buffer+6, rx_buffer, rx_count);
				this->pos_request_sent = false;
			}
		}
		else
		{
			// Prevents the CPU from becoming overloaded
			usleep(MM_CPU_WAIT);
		}
	}
	GlobalTime->GetTime(&curr);
	elapsed = (curr.tv_sec - this->time_sent_pos.tv_sec)*1e6 + (curr.tv_usec - this->time_sent_pos.tv_usec);
}

void MotionMind::FindCurrentStatus()
{
	long elapsed;
	struct timeval curr;
	if (!this->status_request_sent)
	{
		//    printf("%d MotionMind FindCurrentStatus sending status request\n",opaque_id.index);
		uint8_t * buffer = new uint8_t[MESSAGE_LENGTH];
		makeReadStatusCommand(buffer, this->address);
		player_opaque_data_t mData;
		mData.data_count = MESSAGE_LENGTH;
		mData.data = buffer;
		this->MsgWait();
		opaque->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, reinterpret_cast<void*>(&mData),0,NULL);
		GlobalTime->GetTime(&(this->msg_sent));
		delete [] buffer;
		this->status_request_sent = true;
		GlobalTime->GetTime(&(this->time_sent_status));
		usleep(MM_DATA_WAIT);
	}
	while (this->status_request_sent)
	{
		ProcessMessages();
		GlobalTime->GetTime(&curr);
		elapsed = (curr.tv_sec - this->time_sent_status.tv_sec)*1e6 + (curr.tv_usec - this->time_sent_status.tv_usec);
		if ((curr.tv_sec - this->time_sent_status.tv_sec)*1e6 + (curr.tv_usec - this->time_sent_status.tv_usec) > MSG_TIMEOUT)
		{
			printf("%d %ld mm status message request timeout triggered\n",opaque_id.index,elapsed);
			this->status_request_sent = false;
			break;
		}
		// Ensures that you are reading from the right point in the stream
		while( (rx_count > 0) && (rx_buffer[0] != this->address))
		{
			memmove(rx_buffer, rx_buffer+1, rx_count - 1);
			rx_count--;
		}
		if (rx_count >= 4)
		{
			assert (rx_buffer[0] == this->address);
			int32_t status = 0;
			unsigned char* stat = (unsigned char*)&status;
			unsigned int checksum = rx_buffer[0];
			unsigned char* csum = (unsigned char*)&checksum;
			stat[0] = rx_buffer[1];
			checksum += rx_buffer[1];
			stat[1] = rx_buffer[2];
			checksum += rx_buffer[2];
			if (csum[0] != rx_buffer[3])
			{
				// Checksums don't match - not necessarily bad as it can be due to the address header being in the middle of a responce
				// to another driver with a different address e.g. the responce is 02 01 xx xx xx xx, the data being sent to device 2
				// with the info 01 xx xx xx.
				memmove(rx_buffer, rx_buffer+1, rx_count - 1);
				rx_count--;
			}
			else
			{
				this->pos_data.status = 
					(status & MM_STATUS_NEGLIMIT ? 0x01 : 0x00)     // NEGLIMIT -> lim min
					| (status & MM_STATUS_POSLIMIT ? 0x04 : 0x00)     // POSLIMIT -> lim max
					| (status & MM_STATUS_CURRENTLIMIT ? 0x08 : 0x00) // CURRENTLIMIT -> over current
					| (status & MM_STATUS_INPOSITION ? 0x10 : 0x00)   // INPOSITION -> trajectory complete
					| (status & MM_STATUS_BRAKE ? 0x00 : 0x20);        // BRAKE -> is enabled (inverted)
				rx_count -= 4;
				memmove(rx_buffer+4, rx_buffer, rx_count);
				status_request_sent = false;
			}
		}
		else
		{
			// Prevents the CPU from becoming overloaded
			usleep(MM_CPU_WAIT);
		}
	}
	elapsed = (curr.tv_sec - this->time_sent_status.tv_sec)*1e6 + (curr.tv_usec - this->time_sent_status.tv_usec);
}

int MotionMind::MsgWait()
{
	struct timeval curr;
	long elapsed;
	GlobalTime->GetTime(&curr);
	elapsed = (curr.tv_sec - this->msg_sent.tv_sec)*1e6 + (curr.tv_usec - this->msg_sent.tv_usec);
	while (elapsed < MM_MSG_WAIT)
	{
		usleep(MM_MSG_WAIT-elapsed);
		GlobalTime->GetTime(&curr);
		elapsed = (curr.tv_sec - this->msg_sent.tv_sec)*1e6 + (curr.tv_usec - this->msg_sent.tv_usec);
	}
	return 0;
}



