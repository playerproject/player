/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003  Brian Gerkey gerkey@usc.edu 
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
 * Device driver for the Garmin geko 201 handheld GPS unit.  Interacts with
 * the unit by speaking NMEA over a serial line.  As such, this driver may
 * work with other Garmin units, and (likely with some modification) other
 * NMEA-compliant GPS units.
 *
 */


#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>  /* for htons(3) */
#include <errno.h>

#include <device.h>
#include <drivertable.h>
#include <player.h>

#define DEFAULT_GPS_PORT "/dev/ttyS0"

// time that we'll wait to get the first round of data from the unit.
// currently 1s
#define GPS_STARTUP_CYCLE_USEC 100000
#define GPS_STARTUP_CYCLES 10

// these are the standard NMEA sentences that come 
// out of the geko 201
#define NMEA_GPRMB "GPRMB"
#define NMEA_GPRMC "GPRMC"
#define NMEA_GPGGA "GPGGA"
#define NMEA_GPGSA "GPGSA"
#define NMEA_GPGSV "GPGSV"
#define NMEA_GPGLL "GPGLL"
#define NMEA_GPBOD "GPBOD"
#define NMEA_GPRTE "GPRTE"

// these are the proprietary NMEA sentences that 
// come out of the geko 201
#define NMEA_PGRME "PGRME"
#define NMEA_PGRMZ "PGRMZ"
#define NMEA_PSLIB "PSLIB"

// spec says 82; we'll add 1 for a terminating \0
#define NMEA_MAX_SENTENCE_LEN 83

#define NMEA_START_CHAR '$'
#define NMEA_END_CHAR '\n'
#define NMEA_CHKSUM_CHAR '*'

class GarminNMEA:public CDevice 
{
  private:
    int gps_fd;  // file descriptor for the unit
    const char* gps_serial_port; // string name of serial port to use

    char nmea_buf[NMEA_MAX_SENTENCE_LEN];
    size_t nmea_buf_len;

    bool gps_fd_blocking;

    int FillBuffer();
    int ReadSentence(char* buf, size_t len);
    int ParseSentence(const char* buf);
    char* GetNextField(char* field, size_t len, const char* ptr);

  public:
    GarminNMEA(char* interface, ConfigFile* cf, int section);

    virtual int Setup();
    virtual int Shutdown();
    virtual void Main();
};

