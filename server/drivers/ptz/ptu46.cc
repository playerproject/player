/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
 * $Id$
 *
 * methods for initializing, commanding, and getting data out of
 * the directed perceptions ptu-46 pan tilt unit camera
 * 
 * Author: Toby Collett (University of Auckland)
 * Date: 2003-02-10
 *
 */

/* This file is divided into two classes, the first simply deals with 
 * the control of the pan-tilt unit, providing simple interfaces such as
 * set pos and get pos.
 * The second class provides the player interface, hopefully this makes
 * the code easier to understand and a good base for a pretty much minimal
 * set up of a player driver
 */


#if HAVE_CONFIG_H
  #include <config.h>
#endif

// serial includes
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

//serial defines
#define PTU46_DEFAULT_BAUD B9600
#define PTU46_BUFFER_LEN 255

// command defines
#define PTU46_PAN 'p'
#define PTU46_TILT 't'
#define PTU46_MIN 'n'
#define PTU46_MAX 'x'
#define PTU46_MIN_SPEED 'l'
#define PTU46_MAX_SPEED 'u'
#define PTU46_VELOCITY 'v'
#define PTU46_POSITION 'i'

// Includes needed for player
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */
#include <device.h>
#include <drivertable.h>
#include <player.h>

#include <replace/replace.h>

#define DEFAULT_PTZ_PORT "/dev/ttyR1"
#define PTZ_SLEEP_TIME_USEC 100000





//
// Pan-Tilt Control Class
// 



class PTU46
{
public:
	
	PTU46(char * port, int rate);
	~PTU46();


	// get count/degree resolution
	float GetRes(char type);
	// get position limit
	int PTU46::GetLimit(char type, char LimType);

	// get/set position in degrees
	int GetPos(char type);
	bool SetPos(char type, int pos, bool Block = false);

	// get/set speed in degrees/sec
	bool PTU46::SetSpeed(char type, int speed);
	int PTU46::GetSpeed(char type);

	// get/set move mode
	bool SetMode(char type);
	char GetMode();
	
	bool Open() {return fd >0;};

	// Position Limits
	int TMin, TMax, PMin, PMax;
	// Speed Limits
	int TSMin, TSMax, PSMin, PSMax;
		
protected:
	// pan and tilt resolution
	float tr,pr;
	
	// serial port descriptor
	int fd;
	struct termios oldtio;
	
	// read buffer
	char buffer[PTU46_BUFFER_LEN+1];

	int Write(char * data, int length = 0);
};


