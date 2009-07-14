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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
 /** @ingroup drivers */
/** @{ */
/** @defgroup driver_rt3xxx rt3xxx
 * @brief Driver for the RT3XXX
 
Provides a gps interface to an RT3xxx inertial navigation
unit. It may work for other units as it only receives data
and does not communicate with the unit.

@par Compile-time dependencies

- None.

@par Provides

- @ref interface_gps

@par Requires

- None.

@par Configuration requests

- None.

@par Configuration file options

- None.

@par Example

@verbatim
driver
(
  name "rt3xxx"
  provides ["gps:0"] 
)
@endverbatim

@author Mike Roddewig mrroddew@mtu.edu

*/
/** @} */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>

#include <libplayercore/playercore.h>

#define RT_PORT 3000    // The port the rt is broadcasting on.

#define MAXBUFLEN 128

// Message levels

#define MESSAGE_ERROR					0
#define MESSAGE_INFO						1
#define MESSAGE_DEBUG					2



////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class rt3xxx : public ThreadedDriver
{
  public:
	rt3xxx(ConfigFile* cf, int section);

	// This method will be invoked on each incoming message
	virtual int ProcessMessage(QueuePointer &resp_queue, 
							   player_msghdr * hdr,
							   void * data);
	virtual int MainSetup();
	virtual void MainQuit();
  
  private:
	int sockfd;
	char buf[MAXBUFLEN];
	player_gps_data_t gps_data;
	player_devaddr_t gps_addr;
	double lat_long_scale_factor;
	double rt_heading_scale_factor;
  
	// Main function for device thread.
	virtual void Main();
  
	int ProcessPacket();

	void PublishGPSData();
};

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver* 
rt3xxx_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return((Driver*)(new rt3xxx(cf, section)));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void rt3xxx_Register(DriverTable* table)
{
  table->AddDriver("rt3xxx", rt3xxx_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
rt3xxx::rt3xxx(ConfigFile* cf, int section) : ThreadedDriver(cf, section) {
	memset (&this->gps_addr, 0, sizeof (player_devaddr_t));
	
	// Check the config file to see if we are providing a GPS interface.
	if (cf->ReadDeviceAddr(&(this->gps_addr), section, "provides",
		PLAYER_GPS_CODE, -1, NULL) == 0) {
		if (this->AddInterface(this->gps_addr) != 0) {
			PLAYER_ERROR("Error adding GPS interface.");
			SetError(-1);
			return;
		}
	}
	
	return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int rt3xxx::MainSetup()
{   
	struct sockaddr_in address;
	int broadcast = 1;
	
	PLAYER_MSG0(MESSAGE_INFO, "rt3xxx setting up.");

	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	
	if (sockfd == -1) {
		PLAYER_ERROR("rt3xxx: failed to get socket.");
		return -1;
	}
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(RT_PORT);
	
	if (bind(sockfd, (struct sockaddr *) &address, sizeof(address)) == -1) {
		PLAYER_ERROR("rt3xxx: failed to bind socket.");
		return -1;
	}
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1) {
		PLAYER_ERROR("rt3xxx: failed to set broadcast flag.");
		return -1;
	}
	
	this->lat_long_scale_factor = pow(10.0, 7.0);
	this->rt_heading_scale_factor = pow(10.0, -6.0);
	
	PLAYER_MSG0(MESSAGE_INFO, "rt3xxx driver ready.");

	return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void rt3xxx::MainQuit()
{
	PLAYER_MSG0(MESSAGE_INFO, "Shutting rt3xxx driver down.");
	
	close(this->sockfd);
	
	PLAYER_MSG0(MESSAGE_INFO, "rt3xxx driver has been shutdown.");

	return;
}

int rt3xxx::ProcessMessage(QueuePointer & resp_queue, 
								  player_msghdr * hdr,
								  void * data)
{
  // Process messages here.  Send a response if necessary, using Publish().
  // If you handle the message successfully, return 0.  Otherwise,
  // return -1, and a NACK will be sent for you, if a response is required.
  return(-1);
}

int rt3xxx::ProcessPacket() {
	int numbytes;
	
	char nav_status;
	double latitude;
	double * latitude_pointer;
	double longitude;
	double * longitude_pointer;
	float altitude;
	float * altitude_pointer;
	unsigned char num_satellites;
	unsigned char position_mode;
	unsigned char velocity_mode;
	static long gps_time_minutes; 	// The GPS time in minutes is not transmitted
								// with every packet so it needs to persist over
								// multiple function calls.
	double heading;
	unsigned char hdop;
	unsigned char pdop;
	
	
	numbytes = recv(sockfd, &buf, MAXBUFLEN-1 , 0);
	
	if (numbytes == -1) {
		return -1;
	}
	else if (numbytes != 72) {
		// We got a packet, but it's something weird. We'll discard it
		// rather than take our chances.
		
		PLAYER_MSG0(MESSAGE_DEBUG, "Received packet of the wrong size.");
		
		return -2;
	}
	else {
		// So far, so good. Process the packet. There are certainly lots
		// of interesting tidbits that the RT reports, but so far we only
		// use the data supported by the GPS interface.
		
		if (((unsigned char) buf[numbytes-1]) != 0xE7) {
			// We expect the last (?) character to be the "sync" character
			// of 0xE7. The documentation specifies that the sync character
			// should be the first byte of the packet but in reality it appears
			// to be the last. Huh.
			
			PLAYER_MSG0(MESSAGE_DEBUG, "Failed to locate the sync character.");

			return -3;
		}
		
		nav_status = buf[20];
		
		switch (nav_status) {
			case 0x00:
				// All quantities in the packet are invalid?! Time to quit.
			
				PLAYER_MSG0(MESSAGE_DEBUG, "Init state: 0. All quantities invalid.");
			
				return 0;
			case 0x01:
				// IMU measurements only. No position data. Time to quit.
			
				PLAYER_MSG0(MESSAGE_DEBUG, "Init state: 1. IMU measurements only.");

				return 0;
			case 0x02:
				// Initialization mode. No position data (though the time
				// is valid). Time to quit, heh.
			
				PLAYER_MSG0(MESSAGE_DEBUG, "Init state: 2. Initialization mode.");

				return 0;
			case 0x03:
				// The RT is acquiring lock. Apparently this mode lasts a
				// max of 10 seconds, so we can wait (can you?). Time to
				// quit.
			
				PLAYER_MSG0(MESSAGE_DEBUG, "Init state: 3. Acquiring lock.");

				return 0;
			case 0x04:
				// Locked and loaded. Let's get our data!

				this->gps_data.time_sec = (uint32_t) gps_time_minutes * 60;
				this->gps_data.time_usec = buf[0];
				this->gps_data.time_usec += buf[1] << 8;
				
				// Retrieve the lat/long data. It's a real pity that the RT 
				// does not provide position in UTM coordinates, as every
				// true navigation geek knows that UTM is the superior
				// coordinate system ;).
			
				// I don't like it, but it appears we have to be tricky in 
				// order to load this double into memory. We declare a 
				// pointer and set it to start at the location of the double
				// in the buffer.
			
				latitude_pointer = (double *) &buf[22];
				latitude = *latitude_pointer;
			
				longitude_pointer = (double *) &buf[30];
				longitude = *longitude_pointer;
				
				altitude_pointer = (float *) &buf[38];
				altitude = *altitude_pointer;
				
				heading = (double) buf[51];
				heading += (double) (buf[52] << 8);
				heading += (double) (buf[53] << 16);
				
				// Convert the RT units to Player units.
				
				// Lat/long to degrees (the RT reports in radians).
				
				latitude = latitude * (180.0/M_PI);
				longitude = longitude * (180.0/M_PI);
				
				// Apply a scaling of 10^7 (why not a power of two Player?).
				latitude = latitude * this->lat_long_scale_factor;
				longitude = longitude * this->lat_long_scale_factor;

				this->gps_data.latitude = (int32_t) latitude;
				this->gps_data.longitude = (int32_t) longitude;
				
				// Player wants altitude in millimeters.
				
				this->gps_data.altitude = (int32_t) (altitude * 1000);
				
				// Convert heading to radians and then to degrees.
				
				heading = heading * this->rt_heading_scale_factor; // Convert to radians in units of one radian.
				heading += M_PI;
				heading = heading * (180.0/M_PI); // Convert to degrees.
				
				if (buf[61] == 0) {
					// The status information contains data we're interested
					// in.
					
					gps_time_minutes = buf[62];
					gps_time_minutes += buf[63] << 8;
					gps_time_minutes += buf[64] << 16;
					gps_time_minutes += buf[65] << 24;
					
					num_satellites = buf[66];
					position_mode = buf[67];
					velocity_mode = buf[68];
					
					this->gps_data.num_sats = (uint32_t) num_satellites;
					
					if (position_mode < 2) {
						// Invalid.
						
						this->gps_data.quality = 0;
					}
					else if (position_mode < 7) {
						// No SBAS.
						
						this->gps_data.quality = 1;
					}
					else {
						// Some form of augmentation system is present.
						
						this->gps_data.quality = 2;
					}
				}
				else if (buf[61] == 48) {
					hdop = buf[64];
					pdop = buf[65];
					
					this->gps_data.hdop = (uint32_t) (hdop * 10);
				}
				
				return 0;
		}
	}
	
	return 0;
}



////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void rt3xxx::Main() 
{
  // The main loop; interact with the device here
  for(;;)
  {
	ProcessPacket();

	// Publish the received data.
	rt3xxx::PublishGPSData();
  }
}

void rt3xxx::PublishGPSData() {
	this->Publish(
		this->gps_addr,
		PLAYER_MSGTYPE_DATA,
		PLAYER_GPS_DATA_STATE,
		(void *) &this->gps_data,
		sizeof(player_gps_data_t),
		NULL
	);
}

