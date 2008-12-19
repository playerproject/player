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
)

driver(
  name "serialstream"
  port "/dev/ttyS0"
  transfer_rate 19200
  parity "none"
  provides ["opaque:0"]
  alwayson 1
)
@endverbatim

@author Chris Chambers

*/

/** @} */


#include <unistd.h>
#include <string.h>
#include <time.h>

#include <libplayercore/playercore.h>

#define DEFAULT_RX_BUFFER_SIZE 500*1024/8
#define DEFAULT_ADDRESS 1
#define MESSAGE_LENGTH 7
#define MSG_TIMEOUT 0.1 //Seconds before it sends another read request if it hasn't yet got a reply

extern PlayerTime *GlobalTime;

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

	//Requests the pos of the robot if it hasnt been already, othwise publishes where it is
	void FindCurrentPos();

	// Checks whether or not you have sent a request for the position
	bool pos_request_sent;
	struct timeval time_sent;

	//Makes a command to be sent to the opaque driver
	void makeAbsolutePositionCommand(uint8_t* buffer, unsigned int address, float position);

	//Makes a command which requests the current osition bback from the motor ocntroller
	void makeReadPositionCommand(uint8_t* buffer, unsigned int address);

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

	pos_request_sent = false;

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
			//TODO: addd do stuff here to deal with the new data
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
		opaque->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, reinterpret_cast<void*>(&mData),0,NULL);
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

		// Ask for the current position and publish it
		FindCurrentPos();

		//usleep is called in FindCurrentPos to prevent overloading
		//usleep(10000);
	}
	return;
}

// Makes the required serial packet to command a position or request the position
void MotionMind::makeAbsolutePositionCommand(uint8_t* buffer, unsigned int address, float position)
{
	unsigned int checksum = 0;
	int posInt = static_cast<int> (position);
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
	buffer[7] = 0x00;
}

void MotionMind::makeReadPositionCommand(uint8_t* buffer, unsigned int address)
{
	unsigned int checksum = 0;
	buffer[0] = 0x1A;
	checksum += buffer[0];
	buffer[1] = address;
	checksum += buffer[1];
	buffer[2] = 0x01;
	checksum += buffer[2];
	buffer[3] = 0x00;
	checksum += buffer[3];
	buffer[4] = 0x00;
	checksum += buffer[4];
	buffer[5] = 0x00;
	checksum += buffer[5];
	buffer[6] = checksum;
	buffer[7] = 0x00;
}

// Currently this alternates between reading the pedal position and reading the steering position
void MotionMind::FindCurrentPos()
{
	if (!this->pos_request_sent)
	{
		uint8_t * buffer = new uint8_t[MESSAGE_LENGTH];
		makeReadPositionCommand(buffer, this->address);
		player_opaque_data_t mData;
		mData.data_count = MESSAGE_LENGTH;
		mData.data = buffer;
		opaque->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, reinterpret_cast<void*>(&mData),0,NULL);
		delete [] buffer;
		this->pos_request_sent = true;
		GlobalTime->GetTime(&(this->time_sent));
		usleep(10000);
	}
	else
	{
		struct timeval curr;
		GlobalTime->GetTime(&curr);
		if ((curr.tv_sec - this->time_sent.tv_sec)*1e6 + (curr.tv_usec - this->time_sent.tv_usec) > MSG_TIMEOUT*1e6)
		{
			PLAYER_WARN("mm message timeout triggered");
			this->pos_request_sent = false;
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
				player_position1d_data_t data_packet;
				data_packet.pos = position;
				this->Publish(this->device_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION1D_DATA_STATE, (void*)&data_packet);
				rx_count -= 6;
				memmove(rx_buffer+6, rx_buffer, rx_count);
				pos_request_sent = false;
			}
		}
		else
		{
			// Prevents the CPU from becoming overloaded
			usleep(10000);
		}
	}
}





