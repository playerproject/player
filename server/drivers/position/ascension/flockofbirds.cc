#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

// Player includes
#include <player.h>
#include <device.h>
#include <drivertable.h>

// Flock of Birds Serial device interface...
// two classes, the first to do the access, the second to interface with player
// fairly light weight, 
// Probably could do this without a thread....but we will use one for now anyhow.


#define FOB_DEFAULT_PORT "/dev/ttyS0"
#define FOB_DEFAULT_RATE B115200

#define FOB_DATAMODE_POSITION_ANGLE 0x0

#define FOB_SLEEP_TIME_USEC 10000

class FlockOfBirdsSerial
{
public:
	
	FlockOfBirdsSerial(char * port = FOB_DEFAULT_PORT, int rate = FOB_DEFAULT_RATE);
	~FlockOfBirdsSerial();

	bool Open() {return fd >0;};
	int ProcessData(); 
	
	int StartStream() {int ret = WriteCommand(0x40);Stream = ret >= 0; return ret;};
	int StopStream() {Stream = false;return WriteCommand(0x42);};
	
	double * GetPosition();
	double GetRange();
	
	int SetRange(int);

protected:
	// serial port descriptor
	int fd;
	struct termios oldtio;
	
	// position data storage
	double Position[6];
	double SafePosition[6];
	
	int WriteCommand(char command, int Count = 0, char * Values = NULL);
	int ReadShorts(int Count = 0, short * Values = NULL);

	bool Stream;
	char LastCommand;
	int DataMode;
	double Range;

	void Lock();
	void Unlock();
	pthread_mutex_t lock;
};


FlockOfBirdsSerial::FlockOfBirdsSerial(char * port, int rate)
{
	fd = -1;
	
	pthread_mutex_init(&lock,NULL);

	// open the serial port
	fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	if ( fd<0 )
	{
		fprintf(stderr, "Could not open serial device %s\n",port);
		return;
	}
	
	// save the current io settings
	tcgetattr(fd, &oldtio);

	// rtv - CBAUD is pre-POSIX and doesn't exist on OS X
	// should replace this with ispeed and ospeed instead.
	
	// set up new settings
	struct termios newtio;
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = CS8 | /*CLOCAL |*/ CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0/*ICANON*/;
	
	// activate new settings
	tcflush(fd, TCIFLUSH);
	if (cfsetispeed(&newtio, rate) < 0 || cfsetospeed(&newtio, rate) < 0)
	{
		fprintf(stderr,"Failed to set serial baud rate: %d\n", rate);
		tcsetattr(fd, TCSANOW, &oldtio);	
		close(fd);
		fd = -1;				
		return;
	}
	tcsetattr(fd, TCSANOW, &newtio);
	tcflush(fd, TCIOFLUSH);
	
	// clear the input buffer in case junk data is on the port
	usleep(10000);
	tcflush(fd, TCIFLUSH);
	
	// set device into Position/Angle Mode
	WriteCommand(0x59);

	// try get a test point from the port
	WriteCommand(0x42);

	// give device time to respond
	usleep(10000);
	if (ProcessData() <= 0)
	{
		fprintf(stderr,"Failed to initialise the FlockOfBirds Serial Port %s\n",port);
		tcsetattr(fd, TCSANOW, &oldtio);
		close(fd);
		fd = -1;
	}
	
	Range = GetRange();
}


FlockOfBirdsSerial::~FlockOfBirdsSerial()
{
	// restore old port settings
	if (fd > 0)
	{
		tcsetattr(fd, TCSANOW, &oldtio);
		close(fd);	
	}
}

void FlockOfBirdsSerial::Lock()
{
	pthread_mutex_lock(&lock);
}

void FlockOfBirdsSerial::Unlock()
{
	pthread_mutex_unlock(&lock);
}

int FlockOfBirdsSerial::WriteCommand(char command, int length, char * data)
{

	if (fd < 0)
		return -1;

	LastCommand = command;
	if (Stream)
	{
		fprintf(stderr,"Warning, Extra processing is needed to write commands when in stream mode...aborting\n");
		return -1;
	}
	
	// switch to blocking IO for the write
	int flags = fcntl(fd, F_GETFL);
	if (flags < 0 || fcntl(fd,F_SETFL,flags &~O_NONBLOCK) < 0)
	{
		fprintf(stderr,"Error changing to blocking write (%d - %s), disabling\n",errno,strerror(errno));
		tcsetattr(fd, TCSANOW, &oldtio);	
		fd = -1;
		Unlock();
		return -1;
	}	
	
	Lock();
	if((write(fd, &command, 1)) < 1 || (length && (write(fd, data, length)) < length))
	{
		fprintf(stderr,"Error writing to FOB (%d - %s), disabling\n",errno,strerror(errno));
		tcsetattr(fd, TCSANOW, &oldtio);	
		fd = -1;
		Unlock();
		return -1;
	}
	
	// restore flags
	if (fcntl(fd,F_SETFL,flags) < 0)
	{
		fprintf(stderr,"Error restoring file mode (%d - %s), disabling\n",errno,strerror(errno));
		tcsetattr(fd, TCSANOW, &oldtio);	
		fd = -1;
		Unlock();
		return -1;
	}	
	
	Unlock();
	return 0;
}

