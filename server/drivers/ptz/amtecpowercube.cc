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
 * unit.
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

#include <device.h>
#include <drivertable.h>
#include <player.h>

// assuming that we're using a usb->serial gagdet
#define DEFAULT_AMTEC_PORT "/dev/usb/tts/0"

// start, end, and escape chars
#define AMTEC_STX       0x02
#define AMTEC_ETX       0x03
#define AMTEC_DLE       0x10

// sizes
#define MAX_ACMD_SIZE     48

// command IDs
#define AMTEC_CMD_RESET        0x00
#define AMTEC_CMD_HOME         0x01
#define AMTEC_CMD_HALT         0x02
#define AMTEC_CMD_SET_EXT      0x08
#define AMTEC_CMD_GET_EXT      0x0a


// parameter IDs
#define AMTEC_GET_POS   0x3c

class AmtecPowerCube:public CDevice 
{
  private:
    // this function will be run in a separate thread
    virtual void Main();
    bool fd_blocking;

    int send_command(int id, unsigned char* cmd, size_t len);
    int write_data(unsigned char *buf, size_t len);

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

  serial_port = cf->ReadString(section, "port", DEFAULT_AMTEC_PORT);
}

int 
AmtecPowerCube::Setup()
{
  struct termios term;
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
  //if(GetAbsPanTilt(&pan,&tilt))
  if(0)
  {
    printf("Couldn't connect to PTZ device most likely because the camera\n"
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

// The following method is based on one found in CARMEN.  Thanks to the
// authors.
int 
AmtecPowerCube::send_command(int id, unsigned char* cmd, size_t len)
{
  size_t i;
  int ctr, add;
  unsigned char rcmd[MAX_ACMD_SIZE];
  unsigned char bcc;
  unsigned char umnr;
  unsigned char lmnr;

#ifdef IO_DEBUG
  fprintf( stderr, "\n---> " );
  for (i=0;i<len;i++) {
    fprintf( stderr, "(0x%s%x)", cmd[i]<16?"0":"", cmd[i] );
  }
  fprintf( stderr, "\n" );
#endif
  
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

#ifdef IO_DEBUG
  fprintf( stderr, "-*-> " );
  for (i=0;i<ctr;i++) {
    fprintf( stderr, "(0x%s%x)", rcmd[i]<16?"0":"", rcmd[i] );
  }
  fprintf( stderr, "\n" );
#endif
  
  if(write_data(rcmd, ctr)) 
    return(0);
  else
    return(-1);
}

// The following method is based on one found in CARMEN.  Thanks to the
// authors.
int
AmtecPowerCube::write_data(unsigned char *buf, size_t len)
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
  return(0);
}
