/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  Brian Gerkey gerkey@robotics.stanford.edu
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
 * Driver for controlling the Amtec PowerCube Wrist, a powerful pan-tilt
 * unit that can, for example, carry a SICK laser.
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

#define AMTEC_DEFAULT_PORT "/dev/ttyS0"
#define AMTEC_DEFAULT_VEL_DEG_PER_SEC 40
#define AMTEC_DEFAULT_MIN_PAN_DEG -90
#define AMTEC_DEFAULT_MAX_PAN_DEG 90

#define AMTEC_SLEEP_TIME_USEC 20000

// start, end, and escape chars
#define AMTEC_STX       0x02
#define AMTEC_ETX       0x03
#define AMTEC_DLE       0x10

// sizes
#define AMTEC_MAX_CMDSIZE     48

// module IDs
#define AMTEC_MODULE_TILT       11
#define AMTEC_MODULE_PAN        12

// command IDs
#define AMTEC_CMD_RESET        0x00
#define AMTEC_CMD_HOME         0x01
#define AMTEC_CMD_HALT         0x02
#define AMTEC_CMD_SET_EXT      0x08
#define AMTEC_CMD_GET_EXT      0x0a
#define AMTEC_CMD_SET_MOTION   0x0b
#define AMTEC_CMD_SET_ISTEP    0x0d

// parameter IDs
#define AMTEC_PARAM_ACT_POS   0x3c
#define AMTEC_PARAM_MIN_POS   0x45
#define AMTEC_PARAM_MAX_POS   0x46

// motion IDs
#define AMTEC_MOTION_FRAMP       4
#define AMTEC_MOTION_FRAMP_ACK  14
#define AMTEC_MOTION_FSTEP_ACK  16
#define AMTEC_MOTION_FVEL_ACK   17

class AmtecPowerCube:public CDevice 
{
  private:
    // this function will be run in a separate thread
    virtual void Main();
    bool fd_blocking;
    int return_to_home;
    int target_vel_degpersec;
    int minpan, maxpan;

    int SendCommand(int id, unsigned char* cmd, size_t len);
    int WriteData(unsigned char *buf, size_t len);

    int GetAbsPanTilt(short* pan, short* tilt);
    int SetAbsPan(short oldpan, short pan);
    int SetAbsTilt(short oldtilt, short tilt);
    int Home();
    int Halt();
    int Reset();

    int AwaitAnswer(unsigned char* buf, size_t len);
    int AwaitETX(unsigned char* buf, size_t len);
    int ReadAnswer(unsigned char* buf, size_t len);
    size_t ConvertBuffer(unsigned char* buf, size_t len);

    float BytesToFloat(unsigned char *bytes);
    void FloatToBytes(unsigned char *bytes, float f);
    void Uint16ToBytes(unsigned char *bytes, unsigned short s);

  public:
    int fd; // amtec device file descriptor
    /* device used to communicate with the ptz */
    const char* serial_port;

    AmtecPowerCube(char* interface, ConfigFile* cf, int section);

    virtual int Setup();
    virtual int Shutdown();
};

// initialization function
CDevice* AmtecPowerCube_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_PTZ_STRING))
  {
    PLAYER_ERROR1("driver \"amtecpowercube\" does not support interface "
                  "\"%s\"\n", interface);
    return(NULL);
  }
  else
    return((CDevice*)(new AmtecPowerCube(interface, cf, section)));
}

// a driver registration function
void 
AmtecPowerCube_Register(DriverTable* table)
{
  table->AddDriver("amtecpowercube", PLAYER_ALL_MODE, AmtecPowerCube_Init);
}

AmtecPowerCube::AmtecPowerCube(char* interface, ConfigFile* cf, int section) :
  CDevice(sizeof(player_ptz_data_t),sizeof(player_ptz_cmd_t),0,0)
{
  fd = -1;
  player_ptz_data_t data;
  player_ptz_cmd_t cmd;

  data.pan = data.tilt = data.zoom = 0;
  cmd.pan = cmd.tilt = cmd.zoom = 0;

  PutData((unsigned char*)&data,sizeof(data),0,0);
  PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));

  serial_port = cf->ReadString(section, "port", AMTEC_DEFAULT_PORT);
  return_to_home = cf->ReadInt(section, "home", 0);
  target_vel_degpersec = cf->ReadInt(section, "speed",
                                  AMTEC_DEFAULT_VEL_DEG_PER_SEC);
}