int FlockOfBirdsSerial::ReadShorts(int Count, short * Values)
{
	for (int i = 0; i < Count; ++i)
	{
		int ret;
		char temp;
		while((ret = read(fd,&temp,1)) < 1)
		{
			if (ret == -1 && errno != EAGAIN)
				return -1;
		}
		Values[i] = temp;

		while((ret = read(fd,&temp,1)) < 1)
		{
			if (ret == -1 && errno != EAGAIN)
				return -1;
		}
		Values[i] |= temp << 8;
	}
	return 0;
}		
		
	

//Process waiting data for FOB
int FlockOfBirdsSerial::ProcessData()
{
	if (fd < 0)
		return -1;
	
	int Count = 0;
	// for the moment we assume the only data we recieve is position data
	// this will need changed if we ever want to read parameters
	// could do it with a datamode parameter option
	Lock();	
	switch (DataMode)
	{
		case FOB_DATAMODE_POSITION_ANGLE:
		{
			// set up a sort of state machine so that we just process data as it arrives
			static bool Done = true;
			static short Data[6];
			static int NextValue = 0;
			static bool FirstByte = true;
			
			char temp;
			int ret;
			while ((ret = read(fd,&temp,1)) > 0)
			{
				// are we waiting for a start bit
				if (Done && !(temp & 0x80))
					continue;
					
				// if we get a start bit reset our state machine
				if (temp & 0x80)
				{
					Done = false;
					NextValue = 0;
					FirstByte = true;
					temp &= 0x7F;
				}
				
				// now start the processing
				if (FirstByte)
				{
					Data[NextValue] = temp << 2;
					FirstByte = false;
				}
				else
				{
					Data[NextValue] |= temp << 9;
					FirstByte = true;
					NextValue++;
					Count++;
					if (NextValue == 6)
					{
						Done = true;
						// convert 16 bit position to mm
						for (int i = 0;i< 3;++i)
							Position[i] = Range * static_cast<double> (Data[i])/0x7FFF;
						// convert 16 bit angle to degrees
						for (int i = 3;i< 6;++i)
							Position[i] = 180.0 * static_cast<double> (Data[i])/0x7FFF;		
					}
				}
			
			}
			if (ret < 0 && errno != EAGAIN)
				fprintf(stderr,"FlockOfBirds: error reading data (%d - %s)\n",errno,strerror(errno));
		
		}
		break;
		
		default:
			fprintf(stderr,"Unknown Data mode\n");
			tcflush(fd,TCIFLUSH);
			break;
	}
	Unlock();
	return Count;		
}

double * FlockOfBirdsSerial::GetPosition()
{
	Lock();
	memcpy(SafePosition,Position,sizeof(double)*6);
	Unlock();
	return SafePosition;
}

int FlockOfBirdsSerial::SetRange(int Range)
{
	short Temp = htons(Range);
	if (WriteCommand(0x50) < 0)
		return -1;
	return WriteCommand(0x03,2,reinterpret_cast<char *> (&Temp));
}

//return the range of the sensor in mm
double FlockOfBirdsSerial::GetRange()
{
	
	if (WriteCommand(0x4F) < 0)
		return -1;
	if (WriteCommand(0x03) < 0)
		return -1;
	short Range;
	if (ReadShorts(1,&Range) < 0)
		return -1;
	return Range == 0 ? 25.4 * 36 : 25.4 * 72;
	
}

///////////////////////////////////////////////////////////////
// Player Driver Class                                       //
///////////////////////////////////////////////////////////////

class FlockOfBirds_Device : public CDevice 
{
	protected:
		// this function will be run in a separate thread
		virtual void Main();

		int HandleConfig(void *client, unsigned char *buffer, size_t len);


	public:
		FlockOfBirdsSerial * fob;
		
