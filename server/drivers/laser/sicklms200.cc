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
 Desc: Driver for the SICK laser
 Author: Andrew Howard
 Date: 7 Nov 2000
 CVS: $Id$
*/

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_sicklms200 SickLMS200 driver

Driver for the SICK LMS200 scanning laser range-finder.

@par Interfaces
- @ref player_interface_laser

@par Supported configuration requests

- PLAYER_LASER_GET_GEOM
- PLAYER_LASER_GET_CONFIG
- PLAYER_LASER_SET_CONFIG
  
@par Configuration file options

- port "/dev/ttyS1"
  - Serial port to which laser is attached.  If you are using a
    USB/232 or USB/422 converter, this will be "/dev/ttyUSBx".

- rate 38400
  - Default baud rate.  Valid values are 38400 (RS232 or RS422) and
    500000 (RS422 only).
  
- delay 0
  - Delay (in seconds) before laser is initialized (set this to 35 if
    you have a newer generation Pioneer whose laser is switched on
    when the serial port is open).

- resolution 50
  - Angular resolution.  Valid values are:
    - resolution 50 : 0.5 degree increments, 361 readings @ 5Hz (38400) or 32Hz (500000).
    - resolution 100 : 1 degree increments, 181 readings @ 10Hz (38400) or 75Hz (500000).

- range_res 1
  - Range resolution.  Valid values are:
    - range_res 1 : 1mm precision, 8.192m max range.
    - range_res 10 : 10mm precision, 81.92m max range.
    - range_res 100 : 100mm precision, 819.2m max range.
      
@par Example 

@verbatim
driver
(
  driver sicklms200
  devices ["laser:0"]
  port "/dev/ttyS0"
  resolution 100   # Angular resolution 1 degree (181 readings @ 10Hz)
  range_res 10     # Range resolution 1 cm (maximum range 81.92m)
)
@endverbatim
*/
/** @} */
  

  
#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */
#include <sys/ioctl.h>
#include <playertime.h>

#undef HAVE_HI_SPEED_SERIAL
#ifdef HAVE_LINUX_SERIAL_H
  #ifndef DISABLE_HIGHSPEEDSICK
    #include <linux/serial.h>
    #define HAVE_HI_SPEED_SERIAL
  #endif
#endif

extern PlayerTime* GlobalTime;

#define PLAYER_ENABLE_MSG 1
#define PLAYER_ENABLE_TRACE 0

#include "playercommon.h"
#include "drivertable.h"
#include "driver.h"
#include "player.h"

#define DEFAULT_LASER_PORT "/dev/ttyS1"
#define DEFAULT_LASER_PORT_RATE 38400
#define DEFAULT_LASER_RETRIES 3


// The laser device class.
class SickLMS200 : public Driver
{
  public:
    
    // Constructor
    SickLMS200(ConfigFile* cf, int section);

    int Setup();
    int Shutdown();

  private:

    // Main function for device thread.
    virtual void Main();

    // Process configuration requests.  Returns 1 if the configuration
    // has changed.
    int UpdateConfig();

    // Compute the start and end scan segments based on the current resolution and
    // scan angles.  Returns 0 if the configuration is valid.
    int CheckScanConfig();
    
    // Open the terminal
    // Returns 0 on success
    int OpenTerm();

    // Close the terminal
    // Returns 0 on success
    int CloseTerm();
    
    // Set the terminal speed
    // Valid values are 9600 and 38400
    // Returns 0 on success
    int ChangeTermSpeed(int speed);

    // Get the laser type
    int GetLaserType(char *buffer, size_t bufflen);

    // Put the laser into configuration mode
    int SetLaserMode();

    // Set the laser data rate
    // Valid values are 9600 and 38400
    // Returns 0 on success
    int SetLaserSpeed(int speed);

    // Set the laser configuration
    // Returns 0 on success
    int SetLaserConfig(bool intensity);

    // Change the resolution of the laser
    int SetLaserRes(int angle, int res);
    
    // Request data from the laser
    // Returns 0 on success
    int RequestLaserData(int min_segment, int max_segment);

    // Read range data from laser
    int ReadLaserData(uint16_t *data, size_t datalen);

    // Write a packet to the laser
    ssize_t WriteToLaser(uint8_t *data, ssize_t len); 
    
    // Read a packet from the laser
    ssize_t ReadFromLaser(uint8_t *data, ssize_t maxlen, bool ack = false, int timeout = -1);

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

    // Starup delay
    int startup_delay;
  
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
    int port_rate;
    int current_rate;  

#ifdef HAVE_HI_SPEED_SERIAL
  struct serial_struct old_serial;
#endif
};

// a factory creation function
Driver* SickLMS200_Init(ConfigFile* cf, int section)
{
  return((Driver*)(new SickLMS200(cf, section)));
}