int 
AmtecPowerCube::Reset()
{
  unsigned char buf[AMTEC_MAX_CMDSIZE];
  unsigned char cmd[1];

  cmd[0] = AMTEC_CMD_RESET;
  if(SendCommand(AMTEC_MODULE_PAN,cmd,1) < 0)
  {
    PLAYER_ERROR("SendCommand() failed");
    return(-1);
  }
  if(ReadAnswer(buf,sizeof(buf)) < 0)
  {
    PLAYER_ERROR("ReadAnswer() failed");
    return(-1);
  }
  if(SendCommand(AMTEC_MODULE_TILT,cmd,1) < 0)
  {
    PLAYER_ERROR("SendCommand() failed");
    return(-1);
  }
  if(ReadAnswer(buf,sizeof(buf)) < 0)
  {
    PLAYER_ERROR("ReadAnswer() failed");
    return(-1);
  }
  return(0);
}

int 
AmtecPowerCube::Home()
{
  unsigned char buf[AMTEC_MAX_CMDSIZE];
  unsigned char cmd[1];

  cmd[0] = AMTEC_CMD_HOME;
  if(SendCommand(AMTEC_MODULE_PAN,cmd,1) < 0)
  {
    PLAYER_ERROR("SendCommand() failed");
    return(-1);
  }
  if(ReadAnswer(buf,sizeof(buf)) < 0)
  {
    PLAYER_ERROR("ReadAnswer() failed");
    return(-1);
  }
  if(SendCommand(AMTEC_MODULE_TILT,cmd,1) < 0)
  {
    PLAYER_ERROR("SendCommand() failed");
    return(-1);
  }
  if(ReadAnswer(buf,sizeof(buf)) < 0)
  {
    PLAYER_ERROR("ReadAnswer() failed");
    return(-1);
  }
  return(0);
}

int 
AmtecPowerCube::Halt()
{
  unsigned char buf[AMTEC_MAX_CMDSIZE];
  unsigned char cmd[1];

  cmd[0] = AMTEC_CMD_HALT;
  if(SendCommand(AMTEC_MODULE_PAN,cmd,1) < 0)
  {
    PLAYER_ERROR("SendCommand() failed");
    return(-1);
  }
  if(ReadAnswer(buf,sizeof(buf)) < 0)
  {
    PLAYER_ERROR("ReadAnswer() failed");
    return(-1);
  }
  if(SendCommand(AMTEC_MODULE_TILT,cmd,1) < 0)
  {
    PLAYER_ERROR("SendCommand() failed");
    return(-1);
  }
  if(ReadAnswer(buf,sizeof(buf)) < 0)
  {
    PLAYER_ERROR("ReadAnswer() failed");
    return(-1);
  }
  return(0);
}

