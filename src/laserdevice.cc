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
#include <time.h>
#include <sys/time.h>

#define PLAYER_ENABLE_TRACE 0

#include <playercommon.h>
#include <laserdevice.h>


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
CLaserDevice::CLaserDevice(char *port) 
{
    data = new unsigned char[LASER_DATA_BUFFER_SIZE];
    config = new unsigned char[LASER_CONFIG_BUFFER_SIZE];

    config_size = 0;

    data_swapped = false;
    bzero(data, LASER_DATA_BUFFER_SIZE);
  
    strcpy( LASER_SERIAL_PORT, port );
    // just in case...
    LASER_SERIAL_PORT[sizeof(LASER_SERIAL_PORT)-1] = '\0';
}


////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by client thread)
//
size_t CLaserDevice::GetData( unsigned char *dest, size_t maxsize ) 
{
    if(!data_swapped)
    {
        for(int i=0;i<LASER_DATA_BUFFER_SIZE;i+=sizeof(unsigned short))
            *(unsigned short*)&data[i] = htons(*(unsigned short*)&data[i]);
        data_swapped = true;
    }
    memcpy( dest, data, LASER_DATA_BUFFER_SIZE );
    return(LASER_DATA_BUFFER_SIZE);
}


////////////////////////////////////////////////////////////////////////////////
// Put data in buffer (called by device thread)
//
void CLaserDevice::PutData( unsigned char *src, size_t maxsize )
{
    memcpy( data, src, LASER_DATA_BUFFER_SIZE );
    data_swapped = false;
}


////////////////////////////////////////////////////////////////////////////////
// Get command from buffer (called by device thread)
//
void CLaserDevice::GetCommand( unsigned char *dest, size_t maxsize ) 
{
}


////////////////////////////////////////////////////////////////////////////////
// Put command in buffer (called by client thread)
//
void CLaserDevice::PutCommand( unsigned char *src, size_t maxsize) 
{
}


