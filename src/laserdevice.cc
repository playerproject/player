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

///////////////////////////////////////////////////////////////////////////
//
// File: laserdevice.cc
// Author: Andrew Howard
// Date: 7 Nov 2000
// Desc: Driver for the SICK laser
//
// CVS info:
//  $Source$
//  $Author$
//  $Revision$
//
// Usage:
//  (empty)
//
// Theory of operation:
//  (empty)
//
// Known bugs:
//  (empty)
//
// Possible enhancements:
//  (empty)
//
///////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>  /* for sigblock */
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */

#include <playertime.h>
extern PlayerTime* GlobalTime;

#define PLAYER_ENABLE_MSG 0
#define PLAYER_ENABLE_TRACE 0

#include "playercommon.h"
#include "laserdevice.h"


////////////////////////////////////////////////////////////////////////////////
// Device codes

#define STX     0x02
#define ACK     0xA0
#define NACK    0x92
#define CRC16_GEN_POL 0x8005
#define MAX_RETRIES 5


////////////////////////////////////////////////////////////////////////////////
// Error macros
//
#define RETURN_ERROR(erc, m) {PLAYER_ERROR(m); return erc;}


////////////////////////////////////////////////////////////////////////////////
// Constructor
//
CLaserDevice::CLaserDevice(int argc, char** argv) :
  CDevice(sizeof(player_laser_data_t),0,1,1)
{
  strncpy(m_laser_name,DEFAULT_LASER_PORT,sizeof(m_laser_name));
  for(int i=0;i<argc;i++)
  {
    if(!strcmp(argv[i],"port"))
    {
      if(++i<argc)
      {
        strncpy(m_laser_name, argv[i],sizeof(m_laser_name));
        m_laser_name[sizeof(m_laser_name)-1] = '\0';
      }
      else
        fprintf(stderr, "CLaserDevice: missing port; using default: \"%s\"\n",
                m_laser_name);
    }
    else
      fprintf(stderr, "CLaserDevice: ignoring unknown parameter \"%s\"\n",
              argv[i]);
  }

  m_config_size = 0;
  memset(&m_config, 0, sizeof(m_config));
  //memset(&m_data, 0, sizeof(m_data));
}

////////////////////////////////////////////////////////////////////////////////
// Get configuration from buffer (called by device thread)
//
size_t CLaserDevice::GetConfig(unsigned char *dest, size_t maxsize) 
{
  Lock();

  if (m_config_size == 0)
  {
    Unlock();
    return 0;
  }

  m_intensity = m_config.intensity;
  m_scan_res = m_config.resolution;
  int min_angle = (short) m_config.min_angle;
  int max_angle = (short) m_config.max_angle;

  PLAYER_TRACE2("%d %d", min_angle, max_angle);

  // For high res, drop the scan range down to 100 degrees.
  // The angles must be interpreted differently too.
  //
  if (m_scan_res == 25)
  {
    m_scan_width = 100;
    m_scan_min_segment = (min_angle + 5000) / m_scan_res;
    m_scan_max_segment = (max_angle + 5000) / m_scan_res;

    if (m_scan_min_segment < 0)
      m_scan_min_segment = 0;
    if (m_scan_min_segment > 400)
      m_scan_min_segment = 400;

    if (m_scan_max_segment < 0)
      m_scan_max_segment = 0;
    if (m_scan_max_segment > 400)
      m_scan_max_segment = 400;
  }
  else if (m_scan_res == 50 || m_scan_res == 100)
  {
    m_scan_width = 180;
    m_scan_min_segment = (min_angle + 9000) / m_scan_res;
    m_scan_max_segment = (max_angle + 9000) / m_scan_res;

    if (m_scan_min_segment < 0)
      m_scan_min_segment = 0;
    if (m_scan_min_segment > 360)
      m_scan_min_segment = 360;

    if (m_scan_max_segment < 0)
      m_scan_max_segment = 0;
    if (m_scan_max_segment > 360)
      m_scan_max_segment = 360;
  }
  else
    PLAYER_ERROR("invalid laser configuration");

  PLAYER_MSG3("new scan range [%d %d], intensity [%d]",
              (int) m_scan_min_segment, (int) m_scan_max_segment, (int) m_intensity);

  m_config_size = 0;

  Unlock();
  return sizeof(m_config);
}