// initialization function
CDevice* GarminNMEA_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_GPS_STRING))
  {
    PLAYER_ERROR1("driver \"garminnmea\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new GarminNMEA(interface, cf, section)));
}

// a driver registration function
void 
GarminNMEA_Register(DriverTable* table)
{
  table->AddDriver("garminnmea", PLAYER_ALL_MODE, GarminNMEA_Init);
}

GarminNMEA::GarminNMEA(char* interface, ConfigFile* cf, int section) :
  CDevice(sizeof(player_gps_data_t),0,0,0)
{
  gps_fd = -1;
  player_gps_data_t data;

  memset(&data,0,sizeof(data));

  gps_serial_port = cf->ReadString(section, "port", DEFAULT_GPS_PORT);
}

int
GarminNMEA::Setup()
{
  struct termios term;
  int flags;

  printf("GPS connection initializing (%s)...", gps_serial_port);
  fflush(stdout);

  // open it.  non-blocking at first, in case there's no gps unit.
  if((gps_fd = open(gps_serial_port, 
                    O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0)
  {
    PLAYER_ERROR1("open(): %s\n", strerror(errno));
    return(-1);
  }  

  gps_fd_blocking = false;
 
  if(tcflush(gps_fd, TCIFLUSH ) < 0 )
  {
    PLAYER_ERROR1("tcflush(): %s\n", strerror(errno));
    close(gps_fd);
    gps_fd = -1;
    return(-1);
  }
  if(tcgetattr(gps_fd, &term) < 0 )
  {
    PLAYER_ERROR1("tcgetattr(): %s\n", strerror(errno));
    close(gps_fd);
    gps_fd = -1;
    return(-1);
  }
  
#if HAVE_CFMAKERAW
  cfmakeraw(&term);
#endif
  cfsetispeed(&term, B4800);
  cfsetospeed(&term, B4800);
  
  if(tcsetattr(gps_fd, TCSAFLUSH, &term) < 0 )
  {
    PLAYER_ERROR1("tcsetattr(): %s\n", strerror(errno));
    close(gps_fd);
    gps_fd = -1;
    return(-1);
  }

  memset(nmea_buf,0,sizeof(nmea_buf));
  nmea_buf_len=0;
  /* try to read some data, just to make sure we actually have a gps unit */
  if(FillBuffer())
  {
    PLAYER_ERROR1("Couldn't connect to GPS unit, most likely because the \n"
                  "unit is not connected or is connected not to %s\n", 
                    gps_serial_port);
    close(gps_fd);
    gps_fd = -1;
    return(-1);
  }

  /* ok, we got data, so now set NONBLOCK, and continue */
  if((flags = fcntl(gps_fd, F_GETFL)) < 0)
  {
    PLAYER_ERROR1("fcntl(): %s\n", strerror(errno));
    close(gps_fd);
    gps_fd = -1;
    return(1);
  }
  if(fcntl(gps_fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
  {
    PLAYER_ERROR1("fcntl(): %s\n", strerror(errno));
    close(gps_fd);
    gps_fd = -1;
    return(1);
  }

  gps_fd_blocking = true;


  memset(nmea_buf,0,sizeof(nmea_buf));
  nmea_buf_len=0;

  puts("Done.");
  
  // start the thread to talk with the camera
  StartThread();

  return(0);
}

int
GarminNMEA::Shutdown()
{
  StopThread();
  close(gps_fd);
  gps_fd=-1;
  return(0);
}

void
GarminNMEA::Main()
{
  char buf[NMEA_MAX_SENTENCE_LEN];

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

  for(;;)
  {
    pthread_testcancel();
    if(ReadSentence(buf,sizeof(buf)))
    {
      PLAYER_ERROR("while reading from GPS unit; bailing");
      pthread_exit(NULL);
    }
    ParseSentence((const char*)buf);
  }
}

/*
 * Find a complete NMEA sentence and copy it into 'buf', which should be of
 * length 'len'.  This function will call FillBuffer() as necessary to get
 * enough data into 'nmea_buf' to form a complete sentence. 'buf' will be
 * NULL-terminated.
 *
 * TODO: verify checksums. right now they're just dicarded.
 */
int
GarminNMEA::ReadSentence(char* buf, size_t len)
{
  char* ptr;
  char* ptr2;
  size_t sentlen;
  char tmp[8];
  int chksum;
  int oursum; 

  while(!(ptr = strchr((const char*)nmea_buf, NMEA_START_CHAR)))
  {
    nmea_buf_len=0;
    memset(nmea_buf,0,sizeof(nmea_buf));
    if(FillBuffer())
      return(-1);
  }

  nmea_buf_len = strlen(ptr);
  memmove(nmea_buf,ptr,strlen(ptr)+1);
  //printf("found start char:%s:%d\n", nmea_buf,nmea_buf_len);

  while(!(ptr = strchr((const char*)nmea_buf, NMEA_END_CHAR)))
  {
    if(nmea_buf_len >= sizeof(nmea_buf) - 1)
    {
      // couldn't get an end char and the buffer is full.
      PLAYER_WARN("couldn't find an end character; discarding data");
      buf = NULL;
      return(0);
    }
    if(FillBuffer())
      return(-1);
  }
  //printf("found end char:%s:\n", nmea_buf);

  sentlen = nmea_buf_len - strlen(ptr) + 1;
  if(sentlen > len - 1)
  {
    PLAYER_WARN1("NMEA sentence too long (%d bytes); truncating", sentlen);
    sentlen = len - 1;
  }


  // copy in all but the leading $ and trailing carriage return and line feed
  strncpy(buf,nmea_buf+1,sentlen-3);
  buf[sentlen-3] = '\0';

  // verify the checksum, if present.  two hex digits are the XOR of all the 
  // characters between the $ and *.
  if((ptr2 = strchr((const char*)buf,NMEA_CHKSUM_CHAR)) && 
     (strlen(ptr2) == 3))
  {
    strncpy(tmp,ptr2+1,2);
    tmp[2]='\0';
    chksum = strtol(tmp,NULL,16);
    oursum=0;
    for(int i=0;i<(int)(strlen(buf)-strlen(ptr2));i++)
      oursum ^= buf[i];

    if(oursum != chksum)
    {
      PLAYER_WARN("checksum mismatch; discarding sentence");
      buf=NULL;
    }
    else
    {
      // strip off checksum
      *ptr2='\0';
    }
  }
  else
  {
    //PLAYER_WARN("no checksum");
  }

  memmove(nmea_buf,ptr+1,strlen(ptr));
  nmea_buf_len -= sentlen;
  nmea_buf[nmea_buf_len]='\0';
  return(0);
}

/*
 * Read more data into the buffer 'nmea_buf', starting 'nmea_buf_len' chars
 * in, and not overrunning the total length.  'nmea_buf' will be
 * NULL-terminated.
 */
int
GarminNMEA::FillBuffer()
{
  int numread=0;
  int readcnt=0;

  while(numread<=0)
  {
    if((numread = read(gps_fd,nmea_buf+nmea_buf_len,
                       sizeof(nmea_buf)-nmea_buf_len-1)) < 0)
    {
      if(!gps_fd_blocking && (errno == EAGAIN))
      {
        if(readcnt >= GPS_STARTUP_CYCLES)
          return(-1);
        else
        {
          readcnt++;
          usleep(GPS_STARTUP_CYCLE_USEC);
        }
      }
      else
      {
        PLAYER_ERROR1("read(): %s", strerror(errno));
        return(-1);
      }
    }
  }
  nmea_buf_len += numread;
  nmea_buf[nmea_buf_len] = '\0';
  return(0);
}

/*
 * Parse an NMEA sentence, doing something appropriate with each message in
 * which we're interested. 'buf' should be NULL-terminated.
 */
int
GarminNMEA::ParseSentence(const char* buf)
{
  player_gps_data_t data;
  const char* ptr = buf;
  int degrees;
  double minutes;
  double arcseconds;
  char field[32];
  char tmp[8];

  if(!buf)
    return(0);

  // copy in the sentence header, for checking
  strncpy(tmp,buf,5);
  tmp[5]='\0';

  // the GGA msg has the position data that we want
  if(!strcmp(tmp,NMEA_GPGGA))
  {
    printf("got GGA (%s)\n", buf);

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // first field is time of day.
    // TODO: convert this to seconds since the epoch (how?)
    //       and fill it in.

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 2nd field is latitude.  first two chars are degrees.
    strncpy(tmp,field,2);
    tmp[2]='\0';
    degrees = atoi(tmp);
    // next is minutes
    minutes = atof(field+2);

    arcseconds = ((degrees * 60.0) + minutes) * 60.0;

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 3rd field is N or S for north or south. adjust sign accordingly.
    if(field[0] == 'S')
      arcseconds *= -1;

    data.latitude = htonl((int32_t)rint(arcseconds * 60.0));

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 4th field is longitude.  first 3 chars are degrees.
    strncpy(tmp,field,3);
    tmp[3]='\0';
    degrees = atoi(tmp);
    // next is minutes
    minutes = atof(field+3);

    arcseconds = ((degrees * 60.0) + minutes) * 60.0;
    
    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 5th field is E or W for east or west. adjust sign accordingly.
    if(field[0] == 'W')
      arcseconds *= -1;

    data.longitude = htonl((int32_t)rint(arcseconds * 60.0));

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 6th field is fix quality
    data.quality = atoi(field);

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 7th field is number of sats in view
    data.num_sats = atoi(field);

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 8th field is HDOP.  we'll multiply by ten to make it an integer.
    data.hdop = atoi(field) * 10;

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 9th field is altitude, in meters.  we'll convert to mm.
    data.altitude = htonl((int32_t)rint(atof(field) * 1000.0));

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 10th field tells us the reference for measuring altitude. e.g., 'M' is
    // mean sea level.  ignore it.

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 11th field is "height of geoid above WGS84 ellipsoid" ignore it.

    if(!(ptr = GetNextField(field, sizeof(field), ptr)))
      return(-1);
    // 12th field tells us the reference for the above-mentioned geode.  e.g.,
    // 'M' is mean sea level.  ignore it.


    // fields 13 and 14 are for DGPS. ignore them.
    
    PutData(&data,sizeof(player_gps_data_t),0,0);
  }
  
  return(0);
}

char*
GarminNMEA::GetNextField(char* field, size_t len, const char* ptr)
{
  char* start;
  char* end;
  size_t fieldlen;

  if(strlen(ptr) < 2 || !(start = strchr(ptr, ',')))
  {
    field[0]='\0';
    return(NULL);
  }

  if(!(end = strchr(start+1, ',')))
    fieldlen = strlen(ptr) - (start - ptr);
  else
    fieldlen = end - start - 1;

  if(fieldlen > (len - 1))
  {
    PLAYER_WARN("NMEA field too big; truncating");
    fieldlen = len - 1;
  }

  strncpy(field,start+1,fieldlen);
  field[fieldlen] = '\0';

  return(end);
}
