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
 Desc: Driver for the SICK S3000 laser
 Author: Toby Collett (based on lms200 by Andrew Howard)
 Date: 7 Nov 2000
 CVS: $Id$
*/

/** @ingroup drivers Drivers */
/** @{ */
/** @defgroup driver_sicks3000 sicks3000
 * @brief SICK S 3000 laser range-finder


The sicks3000 driver controls the SICK S 3000 safety laser scanner interpreting its data output.
The driver is very basic and assumes the S3000 has already been configured to continuously output
its measured data on the RS422 data lines.

It is also assumed that the laser is outputing its full 190 degree scan in a single scanning block.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_laser

@par Requires

- none

@par Configuration requests

- PLAYER_LASER_REQ_GET_GEOM
- PLAYER_LASER_REQ_GET_CONFIG
  
@par Configuration file options

- port (string)
  - Default: "/dev/ttyS0"
  - Serial port to which laser is attached.  If you are using a
    USB/232 or USB/422 converter, this will be "/dev/ttyUSBx".

- transfer_rate (integer)
  - Rate desired for data transfers, negotiated after connection
  - Default: 38400
  - Baud rate.  Valid values are 9600, 19200, 38400, 125k, 250k, 500k

- pose (length tuple)
  - Default: [0.0 0.0 0.0]
  - Pose (x,y,theta) of the laser, relative to its parent object (e.g.,
    the robot to which the laser is attached).

- size (length tuple)
  - Default: [0.15 0.15]
  - Footprint (x,y) of the laser.
      
@par Example 

@verbatim
driver
(
  name "sicks3000"
  provides ["laser:0"]
  port "/dev/ttyS0"
)
@endverbatim

@author Toby Collett

*/
/** @} */
  

  
#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h> // for htons etc

#undef HAVE_HI_SPEED_SERIAL
#ifdef HAVE_LINUX_SERIAL_H
  #ifndef DISABLE_HIGHSPEEDSICK
    #include <linux/serial.h>
    #define HAVE_HI_SPEED_SERIAL
  #endif
#endif

#include <libplayercore/playercore.h>
#include <replace/replace.h>
extern PlayerTime* GlobalTime;

#define DEFAULT_LASER_PORT "/dev/ttyS0"
#define DEFAULT_LASER_TRANSFER_RATE 38400

#define RX_BUFFER_SIZE 4096

// The laser device class.
class SickS3000 : public Driver
{
  public:
    
    // Constructor
    SickS3000(ConfigFile* cf, int section);
    ~SickS3000();

    int Setup();
    int Shutdown();

    // MessageHandler
    int ProcessMessage(QueuePointer &resp_queue, 
		       player_msghdr * hdr, 
		       void * data);
  private:

    // Main function for device thread.
    virtual void Main();

    // Open the terminal
    // Returns 0 on success
    int OpenTerm();

    // Close the terminal
    // Returns 0 on success
    int CloseTerm();
    
    // Set the terminal speed
	// can be any value valid for the s3000
    // Returns 0 on success
    int ChangeTermSpeed(int speed);

    // Set the laser data rate
    // Valid values are 9600 and 38400
    // Returns 0 on success
    int SetLaserSpeed(int speed);

    // Read range data from laser
    int ReadLaserData();

    // Process range data from laser
    int ProcessLaserData();

    
    // Calculates CRC for a telegram
    unsigned short CreateCRC(uint8_t *data, ssize_t len);

    // Get the time (in ms)
    int64_t GetTime();

  protected:

    // Laser pose in robot cs.
    double pose[3];
    double size[2];
    
    // Name of device used to communicate with the laser
    const char *device_name;
    
    // laser device file descriptor
    int laser_fd;           

    // Scan width and resolution.
    int scan_width, scan_res;

    // Start and end scan angles (for restricted scan).  These are in
    // units of 0.01 degrees.
    int min_angle, max_angle;
    
    // Start and end scan segments (for restricted scan).  These are
    // the values used by the laser.
    int scan_min_segment, scan_max_segment;

    // Range resolution (1 = 1mm, 10 = 1cm, 100 = 10cm).
    int range_res;

    // Turn intensity data on/off
    bool intensity;

    // Is the laser upside-down? (if so, we'll reverse the ordering of the
    // readings)
    int invert;

    bool can_do_hi_speed;
    int connect_rate;  // Desired rate for first connection
    int transfer_rate; // Desired rate for operation
    int current_rate;  // Current rate

    int scan_id;
    
    // rx buffer
    uint8_t * rx_buffer;
    unsigned int rx_buffer_size;
    unsigned int rx_count;

    // storage for outgoing data
    player_laser_data_t data_packet;
    player_laser_config_t config_packet;
    

#ifdef HAVE_HI_SPEED_SERIAL
  struct serial_struct old_serial;
#endif
};

