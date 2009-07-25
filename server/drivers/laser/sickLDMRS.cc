/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2008
 *      Chris Chambers
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


/** @ingroup drivers Drivers */
/** @{ */
/** @defgroup driver_sickLDMRS sickLDMRS
 * @brief SICK LD MRS / Multi plae and multi return laser scanner

This driver is for communicting with the Ibeo LUX laser scanner over ethernet using the tcpstream driver

@par Provides

- laser

@par Requires

 - opaque

@par Supported commands

- none

@par Supported configuration requests

 - none

@par Configuration file options

 - buffer_size (int)
	- default: 100000
	- The size of the buffer in the lux driver
 - layer (int)
	- -1(default) = all layers
	- 0,1,2,3 = only publish layer x of the data (3 = highest)
 - echo (int)
	- -1(default) = all echos
	- 0,1,2,3 = only publish laser returns which are echo x
 - intensity (0,1,2,3)
	- 1(default) - publishes the echo width (in cm) as the intensity variable (cut at 255cm should never be exceeded?)
    - 2 - publishes the echo number as the intensity value (0,1,2,3)
	- 3 - publishes the scan layer as the intensity value (0,1,2,3)
	- 0 - No intensity data

@par Example:

@verbatim
driver
(
  name "sickLDMRS"
  provides ["laser:0"]
  requires ["opaque:0"]
  buffer_size 20480
  layer 3
)

driver
(
  name "tcpstream"
  provides ["opaque:0"]
  port 12002
  ip "10.99.0.1"
  buffer_size 10000
)
@endverbatim

@author Chris Chambers


*/
/** @} */

#define DEG2RAD(x) (((double)(x))*0.01745329251994)
//#define RAD2DEG(x) (((double)(x))*57.29577951308232)

#if !defined (WIN32)
  #include <unistd.h>
  #include <arpa/inet.h>
#endif
#include <string.h>
//#include <stdint.h>
//#include <sys/time.h>
//#include <time.h>
//#include <assert.h>
//#include <math.h>

#include <libplayercore/playercore.h>

#define LUX_DEFAULT_RX_BUFFER_SIZE 100000U
#define LUX_DEFAULT_LAYER -1
#define LUX_DEFAULT_ECHO -1
#define LUX_DEFAULT_INTENSITY 1

#define HEADER_LEN 24U
#define SCAN_HEADER_LEN 46U
#define SCAN_DATA_LEN 24U

#define MESSAGE_LEN 38U

///////////////////////////////////////////////////////////////////////////////
// The class for the driver
class sickLDMRS : public ThreadedDriver
{
  public:
    // Constructor; need that
    sickLDMRS(ConfigFile* cf, int section);
	~sickLDMRS();