// Constructor opens the serial port, and read the config info from it
PTU46::PTU46(char * port, int rate)
{
	tr = pr = 1;
	fd = -1;
		
	// open the serial port
	
	fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
	if ( fd<0 )
	{
		fprintf(stderr, "Could not open serial device %s\n",port);
		return;
	}
	fcntl(fd,F_SETFL, 0);
	
	// save the current io settings
	tcgetattr(fd, &oldtio);

	// rtv - CBAUD is pre-POSIX and doesn't exist on OS X
	// should replace this with ispeed and ospeed instead.
	
	// set up new settings
	struct termios newtio;
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = (rate & CBAUD) | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = ICANON;
	
	// activate new settings
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);
	
	// now set up the pan tilt camera
	Write("ft "); // terse feedback
	Write("ed "); // disable echo
	Write("ci "); // position mode

	// delay here so data has arrived at serial port so we can flush it
	usleep(200000);
	tcflush(fd, TCIFLUSH);

	

	// get pan tilt encoder res
	tr = GetRes(PTU46_TILT);
	pr = GetRes(PTU46_PAN);
	
	PMin = GetLimit(PTU46_PAN, PTU46_MIN);
	PMax = GetLimit(PTU46_PAN, PTU46_MAX);
	TMin = GetLimit(PTU46_TILT, PTU46_MIN);
	TMax = GetLimit(PTU46_TILT, PTU46_MAX);
	PSMin = GetLimit(PTU46_PAN, PTU46_MIN_SPEED);
	PSMax = GetLimit(PTU46_PAN, PTU46_MAX_SPEED);
	TSMin = GetLimit(PTU46_TILT, PTU46_MIN_SPEED);
	TSMax = GetLimit(PTU46_TILT, PTU46_MAX_SPEED);


	if (tr <= 0 || pr <= 0 || PMin == 0 || PMax == 0 || TMin == 0 || TMax == 0)
	{
		// if limit request failed try resetting the unit and then getting limits..
		Write(" r "); // reset pan-tilt unit (also clears any bad input on serial port)

		// wait for reset to complete
		int len = 0;
		char temp;
		char response[10] = "!T!T!P!P*";

		for (int i = 0; i < 9; ++i)
		{
			while((len = read(fd, &temp, 1 )) == 0);
			if ((len != 1) || (temp != response[i]))
			{
				fprintf(stderr,"Error Resetting Pan Tilt unit\n");
				fprintf(stderr,"Stopping access to pan-tilt unit\n");		
				tcsetattr(fd, TCSANOW, &oldtio);	
				fd = -1;
			}	
		}	

		// delay here so data has arrived at serial port so we can flush it
		usleep(100000);
		tcflush(fd, TCIFLUSH);


		// get pan tilt encoder res
		tr = GetRes(PTU46_TILT);
		pr = GetRes(PTU46_PAN);

		PMin = GetLimit(PTU46_PAN, PTU46_MIN);
		PMax = GetLimit(PTU46_PAN, PTU46_MAX);
		TMin = GetLimit(PTU46_TILT, PTU46_MIN);
		TMax = GetLimit(PTU46_TILT, PTU46_MAX);
		PSMin = GetLimit(PTU46_PAN, PTU46_MIN_SPEED);
		PSMax = GetLimit(PTU46_PAN, PTU46_MAX_SPEED);
		TSMin = GetLimit(PTU46_TILT, PTU46_MIN_SPEED);
		TSMax = GetLimit(PTU46_TILT, PTU46_MAX_SPEED);

		if (tr <= 0 || pr <= 0 || PMin == 0 || PMax == 0 || TMin == 0 || TMax == 0)
		{
			// if it really failed give up and disable the driver
			fprintf(stderr,"Error getting pan-tilt resolution...is the serial port correct?\n");
			fprintf(stderr,"Stopping access to pan-tilt unit\n");		
			tcsetattr(fd, TCSANOW, &oldtio);	
			fd = -1;
		}
	}
}


PTU46::~PTU46()
{
	// restore old port settings
	if (fd > 0)
		tcsetattr(fd, TCSANOW, &oldtio);	
}


int PTU46::Write(char * data, int length)
{

	if (fd < 0)
		return -1;
		
	// autocalculate if using short form
	if (length == 0)
		length = strlen(data);
		
	// ugly error handling, if write fails then shut down unit
	if(write(fd, data, length) < length)
	{
		fprintf(stderr,"Error writing to Pan Tilt Unit, disabling\n");
		tcsetattr(fd, TCSANOW, &oldtio);	
		fd = -1;
		return -1;
	}
	return 0;
}


// get count/degree resolution
float PTU46::GetRes(char type)
{
	if (fd < 0)
		return -1;
	char cmd[4] = " r ";
	cmd[0] = type;
	
	// get pan res
	int len = 0;
	Write(cmd);
	len = read(fd, buffer, PTU46_BUFFER_LEN );
	
	if (len < 3 || buffer[0] != '*')
	{
		fprintf(stderr,"Error getting pan-tilt res\n");
		return -1;
	}
		
	buffer[len] = '\0';
	return strtod(&buffer[2],NULL)/3600;	
}

// get position limit
int PTU46::GetLimit(char type, char LimType)
{
	if (fd < 0)
		return -1;
	char cmd[4] = "   ";
	cmd[0] = type;
	cmd[1] = LimType;
	
	// get limit
	int len = 0;
	Write(cmd);
	len = read(fd, buffer, PTU46_BUFFER_LEN );
	
	if (len < 3 || buffer[0] != '*')
	{
		fprintf(stderr,"Error getting pan-tilt limit\n");
		return -1;
	}
		
	buffer[len] = '\0';
	return strtol(&buffer[2],NULL,0);	
}


