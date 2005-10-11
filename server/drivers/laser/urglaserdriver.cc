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

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_urglaser urglaser


The urglaser driver controls the URG scanning laser range-finder.

@par Compile-time dependencies

- none

@par Provides

- @ref player_interface_laser

@par Requires

- none

@par Configuration requests

- PLAYER_LASER_REQ_GET_GEOM
- PLAYER_LASER_REQ_GET_CONFIG
- PLAYER_LASER_REQ_SET_CONFIG
  
@par Configuration file options

- port (string)
  - Default: "/dev/ttyACM0"
  - USB acm device port to which laser is attached.  

- pose (float tuple m m rad)
  - Default: [0.0 0.0 0.0]
  - Pose (x,y,theta) of the laser, relative to its parent object (e.g.,
    the robot to which the laser is attached).

- min_angle, max_angle (float [rad])
  - Default: -2 and 2
  - Minimum and maximum scan angles to return

@par Example 

@verbatim
driver
(
  name "urglaser"
  provides ["laser:0"]
  port "/dev/ttyACM0"
)
@endverbatim

@par Authors

Toby Collett

*/
/** @} */
  


#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <math.h>

#include <vector>
using namespace std;

#include "urg_laser.h"

#include <libplayercore/playercore.h>

class URGLaserDriver : public Driver {
public:
    
	// Constructor; 
	URGLaserDriver(ConfigFile* cf, int section);
	// Destructor
	~URGLaserDriver();

	// Implementations of virtual functions
	int Setup();
	int Shutdown();

	// This method will be invoked on each incoming message
	virtual int ProcessMessage(MessageQueue* resp_queue, 
                               player_msghdr * hdr,
                               void * data);

private:
	// Main function for device thread.
	virtual void Main();

	urg_laser_readings_t * Readings;
	urg_laser Laser;
	
	player_laser_data_t Data;
	player_laser_geom_t Geom;
	player_laser_config_t Conf;
};


////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
URGLaserDriver::URGLaserDriver(ConfigFile* cf, int section)
: Driver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_LASER_CODE)
{

    //init vars
	memset(&Data, 0, sizeof(Data));
	memset(&Geom, 0, sizeof(Geom));
	memset(&Conf, 0, sizeof(Conf));
	Geom.size.sw = (0.050);
	Geom.size.sl = (0.050);
	
	Readings = new urg_laser_readings_t;
	assert(Readings);

    // read options from config file
	Geom.pose.px = (cf->ReadTupleFloat(section,"pose",0,0));
	Geom.pose.py = (cf->ReadTupleFloat(section,"pose",1,0));
	Geom.pose.pa = (cf->ReadTupleFloat(section,"pose",2,0));
	
	//set up config structure
	Conf.min_angle = cf->ReadFloat(section,"min_angle",DTOR(-115));
	Conf.max_angle = cf->ReadFloat(section,"max_angle",DTOR(115));
	Conf.resolution = DTOR(270.0/769.0);
        Conf.max_range = 4.0;
	Conf.range_res = 0.001;
	Conf.intensity = 0;
	
    //config data
	Laser.Open(cf->ReadString(section, "port", "/dev/ttyACM0"));

    return;
}

URGLaserDriver::~URGLaserDriver()
{
	delete Readings;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int URGLaserDriver::Setup() {   
    // Start the device thread; spawns a new thread and executes
    // ExampleDriver::Main(), which contains the main loop for the driver.
	StartThread();


    return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int URGLaserDriver::Shutdown() {
    // Stop and join the driver thread
    StopThread();

    return(0);
}


int URGLaserDriver::ProcessMessage(MessageQueue* resp_queue, 
                                  player_msghdr * hdr,
                                  void * data)
{	
	if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, 
                           PLAYER_LASER_REQ_GET_GEOM, 
                           this->device_addr))
	{
		Publish(device_addr,resp_queue, PLAYER_MSGTYPE_RESP_ACK,hdr->subtype,&Geom,sizeof(Geom),NULL);
	}
	else
	{
		return -1;
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void URGLaserDriver::Main() 
{
	// The main loop; interact with the device here
	for(;;)	{
    	// test if we are supposed to cancel
    	pthread_testcancel();

		// Process any pending messages
		ProcessMessages();
		
		int min_i = static_cast<int> (384 + Conf.min_angle/Conf.resolution);
		int max_i = static_cast<int> (384 + Conf.max_angle/Conf.resolution);
		
		if (min_i > max_i)
			min_i = max_i;
		if (min_i < 0)
			min_i = 0;
		
		if (max_i > 769)
			max_i = 769;
		
		
		// update device data
		Laser.GetReadings(Readings);
		Data.min_angle = Conf.min_angle;
		Data.max_angle = Conf.max_angle;
                // TODO: check this value
                Data.max_range = 4.0;
		Data.resolution = Conf.resolution;
		Data.ranges_count = max_i - min_i;

		for (unsigned int i = 0; i < Data.ranges_count; ++i)
		{
			Data.ranges[i] = Readings->Readings[i+min_i] < 20 ? (4095) : (Readings->Readings[i+min_i]);
			Data.ranges[i]/=1000;
		}
		Publish(device_addr, NULL, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, 
			&Data, sizeof(player_laser_data_t), NULL);
		//printf("Put Data to client\n");
		
    	// Sleep (you might, for example, block on a read() instead)
    	//usleep(10);

	}
}

////////////////////////////////////////////////////////////////////////////////
// Things for building shared object, and functions for registering and creating
//  new instances of the driver
////////////////////////////////////////////////////////////////////////////////

//Factory creation function. This functions is given as an argument when
// the driver is added to the driver table
Driver* URGLaserDriver_Init(ConfigFile* cf, int section) {
    // Create and return a new instance of this driver
	return((Driver*)(new URGLaserDriver(cf, section)));
}

//Registers the driver in the driver table. Called from the 
// player_driver_init function that the loader looks for
int URGLaserDriver_Register(DriverTable* table) {
	table->AddDriver("urglaser", URGLaserDriver_Init);
  return 0;
}