    // Must implement the following methods.
    virtual int MainSetup();
    virtual void MainQuit();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr* hdr,
                               void* data);

  private:

	//Makes a filter command
	void makeStartStopCommand(uint8_t* buffer, bool start = true);

	bool ProcessLaserData();

    // Main function for device thread.
    virtual void Main();

	// Opaque Driver info
    Device *opaque;
    player_devaddr_t opaque_id;

	player_laser_data_scanangle_t data_packet;

	struct timeval debug_time;

	int layer, echo, intensity;

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
sickLDMRS_Init(ConfigFile* cf, int section)
{
	// Create and return a new instance of this driver
	return ((Driver*)(new sickLDMRS(cf, section)));

}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void sickLDMRS_Register(DriverTable* table)
{
	table->AddDriver("sickLDMRS", sickLDMRS_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
sickLDMRS::sickLDMRS(ConfigFile* cf, int section)
    : ThreadedDriver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_LASER_CODE)
{
	this->opaque = NULL;
	// Must have an opaque device
	if (cf->ReadDeviceAddr(&this->opaque_id, section, "requires",
                       PLAYER_OPAQUE_CODE, -1, NULL) != 0)
	{
		PLAYER_ERROR("No Opaque driver specified");
		this->SetError(-1);
		return;
	}

	// Read options from the configuration file

	rx_count = 0;
	rx_buffer_size = cf->ReadInt(section, "buffer_size", LUX_DEFAULT_RX_BUFFER_SIZE);
	rx_buffer = new uint8_t[rx_buffer_size];
	assert(rx_buffer);

	layer = cf->ReadInt(section, "layer", LUX_DEFAULT_LAYER);
	echo = cf->ReadInt(section, "echo", LUX_DEFAULT_ECHO);
	intensity = cf->ReadInt(section, "intensity", LUX_DEFAULT_INTENSITY);

	//this->RegisterProperty ("mirror", &this->mirror, cf, section);

	return;
}

sickLDMRS::~sickLDMRS()
{
	delete [] rx_buffer;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int sickLDMRS::MainSetup()
{
	PLAYER_WARN("Setting up sickLDMRS driver");

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

	uint8_t* buffer = new uint8_t[MESSAGE_LEN];
	assert(buffer);
	makeStartStopCommand(buffer, true);
	player_opaque_data_t mData;
	mData.data_count = MESSAGE_LEN;
	mData.data = buffer;

	opaque->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, reinterpret_cast<void*>(&mData),0,NULL);

	delete [] buffer;
	buffer = NULL;

	PLAYER_WARN("sickLDMRS driver ready");

	return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void sickLDMRS::MainQuit()
{
	uint8_t* buffer = new uint8_t[MESSAGE_LEN];
	assert(buffer);
	makeStartStopCommand(buffer, false);
	player_opaque_data_t mData;
	mData.data_count = MESSAGE_LEN;
	mData.data = buffer;

	opaque->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, reinterpret_cast<void*>(&mData),0,NULL);

	delete [] buffer;

	PLAYER_WARN("sickLDMRS driver shutting down");

	opaque->Unsubscribe(InQueue);

	PLAYER_WARN("sickLDMRS driver has been shutdown");
}



// Process an incoming message
int sickLDMRS::ProcessMessage(QueuePointer & resp_queue,
                                 player_msghdr* hdr,
                                 void* data)
{
	assert(hdr);
	//assert(data);

	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_OPAQUE_DATA_STATE, opaque_id))
	{
		//GlobalTime->GetTime(&(this->debug_time));
		//PLAYER_MSG2(1,"LUX driver received data packet at %d:%d", this->debug_time.tv_sec, this->debug_time.tv_usec);
		//PLAYER_WARN("Received tcp data");
		player_opaque_data_t * recv = reinterpret_cast<player_opaque_data_t * > (data);
		unsigned int messageOffset = rx_count;
		rx_count += recv->data_count;
		if (rx_count > rx_buffer_size)
		{
			PLAYER_WARN("sickLDMRS driver Buffer Full");
			rx_count = 0;
		}
		else
		{
			memcpy(&rx_buffer[messageOffset], recv->data, recv->data_count);
		}
		return 0;
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void sickLDMRS::Main()
{

	// The main loop; interact with the device here
	for(;;)
	{
		// Process incoming messages
		ProcessMessages();

		// Ask for the current position and publish it
		ProcessLaserData();

		if (!this->Wait(1))
		{
			PLAYER_WARN("No TCP data received within 1s, possible loss of connection");
		}
	}
	return;
}

// Currently this alternates between reading the pedal position and reading the steering position
bool sickLDMRS::ProcessLaserData()
{
	//TODO: Implement a timeout here so that if it gets no data back in a time frame it either errors or sends a reset and stuff
	while(rx_count > HEADER_LEN)
	{
		//PLAYER_WARN("Got enough data");
		// find our continuous data header
		unsigned int ii;
		bool found = false;
		for (ii = 0; ii < rx_count - HEADER_LEN; ++ii)
		{
			if (memcmp(&rx_buffer[ii],"\xaf\xfe\xc0\xc2",4) == 0)
			{
				//PLAYER_MSG0(3,"Header string found");
				memmove(rx_buffer, &rx_buffer[ii], rx_count-ii);
				rx_count -= ii;
				found = true;
				break;
			}
		}
		if (!found)
		{
			memmove(rx_buffer, &rx_buffer[ii], rx_count-ii);
			rx_count -= ii;
			return 0;
		}

		// get relevant bits of the header
		// size includes all data in the data block
		// through to the end of the packet including the checksum
		uint32_t size = htonl(*reinterpret_cast<uint32_t*> (&rx_buffer[8])); // Should this be htons????
		if (size > rx_buffer_size - HEADER_LEN)
		{
			PLAYER_WARN("sickLDMRS: Requested Size of data is larger than the buffer size");
			memmove(rx_buffer, &rx_buffer[1], --rx_count);
			return 0;
		}

		// check if we have enough data yet
		if (size > rx_count - HEADER_LEN)
		{
			return 0;
		}
		//PLAYER_MSG0(5, "GOT ENOUGH DATATATATATATATATATATATATATATATATATATATATAT");

		uint8_t * data = &rx_buffer[HEADER_LEN];
		if (memcmp(&rx_buffer[14], "\x22\x01", 2) == 0)
		{
			//PLAYER_MSG0(1, "Got scan data packet");
			if (size < SCAN_HEADER_LEN)
			{
				PLAYER_WARN1("sickLDMRS- bad data count (%d)\n", size);
				memmove(rx_buffer, &rx_buffer[1], --rx_count);
				continue;
			}
			if (size == SCAN_HEADER_LEN)
			{
				PLAYER_MSG0(1, "LUX - no scans returned");
				continue;
			}
			//PLAYER_MSG0(1, "Not bad count");
			//PLAYER_MSG2(1, "Size hex %x %x", rx_buffer[34+HEADER_LEN], rx_buffer[35+HEADER_LEN]);

			uint16_t scan_count = htons(*reinterpret_cast<uint16_t*> (&data[34]));
			//uint16_t scan_count = htons(*reinterpret_cast<uint16_t*> (&rx_buffer[34+HEADER_LEN]));
			//PLAYER_MSG1(4,"Scan count %d", scan_count);
			if (size != (SCAN_HEADER_LEN + SCAN_DATA_LEN*scan_count))
			{
				PLAYER_WARN2("sickLDMRS - data size mismatch, size = %d, number of scans = %d\n", size, scan_count);
				memmove(rx_buffer, &rx_buffer[1], --rx_count);
				continue;
			}

			if (intensity)
				data_packet.intensity = new uint8_t [scan_count];
			data_packet.ranges = new float [scan_count];
			data_packet.angles = new float [scan_count];
			data_packet.max_range = 300;
			// This is needed as the true count will be less than the scan count if you are only interesed in a single layer
			// or a single echo thingy
			int true_count = 0;
			data_packet.id = htons(*reinterpret_cast<uint16_t *>(&data[6]));
			for (int ii = 0; ii < scan_count; ++ii)
			{
				if (!(layer == -1 || layer == data[SCAN_HEADER_LEN+SCAN_DATA_LEN*ii+2]))
					continue;
				if (!(echo == -1 || echo == data[SCAN_HEADER_LEN+SCAN_DATA_LEN*ii+3]))
					continue;
				uint16_t Distance_CM = htons(*reinterpret_cast<uint16_t *> (&data[SCAN_HEADER_LEN+SCAN_DATA_LEN*ii+6]));
				assert(SCAN_HEADER_LEN + SCAN_DATA_LEN*ii+6+HEADER_LEN <= rx_count);
				double distance_m = static_cast<double>(Distance_CM)/100.0;
				data_packet.ranges[true_count] = static_cast<float> (distance_m);
				int16_t angle_i = htons(*reinterpret_cast<uint16_t *> (&data[SCAN_HEADER_LEN+SCAN_DATA_LEN*ii+4]));
				if (angle_i > 180*32)
				{
					angle_i -= 360*32;
				}
				double angle_d = DEG2RAD(angle_i)/32.0;
				data_packet.angles[true_count] = static_cast<float> (angle_d);
				if (intensity == 1)
				{
					// echo width in cm
					uint16_t tmp = htons(*reinterpret_cast<uint16_t *> (&data[SCAN_HEADER_LEN+SCAN_DATA_LEN*ii+8]));
					if (tmp > 255)
						tmp = 255;
					data_packet.intensity[true_count] = static_cast<uint8_t> (tmp);
				}
				else if (intensity == 2)
					data_packet.intensity[true_count] = data[SCAN_HEADER_LEN+SCAN_DATA_LEN*ii+3];
				else if (intensity == 3)
					data_packet.intensity[true_count] = data[SCAN_HEADER_LEN+SCAN_DATA_LEN*ii+2];
				true_count++;

			}
			assert(true_count <= scan_count);
			data_packet.ranges_count = true_count;
			data_packet.angles_count = true_count;
			if (intensity)
				data_packet.intensity_count = true_count;
			else
				data_packet.intensity_count = 0;
			// About to publish stuff
			//GlobalTime->GetTime(&(this->debug_time));
			//PLAYER_MSG2(1,"LUX driver publishing data at time %d:%d", this->debug_time.tv_sec, this->debug_time.tv_usec);
			//PLAYER_MSG0(1, "Publishing data");
			//PLAYER_MSG2(1, "Scan number = %x %x", data[6], data[7]);
			//PLAYER_MSG2(1, "Rx_count %d size %d", rx_count, size);
			this->Publish (this->device_addr, PLAYER_MSGTYPE_DATA,
					 PLAYER_LASER_DATA_SCANANGLE, (void*)&data_packet);
			delete [] data_packet.ranges;
			delete [] data_packet.angles;
			delete [] data_packet.intensity;


		}
		else if (memcmp(&rx_buffer[14], "\x20\x20", 2) == 0)
		{
			if (size != 2)
			{
				PLAYER_ERROR("Ibeo LUX: Only set mode ack returns are currently supported");
			}
			// Got a command message reply
			if (data[1] & 0x80)
				PLAYER_ERROR("Ibeo LUX: The laser returned a failed flag for the command message sent");

		}
		else
			PLAYER_WARN("Ibeo LUX got a unrecognised responce type");
		memmove(rx_buffer, &rx_buffer[size+HEADER_LEN], rx_count - (size+HEADER_LEN));
		rx_count -= (size + HEADER_LEN);
		continue;
	}
	return 1;
}

void sickLDMRS::makeStartStopCommand(uint8_t* buffer, bool start /*= true*/ )
{

	buffer[0] = 0xAF; // magic word
	buffer[1] = 0xFE;
	buffer[2] = 0xC0;
	buffer[3] = 0xC2;
	buffer[4] = 0x00; // Size of previous message, here just left to nullL
	buffer[5] = 0x00;
	buffer[6] = 0x00;
	buffer[7] = 0x00;
	buffer[8] = 0x00; // Size of data block
	buffer[9] = 0x00;
	buffer[10] = 0x00;
	buffer[11] = 0x0e;
	buffer[12] = 0x00;	// Reserved + source Id
	buffer[13] = 0x00;
	buffer[14] = 0x20;	// Data Type - 2010 = command
	buffer[15] = 0x10;
	buffer[16] = 0x00;	// 4* ntpp time (s) + 4* fractions of a second
	buffer[17] = 0x00;
	buffer[18] = 0x00;
	buffer[19] = 0x00;
	buffer[20] = 0x00;
	buffer[21] = 0x00;
	buffer[22] = 0x00;
	buffer[23] = 0x00;
	// Data Block - all little endian
	buffer[24] = 0x02;	// Command Type - 0002 = set mode
	buffer[25] = 0x00;
	buffer[26] = 0x00; // Version
	buffer[27] = 0x00;
	// Start of set mode stuff - all little endian
	buffer[28] = 0xc0; // start angle /32
	buffer[29] = 0x26;
	buffer[30] = 0x40; // end angle /32
	buffer[31] = 0x06;
	buffer[32] = 0x80; // frequency /256, must be 12.5 Hz
	buffer[33] = 0x0c;
	if (start)
	{
		buffer[34] = 0x03;
		buffer[35] = 0x03;
	}
	else
	{
		buffer[34] = 0x00;
		buffer[35] = 0x00;
	}
	buffer[36] = 0x00; // reserved
	buffer[37] = 0x00;


}





