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
 * methods for initializing, configuring, and getting data out of
 * the SICK laser
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>  /* for sigblock */
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */

#include <laserdevice.h>
#define MKSHORT(a,b) ((unsigned short)(a)|((unsigned short)(b)<<8))

#define CRC16_GEN_POL 0x8005

const unsigned char startString[] = { 0x02, 0x80, 0xd6, 0x02, 0xb0, 0x69, 0x41 };
const unsigned char ACK[] = { 0x02, 0x80, 0x03, 0x00, 0xa0 };
const unsigned char NACK[] = { 0x02, 0x80, 0x03, 0x00, 0x92 };
const unsigned char STX = 0x02;

CLaserDevice::CLaserDevice(char *port) 
{
  
  data = new unsigned char[LASER_DATA_BUFFER_SIZE];
  config = new unsigned char[LASER_CONFIG_BUFFER_SIZE];

  config_size = 0;

  data_swapped = false;
  debuglevel=0;
  bzero(data, LASER_DATA_BUFFER_SIZE);
  strcpy( LASER_SERIAL_PORT, port );
  // just in case...
  LASER_SERIAL_PORT[sizeof(LASER_SERIAL_PORT)-1] = '\0';
}

unsigned short CLaserDevice::CreateCRC( unsigned char* commData, unsigned int uLen )
{
  unsigned short uCrc16;
  unsigned char abData[2];
  
  uCrc16 = 0;
  abData[0] = 0;
  
  while(uLen-- ) {
    abData[1] = abData[0];
    abData[0] = *commData++;
    
    if( uCrc16 & 0x8000 ) {
      uCrc16 = (uCrc16 & 0x7fff) << 1;
      uCrc16 ^= CRC16_GEN_POL;
    }
    else {
      uCrc16 <<= 1;
    }
    uCrc16 ^= MKSHORT (abData[0],abData[1]);
  }
  return( uCrc16); 
}

ssize_t CLaserDevice::WriteToLaser( unsigned char *data, ssize_t len ) {
  unsigned char *datagram;
  unsigned short crc;
  ssize_t sz;

  datagram = new unsigned char[len+6];

  datagram[0] = STX; /* start byte */
  datagram[1] = 0x00; /* LMS address */
  datagram[2] = 0x0F & len; /* LSB - number of bytes to follow excluding crc */
  datagram[3] = 0xF0 & len; /* MSB - number of bytes to follow excluding crc */

  memcpy( (void *) &datagram[4], (void *) data, len );

  /* insert CRC */
  len += 4;
  crc = CreateCRC( datagram, len );
  datagram[len] = crc & 0x00FF;
  datagram[len+1] = (crc & 0xFF00) >> 8;
  len += 2;

  if (debuglevel>1) {
    printf("\nSending: ");
    for(int i=0; i<len; i++) {
      printf("%.2xh/", datagram[i]);
    }
    printf("\n");
  }

  sz = write( laser_fd, datagram, len);

  delete datagram;
  return (sz);
}

void CLaserDevice::DecodeStatusByte( unsigned char byte ) {
  unsigned short code;

  /* print laser status */
  printf("Laser Staus: ");
  code = byte & 0x07;
  switch(code) {
  case 0:
    printf("no error ");
    break;
  case 1:
    printf("info ");
    break;
  case 2:
    printf("warning ");
    break;
  case 3:
    printf("error ");
    break;
  case 4:
    printf("fatal error ");
    break;
  default:
    printf("unknown code ");
    break;
  }
 
  code = (byte >> 3) & 0x03;
  switch(code) {
  case 0:
    printf("LMS -xx1 to -xx4 ");
    break;
  case 1:
    printf("LMI ");
    break;
  case 2:
    printf("LMS -xx6 ");
    break;
  case 3:
    printf("reserved ");
    break;
  default:
    printf("unknown device ");
    break;
  }

  printf("restart:%d ", (byte >> 5) & 0x01);

  if ( ( byte >> 6 ) & 0x01 ) 
    printf("Implausible measured value ");

  if ( ( byte >> 7 ) & 0x01 ) 
    printf("Pollution ");

  printf("\n");
}