// get position in degrees
int PTU46::GetPos(char type)
{
	if (fd < 0)
		return -1;

	char cmd[4] = " p ";
	cmd[0] = type;
	
	// get pan pos
	int len = 0;
	Write(cmd);
	len = read(fd, buffer, PTU46_BUFFER_LEN );
	
	if (len < 3 || buffer[0] != '*')
	{
		fprintf(stderr,"Error getting pan-tilt pos\n");
		return -1;
	}
		
	buffer[len] = '\0';
	
	return static_cast<int> (rint(strtod(&buffer[2],NULL) * (type == PTU46_TILT ? tr : pr)));	
}


// set position in degrees
bool PTU46::SetPos(char type, int pos, bool Block)
{

	if (fd < 0)
		return false;

	// get raw encoder count to move		
	int Count = static_cast<int> (pos/(type == PTU46_TILT ? tr : pr));

	// Check limits
	if (Count < (type == PTU46_TILT ? TMin : PMin) || Count > (type == PTU46_TILT ? TMax : PMax))
	{
		fprintf(stderr,"Pan Tilt Value out of Range: %c %d(%d) (%d-%d)\n", type, pos, Count, (type == PTU46_TILT ? TMin : PMin),(type == PTU46_TILT ? TMax : PMax));
		return false;
	}

	char cmd[16];
	snprintf(cmd,16,"%cp%d ",type,Count);
	
	// set pos
	int len = 0;
	Write(cmd);
	len = read(fd, buffer, PTU46_BUFFER_LEN );
	
	if (len <= 0 || buffer[0] != '*')
	{
		fprintf(stderr,"Error setting pan-tilt pos\n");
		return false;
	}
	if (Block)
	{
		while (GetPos(type)!=pos);	
	}
	return true;
}

// get speed in degrees/sec
int PTU46::GetSpeed(char type)
{
	if (fd < 0)
		return -1;

	char cmd[4] = " s ";
	cmd[0] = type;
	
	// get speed
	int len = 0;
	Write(cmd);
	len = read(fd, buffer, PTU46_BUFFER_LEN );
	
	if (len < 3 || buffer[0] != '*')
	{
		fprintf(stderr,"Error getting pan-tilt speed\n");
		return -1;
	}
		
	buffer[len] = '\0';
	
	return static_cast<int> (rint(strtod(&buffer[2],NULL) * (type == PTU46_TILT ? tr : pr)));	
}



// set speed in degrees/sec
bool PTU46::SetSpeed(char type, int pos)
{
	if (fd < 0)
		return false;

	// get raw encoder speed to move		
	int Count = static_cast<int> (pos/(type == PTU46_TILT ? tr : pr));
	// Check limits
	if (abs(Count) < (type == PTU46_TILT ? TSMin : PSMin) || abs(Count) > (type == PTU46_TILT ? TSMax : PSMax))
	{
		fprintf(stderr,"Pan Tilt Speed Value out of Range: %c %d(%d) (%d-%d)\n", type, pos, Count, (type == PTU46_TILT ? TSMin : PSMin),(type == PTU46_TILT ? TSMax : PSMax));
		return false;
	}

	char cmd[16];
	snprintf(cmd,16,"%cs%d ",type,Count);
	
	// set speed
	int len = 0;
	Write(cmd);
	len = read(fd, buffer, PTU46_BUFFER_LEN );
	
	if (len <= 0 || buffer[0] != '*')
	{
		fprintf(stderr,"Error setting pan-tilt speed\n");
		return false;
	}
	return true;
}


// set movement mode (position/velocity)
bool PTU46::SetMode(char type)
{
	if (fd < 0)
		return false;

	char cmd[4] = "c  ";
	cmd[1] = type;

	// set mode
	int len = 0;
	Write(cmd);
	len = read(fd, buffer, PTU46_BUFFER_LEN );
	
	if (len <= 0 || buffer[0] != '*')
	{
		fprintf(stderr,"Error setting pan-tilt move mode\n");
		return false;
	}
	return true;
}

