/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007 Federico Ruiz Ugalde   ruizf /at/ cs.tum.edu
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
/** @defgroup driver_phidgetACC phidgetACC
 * @brief Phidget ACC

The phidgetACC driver communicates with the PhidgetACC (Part# 1059) accelerometer.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_wsn

@par Requires

- libphidget from www.phidgets.com should be installed.

@par Configuration requests

- none

@par Configuration file options

- serial (integer)
  - Default: -1
  - This defines which phidget will be controlled if there is more than one connected to the USB bus.
    You can obtain the number with lsusb, like this:  "lsusb -v |grep iSerial".
    The default is -1 , and it will connect to the first phidget available.

- sampling_rate (integer)
  - Default: 40
  - How often (in mS) should the phidget produce data. 40mS produces ACC data at a rate of 25Hz.

- alarmtime (integer)
  - Default: 45
  - If the data acquisition cycle takes longer than this time (in mS), a warning will be printed.

- provides
  - The "wsn" interface with the 3 accelerometers data

@par Example

@verbatim
driver
(
  name "phidgetAcc"
  provides ["wsn:0"]
  serial -1
  alwayson 1
  samplingrate 40
  alarmtime 45
)
@endverbatim

@author Federico Ruiz Ugalde

 */
/** @} */



#include "phidget21.h"
#include <libplayercore/playercore.h>

#include <unistd.h>
#include <string.h>
#include <iostream>

//For nanosleep:
#include <time.h>
#include <sys/time.h>
//To catch the errors of nanosleep
#include <errno.h>

//This function returns the difference in mS between two timeval structures
inline float timediffms(struct timeval start, struct timeval end) {
    return(end.tv_sec*1000.0 + end.tv_usec/1000.0 - (start.tv_sec*1000.0 + start.tv_usec/1000.0));
}

class PhidgetAcc : public ThreadedDriver {
	public:

    // Constructor;
		PhidgetAcc(ConfigFile* cf, int section);

    //Destructor
		~PhidgetAcc();

		virtual int MainSetup();
		virtual void MainQuit();

		virtual int ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data);

	private:

    // Main function for device thread.
		virtual void Main();

		//!Time between samples (in mS)
		float samplingrate;
		//!Alarm time (mS)
		float alarmtime;

	        // WSN interface
                player_wsn_data_t data;

		//! Pointer to the ACC Phidget Handle
		CPhidgetAccelerometerHandle accel;

		//! Player Interfaces
		player_devaddr_t wsn_id;

		//!Serial number of the phidget
		int serial;

//	        NodeCalibrationValues FindNodeValues (unsigned int nodeID);
        	player_wsn_data_t DecodePacket (struct p_packet *pkt);
        	float ConvertAccel (unsigned short raw_accel, int neg_1g, int pos_1g, int converted);

};


////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
PhidgetAcc::PhidgetAcc(ConfigFile* cf, int section)
        : ThreadedDriver(cf, section) {
    //! Start with a clean device
    memset(&wsn_id,0,sizeof(player_devaddr_t));

    // Creating the wsn interface
    if (cf->ReadDeviceAddr(&(wsn_id), section, "provides", PLAYER_WSN_CODE, -1, NULL) == 0) {
        if (AddInterface(wsn_id) != 0) {
            SetError(-1);
            return;
        }
    } else {
        PLAYER_WARN("wsn interface not created for phidgetAccel driver");
    }

    // Set the phidgetwsn pointer to NULL
    accel=0;

    // Read an option from the configuration file
    serial = cf->ReadInt(section, "serial", -1);

    //Sampling rate and alarm time in mS
    samplingrate = cf->ReadFloat(section, "samplingrate", 40.0);
    alarmtime = cf->ReadFloat(section, "alarmtime", 45.0);

    return;
}

PhidgetAcc::~PhidgetAcc() {}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int PhidgetAcc::MainSetup() {
    PLAYER_MSG0(1,"PhidgetAccel driver initialising");

    //Use the Phidgets library to communicate with the devices
    CPhidgetAccelerometer_create(&accel);
    CPhidget_open((CPhidgetHandle)accel,serial);

    PLAYER_MSG0(1,"Waiting for Attachment.");

    int status(-1);

    //Wait for attachment 1s or aborts.
    status=CPhidget_waitForAttachment((CPhidgetHandle)accel, 1000);

    if (status != 0) {
        PLAYER_ERROR("There was a problem connecting to the PhidgetAccelerometer.");
        return(1);
    } else {
        PLAYER_MSG0(1,"Connection granted to the PhidgetAccelerometer.");
    }

    PLAYER_MSG0(1,"PhidgetAcc driver ready");

}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void PhidgetAcc::MainQuit() {
	PLAYER_MSG0(1,"Shutting PhidgetAcc driver down");

    // Turn of the device and delete the Phidget objects
    CPhidget_close((CPhidgetHandle)accel);
    CPhidget_delete((CPhidgetHandle)accel);
    accel=0;

    PLAYER_MSG0(1,"PhidgetAcc driver has been shutdown");
}