////////////////////////////////////////////////////////////////////////////////
// Put configuration in buffer (called by client thread)
//
void CLaserDevice::PutConfig( unsigned char *src, size_t maxsize) 
{
  Lock();

  if (maxsize != sizeof(m_config))
  {
    PLAYER_ERROR("config request has incorrect size; ignoring");
    Unlock();
    return;
  }

  memcpy(&m_config, src, maxsize);
  m_config_size = maxsize;

  // Byte-swap the configuration data
  //
  m_config.resolution = ntohs(m_config.resolution);
  m_config.min_angle = ntohs(m_config.min_angle);
  m_config.max_angle = ntohs(m_config.max_angle);

  Unlock();
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
//
int CLaserDevice::Setup()
{   
    // Set default configuration
    //
    m_scan_width = 180;
    m_scan_res = 50;
    m_scan_min_segment = 0;
    m_scan_max_segment = 360;
    m_intensity = true;

    printf("Laser initialising (%s)\n", m_laser_name);
    
    // Open the terminal
    if (OpenTerm())
        return 1;

    // Start out at 38400 with non-blocking io
    //
    if (ChangeTermSpeed(38400))
        return 1;

    PLAYER_MSG0("connecting at 38400");
    if (SetLaserMode() != 0)
    {
        if (SetLaserMode() != 0)
        {
            PLAYER_MSG0("connect at 38400 failed, trying 9600");
            if (ChangeTermSpeed(9600))
                return 1;
            if (SetLaserMode() != 0)
            {
                if (SetLaserMode() != 0)
                {
                    PLAYER_ERROR("connection failed");
                    return 1;
                }
            }
            PLAYER_MSG0("laser operating at 9600; changing to 38400");
            if (SetLaserSpeed(38400))
                return 1;
            if (ChangeTermSpeed(38400))
                return 1;
        }
    }

    // Display the laser type
    //
    char type[64];
    if (GetLaserType(type, sizeof(type)))
        return 1;
    PLAYER_MSG1("SICK laser type [%s]", (char*) type);

    // Configure the laser
    //
    if (SetLaserRes(m_scan_width, m_scan_res))
        return 1;
    if (SetLaserConfig(m_intensity))
        return 1;

    puts("laser ready");

    // Start the device thread
    //
    Run();

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
//
int CLaserDevice::Shutdown()
{
  void* dummy;
  /* shutdown laser device */
  pthread_cancel(m_thread);
  if(pthread_join(m_thread,&dummy))
    perror("CLaserDevice::Shutdown:pthread_join()");
  CloseTerm();
  puts("Laser has been shutdown");

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Start the device thread
//
void CLaserDevice::Run() 
{
  pthread_create( &m_thread, NULL, &DummyMain, this );
}


////////////////////////////////////////////////////////////////////////////////
// Dummy main (just calls real main)
//
void* CLaserDevice::DummyMain(void *laserdevice)
{
    ((CLaserDevice*) laserdevice)->Main();
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
//
int CLaserDevice::Main() 
{
    PLAYER_MSG0("laser thread is running");
    
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

#ifdef PLAYER_LINUX
    sigblock(SIGINT);
    sigblock(SIGALRM);
#endif
        
    // Ask the laser to send data
    //
    for (int retry = 0; retry < MAX_RETRIES; retry++)
    {
        if (RequestLaserData(m_scan_min_segment, m_scan_max_segment) == 0)
            break;
        else if (retry >= MAX_RETRIES)
            RETURN_ERROR(1, "laser not responding; exiting laser thread");
    }

    while (true)
    {
        //usleep(1000);

        // test if we are supposed to cancel
        //
        pthread_testcancel();

        // Look for configuration requests
        //
        if(GetConfig(NULL, 0))
        {
            // Change any config settings
            //
            if (SetLaserMode())
                PLAYER_ERROR("request for config mode failed");
            else
            {
                SetLaserRes(m_scan_width, m_scan_res);
                SetLaserConfig(m_intensity);
            }

            // Issue a new request for data
            //
            if (RequestLaserData(m_scan_min_segment, m_scan_max_segment))
                PLAYER_ERROR("request for laser data failed");
        }

        // Get the time at which we started reading
        // This will be a pretty good estimate of when the phenomena occured
        //
        struct timeval time;
        //gettimeofday(&time, NULL);
	GlobalTime->GetTime(&time);
        
        // Process incoming data
        //
        player_laser_data_t data;
        if (ReadLaserData(data.ranges, sizeof(data.ranges) / sizeof(data.ranges[0])) == 0)
        {
            // Prepare packet and byte swap
            //
            data.min_angle = htons(m_scan_min_segment * m_scan_res - m_scan_width * 50);
            data.max_angle = htons(m_scan_max_segment * m_scan_res - m_scan_width * 50);
            data.resolution = htons(m_scan_res);
            data.range_count = htons(m_scan_max_segment - m_scan_min_segment + 1);
            for (int i = 0; i < m_scan_max_segment - m_scan_min_segment + 1; i++)
                data.ranges[i] = htons(data.ranges[i]);

            // Make data available
            //
            PutData((uint8_t*) &data, sizeof(data),
                    time.tv_sec, time.tv_usec);
        }
    }

    PLAYER_TRACE0("exiting laser thread");
    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// Open the terminal
// Returns 0 on success
//
int CLaserDevice::OpenTerm()
{
    m_laser_fd = ::open(m_laser_name, O_RDWR | O_SYNC , S_IRUSR | S_IWUSR );
    if (m_laser_fd < 0)
    {
        PLAYER_ERROR1("unable to open serial port [%s]", (char*) m_laser_name);
        perror("");
        return 1;
    }

    // set the serial port speed to 9600 to match the laser
    // later we can ramp the speed up to the SICK's 38K
    //
    struct termios term;
    if( tcgetattr( m_laser_fd, &term ) < 0 )
        RETURN_ERROR(1, "Unable to get serial port attributes");
  
#ifdef PLAYER_LINUX
    cfmakeraw( &term );
#endif
    cfsetispeed( &term, B9600 );
    cfsetospeed( &term, B9600 );
  
    if( tcsetattr( m_laser_fd, TCSAFLUSH, &term ) < 0 )
        RETURN_ERROR(1, "Unable to set serial port attributes");

    // Make sure queue is empty
    //
    tcflush(m_laser_fd, TCIOFLUSH);
    
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Close the terminal
// Returns 0 on success
//
int CLaserDevice::CloseTerm()
{
    ::close(m_laser_fd);
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set the terminal speed
// Valid values are 9600 and 38400
// Returns 0 on success
//
int CLaserDevice::ChangeTermSpeed(int speed)
{
    struct termios term;
    
    if (speed == 9600)
    {
        PLAYER_MSG0("terminal speed to 9600");
        if( tcgetattr( m_laser_fd, &term ) < 0 )
            RETURN_ERROR(1, "unable to get device attributes");
        
#ifdef PLAYER_LINUX
        cfmakeraw( &term );
#endif
        cfsetispeed( &term, B9600 );
        cfsetospeed( &term, B9600 );
        
        if( tcsetattr( m_laser_fd, TCSAFLUSH, &term ) < 0 )
            RETURN_ERROR(1, "unable to set device attributes");
    }
    else if (speed == 38400)
    {
        PLAYER_MSG0("terminal speed to 38400");
        if( tcgetattr( m_laser_fd, &term ) < 0 )
            RETURN_ERROR(1, "unable to get device attributes");
        
#ifdef PLAYER_LINUX
        cfmakeraw( &term );
#endif
        cfsetispeed( &term, B38400 );
        cfsetospeed( &term, B38400 );
        
        if( tcsetattr( m_laser_fd, TCSAFLUSH, &term ) < 0 )
            RETURN_ERROR(1, "unable to set device attributes");
    }
    
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Put the laser into configuration mode
//
int CLaserDevice::SetLaserMode()
{
    ssize_t len;
    uint8_t packet[20];

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
        RETURN_ERROR(1, "error reading from laser")
    else if (len < 1)
    {
        PLAYER_TRACE0("no reply from laser");
        return 1;
    }
    else if (packet[0] == NACK)
        RETURN_ERROR(1, "request denied by laser")
    else if (packet[0] != ACK)
        RETURN_ERROR(1, "unexpected packet type");

    PLAYER_TRACE0("configuration mode request ok");

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set the laser data rate
// Valid values are 9600 and 38400
// Returns 0 on success
//
int CLaserDevice::SetLaserSpeed(int speed)
{
    ssize_t len;
    uint8_t packet[20];

    packet[0] = 0x20;
    packet[1] = (speed == 9600 ? 0x42 : 0x40);
    len = 2;

    PLAYER_TRACE0("sending baud rate request to laser");
    if (WriteToLaser(packet, len) < 0)
        return 1;
            
    // Wait for laser to return ack
    // This could take a while...
    //
    PLAYER_TRACE0("waiting for acknowledge");
    len = ReadFromLaser(packet, sizeof(packet), true, 2000);
    if (len < 0)
        return 1;
    else if (len < 1)
        RETURN_ERROR(1, "no reply from laser")
    else if (packet[0] == NACK)
        RETURN_ERROR(1, "request denied by laser")
    else if (packet[0] != ACK)
        RETURN_ERROR(1, "unexpected packet type");

    PLAYER_TRACE0("baud rate request ok");
            
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get the laser type
//
int CLaserDevice::GetLaserType(char *buffer, size_t bufflen)
{
    ssize_t len;
    uint8_t packet[512];

    packet[0] = 0x3A;
    len = 1;

    PLAYER_TRACE0("sending get type request to laser");
    if (WriteToLaser(packet, len) < 0)
        return 1;

    // Wait for laser to return data
    // This could take a while...
    //
    PLAYER_TRACE0("waiting for reply");
    len = ReadFromLaser(packet, sizeof(packet), false, -1);
    if (len < 0)
        return 1;
    else if (len < 1)
        RETURN_ERROR(1, "no reply from laser")
    else if (packet[0] == NACK)
        RETURN_ERROR(1, "request denied by laser")
    else if (packet[0] != 0xBA)
        RETURN_ERROR(1, "unexpected packet type");

    // NULL terminate the return string
    //
    assert((size_t) len + 1 < sizeof(packet));
    packet[len + 1] = 0;

    // Copy to buffer
    //
    assert(bufflen >= (size_t) len - 1);
    strcpy(buffer, (char*) (packet + 1));

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set the laser configuration
// Returns 0 on success
//
int CLaserDevice::SetLaserConfig(bool intensity)
{
    ssize_t len;
    uint8_t packet[512];

    packet[0] = 0x74;
    len = 1;

    PLAYER_TRACE0("sending get configuration request to laser");
    if (WriteToLaser(packet, len) < 0)
        return 1;

    // Wait for laser to return data
    // This could take a while...
    //
    PLAYER_TRACE0("waiting for reply");
    len = ReadFromLaser(packet, sizeof(packet), false, -1);
    if (len < 0)
        return 1;
    else if (len < 1)
        RETURN_ERROR(1, "no reply from laser")
    else if (packet[0] == NACK)
        RETURN_ERROR(1, "request denied by laser")
    else if (packet[0] != 0xF4)
        RETURN_ERROR(1, "unexpected packet type");

    PLAYER_TRACE0("get configuration request ok");

    // Modify the configuration and send it back
    //
    packet[0] = 0x77;
    packet[6] = (intensity ? 0x01 : 0x00); // Return intensity in top 3 data bits

    PLAYER_TRACE0("sending set configuration request to laser");
    if (WriteToLaser(packet, len) < 0)
        return 1;

    // Wait for the change to "take"
    //
    PLAYER_TRACE0("waiting for acknowledge");
    len = ReadFromLaser(packet, sizeof(packet), false, -1);
    if (len < 0)
        return 1;
    else if (len < 1)
        RETURN_ERROR(1, "no reply from laser")
    else if (packet[0] == NACK)
        RETURN_ERROR(1, "request denied by laser")
    else if (packet[0] != 0xF7)
        RETURN_ERROR(1, "unexpected packet type");

    PLAYER_TRACE0("set configuration request ok");

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Change the resolution of the laser
// Valid widths are: 100, 180 (degrees)
// Valid resolitions are: 25, 50, 100 (1/100 degree)
//
int CLaserDevice::SetLaserRes(int width, int res)
{
    ssize_t len;
    uint8_t packet[512];

    len = 0;
    packet[len++] = 0x3B;
    packet[len++] = (width & 0xFF);
    packet[len++] = (width >> 8);
    packet[len++] = (res & 0xFF);
    packet[len++] = (res >> 8);

    PLAYER_TRACE0("sending set variant request to laser");
    if (WriteToLaser(packet, len) < 0)
        return 1;

    // Wait for laser to return data
    // This could take a while...
    //
    PLAYER_TRACE0("waiting for reply");
    len = ReadFromLaser(packet, sizeof(packet), false, -1);
    if (len < 0)
        return 1;
    else if (len < 1)
        RETURN_ERROR(1, "no reply from laser")
    else if (packet[0] == NACK)
        RETURN_ERROR(1, "request denied by laser")
    else if (packet[0] != 0xBB)
        RETURN_ERROR(1, "unexpected packet type");

    // See if the request was accepted
    //
    if (packet[1] == 0)
        RETURN_ERROR(1, "variant request ignored");
        
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Request data from the laser
// Returns 0 on success
//
int CLaserDevice::RequestLaserData(int min_segment, int max_segment)
{
    ssize_t len = 0;
    uint8_t packet[20];
    
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

    PLAYER_TRACE0("sending scan data request to laser");
    if (WriteToLaser(packet, len) < 0)
        return 1;

    // Wait for laser to return ack
    // This should be fairly prompt
    //
    PLAYER_TRACE0("waiting for acknowledge");
    len = ReadFromLaser(packet, sizeof(packet), true, -1);
    if (len < 0)
        return 1;
    else if (len < 1)
        RETURN_ERROR(1, "no reply from laser")
    else if (packet[0] == NACK)
        RETURN_ERROR(1, "request denied by laser")
    else if (packet[0] != ACK)
        RETURN_ERROR(1, "unexpected packet type");

    PLAYER_TRACE0("scan data request ok");
   
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Read range data from laser
//
int CLaserDevice::ReadLaserData(uint16_t *data, size_t datalen)
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
ssize_t CLaserDevice::WriteToLaser(uint8_t *data, ssize_t len)
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
    tcflush(m_laser_fd, TCIOFLUSH);
    
    // Write the data to the port
    //
    ssize_t bytes = ::write( m_laser_fd, buffer, 4 + len + 2);

    // Make sure the queue is drained
    // Synchronous IO doesnt always work
    //
    ::tcdrain(m_laser_fd);
    
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
ssize_t CLaserDevice::ReadFromLaser(uint8_t *data, ssize_t maxlen, bool ack, int timeout)
{
    // If the timeout is infinite,
    // go to blocking io
    //
    if (timeout < 0)
    {
        PLAYER_TRACE0("using blocking io");
        int flags = ::fcntl(m_laser_fd, F_GETFL);
        if (flags < 0)
        {
            PLAYER_ERROR("unable to get device flags");
            return 0;
        }
        if (::fcntl(m_laser_fd, F_SETFL, flags & (~O_NONBLOCK)) < 0)
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
        int flags = ::fcntl(m_laser_fd, F_GETFL);
        if (flags < 0)
        {
            PLAYER_ERROR("unable to get device flags");
            return 0;
        }
        if (::fcntl(m_laser_fd, F_SETFL, flags | O_NONBLOCK) < 0)
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
        bytes = ::read(m_laser_fd, header + sizeof(header) - 1, 1);
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
        bytes += ::read(m_laser_fd, data + bytes, len - bytes);
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
        bytes += ::read(m_laser_fd, footer + bytes, 3 - bytes);
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
unsigned short CLaserDevice::CreateCRC(uint8_t* data, ssize_t len)
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
int64_t CLaserDevice::GetTime()
{
  struct timeval tv;
  //gettimeofday(&tv, NULL);
  GlobalTime->GetTime(&tv);
  return (int64_t) tv.tv_sec * 1000 + (int64_t) tv.tv_usec / 1000;
}


/* demo of how to make a shared object for Player to load at runtime */
#if 0
#include <devicetable.h>
extern CDeviceTable* deviceTable;
extern int global_playerport;

/* need the extern to avoid C++ name-mangling  */
extern "C" {
void _init(void)
{
  puts("Laser device initializing");
  deviceTable->AddDevice(global_playerport,PLAYER_LASER_CODE,0, 
                         PLAYER_READ_MODE, new CLaserDevice(0,NULL));
  puts("Laser device done");
}

void _fini(void)
{
  puts("Laser device closing");

  /* probably don't need any code here; the destructor for the device
   * will be called when Player shuts down.  this function is only useful
   * if you want to dlclose() the shared object before Player exits
   */

  puts("Laser device closed");
}
}
#endif