// get position in degrees
char PTU46::GetMode()
{
	if (fd < 0)
		return -1;

	// get pan tilt mode
	int len = 0;
	Write("c ");
	len = read(fd, buffer, PTU46_BUFFER_LEN );
	
	if (len < 3 || buffer[0] != '*')
	{
		fprintf(stderr,"Error getting pan-tilt pos\n");
		return -1;
	}
		
	if (buffer[2] == 'p')
		return PTU46_VELOCITY;
	else if (buffer[2] == 'i')
		return PTU46_POSITION;
	else
		return -1;
}




///////////////////////////////////////////////////////////////
// Player Driver Class                                       //
///////////////////////////////////////////////////////////////

class PTU46_Device:public CDevice 
{
	protected:
		// this function will be run in a separate thread
		virtual void Main();

		int HandleConfig(void *client, unsigned char *buffer, size_t len);


	public:
		PTU46 * pantilt;
		
		// config params
		char ptz_serial_port[MAX_FILENAME_SIZE];
		int Rate;
		unsigned char MoveMode;


		PTU46_Device(char* interface, ConfigFile* cf, int section);

		virtual int Setup();
		virtual int Shutdown();
};
  
// initialization function
CDevice* PTU46_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_PTZ_STRING))
  {
    PLAYER_ERROR1("driver \"ptu46\" does not support interface \"%s\"\n", interface);
    return(NULL);
  }
  else
    return static_cast<CDevice*> (new PTU46_Device(interface, cf, section));
}


// a driver registration function
void 
PTU46_Register(DriverTable* table)
{
  table->AddDriver("ptu46", PLAYER_ALL_MODE, PTU46_Init);
}

PTU46_Device::PTU46_Device(char* interface, ConfigFile* cf, int section) :
  CDevice(sizeof(player_ptz_data_t),sizeof(player_ptz_cmd_t),1,1)
{
  player_ptz_data_t data;
  player_ptz_cmd_t cmd;

  data.pan = data.tilt = data.zoom = data.panspeed = data.tiltspeed = 0;
  cmd.pan = cmd.tilt = cmd.zoom = cmd.panspeed = data.tiltspeed = 0;

  PutData((unsigned char*)&data,sizeof(data),0,0);
  PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));

	MoveMode = PLAYER_PTZ_POSITION_CONTROL;

	strncpy(ptz_serial_port,
          cf->ReadString(section, "port", DEFAULT_PTZ_PORT),
          sizeof(ptz_serial_port));
	Rate = cf->ReadInt(section, "baudrate", PTU46_DEFAULT_BAUD);
	
}

int 
PTU46_Device::Setup()
{

  printf("PTZ connection initializing (%s)...", ptz_serial_port);
  fflush(stdout);

	pantilt = new PTU46(ptz_serial_port,Rate);
	if (pantilt != NULL && pantilt->Open())
		printf("Success\n");
	else
	{
		printf("Failed\n");
		return -1;
	}

	player_ptz_cmd_t cmd;
	cmd.pan = cmd.tilt = 0;
	cmd.zoom = 0;
	cmd.panspeed = 0;
	cmd.tiltspeed = 0;
  

  // zero the command buffer
  PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));

  // start the thread to talk with the camera
  StartThread();

  return(0);
}

int 
PTU46_Device::Shutdown()
{
	puts("PTU46_Device::Shutdown");

	StopThread();

	delete pantilt;

	puts("PTZ camera has been shutdown");
	return(0);
}


/* handle a configuration request
 *
 * returns: 0 on success, -1 on error
 */
