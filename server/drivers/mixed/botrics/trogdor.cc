/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     Brian Gerkey
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
 * $Id$
 *
 * Driver for the so-called "Trogdor" robots, made by Botrics.  They're
 * small, very fast robots that carry SICK lasers (talk to the laser over a
 * normal serial port using the sicklms200 driver).
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */
#include <math.h>

#include <device.h>
#include <drivertable.h>
#include <player.h>

#define TROGDOR_DEFAULT_PORT "/dev/usb/ttyUSB1"

// might need to define a longer delay to wait for acks
#define TROGDOR_DELAY_US 10000

#define TROGDOR_ACK   6 // if command acknowledged
#define TROGDOR_NACK 21 // if garbled message

#define TROGDOR_INIT1 253 // The init commands are used in sequence(1,2,3)
#define TROGDOR_INIT2 252 // to initialize a link to a cerebellum.
#define TROGDOR_INIT3 251 // It will then blink green and start accepting other 
                  // commands.

#define TROGDOR_DEINIT 250

#define TROGDOR_SET_VELOCITIES 118 // 'v'(left_vel, right_vel) as 16-bit signed ints
#define TROGDOR_SET_ACCELERATIONS 97 // 'a'(left_accel, right_accel) as 16-bit unsigned ints
#define TROGDOR_ENABLE_VEL_CONTROL  101 // 'e'()
#define TROGDOR_DISABLE_VEL_CONTROL 100 // 'd'()
#define TROGDOR_GET_ODOM            111 // 'o'()->(left_count, right_count, left_vel, right_vel)
#define TROGDOR_GET_VOLTAGE          98 // 'b'()->(batt_voltage)
#define TROGDOR_STOP                115 // 's'()  [shortcut for set_velocities(0,0)]
#define TROGDOR_KILL                107 // 'k'()  [shortcut for disable_velocity_control]

#define TROGDOR_HEARTBEAT           104 // 'h'() sends keepalive

class Trogdor : public CDevice 
{
  private:
    // this function will be run in a separate thread
    virtual void Main();
    
    // bookkeeping
    bool fd_blocking;

    // methods for internal use
    int WriteBuf(unsigned char* s, size_t len);
    int ReadBuf(unsigned char* s, size_t len);
    int BytesToInt32(unsigned char *ptr);
    void Int32ToBytes(unsigned char* buf, int i);
    int ValidateChecksum(unsigned char *ptr, size_t len);
    int GetOdom(int *ltics, int *rtics, int *lvel, int *rvel);
    void UpdateOdom(int ltics, int rtics);
    unsigned char ComputeChecksum(unsigned char *ptr, size_t len);
    int SendCommand(unsigned char cmd, int val1, int val2);
    int SetVelocity(int lvel, int rvel);

  public:
    int fd; // device file descriptor
    const char* serial_port; // name of dev file

    Trogdor(char* interface, ConfigFile* cf, int section);

    virtual int Setup();
    virtual int Shutdown();
};


// initialization function
CDevice* Trogdor_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
  {
    PLAYER_ERROR1("driver \"trogdor\" does not support interface "
                  "\"%s\"\n", interface);
    return(NULL);
  }
  else
    return((CDevice*)(new Trogdor(interface, cf, section)));
}

// a driver registration function
void 
Trogdor_Register(DriverTable* table)
{
  table->AddDriver("trogdor", PLAYER_ALL_MODE, Trogdor_Init);
}

Trogdor::Trogdor(char* interface, ConfigFile* cf, int section) :
  CDevice(sizeof(player_position_data_t),sizeof(player_position_cmd_t),1,1)
{
  fd = -1;
  player_position_data_t data;
  player_position_cmd_t cmd;

  data.xpos = data.ypos = data.yaw = 0;
  data.xspeed = data.yspeed = data.yawspeed = 0;
  cmd.xpos = cmd.ypos = cmd.yaw = 0;
  cmd.xspeed = cmd.yspeed = cmd.yawspeed = 0;

  PutData((unsigned char*)&data,sizeof(data),0,0);
  PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));

  this->serial_port = cf->ReadString(section, "port", TROGDOR_DEFAULT_PORT);
}

