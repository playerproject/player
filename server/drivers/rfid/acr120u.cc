/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007
 *  Federico Ruiz Ugalde   ruizf /at/ cs.tum.edu, memeruiz /at/ gmail.com
 *  Lorenz Moesenlechner moesenle /at/ in.tum.de
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
/** @defgroup driver_acr120u acr_120u
 * @brief ACR120U RFID reader

The acr120u driver communicates with the ACR120U (Part# ACR120U-TK-R, Firmware V2.2U) reader. (13.56MHz Read-Write multitag, anti-collision and USB Powered).

@par Compile-time dependencies

- none

@par Provides

- @ref interface_rfid

@par Requires

- libusb should be installed.

@par Configuration requests

- none

@par Configuration file options

- sampling_rate (integer)
  - Default: 200
  - How often (in mS) should the phidget produce data. Minimum is around 100mS

- alarmtime (integer)
  - Default: 210
  - If the data acquisition cycle takes longer than this time (in mS), a warning will be printed.

- provides
  - The driver supports the "rfid" interface.
  - No support for the buzzer and led yet.

@par Example

@verbatim
driver
(
  name "acr120u"
  provides ["rfid:0"]
  alwayson 0
  samplingrate 200
  alarmtime 210
)
@endverbatim

@author Federico Ruiz Ugalde, Lorenz Moesenlechner

 */
/** @} */



#include <libplayercore/playercore.h>
#include <usb.h>

#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include <time.h>
#include <sys/time.h>
#include <errno.h>

//This function returns the difference in mS between two timeval structures
inline float timediffms(struct timeval start, struct timeval end) {
    return(end.tv_sec*1000.0 + end.tv_usec/1000.0 - (start.tv_sec*1000.0 + start.tv_usec/1000.0));
}

class Acr120u : public ThreadedDriver {
	public:

		Acr120u(ConfigFile* cf, int section);
		~Acr120u();

		virtual int MainSetup();
		virtual void MainQuit();

		virtual int ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data);

	private:

		// Main function for device thread.
		virtual void Main();

	        int intFromHexTuple( char c1, char c2 ) const;
        	int intFromHexTuple( const char *c ) const { return intFromHexTuple( c[0], c[1] ); }
		//!Time between samples (in mS)
		float samplingrate;
		//!Alarm time (mS)
		float alarmtime;

		//!Libusb data
		struct usb_bus *busses,*bus,*bus_temp;
		usb_dev_handle *HANDLE;
		struct usb_device *dev,*dev_temp;

		enum Acr120uCmds { RESET=0, TURN_ON_RADIO, LIST_TAGS };
		static const int Acr120uCmdLength=14;
		static const char Acr120uCmdStrings[][Acr120uCmdLength ];
		static const int Acr120uResponseLength=3*8;
		static const int vendorId = 0x072f;
		static const int productId = 0x8003;
		static const int tag_count_position = 20;
		static const int tag_startoffset = 1;

		//! Player Interfaces
		player_devaddr_t rfid_id;
		//player_devaddr_t dio_id; //Use this for the buzzer and led latter
};

const char Acr120u::Acr120uCmdStrings[][Acr120u::Acr120uCmdLength ] =
{
    { 0x02, 0x30, 0x31, 0x45, 0x30, 0x30, 0x32, 0x30, 0x35, 0x30, 0x30, 0x45, 0x36, 0x03 },      // Reset
    { 0x02, 0x30, 0x31, 0x45, 0x30, 0x30, 0x32, 0x31, 0x33, 0x30, 0x30, 0x46, 0x30, 0x03 },      // Turn on Radio
    { 0x02, 0x30, 0x31, 0x45, 0x30, 0x30, 0x32, 0x30, 0x33, 0x30, 0x30, 0x45, 0x30, 0x03 }       // List Tags
};