int 
AmtecPowerCube::Setup()
{
  struct termios term;
  short pan,tilt;
  int flags;

  player_ptz_cmd_t cmd;
  cmd.pan = cmd.tilt = cmd.zoom = 0;

  printf("Amtec PowerCube connection initializing (%s)...", serial_port);
  fflush(stdout);

  // open it.  non-blocking at first, in case there's no ptz unit.
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
  cfsetispeed(&term, B38400);
  cfsetospeed(&term, B38400);
  
  if(tcsetattr(fd, TCSAFLUSH, &term) < 0 )
  {
    PLAYER_ERROR1("tcsetattr() failed: %s", strerror(errno));
    close(fd);
    fd = -1;
    return(-1);
  }

  fd_blocking = false;
  /* try to get current state, just to make sure we actually have a camera */
  if(GetAbsPanTilt(&pan,&tilt))
  {
    printf("Couldn't connect to Amtec PowerCube most likely because the unit\n"
                    "is not connected or is connected not to %s\n", 
                    serial_port);
    close(fd);
    fd = -1;
    return(-1);
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

  // start the thread to talk with the camera
  StartThread();

  return(0);
}

int
AmtecPowerCube::Shutdown()
{
  if(fd == -1)
    return(0);

  StopThread();

  // stop the unit
  if(Halt())
    PLAYER_WARN("Halt() failed.");

  // maybe return it to home
  if(return_to_home && Home())
    PLAYER_WARN("Home() failed.");

  if(close(fd))
    PLAYER_ERROR1("close() failed:%s",strerror(errno));
  fd = -1;
  puts("Amtec PowerCube has been shutdown");
  return(0);
}

////////////////////////////////////////////////////////////////////////////
// The following methods are based on some found in CARMEN.  Thanks to the
// authors.

// NOTE: these conversion methods only work on little-endian machines
// (the Amtec protocol also uses little-endian).
float
AmtecPowerCube::BytesToFloat(unsigned char *bytes)
{
  float f;
  memcpy((void*)&f, bytes, 4);
  return(f);
}
void
AmtecPowerCube::FloatToBytes(unsigned char *bytes, float f)
{
  memcpy(bytes, (void*)&f, 4);
}
void
AmtecPowerCube::Uint16ToBytes(unsigned char *bytes, unsigned short s)
{
  memcpy(bytes, (void*)&s, 2);
}

int 
AmtecPowerCube::SendCommand(int id, unsigned char* cmd, size_t len)
{
  size_t i;
  int ctr, add;
  unsigned char rcmd[AMTEC_MAX_CMDSIZE];
  unsigned char bcc;
  unsigned char umnr;
  unsigned char lmnr;

  add  = 0;
  lmnr = id & 7;
  lmnr = lmnr << 5;
  umnr = id >> 3;
  umnr = umnr | 4;
  for (i=0;i<len;i++) {
    if ( (cmd[i]==0x02) ||
	 (cmd[i]==0x03) ||
	 (cmd[i]==0x10) ) {
      add++;
    }
  }
  lmnr = lmnr + len;
  rcmd[0] = AMTEC_STX;
  rcmd[1] = umnr;
  rcmd[2] = lmnr;
  ctr = 3;
  for (i=0;i<len;i++) {
    switch(cmd[i]) {
    case 0x02:
      rcmd[ctr] = 0x10;
      rcmd[++ctr] = 0x82;
      break;
    case 0x03:
      rcmd[ctr] = 0x10;
      rcmd[++ctr] = 0x83;
      break;
    case 0x10:
      rcmd[ctr] = 0x10;
      rcmd[++ctr] = 0x90;
      break;
    default:
      rcmd[ctr] = cmd[i];
    }
    ctr++;
  }
  bcc = id;
  for (i=0;i<len;i++) {
    bcc += cmd[i];
  }
  bcc = bcc + (bcc>>8);
  switch(bcc) {
  case 0x02:
    rcmd[ctr++] = 0x10;
    rcmd[ctr++] = 0x82;
    break;
  case 0x03:
    rcmd[ctr++] = 0x10;
    rcmd[ctr++] = 0x83;
    break;
  case 0x10:
    rcmd[ctr++] = 0x10;
    rcmd[ctr++] = 0x90;
    break;
  default:
    rcmd[ctr++] = bcc;
  }
  rcmd[ctr++] = AMTEC_ETX;

  if(WriteData(rcmd, ctr) == ctr)
    return(0);
  else
  {
    PLAYER_ERROR("short write");
    return(-1);
  }
}

int
AmtecPowerCube::WriteData(unsigned char *buf, size_t len)
{
  size_t written = 0;
  int tmp = 0;

  while(written < len)
  {
    if((tmp = write(fd, buf, len)) < 0)
    {
      PLAYER_ERROR1("write() failed: %s", strerror(errno));
      return(-1);
    }

    written += tmp;
  }
  return(written);
}

int
AmtecPowerCube::AwaitETX(unsigned char* buf, size_t len)
{
  int pos, loop, numread, totalnumread;
  pos = 0; loop = 0;
  while(loop<10)
  {
    if((numread = read(fd,buf+pos,len-pos)) < 0)
    {
      PLAYER_ERROR1("read() failed:%s", strerror(errno));
      return(-1);
    }
    else if(!numread)
    {
      if(!fd_blocking)
        usleep(10000);
      loop++;
    }
    else
    {
      if(buf[pos+numread-1]==AMTEC_ETX)
      {
	totalnumread = pos+numread-1;
	return(totalnumread);
      }
      pos += numread;
    }
  }
  PLAYER_ERROR("never found ETX");
  return(-1);
}

int
AmtecPowerCube::AwaitAnswer(unsigned char* buf, size_t len)
{
  int numread;

  // if we're not blocking, give the unit some time to respond
  if(!fd_blocking)
    usleep(AMTEC_SLEEP_TIME_USEC);

  for(;;)
  {
    if((numread = read(fd, buf, 1)) < 0)
    {
      PLAYER_ERROR1("read() failed:%s", strerror(errno));
      return(-1);
    }
    else if(!numread)
    {
      // hmm...we were expecting something, yet we read
      // zero bytes. some glitch.  drain input, and return
      // zero.  we'll get a message next time through.
      PLAYER_WARN("read 0 bytes");
      if(tcflush(fd, TCIFLUSH ) < 0 )
      {
        PLAYER_ERROR1("tcflush() failed:%s",strerror(errno));
        return(-1);
      }
      return(0);
    }
    else
    {
      if(buf[0]==AMTEC_STX) 
        return(AwaitETX(buf,len));
      else
        continue;
    }
  }
}

size_t
AmtecPowerCube::ConvertBuffer(unsigned char* buf, size_t len)
{
  size_t i, j, actual_len;

  actual_len = len;

  for (i=0;i<len;i++) 
  {
    if(buf[i]==AMTEC_DLE) 
    {
      switch(buf[i+1]) 
      {
        case 0x82:
          buf[i] = 0x02;
          for(j=i+2;j<len;j++) 
            buf[j-1] = buf[j];
          actual_len--;
          break;
        case 0x83:
          buf[i] = 0x03;
          for(j=i+2;j<len;j++) 
            buf[j-1] = buf[j];
          actual_len--;
          break;
        case 0x90:
          buf[i] = 0x10;
          for(j=i+2;j<len;j++) 
            buf[j-1] = buf[j];
          actual_len--;
          break;
      }
    }
  }
  return(actual_len);
}

int
AmtecPowerCube::ReadAnswer(unsigned char* buf, size_t len)
{
  int actual_len;

  if((actual_len = AwaitAnswer(buf, len)) <= 0)
    return(actual_len);
  else
    return((int)ConvertBuffer(buf, (size_t)actual_len));
}
// The preceding methods are based some found in CARMEN.  Thanks to the
// authors.
////////////////////////////////////////////////////////////////////////////

int
AmtecPowerCube::GetAbsPanTilt(short* pan, short* tilt)
{
  unsigned char buf[AMTEC_MAX_CMDSIZE];
  unsigned char cmd[2];

  cmd[0] = AMTEC_CMD_GET_EXT;
  cmd[1] = AMTEC_PARAM_ACT_POS;

  // get the pan
  if(SendCommand(AMTEC_MODULE_PAN, cmd, 2) < 0)
  {
    PLAYER_ERROR("SendCommand() failed");
    return(-1);
  }

  if(ReadAnswer(buf,sizeof(buf)) < 0)
  {
    PLAYER_ERROR("ReadAnswer() failed");
    return(-1);
  }

  // reverse pan angle, to increase ccw, then normalize
  *pan = -(short)RTOD(NORMALIZE(BytesToFloat(buf+4)));
  //printf("pan: %d\n", *pan);
  
  // get the tilt
  if(SendCommand(AMTEC_MODULE_TILT, cmd, 2) < 0)
  {
    PLAYER_ERROR("SendCommand() failed");
    return(-1);
  }

  if(ReadAnswer(buf,sizeof(buf)) < 0)
  {
    PLAYER_ERROR("ReadAnswer() failed");
    return(-1);
  }

  *tilt = (short)(RTOD(BytesToFloat(buf+4)));
  //printf("tilt: %d\n", *tilt);

  return(0);
}

int
AmtecPowerCube::SetAbsPan(short oldpan, short pan)
{
  unsigned char buf[AMTEC_MAX_CMDSIZE];
  unsigned char cmd[8];
  float newpan;
  unsigned short time;

  newpan = DTOR(pan);

  // compute time, in milliseconds, to achieve given target velocity
  time = (unsigned short)rint(((double)abs(pan - oldpan) / 
                               (double)target_vel_degpersec)
                              *1000.0);

  cmd[0] = AMTEC_CMD_SET_MOTION;
  /*
  cmd[1] = AMTEC_MOTION_FSTEP_ACK;
  FloatToBytes(cmd+2,newpan);
  Uint16ToBytes(cmd+6,time);
  */
  cmd[1] = AMTEC_MOTION_FVEL_ACK;
  FloatToBytes(cmd+2,newpan);
  printf("sending pan command: %d (%f)\n", pan, newpan);
  if(SendCommand(AMTEC_MODULE_PAN,cmd,8) < 0)
    return(-1);
  if(ReadAnswer(buf,sizeof(buf)) < 0)
    return(-1);
  printf("module state:0x%x\n", buf[6]);
  return(0);
}

int
AmtecPowerCube::SetAbsTilt(short oldtilt, short tilt)
{
  unsigned char buf[AMTEC_MAX_CMDSIZE];
  unsigned char cmd[8];
  float newtilt;

  newtilt = DTOR(tilt);

  cmd[0] = AMTEC_CMD_SET_MOTION;
  cmd[1] = AMTEC_MOTION_FRAMP;
  FloatToBytes(cmd+2,newtilt);
  if(SendCommand(AMTEC_MODULE_TILT,cmd,6) < 0)
    return(-1);
  if(ReadAnswer(buf,sizeof(buf)) < 0)
    return(-1);
  return(0);
}

void 
AmtecPowerCube::Main()
{
  player_ptz_data_t data;
  player_ptz_cmd_t command;
  short lastpan, lasttilt;
  short newpan, newtilt;
  short currpan, currtilt;

  // first things first.  reset and home the unit.
  if(Reset() < 0)
  {
    PLAYER_ERROR("Reset() failed; bailing.");
    pthread_exit(NULL);
  }
  if(return_to_home && (Home() < 0))
  {
    PLAYER_ERROR("Home() failed; bailing.");
    pthread_exit(NULL);
  }

  if(GetAbsPanTilt(&lastpan,&lasttilt) < 0)
  {
    PLAYER_ERROR("GetAbsPanTilt() failed; bailing.");
    pthread_exit(NULL);
  }

  for(;;)
  {
    pthread_testcancel();

    GetCommand((unsigned char*)&command, sizeof(player_ptz_cmd_t));
    // reverse pan angle, to increase ccw
    newpan = -(short)ntohs(command.pan);
    newtilt = (short)ntohs(command.tilt);

    if(newpan != lastpan)
    {
      // send new pan position
      if(SetAbsPan(lastpan,newpan))
      {
        PLAYER_ERROR("SetAbsPan() failed(); bailing.");
        pthread_exit(NULL);
      }

      lastpan = newpan;
    }

    if(newtilt != lasttilt)
    {
      // send new tilt position
      if(SetAbsTilt(lasttilt,newtilt))
      {
        PLAYER_ERROR("SetAbsTilt() failed(); bailing.");
        pthread_exit(NULL);
      }

      lasttilt = newtilt;
    }

    if(GetAbsPanTilt(&currpan,&currtilt))
    {
      PLAYER_ERROR("GetAbsPanTilt() failed(); bailing.");
      pthread_exit(NULL);
    }

    data.pan = htons(currpan);
    data.tilt = htons(currtilt);
    data.zoom = 0;

    PutData((unsigned char*)&data, sizeof(player_ptz_data_t),0,0);

    usleep(AMTEC_SLEEP_TIME_USEC);
  }
}