int 
Trogdor::Setup()
{
  struct termios term;
  int flags;
  int ltics,rtics,lvel,rvel;
  player_position_cmd_t cmd;

  cmd.xpos = cmd.ypos = cmd.yaw = 0;
  cmd.xspeed = cmd.yspeed = cmd.yawspeed = 0;

  printf("Botrics Trogdor connection initializing (%s)...", serial_port);
  fflush(stdout);

  // open it.  non-blocking at first, in case there's no robot
  if((fd = open(serial_port, O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 )
  {
    PLAYER_ERROR1("open() failed: %s", strerror(errno));
    return(-1);
  }  
 
  if(tcflush(fd, TCIFLUSH ) < 0 )
  {
    PLAYER_ERROR1("tcflush() failed: %s", strerror(errno));
    close(fd);
    fd = -1;
    return(-1);
  }
  if(tcgetattr(fd, &term) < 0 )
  {
    PLAYER_ERROR1("tcgetattr() failed: %s", strerror(errno));
    close(fd);
    fd = -1;
    return(-1);
  }
  
#if HAVE_CFMAKERAW
  cfmakeraw(&term);
#endif
  cfsetispeed(&term, B57600);
  cfsetospeed(&term, B57600);
  
  if(tcsetattr(fd, TCSAFLUSH, &term) < 0 )
  {
    PLAYER_ERROR1("tcsetattr() failed: %s", strerror(errno));
    close(fd);
    fd = -1;
    return(-1);
  }

  fd_blocking = false;
  
  // initialize the robot
  unsigned char initstr[3];

  initstr[0] = TROGDOR_INIT1;
  initstr[1] = TROGDOR_INIT2;
  initstr[2] = TROGDOR_INIT3;

  if(WriteBuf(initstr,sizeof(initstr)) < 0)
  {
    PLAYER_ERROR("failed on write of initialization string");
    close(fd);
    fd = -1;
    return(1);
  }

  /* try to get current odometry, just to make sure we actually have a robot */
  if(GetOdom(&ltics,&rtics,&lvel,&rvel) < 0)
  {
    PLAYER_ERROR("failed to get odometry");
    close(fd);
    fd = -1;
    return(1);
  }

  /* ok, we got data, so now set NONBLOCK, and continue */
  if((flags = fcntl(fd, F_GETFL)) < 0)
  {
    PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
    close(fd);
    fd = -1;
    return(1);
  }
  if(fcntl(fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
  {
    PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
    close(fd);
    fd = -1;
    return(1);
  }
  fd_blocking = true;
  puts("Done.");

  // zero the command buffer
  PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));

  // start the thread to talk with the robot
  StartThread();

  return(0);
}

int
Trogdor::Shutdown()
{
  if(fd == -1)
    return(0);

  StopThread();

  SetVelocity(0,0);

  if(close(fd))
    PLAYER_ERROR1("close() failed:%s",strerror(errno));
  fd = -1;
  puts("Botrics Trogdor has been shutdown");
  return(0);
}

void 
Trogdor::Main()
{
  player_position_cmd_t command;
  player_position_data_t data;
  int lvel, rvel;
  int ltics, rtics;

  // TODO: push a pthread cleanup function that stops the robots

  for(;;)
  {
    pthread_testcancel();
    GetCommand((unsigned char*)&command, sizeof(player_ptz_cmd_t));

    // TODO: convert (tv,rv) to (lv,rv) and send to robot

    if(GetOdom(&ltics,&rtics,&lvel,&rvel) < 0)
    {
      PLAYER_ERROR("failed to get odometry");
      pthread_exit(NULL);
    }
  }
}

int
Trogdor::ReadBuf(unsigned char* s, size_t len)
{
  int thisnumread;
  size_t numread = 0;
  int bytesperread=1;

  // if we're not blocking, give the unit some time to respond
  if(!fd_blocking)
    usleep(TROGDOR_DELAY_US);
  while(numread < len)
  {
    // apparently the underlying PIC gets overwhelmed if we read too fast
    // wait...how can that be?
    if((thisnumread = read(this->fd,s+numread,bytesperread)) < 0)
    {
      PLAYER_ERROR1("read() failed: %s", strerror(errno));
      return(-1);
    }
    if(thisnumread == 0)
      PLAYER_WARN("short read");
    numread += thisnumread;
  }
  return(0);
}

int
Trogdor::WriteBuf(unsigned char* s, size_t len)
{
  int numwritten;
  int bytesperwrite = 1;
  unsigned char ack[1];

  // apparently the underlying PIC gets overwhelmed if we write too fast
  for(size_t i=0;i<len;i++)
  {
    if((numwritten = write(this->fd,s+i,bytesperwrite)) < 0)
    {
      PLAYER_ERROR1("write() failed: %s", strerror(errno));
      return(-1);
    }
    if(numwritten != bytesperwrite)
      PLAYER_WARN("short write");
  }

  // get acknowledgement
  if(ReadBuf(ack,1) < 0)
  {
    PLAYER_ERROR("failed to get acknowledgement");
    return(-1);
  }

  switch(ack[0])
  {
    case TROGDOR_ACK:
      return(0);
    case TROGDOR_NACK:
      PLAYER_WARN("got NACK");
      return(-1);
    default:
      PLAYER_WARN("got unknown value for acknowledgement");
      return(-1);
  }
}

int 
Trogdor::BytesToInt32(unsigned char *ptr)
{
  unsigned char char0,char1,char2,char3;
  int data = 0;

  char0 = ptr[0];
  char1 = ptr[1];
  char2 = ptr[2];
  char3 = ptr[3];

  data |=  ((int)char0)        & 0x000000FF;
  data |= (((int)char1) << 8)  & 0x0000FF00;
  data |= (((int)char2) << 16) & 0x00FF0000;
  data |= (((int)char3) << 24) & 0xFF000000;

  return data;
}

void
Trogdor::Int32ToBytes(unsigned char* buf, int i)
{
  buf[0] = (i >> 0) && 0xFF;
  buf[1] = (i >> 8)  && 0xFF;
  buf[2] = (i >> 16) && 0xFF;
  buf[3] = (i >> 24) && 0xFF;
}

int
Trogdor::GetOdom(int *ltics, int *rtics, int *lvel, int *rvel)
{
  unsigned char buf[20];
  int index;

  buf[0] = TROGDOR_GET_ODOM;
  if(WriteBuf(buf,1) < 0)
  {
    PLAYER_ERROR("failed to send command to retrieve odometry");
    return(-1);
  }
  usleep(TROGDOR_DELAY_US);
  
  // read 4 int32's, 1 error byte, and 1 checksum
  if(ReadBuf(buf, 18) < 0)
  {
    PLAYER_ERROR("failed to read odometry");
    return(-1);
  }

  if(ValidateChecksum(buf, 17) < 0)
  {
    PLAYER_ERROR("checksum failed on odometry packet");
    return(-1);
  }
  if(buf[16] == 1)
  {
    PLAYER_ERROR("Cerebellum error with encoder board");
    return(-1);
  }

  index = 0;
  *rtics = BytesToInt32(buf+index);
  index += 4;
  *ltics = BytesToInt32(buf+index);
  index += 4;
  *rvel = BytesToInt32(buf+index);
  index += 4;
  *lvel = BytesToInt32(buf+index);

  return(0);
}

void
Trogdor::UpdateOdom(int ltics, int rtics)
{
}

// Validate XOR checksum
int
Trogdor::ValidateChecksum(unsigned char *ptr, size_t len)
{
  size_t i;
  unsigned char checksum = 0;

  for(i = 0; i < len-1; i++)
    checksum ^= ptr[i];

  if(checksum == ptr[len-1])
    return(0);
  else
    return(-1);
}

// Compute XOR checksum
unsigned char
Trogdor::ComputeChecksum(unsigned char *ptr, size_t len)
{
  size_t i;
  unsigned char chksum = 0;

  for(i = 0; i < len; i++)
    chksum ^= ptr[i];

  return(chksum);
}

int
Trogdor::SendCommand(unsigned char cmd, int val1, int val2)
{
  unsigned char buf[10];
  int i;

  i=0;
  buf[i] = cmd;
  i+=1;
  Int32ToBytes(buf+i,val1);
  i+=4;
  Int32ToBytes(buf+i,val2);
  i+=4;
  buf[i] = ComputeChecksum(buf,i);

  if(WriteBuf(buf,10) < 0)
  {
    PLAYER_ERROR("failed to send command");
    return(-1);
  }

  return(0);
}

int
Trogdor::SetVelocity(int lvel, int rvel)
{
  if(SendCommand(TROGDOR_SET_VELOCITIES,lvel,rvel) < 0)
  {
    PLAYER_ERROR("failed to set velocities");
    return(-1);
  }
  return(0);
}