////////////////////////////////////////////////////////////////////////////////
// Get configuration from buffer (called by device thread)
//
size_t CLaserDevice::GetConfig(unsigned char *dest, size_t maxsize) 
{
    if (config_size == 0)
        return 0;
    if (config_size != 5)
    {
        PLAYER_MSG1("config_size = %d", (int) config_size);
        config_size = 0;
        RETURN_ERROR(0, "config data has incorrect length");
    }

    min_segment = ntohs(MAKEUINT16(config[0], config[1]));
    max_segment = ntohs(MAKEUINT16(config[2], config[3]));
    intensity = (bool) config[4];
    PLAYER_MSG3("new scan range [%d %d], intensity [%d]",
         (int) min_segment, (int) max_segment, (int) intensity);
    
    config_size = 0;
    return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Put configuration in buffer (called by client thread)
//
void CLaserDevice::PutConfig( unsigned char *src, size_t maxsize) 
{
    if (maxsize > LASER_CONFIG_BUFFER_SIZE)
    {
        PLAYER_ERROR("config request too big; ignoring");
        return;
    }

    memcpy(config, src, maxsize);
    config_size = maxsize;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
//
int CLaserDevice::Setup()
{   
    // Set scan range to default
    //
    min_segment = 0;
    max_segment = 360;
    intensity = false;

    printf("Laser connection initializing...");
    fflush(stdout);
    if (OpenTerm())
        return 1;

    // Start out at 9600 with non-blocking io
    //
    if (ChangeTermSpeed(9600))
        return 1;

    // Open the laser and set it to the correct speed
    //
    PLAYER_MSG0("changing laser mode at 9600");
    if (SetLaserMode() == 0)
    {
        PLAYER_MSG0("laser operating at 9600; changing to 38400");
        if (SetLaserSpeed(38400))
            return 1;
        if (ChangeTermSpeed(38400))
            return 1;
    }
    else
    {
        PLAYER_MSG0("could not change laser mode at 9600; trying 38400");
        if (ChangeTermSpeed(38400))
            return 1;
        if (SetLaserMode())
            return 1;
    }

    // Configure the laser
    //
    if (SetLaserConfig())
        return 1;

    CloseTerm();

    PLAYER_MSG0("laser ready");
    puts("Done.");

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
  /* shutdown laser device */
  close(laser_fd);
  pthread_cancel( thread );
  PLAYER_MSG0("Laser has been shutdown");
  puts("Laser has been shutdown");

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Start the device thread
//
void CLaserDevice::Run() 
{
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create( &thread, &attr, &DummyMain, this );
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
    sigblock(SIGINT);
    sigblock(SIGALRM);

    if (OpenTerm())
        return 1;

    // Change to 38400
    //
    if (ChangeTermSpeed(38400))
        return 1;

    // Ask the laser to send data
    //
    for (int retry = 0; retry < MAX_RETRIES; retry++)
    {
        if (RequestLaserData() == 0)
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
        if (GetLock()->GetConfig(this, NULL, 0))
        {
            // Change any config settings
            //
            if (SetLaserMode() == 0)
                SetLaserConfig();

            // Issue a new request for data
            //
            RequestLaserData();
        }
        
        // Process incoming data
        //
        ProcessLaserData();
    }

    CloseTerm();

    PLAYER_TRACE0("exiting laser thread");
    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// Open the terminal
// Returns 0 on success
//
int CLaserDevice::OpenTerm()
{
    laser_fd = ::open( LASER_SERIAL_PORT, O_RDWR | O_SYNC , S_IRUSR | S_IWUSR );
    if (laser_fd < 0)
        RETURN_ERROR(1, "Unable to open serial port");

    // set the serial port speed to 9600 to match the laser
    // later we can ramp the speed up to the SICK's 38K
    //
    struct termios term;
    if( tcgetattr( laser_fd, &term ) < 0 )
        RETURN_ERROR(1, "Unable to get serial port attributes");
  
    cfmakeraw( &term );
    cfsetispeed( &term, B9600 );
    cfsetospeed( &term, B9600 );
  
    if( tcsetattr( laser_fd, TCSAFLUSH, &term ) < 0 )
        RETURN_ERROR(1, "Unable to set serial port attributes");

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Close the terminal
// Returns 0 on success
//
int CLaserDevice::CloseTerm()
{
    ::close(laser_fd);
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
        if( tcgetattr( laser_fd, &term ) < 0 )
            RETURN_ERROR(1, "unable to get device attributes");
        
        cfmakeraw( &term );
        cfsetispeed( &term, B9600 );
        cfsetospeed( &term, B9600 );
        
        if( tcsetattr( laser_fd, TCSAFLUSH, &term ) < 0 )
            RETURN_ERROR(1, "unable to set device attributes");
    }
    else if (speed == 38400)
    {
        PLAYER_MSG0("terminal speed to 38400");
        if( tcgetattr( laser_fd, &term ) < 0 )
            RETURN_ERROR(1, "unable to get device attributes");
        
        cfmakeraw( &term );
        cfsetispeed( &term, B38400 );
        cfsetospeed( &term, B38400 );
        
        if( tcsetattr( laser_fd, TCSAFLUSH, &term ) < 0 )
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
    usleep(500000);

    len = ReadFromLaser(packet, sizeof(packet), true, 200);
    if (len < 0)
        RETURN_ERROR(1, "error reading from laser")
    else if (len < 1)
        RETURN_ERROR(1, "no reply from laser")
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
    usleep(500000);

    len = ReadFromLaser(packet, sizeof(packet), true, 200);
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
// Set the laser configuration
// Returns 0 on success
//
int CLaserDevice::SetLaserConfig()
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
    usleep(200000);

    len = ReadFromLaser(packet, sizeof(packet), false, 200);
    if (len < 0)
        return 1;
    else if (len < 1)
        RETURN_ERROR(1, "no reply from laser")
    else if (packet[0] == NACK)
        RETURN_ERROR(1, "request denied by laser")
    else if (packet[0] != 0xF4)
        RETURN_ERROR(1, "unexpected packet type");

    PLAYER_TRACE0("get configuration request ok");

    // *** TESTING ***
    //
    /*
    for (int i = 0; i < len; i++)
        printf("%02X ", (int) packet[i]);
    printf("\n");
    */

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
    usleep(200000L);

    len = ReadFromLaser(packet, sizeof(packet), false, 200);
    if (len < 0)
        return 1;
    else if (len < 1)
        RETURN_ERROR(1, "no reply from laser")
    else if (packet[0] == NACK)
        RETURN_ERROR(1, "request denied by laser")
    else if (packet[0] != 0xF7)
        RETURN_ERROR(1, "unexpected packet type");

    // *** TESTING ***
    //
    /*
    for (int i = 0; i < len; i++)
        printf("%02X ", (int) packet[i]);
    printf("\n");
    */

    PLAYER_TRACE0("set configuration request ok");

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Request data from the laser
// Returns 0 on success
//
int CLaserDevice::RequestLaserData()
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
    usleep(100000);
    
    len = ReadFromLaser(packet, sizeof(packet), true, 200);
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
// Process data from the incoming data stream
// Returns 0 on success
//
int CLaserDevice::ProcessLaserData()
{
    uint8_t raw_data[LASER_DATA_BUFFER_SIZE + 7];
    uint8_t final_data[LASER_DATA_BUFFER_SIZE] = {0};

    // Read a packet from the laser
    //
    int len = ReadFromLaser(raw_data, sizeof(raw_data));
    if (len == 0)
        return 0;

    // Dump the packet
    //
    /*
    for (int i = 0; i < 6; i++)
        printf("%X ", (int) (raw_data[i]));
    printf("\n");
    */
    
    // Process raw packets
    //
    if (raw_data[0] == 0xB0)
    {
        // Determine the number of values returned
        //
        //int units = raw_data[2] >> 6;
        int count = (int) raw_data[1] | ((int) (raw_data[2] & 0x3F) << 8);
        ASSERT(count < LASER_DATA_BUFFER_SIZE);
        
        // Strip the status info and shift everything down a few bytes
        // to remove packet header.
        //
        for (int i = 0; i < count; i++)
        {
            int src = 2 * i + 3;
            int dest = 2 * i;
            final_data[dest + 0] = raw_data[src + 0];
            final_data[dest + 1] = raw_data[src + 1];
        }
    
        GetLock()->PutData(this, final_data, 2 * count);
    }
    else if (raw_data[0] == 0xB7)
    {
        // Determine which values were returned
        //
        int first = ((int) raw_data[1] | ((int) raw_data[2] << 8)) - 1;
        //int last =  ((int) raw_data[3] | ((int) raw_data[4] << 8)) - 1;
        
        // Determine the number of values returned
        //
        //int units = raw_data[6] >> 6;
        int count = (int) raw_data[5] | ((int) (raw_data[6] & 0x3F) << 8);
        ASSERT(count < LASER_DATA_BUFFER_SIZE);
       
        // Strip the status info and shift everything down a few bytes
        // to remove packet header.
        //
        for (int i = 0; i < count; i++)
        {
            int src = 2 * i + 7;
            int dest = 2 * (i + first);
            final_data[dest + 0] = raw_data[src + 0];
            final_data[dest + 1] = raw_data[src + 1];
        }
    
        GetLock()->PutData(this, final_data, 2 * count);
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
    ASSERT(4 + len + 2 < (ssize_t) sizeof(buffer));

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

    // Write the data to the port
    //
    ssize_t bytes = ::write( laser_fd, buffer, 4 + len + 2);
    
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
        //PLAYER_TRACE0("using blocking io");
        timeout = 1000;
        int flags = ::fcntl(laser_fd, F_GETFL);
        if (flags < 0)
        {
            PLAYER_ERROR("unable to get device flags");
            return 0;
        }
        if (::fcntl(laser_fd, F_SETFL, flags & (~O_NONBLOCK)) < 0)
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
        int flags = ::fcntl(laser_fd, F_GETFL);
        if (flags < 0)
        {
            PLAYER_ERROR("unable to get device flags");
            return 0;
        }
        if (::fcntl(laser_fd, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            PLAYER_ERROR("unable to set device flags");
            return 0;
        }
    }

    int start_time = GetTime();
    int stop_time = start_time + timeout;

    int bytes = 0;
    uint8_t header[5] = {0};
    uint8_t footer[3];
    
    // Read until we get a valid header
    // or we timeout
    //
    while (true)
    {
        bytes = ::read(laser_fd, header + sizeof(header) - 1, 1);
        if (header[0] == STX && header[1] == 0x80)
        {
            if (!ack)
                break;
            if (header[4] == ACK || header[4] == NACK)
                break;
        }
        memmove(header, header + 1, sizeof(header) - 1);
        if (GetTime() >= stop_time)
            RETURN_ERROR(0, "timeout on read (1)");
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
        bytes += ::read(laser_fd, data + bytes, len - bytes);
        if (GetTime() >= stop_time)
            RETURN_ERROR(0, "timeout on read (3)");
    }

    // Read in footer
    //
    bytes = 0;
    while (bytes < 3)
    {
        bytes += ::read(laser_fd, footer + bytes, 3 - bytes);
        if (GetTime() >= stop_time)
            RETURN_ERROR(0, "timeout on read (4)");
    }
    
    // Construct entire packet
    // And check the CRC
    //
    uint8_t buffer[4 + 1024 + 1];
    ASSERT(4 + len + 1 < (ssize_t) sizeof(buffer));
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
int CLaserDevice::GetTime()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    return ((1000 * tv.tv_sec + tv.tv_usec / 1000) % 0x7FFFFFF);
}