// a driver registration function
void SickLMS200_Register(DriverTable* table)
{
  table->AddDriver("sicklms200", SickLMS200_Init);
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
SickLMS200::SickLMS200(ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_LASER_CODE, PLAYER_READ_MODE,
             sizeof(player_laser_data_t),0,10,10)
{
  // Laser geometry.
  this->pose[0] = cf->ReadTupleLength(section, "pose", 0, 0.0);
  this->pose[1] = cf->ReadTupleLength(section, "pose", 1, 0.0);;
  this->pose[2] = cf->ReadTupleLength(section, "pose", 2, 0.0);;
  this->size[0] = 0.15;
  this->size[1] = 0.15;

  // Serial port
  this->device_name = cf->ReadString(section, "port", DEFAULT_LASER_PORT);

  // Serial rate
  this->port_rate = cf->ReadInt(section, "rate", DEFAULT_LASER_PORT_RATE);
  this->current_rate = this->port_rate;

#ifdef HAVE_HI_SPEED_SERIAL
  this->can_do_hi_speed = true;
#else
  this->can_do_hi_speed = false;
#endif

  if (!this->can_do_hi_speed && this->port_rate > 38400)
  {
    PLAYER_ERROR("sicklms200: requested hi speed serial, but no support compiled in. Defaulting to 38400 bps.");
    this->port_rate = 38400;
  }

  // Set default configuration
  this->startup_delay = cf->ReadInt(section, "delay", 0);
  this->scan_width = 180;
  this->scan_res = cf->ReadInt(section, "resolution", 50);
  this->min_angle = -9000;
  this->max_angle = +9000;
  this->scan_min_segment = 0;
  this->scan_max_segment = 360;
  this->intensity = true;
  this->range_res = cf->ReadInt(section, "range_res", 1);
  this->invert = cf->ReadInt(section, "invert", 0);

  if (this->CheckScanConfig() != 0)
    PLAYER_ERROR("invalid scan configuration");

  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
int SickLMS200::Setup()
{
  int rate = 0;
    
  PLAYER_MSG1("Laser initialising (%s)", this->device_name);
    
  // Open the terminal
  if (OpenTerm())
    return 1;

  // Some Pioneers only power laser after the terminal is opened; wait
  // for the laser to initialized
  sleep(this->startup_delay);
  
  // Try connecting at the given rate
  PLAYER_MSG1("connecting at %d", this->port_rate);
  if (ChangeTermSpeed(this->port_rate))
    return 1;
  if (SetLaserMode() == 0)
    rate = this->port_rate;
  else if (SetLaserMode() == 0)
    rate = this->port_rate;

  // Try connecting at 9600
  if (rate == 0 && this->port_rate != 9600)
  {
    PLAYER_MSG0("connecting at 9600");
    if (ChangeTermSpeed(9600))
      return 1;
    if (SetLaserMode() == 0)
      rate = 9600;
  }

  // Try connecting at 38400
  if (rate == 0 && this->port_rate != 38400)
  {
    PLAYER_MSG0("connecting at 38400");
    if (ChangeTermSpeed(38400))
      return 1;
    if (SetLaserMode() == 0)
      rate = 38400;
  }
  
  // Try connecting at 500000
  if (rate == 0 && this->port_rate != 500000 && this->can_do_hi_speed)
  {
    PLAYER_MSG0("connecting at 500000");
    if (ChangeTermSpeed(500000))
      return 1;
    if (SetLaserMode() == 0)
      rate = 500000;
  }

  // Could not find the laser
  if (rate == 0)
  {
    PLAYER_ERROR("unable to connect to laser");
    return 1;
  }

  // Jump up to 38400
  if (rate != 38400 && this->port_rate == 38400)
  {
    PLAYER_MSG2("laser operating at %d; changing to %d", rate, this->port_rate);
    if (SetLaserSpeed(38400))
      return 1;
    sleep(1);
    if (ChangeTermSpeed(38400))
      return 1;
    sleep(1);
  }

  // Jump up to 500000
  else if (rate != 500000 && this->port_rate == 500000 && this->can_do_hi_speed)
  {
    PLAYER_MSG2("laser operating at %d; changing to %d", rate, this->port_rate);
    if (SetLaserSpeed(this->port_rate))
      return 1;
    sleep(1);
    if (ChangeTermSpeed(this->port_rate))
      return 1;
    sleep(1);
  }

  // Dont know this rate
  else if (rate != this->port_rate)
  {
    PLAYER_ERROR1("unsupported rate %d", this->port_rate);
    return 1;
  }

  // Display the laser type
  char type[64];
  if (GetLaserType(type, sizeof(type)))
    return 1;
  PLAYER_MSG3("SICK laser type [%s] at [%s:%d]", type, this->device_name, this->port_rate);

  // Configure the laser
  if (SetLaserRes(this->scan_width, this->scan_res))
    return 1;
  if (SetLaserConfig(this->intensity))
    return 1;

  PLAYER_MSG0("laser ready");

  // Start the device thread
  StartThread();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int SickLMS200::Shutdown()
{
  // shutdown laser device
  StopThread();
  
  CloseTerm();

  PLAYER_MSG0("laser shutdown");
  
  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void SickLMS200::Main() 
{
  uint16_t tmp;
  bool first = true;
  
  // Ask the laser to send data
  if (RequestLaserData(this->scan_min_segment, this->scan_max_segment) != 0)
  {
    PLAYER_ERROR("laser not responding; exiting laser thread");
    return;
  }

  while (true)
  {
    // test if we are supposed to cancel
    pthread_testcancel();

    // Update the configuration.
    if (UpdateConfig())
    {
      if (SetLaserMode() != 0)
        PLAYER_ERROR("request for config mode failed");
      else
      {
        if (SetLaserRes(this->scan_width, this->scan_res) != 0)
          PLAYER_ERROR("failed setting resolution");
        if (SetLaserConfig(this->intensity) != 0)
          PLAYER_ERROR("failed setting intensity");          
      }

      // Issue a new request for data
      if (RequestLaserData(this->scan_min_segment, this->scan_max_segment))
        PLAYER_ERROR("request for laser data failed");
    }

    // Get the time at which we started reading
    // This will be a pretty good estimate of when the phenomena occured
    struct timeval time;
    GlobalTime->GetTime(&time);
    
    // Process incoming data
    player_laser_data_t data;
    if (ReadLaserData(data.ranges, sizeof(data.ranges) / sizeof(data.ranges[0])) == 0)
    {
      if (first)
      {
        PLAYER_MSG0("receiving data");
        first = false;
      }
      
      // Prepare packet and byte swap
      data.min_angle = htons(this->scan_min_segment * this->scan_res - this->scan_width * 50);
      data.max_angle = htons(this->scan_max_segment * this->scan_res - this->scan_width * 50);
      data.resolution = htons(this->scan_res);
      data.range_count = htons(this->scan_max_segment - this->scan_min_segment + 1);
      data.range_res = htons((uint16_t) this->range_res);
      for (int i = 0; i < this->scan_max_segment - this->scan_min_segment + 1; i++)
      {
        data.intensity[i] = ((data.ranges[i] >> 13) & 0x000E);
        data.ranges[i] = htons((uint16_t) (data.ranges[i] & 0x1FFF));
      }

      // if the laser is upside-down, reverse the data and intensity
      // arrays, in place.  this could probably be made more efficient by
      // burying it in a lower-level loop where the data is being read, but
      // i can't be bothered to figure out where.
      if(this->invert)
      {
        for (int i = 0; 
             i < (this->scan_max_segment - this->scan_min_segment + 1)/2; 
             i++)
        {
          tmp=data.ranges[i];
          data.ranges[i]=data.ranges[this->scan_max_segment-this->scan_min_segment-i];
          data.ranges[this->scan_max_segment-this->scan_min_segment-i] = tmp;
          tmp=data.intensity[i];
          data.intensity[i]=data.intensity[this->scan_max_segment-this->scan_min_segment-i];
          data.intensity[this->scan_max_segment-this->scan_min_segment-i] = tmp;
        }
      }
      
      // Make data available
      PutData((uint8_t*) &data, sizeof(data), &time);
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// Process configuration requests.  Returns 1 if the configuration has changed.
int SickLMS200::UpdateConfig()
{
  int len;
  void *client;
  char buffer[PLAYER_MAX_REQREP_SIZE];
  player_laser_config_t config;
  player_laser_geom_t geom;
  
  while ((len = GetConfig(&client, &buffer, sizeof(buffer),NULL)) > 0)
  {
    switch (buffer[0])
    {
      case PLAYER_LASER_SET_CONFIG:
      {
        if (len != sizeof(player_laser_config_t))
        {
          PLAYER_ERROR2("config request len is invalid (%d != %d)", len, sizeof(config));
          if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
            PLAYER_ERROR("PutReply() failed");
          continue;
        }

        memcpy(&config, buffer, sizeof(config));
        this->intensity = config.intensity;
        this->scan_res = ntohs(config.resolution);
        this->min_angle = (short) ntohs(config.min_angle);
        this->max_angle = (short) ntohs(config.max_angle);
	this->range_res = ntohs(config.range_res);

        if (this->CheckScanConfig() == 0)
        {
          if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, 
                      &config, sizeof(config), NULL) != 0)
            PLAYER_ERROR("PutReply() failed");
          return 1;
        }
        else
        {
          if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
            PLAYER_ERROR("PutReply() failed");
        }
        break;
      }

      case PLAYER_LASER_GET_CONFIG:
      {
        if (len != 1)
        {
          PLAYER_ERROR2("config request len is invalid (%d != %d)", len, 1);
          if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
            PLAYER_ERROR("PutReply() failed");
          continue;
        }

        config.intensity = this->intensity;
        config.resolution = htons(this->scan_res);
        config.min_angle = htons((short) this->min_angle);
        config.max_angle = htons((short) this->max_angle);
        config.range_res = htons(this->range_res);

        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, 
                    &config, sizeof(config), NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }

      case PLAYER_LASER_GET_GEOM:
      {
        if (len != 1)
        {
          PLAYER_ERROR2("config request len is invalid (%d != %d)", len, 1);
          if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
            PLAYER_ERROR("PutReply() failed");
          continue;
        }

        geom.pose[0] = htons((short) (this->pose[0] * 1000));
        geom.pose[1] = htons((short) (this->pose[1] * 1000));
        geom.pose[2] = htons((short) (this->pose[2] * 180/M_PI));
        geom.size[0] = htons((short) (this->size[0] * 1000));
        geom.size[1] = htons((short) (this->size[1] * 1000));
        
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, 
                    &geom, sizeof(geom), NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }

      default:
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Compute the start and end scan segments based on the current resolution and
// scan angles.  Returns 0 if the configuration is valid.
int SickLMS200::CheckScanConfig()
{
  if (this->scan_res == 25)
  {
    // For high res, drop the scan range down to 100 degrees.
    // The angles must be interpreted differently too.
    this->scan_width = 100;
    this->scan_min_segment = (this->min_angle + 5000) / this->scan_res;
    this->scan_max_segment = (this->max_angle + 5000) / this->scan_res;

    if (this->scan_min_segment < 0)
      this->scan_min_segment = 0;
    if (this->scan_min_segment > 400)
      this->scan_min_segment = 400;
    
    if (this->scan_max_segment < 0)
      this->scan_max_segment = 0;
    if (this->scan_max_segment > 400)
      this->scan_max_segment = 400;

    return 0;
  }
  else if (this->scan_res == 50 || this->scan_res == 100)
  {
    this->scan_width = 180;
    this->scan_min_segment = (this->min_angle + 9000) / this->scan_res;
    this->scan_max_segment = (this->max_angle + 9000) / this->scan_res;
    
    if (this->scan_min_segment < 0)
      this->scan_min_segment = 0;
    if (this->scan_min_segment > 360)
      this->scan_min_segment = 360;

    if (this->scan_max_segment < 0)
      this->scan_max_segment = 0;
    if (this->scan_max_segment > 360)
      this->scan_max_segment = 360;

    return 0;
  }

  if (!(this->range_res == 1 || this->range_res == 10 || this->range_res == 100))
    return -1;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Open the terminal
// Returns 0 on success
int SickLMS200::OpenTerm()
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
  
#if HAVE_CFMAKERAW
  cfmakeraw( &term );
#endif
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
int SickLMS200::CloseTerm()
{
#ifdef HAVE_HI_SPEED_SERIAL
  if (ioctl(this->laser_fd, TIOCSSERIAL, &this->old_serial) < 0) {
    //RETURN_ERROR(1, "error trying to reset serial to old state");
    PLAYER_WARN("ioctl() failed while trying to reset serial to old state");
  }
#endif

  ::close(this->laser_fd);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set the terminal speed
// Valid values are 9600 and 38400
// Returns 0 on success
//
int SickLMS200::ChangeTermSpeed(int speed)
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

  switch(speed)
  {
    case 9600:
      //PLAYER_MSG0("terminal speed to 9600");
      if( tcgetattr( this->laser_fd, &term ) < 0 )
        RETURN_ERROR(1, "unable to get device attributes");
        
#if HAVE_CFMAKERAW
      cfmakeraw( &term );
#endif
      cfsetispeed( &term, B9600 );
      cfsetospeed( &term, B9600 );
        
      if( tcsetattr( this->laser_fd, TCSAFLUSH, &term ) < 0 )
        RETURN_ERROR(1, "unable to set device attributes");
      break;

    case 38400:
      //PLAYER_MSG0("terminal speed to 38400");
      if( tcgetattr( this->laser_fd, &term ) < 0 )
        RETURN_ERROR(1, "unable to get device attributes");
        
#if HAVE_CFMAKERAW
      cfmakeraw( &term );
#endif
      cfsetispeed( &term, B38400 );
      cfsetospeed( &term, B38400 );
        
      if( tcsetattr( this->laser_fd, TCSAFLUSH, &term ) < 0 )
        RETURN_ERROR(1, "unable to set device attributes");
      break;

    case 500000:
      //PLAYER_MSG0("terminal speed to 500000");

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

#if HAVE_CFMAKERAW
      cfmakeraw( &term );
#endif
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
// Put the laser into configuration mode
//
int SickLMS200::SetLaserMode()
{
  int tries;
  ssize_t len;
  uint8_t packet[20];

  for (tries = 0; tries < DEFAULT_LASER_RETRIES; tries++)
  {
    packet[0] = 0x20; /* mode change command */
    packet[1] = 0x00; /* configuration mode */
    packet[2] = 0x53; // S - the password 
    packet[3] = 0x49; // I
    packet[4] = 0x43; // C
    packet[5] = 0x4B; // K
    packet[6] = 0x5F; // _
    packet[7] = 0x4C; // L
    packet[8] = 0x4D; // M
    packet[9] = 0x53; // S
    len = 10;
  
    PLAYER_TRACE0("sending configuration mode request to laser");
    if (WriteToLaser(packet, len) < 0)
      return 1;

    // Wait for laser to return ack
    // This could take a while...
    //
    PLAYER_TRACE0("waiting for acknowledge");
    len = ReadFromLaser(packet, sizeof(packet), true, 1000);
    if (len < 0)
      return 1;
    else if (len < 1)
    {
      PLAYER_WARN("timeout");
      continue;
    }
    else if (packet[0] == NACK)
    {
      PLAYER_ERROR("request denied by laser");
      return 1;
    }
    else if (packet[0] != ACK)
    {
      PLAYER_ERROR("unexpected packet type");
      return 1;
    }
    break;
  }
  return (tries >= DEFAULT_LASER_RETRIES);
}


////////////////////////////////////////////////////////////////////////////////
// Set the laser data rate
// Valid values are 9600 and 38400
// Returns 0 on success
//
int SickLMS200::SetLaserSpeed(int speed)
{
  int tries;
  ssize_t len;
  uint8_t packet[20];

  for (tries = 0; tries < DEFAULT_LASER_RETRIES; tries++)
  {
    packet[0] = 0x20;
    packet[1] = (speed == 9600 ? 0x42 : (speed == 38400 ? 0x40 : 0x48));
    len = 2;

    //PLAYER_TRACE0("sending baud rate request to laser");
    if (WriteToLaser(packet, len) < 0)
      return 1;
            
    // Wait for laser to return ack
    //PLAYER_TRACE0("waiting for acknowledge");
    len = ReadFromLaser(packet, sizeof(packet), true, 2000);
    if (len < 0)
      return 1;
    else if (len < 1)
    {
      PLAYER_ERROR("no reply from laser");
      return 1;
    }
    else if (packet[0] == NACK)
    {
      PLAYER_ERROR("request denied by laser");
      return 1;
    }
    else if (packet[0] != ACK)
    {
      PLAYER_ERROR("unexpected packet type");
      return 1;      
    }
    break;
  }
  return (tries >= DEFAULT_LASER_RETRIES);
}


////////////////////////////////////////////////////////////////////////////////
// Get the laser type
//
int SickLMS200::GetLaserType(char *buffer, size_t bufflen)
{
  int tries;
  ssize_t len;
  uint8_t packet[512];

  for (tries = 0; tries < DEFAULT_LASER_RETRIES; tries++)
  {
    packet[0] = 0x3A;
    len = 1;

    //PLAYER_TRACE0("sending get type request to laser");
    if (WriteToLaser(packet, len) < 0)
      return 1;

    // Wait for laser to return data
    len = ReadFromLaser(packet, sizeof(packet), false, 1000);
    if (len < 0)
      return 1;
    else if (len < 1)
    {
      PLAYER_WARN("timeout");
      continue;
    }
    else if (packet[0] == NACK)
    {
      PLAYER_ERROR("request denied by laser");
      return 1;
    }
    else if (packet[0] != 0xBA)
    {
      PLAYER_ERROR("unexpected packet type");
      return 1;
    }

    // NULL terminate the return string
    assert((size_t) len + 1 < sizeof(packet));
    packet[len + 1] = 0;

    // Copy to buffer
    assert(bufflen >= (size_t) len - 1);
    strcpy(buffer, (char*) (packet + 1));

    break;
  }

  return (tries >= DEFAULT_LASER_RETRIES);
}


////////////////////////////////////////////////////////////////////////////////
// Set the laser configuration
// Returns 0 on success
//
int SickLMS200::SetLaserConfig(bool intensity)
{
  int tries;
  ssize_t len;
  uint8_t packet[512];

  for (tries = 0; tries < DEFAULT_LASER_RETRIES; tries++)
  {
    packet[0] = 0x74;
    len = 1;

    //PLAYER_TRACE0("sending get configuration request to laser");
    if (WriteToLaser(packet, len) < 0)
      return 1;

    // Wait for laser to return data
    //PLAYER_TRACE0("waiting for reply");
    len = ReadFromLaser(packet, sizeof(packet), false, 1000);
    if (len < 0)
      return 1;
    else if (len < 1)
    {
      PLAYER_WARN("timeout");
      continue;
    }
    else if (packet[0] == NACK)
    {
      PLAYER_ERROR("request denied by laser");
      return 1;
    }
    else if (packet[0] != 0xF4)
    {
      PLAYER_ERROR("unexpected packet type");
      return 1;
    }
    break;
  }
  if (tries >= DEFAULT_LASER_RETRIES)
    return 1;
  
  //PLAYER_TRACE0("get configuration request ok");
  //PLAYER_TRACE1("laser units [%d]", (int) packet[7]);

  for (tries = 0; tries < DEFAULT_LASER_RETRIES; tries++)
  {
    // Modify the configuration and send it back
    packet[0] = 0x77;

    // Return intensity in top 3 data bits
    packet[6] = (intensity ? 0x01 : 0x00); 

    // Set the units for the range reading
    if (this->range_res == 1)
      packet[7] = 0x01;
    else if (this->range_res == 10)
      packet[7] = 0x00;
    else if (this->range_res == 100)
      packet[7] = 0x02;
    else
      packet[7] = 0x01;

    //PLAYER_TRACE0("sending set configuration request to laser");
    if (WriteToLaser(packet, len) < 0)
      return 1;

    // Wait for the change to "take"
    //PLAYER_TRACE0("waiting for acknowledge");
    len = ReadFromLaser(packet, sizeof(packet), false, 1000);
    if (len < 0)
      return 1;
    else if (len < 1)
    {
      PLAYER_ERROR("timeout");
      continue;
    }
    else if (packet[0] == NACK)
    {
      PLAYER_ERROR("request denied by laser");
      return 1;
    }
    else if (packet[0] != 0xF7)
    {
      PLAYER_ERROR("unexpected packet type");
      return 1;
    }

    //PLAYER_TRACE0("set configuration request ok");
    break;
  }

  return (tries >= DEFAULT_LASER_RETRIES);
}


////////////////////////////////////////////////////////////////////////////////
// Change the resolution of the laser
// Valid widths are: 100, 180 (degrees)
// Valid resolitions are: 25, 50, 100 (1/100 degree)
//
int SickLMS200::SetLaserRes(int width, int res)
{
  int tries;
  ssize_t len;
  uint8_t packet[512];

  for (tries = 0; tries < DEFAULT_LASER_RETRIES; tries++)
  {
    len = 0;
    packet[len++] = 0x3B;
    packet[len++] = (width & 0xFF);
    packet[len++] = (width >> 8);
    packet[len++] = (res & 0xFF);
    packet[len++] = (res >> 8);

    //PLAYER_TRACE0("sending set variant request to laser");
    if (WriteToLaser(packet, len) < 0)
      return 1;

    // Wait for laser to return data
    //PLAYER_TRACE0("waiting for reply");
    len = ReadFromLaser(packet, sizeof(packet), false, 1000);
    if (len < 0)
      return 1;
    else if (len < 1)
    {
      PLAYER_WARN("timeout");
      continue;
    }
    else if (packet[0] == NACK)
    {
      PLAYER_ERROR("request denied by laser");
      return 1;
    }
    else if (packet[0] != 0xBB)
    {
      PLAYER_ERROR("unexpected packet type");
      return 1;
    }

    // See if the request was accepted
    if (packet[1] == 0)
    {
      PLAYER_ERROR("variant request ignored");
      return 1;
    }
    
    break;
  }

  return (tries >= DEFAULT_LASER_RETRIES);
}


////////////////////////////////////////////////////////////////////////////////
// Request data from the laser
// Returns 0 on success
//
int SickLMS200::RequestLaserData(int min_segment, int max_segment)
{
  int tries;
  ssize_t len;
  uint8_t packet[20];

  for (tries = 0; tries < DEFAULT_LASER_RETRIES; tries++)
  {
    len = 0;
    packet[len++] = 0x20; /* mode change command */
    
    if (min_segment == 0 && max_segment == 360)
    {
      // Use this for raw scan data...
      //
      packet[len++] = 0x24;
    }
    else
    {        
      // Or use this for selected scan data...
      //
      int first = min_segment + 1;
      int last = max_segment + 1;
      packet[len++] = 0x27;
      packet[len++] = (first & 0xFF);
      packet[len++] = (first >> 8);
      packet[len++] = (last & 0xFF);
      packet[len++] = (last >> 8);
    }

    //PLAYER_TRACE0("sending scan data request to laser");
    //printf("LASER: RLD: writing scan data\n");
    if (WriteToLaser(packet, len) < 0)
      return 1;

    // Wait for laser to return ack
    // This should be fairly prompt
    len = ReadFromLaser(packet, sizeof(packet), true, 1000);
    if (len < 0)
      return 1;
    else if (len < 1)
    {
      PLAYER_WARN("timeout");
      continue;
    }
    else if (packet[0] == NACK)
    {
      PLAYER_ERROR("request denied by laser");
      return 1;
    }
    else if (packet[0] != ACK)
    {
      PLAYER_ERROR("unexpected packet type");;
      return 1;
    }

    break;
  }

  return (tries >= DEFAULT_LASER_RETRIES);
}


////////////////////////////////////////////////////////////////////////////////
// Read range data from laser
//
int SickLMS200::ReadLaserData(uint16_t *data, size_t datalen)
{
  uint8_t raw_data[1024];

  // Read a packet from the laser
  //
  int len = ReadFromLaser(raw_data, sizeof(raw_data));
  if (len == 0)
  {
    PLAYER_TRACE0("empty packet");
    return 1;
  }

  // Process raw packets
  //
  if (raw_data[0] == 0xB0)
  {
    // Determine the number of values returned
    //
    //int units = raw_data[2] >> 6;
    int count = (int) raw_data[1] | ((int) (raw_data[2] & 0x3F) << 8);
    assert((size_t) count <= datalen);

    // Strip the status info and shift everything down a few bytes
    // to remove packet header.
    //
    for (int i = 0; i < count; i++)
    {
      int src = 2 * i + 3;
      data[i] = raw_data[src + 0] | (raw_data[src + 1] << 8);
    }
  }
  else if (raw_data[0] == 0xB7)
  {
    // Determine which values were returned
    //
    //int first = ((int) raw_data[1] | ((int) raw_data[2] << 8)) - 1;
    //int last =  ((int) raw_data[3] | ((int) raw_data[4] << 8)) - 1;
        
    // Determine the number of values returned
    //
    //int units = raw_data[6] >> 6;
    int count = (int) raw_data[5] | ((int) (raw_data[6] & 0x3F) << 8);
    assert((size_t) count <= datalen);

    // Strip the status info and shift everything down a few bytes
    // to remove packet header.
    //
    for (int i = 0; i < count; i++)
    {
      int src = 2 * i + 7;
      data[i] = raw_data[src + 0] | (raw_data[src + 1] << 8);
    }
  }
  else
    RETURN_ERROR(1, "unexpected packet type");
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Write a packet to the laser
//
ssize_t SickLMS200::WriteToLaser(uint8_t *data, ssize_t len)
{
  uint8_t buffer[4 + 1024 + 2];
  assert(4 + len + 2 < (ssize_t) sizeof(buffer));

  // Create header
  //
  buffer[0] = STX;
  buffer[1] = 0;
  buffer[2] = LOBYTE(len);
  buffer[3] = HIBYTE(len);

  // Copy body
  //
  memcpy(buffer + 4, data, len);

  // Create footer (CRC)
  //
  uint16_t crc = CreateCRC(buffer, 4 + len);
  buffer[4 + len + 0] = LOBYTE(crc);
  buffer[4 + len + 1] = HIBYTE(crc);

  // Make sure both input and output queues are empty
  //
  tcflush(this->laser_fd, TCIOFLUSH);

  ssize_t bytes = 0;
  struct timeval start, end;

#ifdef HAVE_HI_SPEED_SERIAL
  // have to write one char at a time, because if we're
  // high speed, then must take no longer than 55 us between
  // chars

  int ret;
  if (current_rate > 38400) {
    //printf("LASER: writing %d bytes\n", 6+len);
    for (int i =0; i < 6 + len; i++) {
      do {
        gettimeofday(&start, NULL);
        ret = ::write(this->laser_fd, buffer + i, 1);
      } while (!ret);
      if (ret > 0) {
        bytes += ret;
      }

      // need to do this sort of busy wait to ensure the right timing
      // although I've noticed you will get some anamolies that are
      // in the ms range; this could be a problem...
      int usecs; 
      do {
        gettimeofday(&end, NULL);
        usecs= (end.tv_sec - start.tv_sec)*1000000 +
          (end.tv_usec - start.tv_usec);
      } while (usecs < 60);

      //printf("usecs: %d bytes=%02X\n", (end.tv_sec - start.tv_sec)*1000000 +
      //     (end.tv_usec - start.tv_usec), *(buffer + i));
      
    }
  } else {
    bytes = ::write( this->laser_fd, buffer, 4 + len + 2);
  }
#else
    
  // Write the data to the port
  //
  bytes = ::write( this->laser_fd, buffer, 4 + len + 2);
#endif
  // Make sure the queue is drained
  // Synchronous IO doesnt always work
  //
  ::tcdrain(this->laser_fd);
    
  // Return the actual number of bytes sent, including header and footer
  //
  return bytes;
}


////////////////////////////////////////////////////////////////////////////////
// Read a packet from the laser
// Set ack to true to ignore all packets except ack and nack
// Set timeout to -1 to make this blocking, otherwise it will return in timeout ms.
// Returns the packet length (0 if timeout occurs)
//
ssize_t SickLMS200::ReadFromLaser(uint8_t *data, ssize_t maxlen, bool ack, int timeout)
{
  // If the timeout is infinite,
  // go to blocking io
  //
  if (timeout < 0)
  {
    PLAYER_TRACE0("using blocking io");
    int flags = ::fcntl(this->laser_fd, F_GETFL);
    if (flags < 0)
    {
      PLAYER_ERROR("unable to get device flags");
      return 0;
    }
    if (::fcntl(this->laser_fd, F_SETFL, flags & (~O_NONBLOCK)) < 0)
    {
      PLAYER_ERROR("unable to set device flags");
      return 0;
    }
  }
  //
  // Otherwise, use non-blocking io
  //
  else
  {
    PLAYER_TRACE0("using non-blocking io");
    int flags = ::fcntl(this->laser_fd, F_GETFL);
    if (flags < 0)
    {
      PLAYER_ERROR("unable to get device flags");
      return 0;
    }
    if (::fcntl(this->laser_fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      PLAYER_ERROR("unable to set device flags");
      return 0;
    }
  }

  int64_t start_time = GetTime();
  int64_t stop_time = start_time + timeout;

  PLAYER_TRACE2("%Ld %Ld", start_time, stop_time);

  int bytes = 0;
  uint8_t header[5] = {0};
  uint8_t footer[3];
    
  // Read until we get a valid header
  // or we timeout
  //
  while (true)
  {
    if (timeout >= 0)
      usleep(1000);

    //printf("reading %d\n", timeout); fflush(stdout);
    bytes = ::read(this->laser_fd, header + sizeof(header) - 1, 1);
    //printf("bytes read %d\n", bytes); fflush(stdout);
    
    if (header[0] == STX && header[1] == 0x80)
    {
      if (!ack)
        break;
      if (header[4] == ACK || header[4] == NACK)
        break;
    }
    memmove(header, header + 1, sizeof(header) - 1);
    if (timeout >= 0 && GetTime() >= stop_time)
    {
      PLAYER_TRACE2("%Ld %Ld", GetTime(), stop_time);
      PLAYER_TRACE0("timeout on read (1)");
      return 0;
    }
  }

  // Determine data length
  // Includes status, but not CRC, so subtract status to get data packet length.
  //
  ssize_t len = ((int) header[2] | ((int) header[3] << 8)) - 1;
    
  // Check for buffer overflows
  //
  if (len > maxlen)
    RETURN_ERROR(0, "buffer overflow (len > max_len)");

  // Read in the data
  // Note that we smooge the packet type from the header
  // onto the front of the data buffer.
  //
  bytes = 0;
  data[bytes++] = header[4];
  while (bytes < len)
  {
    if (timeout >= 0)
      usleep(1000);
    bytes += ::read(this->laser_fd, data + bytes, len - bytes);
    if (timeout >= 0 && GetTime() >= stop_time)
    {
      PLAYER_TRACE2("%Ld %Ld", GetTime(), stop_time);
      RETURN_ERROR(0, "timeout on read (3)");
    }
  }

  // Read in footer
  //
  bytes = 0;
  while (bytes < 3)
  {
    if (timeout >= 0)
      usleep(1000);
    bytes += ::read(this->laser_fd, footer + bytes, 3 - bytes);
    if (timeout >= 0 && GetTime() >= stop_time)
    {
      PLAYER_TRACE2("%Ld %Ld", GetTime(), stop_time);
      RETURN_ERROR(0, "timeout on read (4)");
    }
  }
    
  // Construct entire packet
  // And check the CRC
  //
  uint8_t buffer[4 + 1024 + 1];
  assert(4 + len + 1 < (ssize_t) sizeof(buffer));
  memcpy(buffer, header, 4);
  memcpy(buffer + 4, data, len);
  memcpy(buffer + 4 + len, footer, 1);
  uint16_t crc = CreateCRC(buffer, 4 + len + 1);
  if (crc != MAKEUINT16(footer[1], footer[2]))
    RETURN_ERROR(0, "CRC error, ignoring packet");
    
  return len;
}

           
////////////////////////////////////////////////////////////////////////////////
// Create a CRC for the given packet
//
unsigned short SickLMS200::CreateCRC(uint8_t* data, ssize_t len)
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


////////////////////////////////////////////////////////////////////////////////
// Get the time (in ms)
//
int64_t SickLMS200::GetTime()
{
  struct timeval tv;
  //gettimeofday(&tv, NULL);
  GlobalTime->GetTime(&tv);
  return (int64_t) tv.tv_sec * 1000 + (int64_t) tv.tv_usec / 1000;
}