int
PTU46_Device::HandleConfig(void *client, unsigned char *buffer, size_t len)
{
	bool success = false;

  switch(buffer[0]) {
  case PLAYER_PTZ_GENERIC_CONFIG_REQ:
	// we dont have any generic config at the moment
	// could be usig in future to set power mode etc of pan tilt unit
	// and acceleration etc, for now respond NACK to any request

	if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK)) {
	  PLAYER_ERROR("PTU46: Failed to PutReply\n");
	}
	break;
  case PLAYER_PTZ_CONTROL_MODE_REQ:
	if (buffer[1] != MoveMode && buffer[1] == PLAYER_PTZ_VELOCITY_CONTROL)
	{
		if(pantilt->SetMode(PTU46_VELOCITY))
		{
			success = true;
			MoveMode = PLAYER_PTZ_VELOCITY_CONTROL;
		}		
	}
	else if (buffer[1] != MoveMode && buffer[1] == PLAYER_PTZ_POSITION_CONTROL)
	{
		if(pantilt->SetMode(PTU46_POSITION))
		{
			success = true;
			MoveMode = PLAYER_PTZ_POSITION_CONTROL;
		}		
	}

	if (success)
	{
		if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK)) {
		  PLAYER_ERROR("PTU46: Failed to PutReply\n");
		}
	}
	else
	{
		if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK)) {
		  PLAYER_ERROR("PTU46: Failed to PutReply\n");
		}
	}
    break;

  default:
    return -1;
  }

  return 0;
}

// this function will be run in a separate thread
void 
PTU46_Device::Main()
{
  player_ptz_data_t data;
  player_ptz_cmd_t command;
  char buffer[256];
  size_t buffer_len;
  void *client;
  short pan=0, tilt=0;
  short panspeed=0, tiltspeed=0;
  
  while(1) {
    pthread_testcancel();
    GetCommand((unsigned char*)&command, sizeof(player_ptz_cmd_t));
    pthread_testcancel();


	command.pan = ntohs(command.pan);
	command.tilt = ntohs(command.tilt);
	command.zoom = ntohs(command.zoom);
	command.panspeed = ntohs(command.panspeed);
	command.tiltspeed = ntohs(command.tiltspeed);

	bool success = true;
	// Different processing depending on movement mode
	if (MoveMode == PLAYER_PTZ_VELOCITY_CONTROL)
	{
		// ignore pan and tilt, just use velocity
		if (command.panspeed != panspeed)
		{
			if(pantilt->SetSpeed(PTU46_PAN,command.panspeed))
				panspeed = command.panspeed;
			else
				success = false;
		}
		if (command.tiltspeed != tiltspeed)
		{
			if(pantilt->SetSpeed(PTU46_TILT,command.tiltspeed))
				tiltspeed = command.tiltspeed;
			else
				success = false;
		}
	}
	else
	{
		// position move mode, ignore zero velocities, set pan tilt pos
		if (command.pan != pan)
		{
			if(pantilt->SetPos(PTU46_PAN,command.pan))
				pan = command.pan;
			else
				success = false;
		}
		if (command.tilt != tilt)
		{
			if(pantilt->SetPos(PTU46_TILT,command.tilt))
				tilt = command.tilt;
			else
				success = false;
		}
		if (command.panspeed != panspeed && command.panspeed != 0)
		{
			if(pantilt->SetSpeed(PTU46_PAN,command.panspeed))
				panspeed = command.panspeed;
			else
				success = false;
		}
		if (command.tiltspeed != tiltspeed && command.tiltspeed != 0)
		{
			if(pantilt->SetSpeed(PTU46_TILT,command.tiltspeed))
				tiltspeed = command.tiltspeed;
			else
				success = false;
		}
	}
    
  
    // Copy the data.
    data.pan = htons(pantilt->GetPos(PTU46_PAN));
    data.tilt = htons(pantilt->GetPos(PTU46_TILT));
    data.zoom = 0;
	data.panspeed = htons(pantilt->GetSpeed(PTU46_PAN));
	data.tiltspeed = htons(pantilt->GetSpeed(PTU46_TILT));
    
    /* test if we are supposed to cancel */
    pthread_testcancel();
    PutData((unsigned char*)&data, sizeof(player_ptz_data_t),0,0);
    

    // check for config requests 
    if ((buffer_len = GetConfig(&client, (void *)buffer, sizeof(buffer))) > 0) {
      
      if (HandleConfig(client, (uint8_t *)buffer, buffer_len) < 0) {
		fprintf(stderr, "PTU46: error handling config request\n");
      }
    }

	// repeat frequency (default to 10 Hz)
    usleep(PTZ_SLEEP_TIME_USEC);
    }
}