int Acr120u::intFromHexTuple( char c1, char c2 ) const
{
    char c_str[] = { c1, c2, 0x00 };
    std::istringstream strm( c_str );
    int num;

    strm.setf( std::ios::hex, std::ios::basefield );
    strm >> num;

    return num;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
Acr120u::Acr120u(ConfigFile* cf, int section)
        : ThreadedDriver(cf, section) {
    //! Start with a clean device
    memset(&rfid_id,0,sizeof(player_devaddr_t));
    //memset(&dio_id,0,sizeof(player_devaddr_t)); //For the buzzer and led

    // Creating the rfid interface
    if (cf->ReadDeviceAddr(&(rfid_id), section, "provides", PLAYER_RFID_CODE, -1, NULL) == 0) {
        if (AddInterface(rfid_id) != 0) {
            SetError(-1);
            return;
        }
    } else {
        PLAYER_WARN("rfid interface not created for acr120u driver");
    }
    /*
    //For the buzzer and led
    if (cf->ReadDeviceAddr(&(dio_id), section, "provides", PLAYER_DIO_CODE, -1, NULL) == 0) {
        if (AddInterface(dio_id) != 0) {
            SetError(-1);
            return;
        }
    } else {
        PLAYER_WARN("dio interface not created for phidgetrfid driver");
    }*/


    // Set the libusb pointers to NULL
    busses=0;
    dev=0;
    dev_temp=0;

    // Read an option from the configuration file
    //serial = cf->ReadInt(section, "serial", -1);

    //Sampling rate and alarm time in mS
    samplingrate = cf->ReadFloat(section, "samplingrate", 200.0);
    alarmtime = cf->ReadFloat(section, "alarmtime", 210.0);

    return;
}

Acr120u::~Acr120u() {}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int Acr120u::MainSetup() {
    PLAYER_MSG0(1,"ACR120U driver initialising");
    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses=usb_get_busses();
    PLAYER_MSG0(1,"Searching for the device...");
    int found=0;
    while ((!found) && busses) {
    	dev_temp=busses->devices;
    	while ((!found) && dev_temp) {
    		if (dev_temp->descriptor.idVendor == 0x072f && dev_temp->descriptor.idProduct == 0x8003) {
                        dev=dev_temp;
                        HANDLE=usb_open(dev);
			if (usb_claim_interface(HANDLE,0) == 0) {
				found=1;
			} else {
				usb_close(HANDLE);
			}
		}
            	dev_temp=dev_temp->next;
    	}
    	busses=busses->next;
    }

    if (found==1) {
	PLAYER_MSG0(1,"Device found. Connection granted to the ACR120U.");
    } else {
	PLAYER_ERROR("There was a problem connecting to the ACR120u. You don't have device access permissions?");
	return(-1);
    }

    //Setting up the device
    char acrResponse[Acr120uResponseLength];
    char acrCommand[Acr120uCmdLength];
    //Reset command
    memcpy(acrCommand, Acr120uCmdStrings[RESET], Acr120uCmdLength);
    usb_control_msg(HANDLE,0x40,0x00,0x00,0x00,acrCommand,Acr120uCmdLength,0);
    for (int i=0;i<3;i++) {
	    usb_interrupt_read(HANDLE,0x01,&acrResponse[i*8],8,0);
    }
    //Turn on the Radio
    memcpy(acrCommand, Acr120uCmdStrings[TURN_ON_RADIO], Acr120uCmdLength);
    usb_control_msg(HANDLE,0x40,0x00,0x00,0x00,acrCommand,Acr120uCmdLength,0);
    for (int i=0;i<3;i++) {
	    usb_interrupt_read(HANDLE,0x01,&acrResponse[i*8],8,0);
    }

    PLAYER_MSG0(1,"ACR120U driver ready");
    // Start the device thread; spawns a new thread and executes
    // Phidgetrfid::Main(), which contains the main loop for the driver.
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void Acr120u::MainQuit() {

    PLAYER_MSG0(1,"Shutting ACR120U driver down");
    //Turn off the device and delete the ACR120U objects
    //Turn power off. This is missing yet
    //Release interface
    usb_clear_halt(HANDLE,0);
    usb_clear_halt(HANDLE,1);
    usb_release_interface(HANDLE,0);
    //Close device
    usb_reset(HANDLE);
    usb_close(HANDLE);
    busses=0;
    dev=0;
    dev_temp=0;

    PLAYER_MSG0(1,"ACR120U driver has been shutdown");
}

int Acr120u::ProcessMessage(QueuePointer &resp_queue,
                                      player_msghdr * hdr,
                                      void * data) {
    return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Acr120u::Main() {

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

        // test if we are supposed to cancel
        pthread_testcancel();

        // Process incoming messages.  Phidgetrfid::ProcessMessage() is
        // called on each message.
        ProcessMessages();

        //Get the Tags
        player_rfid_data_t data_rfid;
	char acrCommand[Acr120uCmdLength];
	char acrResponse[Acr120uResponseLength];
	//ListTag
	memcpy(acrCommand, Acr120uCmdStrings[LIST_TAGS], Acr120uCmdLength);
	usb_control_msg(HANDLE,0x40,0x00,0x00,0x00,acrCommand,14,0);
	for (int i=0;i<3;i++) {
		usb_interrupt_read(HANDLE,0x01,&acrResponse[i*8],8,0);
	}
        data_rfid.tags_count=acrResponse[tag_count_position]-0x30;
        data_rfid.tags = new player_rfid_tag_t[data_rfid.tags_count+1];

        //Receiving Tags
        for (unsigned int i=0;i<data_rfid.tags_count;i++) {
	        data_rfid.tags[i].guid = new char[8];
	        data_rfid.tags[i].type=1;
	        data_rfid.tags[i].guid_count=8;
		for (int j=0;j<3;j++) {
			usb_interrupt_read(HANDLE,0x01,&acrResponse[j*8],8,0);
		}
		for (int j=0;j<8;j++) {
			data_rfid.tags[i].guid[7-j] = intFromHexTuple(&acrResponse[j*2+tag_startoffset]);
		}
        }

        //Publishing data.
        if (rfid_id.interf !=0) {
            Publish(rfid_id, PLAYER_MSGTYPE_DATA, PLAYER_RFID_DATA_TAGS, (unsigned char*)&data_rfid, sizeof(player_rfid_data_t), NULL);
        }
        for (unsigned int i=0;i<data_rfid.tags_count;i++){
        	delete [] data_rfid.tags[i].guid;
	}
        delete [] data_rfid.tags;

        //point to calculate how much to sleep, call nanosleep, after sleep restart the timer
        //Get the ammount of time passed:
        gettimeofday( &tv_framerate_end, NULL );
        // figure out how much to sleep
        long usecs    = tv_framerate_end.tv_usec - tv_framerate_start.tv_usec;
        long secs     = tv_framerate_end.tv_sec  - tv_framerate_start.tv_sec;
        long elapsed_usecs = 1000000*secs  + usecs;
        long us_tosleep = static_cast<long>(samplingrate*1000) - elapsed_usecs;
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
Acr120u_Init(ConfigFile* cf, int section) {
    // Create and return a new instance of this driver
	return((Driver*)(new Acr120u(cf, section)));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void acr120u_Register(DriverTable* table) {
	table->AddDriver("acr120u", Acr120u_Init);
}