		// config params
		char fob_serial_port[MAX_FILENAME_SIZE];
		int Rate;
		unsigned char MoveMode;


		FlockOfBirds_Device(char* interface, ConfigFile* cf, int section);

		virtual int Setup();
		virtual int Shutdown();
};
  
// initialization function
CDevice* FlockOfBirds_Init(char* interface, ConfigFile* cf, int section)
{
	if(strcmp(interface, PLAYER_POSITION3D_STRING))
	{
		PLAYER_ERROR1("driver \"flockofbirds\" does not support interface \"%s\"\n", interface);
    		return(NULL);
  	}
  	else
    		return static_cast<CDevice*> (new FlockOfBirds_Device(interface, cf, section));
}


// a driver registration function
void 
FlockOfBirds_Register(DriverTable* table)
{
	table->AddDriver("flockofbirds", PLAYER_ALL_MODE, FlockOfBirds_Init);
}

FlockOfBirds_Device::FlockOfBirds_Device(char* interface, ConfigFile* cf, int section) :
	CDevice(sizeof(player_position3d_data_t),sizeof(player_position3d_cmd_t),1,1)
{
	fob = NULL;
	
	strncpy(fob_serial_port,
          cf->ReadString(section, "port", FOB_DEFAULT_PORT),
          sizeof(fob_serial_port));
	Rate = cf->ReadInt(section, "baudrate", FOB_DEFAULT_RATE);	
}

int 
FlockOfBirds_Device::Setup()
{

	printf("FOB connection initializing (%s)...", fob_serial_port);
  	fflush(stdout);
	fob = new FlockOfBirdsSerial(fob_serial_port,Rate);
	if (fob != NULL && fob->Open() && !fob->StartStream())
		printf("Success\n");
	else
	{
		printf("Failed\n");
		return -1;
	}

	player_position3d_data_t data;
	player_position3d_cmd_t cmd;

	bzero(&data,sizeof(data));
	bzero(&cmd,sizeof(cmd));

  	PutData((unsigned char*)&data,sizeof(data),0,0);
  	PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));

	// start the thread to talk with the camera
	StartThread();

	return(0);
}

int 
FlockOfBirds_Device::Shutdown()
{
	puts("FlockOfBirds_Device::Shutdown");


	fob->StopStream();
	StopThread();

	delete fob;
	fob=NULL;

	return(0);
}


/* handle a configuration request
 *
 * returns: 0 on success, -1 on error
 */
int
FlockOfBirds_Device::HandleConfig(void *client, unsigned char *buffer, size_t len)
{
	//bool success = false;

	// we dont support any config, so just NACK them all
	if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK)) {
		  PLAYER_ERROR("PTU46: Failed to PutReply\n");
		  return -1;
	}
	return 0;
}

// this function will be run in a separate thread
void 
FlockOfBirds_Device::Main()
{
  player_position3d_data_t data;
  player_position3d_cmd_t command;
  char buffer[256];
  size_t buffer_len;
  void *client;
  //short pan=0, tilt=0;
  //short panspeed=0, tiltspeed=0;
  
  while(1) 
  {
	pthread_testcancel();
    	GetCommand((unsigned char*)&command, sizeof(player_position3d_cmd_t));
    	pthread_testcancel();

	// update the position data
	if (fob->ProcessData()>0)
	{
		pthread_testcancel();
		double * PosData = fob->GetPosition();
		data.xpos = htonl(static_cast<long> (PosData[0]));
		data.ypos = htonl(static_cast<long> (PosData[1]));
		data.zpos = htonl(static_cast<long> (-PosData[2]));
		// translate degerees to arc seconds
		data.yaw = htonl(static_cast<long> ((PosData[3] < 0 ? (PosData[3] + 360) : PosData[3]) * 3600));
		data.pitch = htonl(static_cast<long> ((PosData[4] < 0 ? (PosData[4] + 360) : PosData[4]) * 3600));
		data.roll = htonl(static_cast<long> ((PosData[5] < 0 ? (PosData[5] + 360) : PosData[5]) * 3600));

		PutData((unsigned char*)&data, sizeof(player_position3d_data_t),0,0);
	}    

    	// check for config requests 
    	if ((buffer_len = GetConfig(&client, (void *)buffer, sizeof(buffer))) > 0) 
	{
        	if (HandleConfig(client, (uint8_t *)buffer, buffer_len) < 0) 
			fprintf(stderr, "FlockOfBirds: error handling config request\n");
	}

	// repeat frequency (default to 10 Hz)
    	usleep(FOB_SLEEP_TIME_USEC);
   }
}