bool CLaserDevice::CheckDatagram( unsigned char *datagram ) {
  unsigned short crc;

  if (debuglevel>1) {
    printf("\nReceived: ");
    for(int i=0; i<9; i++) {
      printf("%.2xh/", datagram[i]); fflush(stdout);
    }
    printf("\n");fflush(stdout);
  }

  /* check CRC */
  crc = CreateCRC( &datagram[0], 7 );
  if ( datagram[7] != (crc & 0x00FF) || datagram[8] != ((crc & 0xFF00) >> 8) ) {
    if (debuglevel>1) {
      printf("\ncrc incorrect: expected 0x%.2x 0x%.2x got 0x%.2x 0x%.2x\n",
	     ((crc & 0xFF00) >> 8), (crc & 0x00FF), datagram[7], datagram[8] );
    }
    return(false);
  }

  /* print status information */
  if (debuglevel>1) {
    DecodeStatusByte( datagram[6] );
  }

  /* decode message */
  if (strncmp( (const char *) datagram, (const char *) ACK, 5 ) == 0 ) {
    switch( datagram[5] ) {
    case 0x00: 
      puts("ok");
      break;
    case 0x01:
      puts("request denied - incorrect password");
      break;
    case 0x02:
      puts("request denied - LMI fault");
      break;
    }
  }
  else if (strncmp( (const char *) datagram, (const char *) NACK, 5 ) == 0 ) {
    puts("not acknowledged");
  }

  return(true);
} 

ssize_t CLaserDevice::RecieveAck() {
  unsigned char datagram[9];
  ssize_t sz;

  /* laser sends acknowledge within 60ms therefore 
     with 70ms we are on the safe side */
  usleep(700000);
  
  while(1) {
    if ( (sz = read( laser_fd, &datagram[8], 1 )) < 0 ) {
      puts("no acknowledge received");
      return(sz);
    }
    
    if (datagram[0]==STX && CheckDatagram( datagram ) )
      break;
    else
      for(int i=0;i<8;i++) datagram[i]=datagram[i+1];
  }

  return (sz);
}


int CLaserDevice::ChangeMode(  )
{ 
  ssize_t len;
  unsigned char request[20];

  request[0] = 0x20; /* mode change command */
  request[1] = 0x00; /* configuration mode */
  request[2] = 0x53; // S - the password 
  request[3] = 0x49; // I
  request[4] = 0x43; // C
  request[5] = 0x4B; // K
  request[6] = 0x5F; // _
  request[7] = 0x4C; // L
  request[8] = 0x4D; // M
  request[9] = 0x53; // S

  len = 10;
  
  printf("Sending configuration mode request to laser.."); fflush(stdout);
  if ( (len = WriteToLaser( request, len)) < 0 ) {
    perror("ChangeMode");
    return(1);
  }

  if ( ( len = RecieveAck() ) < 0 ) {
    perror("ChangeMode");
    return(1);
  }

  return 0;
}

int CLaserDevice::Request38k()
{ 
  ssize_t len;
  unsigned char request[20];

  request[0] = 0x20; /* mode change command */
  request[1] = 0x40; /* 38k */

  len = 2;
  
  printf("Sending 38k request to laser.."); fflush(stdout);
  if ( (len = WriteToLaser( request, len)) < 0 ) {
    perror("Request38k");
    return(1);
  }

  if ( ( len = RecieveAck() ) < 0 ) {
    perror("Request38k");
    return(1);
  }

  return 0;
}

int CLaserDevice::RequestData()
{ 
  ssize_t len;
  unsigned char request[20];

  request[0] = 0x20; /* mode change command */
  request[1] = 0x24; /* request data */

  len = 2;
  
  printf("Sending data request to laser.."); fflush(stdout);
  if ( (len = WriteToLaser( request, len)) < 0 ) {
    perror("RequestData");
    return(1);
  }

  if ( ( len = RecieveAck() ) < 0 ) {
    perror("RequestData");
    return(1);
  }

  return 0;
}

int CLaserDevice::Setup() {
  struct termios term;

  if( (laser_fd = open( LASER_SERIAL_PORT, O_RDWR | O_SYNC | O_NONBLOCK , S_IRUSR | S_IWUSR )) < 0 ) {
    perror("CLaserDevice:Setup");
    return(1);
  }  
 
  // set the serial port speed to 9600 to match the laser
  // later we can ramp the speed up to the SICK's 38K
  if( tcgetattr( laser_fd, &term ) < 0 )
    printf( "get attributes error\n" );
  
  cfmakeraw( &term );
  cfsetispeed( &term, B9600 );
  cfsetospeed( &term, B9600 );
  
  if( tcsetattr( laser_fd, TCSAFLUSH, &term ) < 0 )
    printf( "set attributes error\n" );

  if( ChangeMode() != 0 ) {
    puts("Change mode failed most likely because the laser");
    puts("is running 38K..switching terminal to 38k");
  }
  else if( Request38k() != 0 ) {
    puts("CLaserDevice:Setup: Couldn't change laser speed to 38k");
    return( 1 );
  }

  // set serial port speed to 38k
  if( tcgetattr( laser_fd, &term ) < 0 )
    printf( "get attributes error\n" );
  cfmakeraw( &term );
  cfsetispeed( &term, B38400 );
  cfsetospeed( &term, B38400 );
  if( tcsetattr( laser_fd, TCSAFLUSH, &term ) < 0 )
    printf( "set attributes error\n" );

  if( RequestData() != 0 ) {
    printf("Couldn't request data most likely because the laser\nis not connected or is connected not to %s\n", LASER_SERIAL_PORT);
    return( 1 );
  }

  puts("Laser ready");
  fflush(stdout);
  /* success: start the "Run" thread and report success */

  close(laser_fd);

  Run();
  return(0);
}