int PhidgetAcc::ProcessMessage(QueuePointer &resp_queue,
                                      player_msghdr * hdr,
                                      void * data) {
    return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void PhidgetAcc::Main() {

    //Need two timers: one for calculating the sleep time to keep a desired framerate.
    // The other for measuring the real elapsed time. (And maybe give an alarm)
    struct timeval tv_framerate_start;
    struct timeval tv_framerate_end;
    struct timeval tv_realtime_start;
    struct timeval tv_realtime_end;

    gettimeofday( &tv_framerate_start, NULL );  // NULL -> don't want timezone information
    tv_realtime_start = tv_framerate_start;

    // The main loop; interact with the device here
    while (true)  {
        //find out the real elapsed time
        gettimeofday( &tv_realtime_end, NULL );
        //calculate the time in mS
        float real_elapsed=timediffms(tv_realtime_start,tv_realtime_end);
        //restart the timer
        gettimeofday( &tv_realtime_start, NULL );

        //check if the time was too long
        static bool gavewarning(false);
        if ((!gavewarning) && (real_elapsed > alarmtime)) {
		PLAYER_WARN2("Cycle took %d mS instead of the desired %d mS. (Only warning once)\n",real_elapsed , samplingrate);
     	gavewarning=true;
        }
        //std::cout << real_elapsed << "mS\n";


        // test if we are supposed to cancel
        pthread_testcancel();

        // Process incoming messages.  PhidgetAcc::ProcessMessage() is
        // called on each message.
        ProcessMessages();

        data.node_type=1;
        data.node_id=1;
        data.node_parent_id=1;
    	data.data_packet.light       = -1;
    	data.data_packet.mic         = -1;
    	data.data_packet.magn_x      = -1;
    	data.data_packet.magn_y      = -1;
    	data.data_packet.magn_z      = -1;
    	data.data_packet.temperature = -1;
    	data.data_packet.battery     = -1;
	int n_axis;
	if(CPhidgetAccelerometer_getNumAxis(accel,&n_axis)) return;
	double *p_accel;
	p_accel= new double[n_axis];
	for (int i=0;i<n_axis;++i) {
		if(CPhidgetAccelerometer_getAcceleration(accel, i, &p_accel[i])) return;
	}
	data.data_packet.accel_x=p_accel[0];
	data.data_packet.accel_y=p_accel[1];
	data.data_packet.accel_z=p_accel[2];
	delete [] p_accel;

        //Publishing data.
        if (wsn_id.interf !=0) {
            Publish(wsn_id, PLAYER_MSGTYPE_DATA, PLAYER_WSN_DATA_STATE, (unsigned char*)&data, sizeof(player_wsn_data_t), NULL);
        }

        //point to calculate how much to sleep, call nanosleep, after sleep restart the timer
        //Get the ammount of time passed:
        gettimeofday( &tv_framerate_end, NULL );
        // figure out how much to sleep
        long usecs    = tv_framerate_end.tv_usec - tv_framerate_start.tv_usec;
        long secs     = tv_framerate_end.tv_sec  - tv_framerate_start.tv_sec;
        long elapsed_usecs = 1000000*secs  + usecs;

        long us_tosleep = static_cast<long>(samplingrate*1000) - elapsed_usecs;
        //cout << "usec to sleep: " << us_tosleep << endl;

        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = us_tosleep*1000;
        int done=nanosleep(&ts, NULL);

        //restart the counter
        gettimeofday( &tv_framerate_start, NULL );

        if (done != 0) {
            std::cout << "Error in nanosleep! ERRNO: " << errno << " ";
            if (errno == EINTR) {
                std::cout << "EINTR" ;
            } else if (errno == EINVAL) {
                std::cout << "EINVAL" ;
            }
            std::cout << std::endl;
        }

    }
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver*
PhidgetAcc_Init(ConfigFile* cf, int section) {
    // Create and return a new instance of this driver
	return((Driver*)(new PhidgetAcc(cf, section)));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void phidgetAcc_Register(DriverTable* table) {
	table->AddDriver("phidgetAcc", PhidgetAcc_Init);
}