// a factory creation function
Driver* SickS3000_Init(ConfigFile* cf, int section)
{
  return((Driver*)(new SickS3000(cf, section)));
}

// a driver registration function
void SickS3000_Register(DriverTable* table)
{
  table->AddDriver("sicks3000", SickS3000_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Device codes

#define STX     0x02
#define ACK     0xA0
#define NACK    0x92
#define CRC16_GEN_POL 0x8005


////////////////////////////////////////////////////////////////////////////////
// Error macros
#define RETURN_ERROR(erc, m) {PLAYER_ERROR(m); return erc;}
 
////////////////////////////////////////////////////////////////////////////////
// Constructor
SickS3000::SickS3000(ConfigFile* cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_LASER_CODE)
{
  // allocate our recieve buffer
  rx_buffer_size = RX_BUFFER_SIZE;
  rx_buffer = new uint8_t[rx_buffer_size];
  assert(rx_buffer);
  
  memset(&data_packet,0,sizeof(data_packet));
  data_packet.min_angle = DTOR(-95);
  data_packet.max_angle = DTOR(95);
  data_packet.resolution = DTOR(0.25);
  data_packet.max_range = 49;

  memset(&config_packet,0,sizeof(config_packet));
  config_packet.min_angle = DTOR(-95);
  config_packet.max_angle = DTOR(95);
  config_packet.resolution = DTOR(0.25);
  config_packet.max_range = 49;
  
  // Laser geometry.
  this->pose[0] = cf->ReadTupleLength(section, "pose", 0, 0.0);
  this->pose[1] = cf->ReadTupleLength(section, "pose", 1, 0.0);;
  this->pose[2] = cf->ReadTupleLength(section, "pose", 2, 0.0);;
  this->size[0] = 0.15;
  this->size[1] = 0.15;

  // Serial port
  this->device_name = cf->ReadString(section, "port", DEFAULT_LASER_PORT);

  // Serial rate
  this->transfer_rate = cf->ReadInt(section, "transfer_rate", DEFAULT_LASER_TRANSFER_RATE);
  this->current_rate = 0;

#ifdef HAVE_HI_SPEED_SERIAL
  this->can_do_hi_speed = true;
#else
  this->can_do_hi_speed = false;
#endif

  if (!this->can_do_hi_speed && this->transfer_rate > 38400)
  {
    PLAYER_ERROR1("sicklms200: requested hi speed serial, but no support compiled in. Defaulting to %d bps.",
		    DEFAULT_LASER_TRANSFER_RATE);
    this->connect_rate = DEFAULT_LASER_TRANSFER_RATE;
  }

  return;
}

SickS3000::~SickS3000()
{
  delete [] rx_buffer; 
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
int SickS3000::Setup()
{
  PLAYER_MSG1(2, "Laser initialising (%s)", this->device_name);
    
  // Open the terminal
  if (OpenTerm())
    return 1;

  if (ChangeTermSpeed(this->transfer_rate))
  {
    return 1;
  }

  PLAYER_MSG0(2, "laser ready");

  // Start the device thread
  StartThread();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int SickS3000::Shutdown()
{
  // shutdown laser device
  StopThread();

  CloseTerm();

  PLAYER_MSG0(2, "laser shutdown");
  
  return(0);
}


int 
SickS3000::ProcessMessage(QueuePointer &resp_queue, 
                           player_msghdr * hdr,
                           void * data)
{
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                 PLAYER_LASER_REQ_GET_CONFIG,
                                 this->device_addr))
  {
    this->Publish(this->device_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_LASER_REQ_GET_CONFIG,
                  (void*)&config_packet, sizeof(config_packet), NULL);
    return(0);
  }
  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                 PLAYER_LASER_REQ_GET_GEOM,
                                 this->device_addr))
  {
    player_laser_geom_t geom;
    memset(&geom, 0, sizeof(geom));
    geom.pose.px = pose[0];
    geom.pose.py = pose[1];
    geom.pose.pyaw = pose[2];
    geom.size.sl = size[0];
    geom.size.sw = size[1];

    this->Publish(this->device_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_LASER_REQ_GET_GEOM,
                  (void*)&geom, sizeof(geom), NULL);
    return(0);
  }

  // Don't know how to handle this message.
  return(-1);
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void SickS3000::Main() 
{
  for(;;)
  {
    // test if we are supposed to cancel
    pthread_testcancel();
    
    // process any pending messages
    ProcessMessages();
    
    // read data into our ring buffer and then process it
    int ret = ReadLaserData();
	if (ret < 0)
	{
		PLAYER_WARN("Error reading from S3000 device");
	}
	else if (ret > 0)
    	ProcessLaserData();
  }
}



////////////////////////////////////////////////////////////////////////////////
// Open the terminal
// Returns 0 on success
int SickS3000::OpenTerm()
{
  this->laser_fd = ::open(this->device_name, O_RDWR | O_SYNC , S_IRUSR | S_IWUSR );
  if (this->laser_fd < 0)
  {
    PLAYER_ERROR2("unable to open serial port [%s]; [%s]",
                  (char*) this->device_name, strerror(errno));
    return 1;
  }

  // set the serial port speed to 9600 to match the laser
  // later we can ramp the speed up to the SICK's 38K
  //
  struct termios term;
  if( tcgetattr( this->laser_fd, &term ) < 0 )
    RETURN_ERROR(1, "Unable to get serial port attributes");
  
  cfmakeraw( &term );
  cfsetispeed( &term, B9600 );
  cfsetospeed( &term, B9600 );
  
  if( tcsetattr( this->laser_fd, TCSAFLUSH, &term ) < 0 )
    RETURN_ERROR(1, "Unable to set serial port attributes");

  // Make sure queue is empty
  //
  tcflush(this->laser_fd, TCIOFLUSH);
    
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Close the terminal
// Returns 0 on success
//
int SickS3000::CloseTerm()
{
  ::close(this->laser_fd);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set the terminal speed
// Valid values are 9600 and 38400
// Returns 0 on success
//
int SickS3000::ChangeTermSpeed(int speed)
{
  struct termios term;

  current_rate = speed;

#ifdef HAVE_HI_SPEED_SERIAL
  struct serial_struct serial;

  // we should check and reset the AYSNC_SPD_CUST flag
  // since if it's set and we request 38400, we're likely
  // to get another baud rate instead (based on custom_divisor)
  // this way even if the previous player doesn't reset the
  // port correctly, we'll end up with the right speed we want
  if (ioctl(this->laser_fd, TIOCGSERIAL, &serial) < 0) 
  {
    //RETURN_ERROR(1, "error on TIOCGSERIAL in beginning");
    PLAYER_WARN("ioctl() failed while trying to get serial port info");
  }
  else
  {
    serial.flags &= ~ASYNC_SPD_CUST;
    serial.custom_divisor = 0;
    if (ioctl(this->laser_fd, TIOCSSERIAL, &serial) < 0) 
    {
      //RETURN_ERROR(1, "error on TIOCSSERIAL in beginning");
      PLAYER_WARN("ioctl() failed while trying to set serial port info");
    }
  }
#endif  

  //printf("LASER: change TERM speed: %d\n", speed);

  int term_speed;
  switch(speed)
  {
    case 9600:
		term_speed = B9600;
		break;
	case 19200:
		term_speed = B19200;
		break;
	case 38400:
		term_speed = B38400;
		break;
	default:
		term_speed = speed;
  }

  switch(term_speed)
  {
    case B9600:
    case B19200:
    case B38400:
      //PLAYER_MSG0(2, "terminal speed to 9600");
      if( tcgetattr( this->laser_fd, &term ) < 0 )
        RETURN_ERROR(1, "unable to get device attributes");
        
      cfmakeraw( &term );
	  cfsetispeed( &term, term_speed );
	  cfsetospeed( &term, term_speed );
        
      if( tcsetattr( this->laser_fd, TCSAFLUSH, &term ) < 0 )
        RETURN_ERROR(1, "unable to set device attributes");
      break;

    case 500000:
      //PLAYER_MSG0(2, "terminal speed to 500000");

#ifdef HAVE_HI_SPEED_SERIAL    
      if (ioctl(this->laser_fd, TIOCGSERIAL, &this->old_serial) < 0) {
        RETURN_ERROR(1, "error on TIOCGSERIAL ioctl");
      }
    
      serial = this->old_serial;
    
      serial.flags |= ASYNC_SPD_CUST;
      serial.custom_divisor = 48; // for FTDI USB/serial converter divisor is 240/5
    
      if (ioctl(this->laser_fd, TIOCSSERIAL, &serial) < 0) {
        RETURN_ERROR(1, "error on TIOCSSERIAL ioctl");
      }
    
#else
      fprintf(stderr, "sicklms200: Trying to change to 500kbps, but no support compiled in, defaulting to 38.4kbps.\n");
#endif

      // even if we are doing 500kbps, we have to set the speed to 38400...
      // the driver will know we want 500000 instead.

      if( tcgetattr( this->laser_fd, &term ) < 0 )
        RETURN_ERROR(1, "unable to get device attributes");    

      cfmakeraw( &term );
      cfsetispeed( &term, B38400 );
      cfsetospeed( &term, B38400 );
    
      if( tcsetattr( this->laser_fd, TCSAFLUSH, &term ) < 0 )
        RETURN_ERROR(1, "unable to set device attributes");
    
      break;
    default:
      PLAYER_ERROR1("unknown speed %d", speed);
  }
  return 0;
}


  



////////////////////////////////////////////////////////////////////////////////
// Read range data from laser
//
int SickS3000::ReadLaserData()
{
  if (rx_count == rx_buffer_size)
  {
    PLAYER_WARN("S3000 RX buffer full\n");
    return 0; 
  }
  // Read a packet from the laser
  //
  //int len = ReadFromLaser(&rx_buffer[rx_count], rx_buffer_size - rx_count);
  int len = read(this->laser_fd, &rx_buffer[rx_count], rx_buffer_size - rx_count);
  if (len == 0)
  {
    PLAYER_MSG0(2, "empty packet");
    return 0;
  }
  if (len < 0)
  {
    PLAYER_ERROR2("error reading form s3000: %d %s", errno, strerror(errno));
    return -1;
  }

  rx_count += len;
   
  return len;
}


int SickS3000::ProcessLaserData()
{
  while(rx_count >= 22)
  {
    // find our continuous data header
    unsigned int ii;
    bool found = false;
    for (ii = 0; ii < rx_count - 22; ++ii)
    {
      if (memcmp(&rx_buffer[ii],"\0\0\0\0\0\0",6) == 0)
      {
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
    // size includes all data from the data block number
    // through to the end of the packet including the checksum
    unsigned short size = 2*htons(*reinterpret_cast<unsigned short *> (&rx_buffer[6]));
    
    // check if we have enough data yet
    if (size > rx_count - 4)
      return 0;
      
    unsigned short packet_checksum = *reinterpret_cast<unsigned short *> (&rx_buffer[size+2]);
    unsigned short calc_checksum = CreateCRC(&rx_buffer[4], size-2);
    if (packet_checksum != calc_checksum)
    {
      PLAYER_WARN("Checksum's dont match, thats bad\n");
      memmove(rx_buffer, &rx_buffer[1], --rx_count);
      continue;
    }
    else
    {
      uint8_t * data = &rx_buffer[20];
      if (data[0] != data[1])
      {
        PLAYER_WARN("S3000: Bad type header bytes dont match\n");
      }
      else
      {
        if (data[0] == 0xAA)
        {
          PLAYER_WARN("We got a I/O data packet we dont know what to do with it\n");
        }
        else if (data[0] == 0xBB)
        {
          int data_count = (size - 22) / 2;
          data_packet.ranges_count = data_count;
          for (int ii = 0; ii < data_count; ++ii)
          {
            unsigned short Distance_CM = (*reinterpret_cast<unsigned short *> (&data[4 + 2*ii]));
            Distance_CM &= 0x1fff; // remove status bits
            double distance_m = static_cast<double>(Distance_CM)/100.0;
            data_packet.ranges[ii] = distance_m;
          }
          
          this->Publish(this->device_addr,
                        PLAYER_MSGTYPE_DATA,
                        PLAYER_LASER_DATA_SCAN,
                        (void*)&data_packet, sizeof(data_packet), NULL);
          
        }
        else if (data[0] == 0xCC)
        {
          PLAYER_WARN("We got a reflector data packet we dont know what to do with it\n");
        }
        else
        {
          PLAYER_WARN("We got an unknown packet\n");
        }
      }
    }
      
    memmove(rx_buffer, &rx_buffer[size+4], size+4);
    rx_count -= (size + 4);
    continue;
  }
  return 1;
}


static const unsigned short crc_table[256] = {
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
  0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
  0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
  0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
  0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
  0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
  0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
  0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
  0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
  0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
  0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
  0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
  0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
  0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
  0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
  0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
  0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
  0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
  0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
  0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
  0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
  0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
  0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
  0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
  0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
  0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
  0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};
           
unsigned short SickS3000::CreateCRC(uint8_t *Data, ssize_t length)
{
  unsigned short CRC_16 = 0xFFFF;
  unsigned short i;
  for (i = 0; i < length; i++)
  {
    CRC_16 = (CRC_16 << 8) ^ (crc_table[(CRC_16 >> 8) ^ (Data[i])]);
  }
  return CRC_16;
}
           
           
           
/*           
////////////////////////////////////////////////////////////////////////////////
// Create a CRC for the given packet
//
unsigned short SickS3000::CreateCRC(uint8_t* data, ssize_t len)
{
  uint16_t uCrc16;
  uint8_t abData[2];
  
  uCrc16 = 0;
  abData[0] = 0;
  
  while(len-- )
  {
    abData[1] = abData[0];
    abData[0] = *data++;
    
    if( uCrc16 & 0x8000 )
    {
      uCrc16 = (uCrc16 & 0x7fff) << 1;
      uCrc16 ^= CRC16_GEN_POL;
    }
    else
    {    
      uCrc16 <<= 1;
    }
    uCrc16 ^= MAKEUINT16(abData[0],abData[1]);
  }
  return (uCrc16); 
}
*/