int CLaserDevice::Shutdown() {
  /* shutdown laser device */
  close(laser_fd);
  pthread_cancel( thread );
  puts("Laser has been shutdown");

  return(0);
}

void *RunLaserThread( void *laserdevice ) 
{
  // 7-byte header + 722-byte sample + 1-byte status + 2-byte CRC
  unsigned char data[7+LASER_DATA_BUFFER_SIZE+1+2];
  int n,c;
  int bytes = 0;
  unsigned short crc;

  CLaserDevice *ld = (CLaserDevice *) laserdevice;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
  sigblock(SIGINT);
  sigblock(SIGALRM);
  memset( data, 0, 7 );

  if( (ld->laser_fd = open( ld->LASER_SERIAL_PORT, O_RDWR | O_SYNC , S_IRUSR | S_IWUSR )) < 0 ) {
    perror("CLaserDevice:RunLaserThread");
    pthread_exit(0);
  }  

  while(1) 
  {
    /* test if we are supposed to cancel */
    pthread_testcancel();

    /* get laser scans one at a time */

    // read the stream of bytes one by one until we see the message header

    // move the first 7 bytes in the data buffer one place left
    for( n=0; n<6; n++ ) 
    {
      data[n] = data[n+1];
    }

    bytes += read( ld->laser_fd, &data[6], 1 );

    if( strncmp( (char *) data, (char *) startString, 7 ) == 0 ) // ok we've got the start of the data 
    {
      // now read the measured values (361*2 bytes) plus status (1 byte) and crc (2 bytes)
      bytes = 0;
      while(bytes<LASER_DATA_BUFFER_SIZE+1+2) {
        //bytes += read( ld->laser_fd, &data[bytes+7], 725-bytes );
        bytes += read( ld->laser_fd, &data[bytes+7], 
                        (LASER_DATA_BUFFER_SIZE+1+2)-bytes );
      }

      //crc = ld->CreateCRC( data, 730 );
      crc = ld->CreateCRC( data, 7+LASER_DATA_BUFFER_SIZE+1);

      if ( data[7+LASER_DATA_BUFFER_SIZE+1] != (crc & 0x00FF) || 
           data[7+LASER_DATA_BUFFER_SIZE+2] != ((crc & 0xFF00) >> 8) ) {
        printf("crc incorrect: expected 0x%.2x 0x%.2x got 0x%.2x 0x%.2x - ignoring scan\n",
		 ((crc & 0xFF00) >> 8), (crc & 0x00FF), data[730], data[731] );
        continue;
      }

      for( c=7; c<7+LASER_DATA_BUFFER_SIZE; c+=sizeof(unsigned short) ) 
      {
        // check to see if laser is dazzled
        if ( (data[c+1] & 0x20) == 0x20 ) {
          puts("Laser dazzled - ignoring scan");
          continue;
        }

        // mask b to strip off the status bits 13-15
        data[c+1] &= 0x1F;     
      }

      /* test if we are supposed to cancel */
      pthread_testcancel();

      // RTV
      ld->GetLock()->PutData( ld, data );
      //ld->lock.PutData( ld, data );
      // !RTV
    }
  }
}

void CLaserDevice::Run() 
{
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create( &thread, &attr, &RunLaserThread, this );
}

int CLaserDevice::GetData( unsigned char *dest ) 
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

void CLaserDevice::PutData( unsigned char *src ) {
  memcpy( data, src+7, LASER_DATA_BUFFER_SIZE );
  data_swapped = false;

  // RTV
  //for(int i=0;i<LASER_DATA_BUFFER_SIZE;i+=sizeof(unsigned short))
    //*(unsigned short*)&data[i] = htons( *(unsigned short*)&data[i] );
  // !RTV
}

void CLaserDevice::GetCommand( unsigned char *dest ) 
{
}

void CLaserDevice::PutCommand( unsigned char *src ,int size) 
{
}

int CLaserDevice::GetConfig( unsigned char *dest ) 
{
  if(config_size)
  {
    memcpy(dest, config, config_size);
  }
  return(config_size);
}

void CLaserDevice::PutConfig( unsigned char *src ,int size) 
{
  if(size > LASER_CONFIG_BUFFER_SIZE)
    puts("CLaserDevice::PutConfig(): config request too big. ignoring");
  else
  {
    memcpy(config, src, size);
    config_size = size;
  }
}

