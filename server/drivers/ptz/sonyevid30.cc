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
 * methods for initializing, commanding, and getting data out of
 * the Sony EVI-D30 PTZ camera
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

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

#define PTZ_SLEEP_TIME_USEC 100000

#define MAX_PTZ_PACKET_LENGTH 16
#define MAX_PTZ_MESSAGE_LENGTH 14
#define MAX_PTZ_REPLY_LENGTH 11

/* here we calculate our conversion factors.
 *  0x370 is max value for the PTZ pan command, and in the real
 *    world, it has +- 25.0 range.
 *  0x12C is max value for the PTZ tilt command, and in the real
 *    world, it has +- 100.0 range.
 */
#define PTZ_PAN_MAX 100.0
#define PTZ_TILT_MAX 25.0
#define PTZ_PAN_CONV_FACTOR (0x370 / PTZ_PAN_MAX)
#define PTZ_TILT_CONV_FACTOR (0x12C / PTZ_TILT_MAX)

#define DEFAULT_PTZ_PORT "/dev/ttyS2"

class SonyEVID30:public CDevice 
{
 private:
  bool command_pending1;  // keep track of how many commands are pending;
  bool command_pending2;  // that way, we can cancel them if necessary
  bool ptz_fd_blocking;
  
  // internal methods
  int Send(unsigned char* str, int len, unsigned char* reply);
  int Receive(unsigned char* reply);
  int SendCommand(unsigned char* str, int len);
  int CancelCommand(char socket);
  int SendRequest(unsigned char* str, int len, unsigned char* reply);

  // this function will be run in a separate thread
  virtual void Main();

  virtual int SendAbsPanTilt(short pan, short tilt);
  virtual int SendAbsZoom(short zoom);
  virtual int GetAbsZoom(short* zoom);
  virtual int GetAbsPanTilt(short* pan, short* tilt);
  virtual void PrintPacket(char* str, unsigned char* cmd, int len);

 public:
  player_ptz_cmd_t* command;
  player_ptz_data_t* data;

  int ptz_fd; // ptz device file descriptor
  /* device used to communicate with the ptz */
  char ptz_serial_port[MAX_FILENAME_SIZE];

  // Min and max values for camera field of view (degrees).
  // These are used to compute appropriate zoom values.
  int maxfov, minfov;

  SonyEVID30(char* interface, ConfigFile* cf, int section);

  virtual int Setup();
  virtual int Shutdown();
};
  
// initialization function
CDevice* SonyEVID30_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_PTZ_STRING))
  {
    PLAYER_ERROR1("driver \"sonyevid30\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new SonyEVID30(interface, cf, section)));
}

// a driver registration function
void 
SonyEVID30_Register(DriverTable* table)
{
  table->AddDriver("sonyevid30", PLAYER_ALL_MODE, SonyEVID30_Init);
}

SonyEVID30::SonyEVID30(char* interface, ConfigFile* cf, int section) :
  CDevice(sizeof(player_ptz_data_t),sizeof(player_ptz_cmd_t),0,0)
{
  ptz_fd = -1;
  command_pending1 = false;
  command_pending2 = false;
  player_ptz_data_t data;
  player_ptz_cmd_t cmd;

  // TODO: check field of view values.
  this->minfov = (int) cf->ReadTupleAngle(section, "fov", 0, 3);
  this->maxfov = (int) cf->ReadTupleAngle(section, "fov", 1, 30);
  
  data.pan = data.tilt = data.zoom = 0;
  cmd.pan = cmd.tilt = cmd.zoom = 0;

  PutData((unsigned char*)&data,sizeof(data),0,0);
  PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));

  strncpy(ptz_serial_port,
          cf->ReadString(section, "port", DEFAULT_PTZ_PORT),
          sizeof(ptz_serial_port));
}

int 
SonyEVID30::Setup()
{
  struct termios term;
  short pan,tilt;
  int flags;

  player_ptz_cmd_t cmd;
  cmd.pan = cmd.tilt = 0;
  cmd.zoom = this->maxfov;

  printf("PTZ connection initializing (%s)...", ptz_serial_port);
  fflush(stdout);

  // open it.  non-blocking at first, in case there's no ptz unit.
  if((ptz_fd = open(ptz_serial_port, O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 )
  {
    perror("SonyEVID30::Setup():open():");
    return(-1);
  }  
 
  if(tcflush(ptz_fd, TCIFLUSH ) < 0 )
  {
    perror("SonyEVID30::Setup():tcflush():");
    close(ptz_fd);
    ptz_fd = -1;
    return(-1);
  }
  if(tcgetattr(ptz_fd, &term) < 0 )
  {
    perror("SonyEVID30::Setup():tcgetattr():");
    close(ptz_fd);
    ptz_fd = -1;
    return(-1);
  }
  
#if HAVE_CFMAKERAW
  cfmakeraw(&term);
#endif
  cfsetispeed(&term, B9600);
  cfsetospeed(&term, B9600);
  
  if(tcsetattr(ptz_fd, TCSAFLUSH, &term) < 0 )
  {
    perror("SonyEVID30::Setup():tcsetattr():");
    close(ptz_fd);
    ptz_fd = -1;
    return(-1);
  }

  ptz_fd_blocking = false;
  /* try to get current state, just to make sure we actually have a camera */
  if(GetAbsPanTilt(&pan,&tilt))
  {
    printf("Couldn't connect to PTZ device most likely because the camera\n"
                    "is not connected or is connected not to %s\n", 
                    ptz_serial_port);
    close(ptz_fd);
    ptz_fd = -1;
    return(-1);
  }

  /* ok, we got data, so now set NONBLOCK, and continue */
  if((flags = fcntl(ptz_fd, F_GETFL)) < 0)
  {
    perror("SonyEVID30::Setup():fcntl()");
    close(ptz_fd);
    ptz_fd = -1;
    return(1);
  }
  if(fcntl(ptz_fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
  {
    perror("SonyEVID30::Setup():fcntl()");
    close(ptz_fd);
    ptz_fd = -1;
    return(1);
  }
  ptz_fd_blocking = true;
  puts("Done.");

  // zero the command buffer
  PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));

  // start the thread to talk with the camera
  StartThread();

  return(0);
}

int 
SonyEVID30::Shutdown()
{
  puts("SonyEVID30::Shutdown");

  if(ptz_fd == -1)
    return(0);

  StopThread();

  // put the camera back to center
  usleep(PTZ_SLEEP_TIME_USEC);
  SendAbsPanTilt(0,0);
  usleep(PTZ_SLEEP_TIME_USEC);
  SendAbsZoom(0);

  if(close(ptz_fd))
    perror("SonyEVID30::Shutdown():close():");
  ptz_fd = -1;
  puts("PTZ camera has been shutdown");
  return(0);
}

int
SonyEVID30::Send(unsigned char* str, int len, unsigned char* reply)
{
  unsigned char command[MAX_PTZ_PACKET_LENGTH];
  int i;

  if(len > MAX_PTZ_MESSAGE_LENGTH)
  {
    fprintf(stderr, "SonyEVID30::Send(): message is too large (%d bytes)\n",
                    len);
    return(-1);
  }

  command[0] = 0x81; // controller address is 0, camera address 1
  for(i=0;i<len;i++)
    command[i+1] = str[i];

  command[i+1] = 0xFF;  // packet terminator

  //PrintPacket("Sending", command, i+2);
  
  // send the command
  if(write(ptz_fd, command, i+2) < 0)
  {
    perror("SonyEVID30::Send():write():");
    return(-1);
  }

  //puts("Send(): calling Receive()");
  return(Receive(reply));
}

int
SonyEVID30::Receive(unsigned char* reply)
{
  static unsigned char buffer[MAX_PTZ_PACKET_LENGTH];
  static int numread = 0;

  unsigned char temp_reply[MAX_PTZ_PACKET_LENGTH];
  int newnumread = 0;
  int bufptr = -1;
  int i;
  int temp;
  
  // if we're non-blocking, then we should wait a bit to give the
  // camera a chance to respond. 
  if(!ptz_fd_blocking)
    usleep(PTZ_SLEEP_TIME_USEC);

  memset(temp_reply,0,MAX_PTZ_PACKET_LENGTH);
  memset(reply,0,MAX_PTZ_PACKET_LENGTH);
  if(numread > 0)
  {
    //printf("copying %d old bytes\n", numread);
    memcpy(temp_reply,buffer,numread);
    // look for the terminator
    for(i=0;i<numread;i++)
    {
      if(temp_reply[i] == 0xFF)
      {
        bufptr = i;
        break;
      }
    }
  }

  while(bufptr < 0)
  {
    newnumread = read(ptz_fd, temp_reply+numread, MAX_PTZ_REPLY_LENGTH-numread);
    if((numread += newnumread) < 0)
    {
      perror("SonyEVID30::Send():read():");
      return(-1);
    }
    else if(!newnumread)
    {
      // hmm...we were expecting something, yet we read
      // zero bytes. some glitch.  drain input, and return
      // zero.  we'll get a message next time through
      //puts("Receive(): read() returned 0");
      if(tcflush(ptz_fd, TCIFLUSH ) < 0 )
      {
        perror("SonyEVID30::Send():tcflush():");
        return(-1);
      }
      numread = 0;
      return(0);
    }
    // look for the terminator
    for(i=0;i<numread;i++)
    {
      if(temp_reply[i] == 0xFF)
      {
        bufptr = i;
        break;
      }
    }
  }

  temp = numread;
  // if we read extra bytes, keep them around
  if(bufptr == numread-1)
    numread = 0;
  else
  {
    //printf("storing %d bytes\n", numread-(bufptr+1));
    memcpy(buffer,temp_reply+bufptr+1,numread-(bufptr+1));
    numread = numread-(bufptr+1);
  }

  //PrintPacket("Really Received", temp_reply, temp);
  //PrintPacket("Received", temp_reply, bufptr+1);
  
  // strip off leading trash, up to start character 0x90
  for(i = 0;i< bufptr;i++)
  {
    if(temp_reply[i] == 0x90 && temp_reply[i+1] != 0x90)
      break;
  }
  //if(i)
    //printf("SonyEVID30::Receive(): strip off zeros up to: %d\n", i);
  if(i == bufptr)
    return(0);
  memcpy(reply,temp_reply+i,bufptr+1-i);

  // if it's a command completion, record it, then go again
  if((reply[0] == 0x90) && ((reply[1] >> 4) == 0x05) && (reply[2] == 0xFF))
  {
    //puts("got command completion");
    if((reply[1] & 0x0F) == 0x01)
      command_pending1 = false;
    else if((reply[1] & 0x0F) == 0x02)
      command_pending2 = false;
  }

  return(bufptr+1-i);
}

int
SonyEVID30::CancelCommand(char socket)
{
  unsigned char command[MAX_PTZ_MESSAGE_LENGTH];
  unsigned char reply[MAX_PTZ_MESSAGE_LENGTH];
  int reply_len;
  
  //printf("Canceling socket %d\n", socket);

  command[0] = socket;
  command[0] |= 0x20;
  
  if((reply_len = Send(command, 1, reply)) <= 0)
    return(reply_len);
  
  // wait for the response
  while((reply[0] != 0x90) || ((reply[1] >> 4) != 0x06) || 
        !((reply[2] == 0x04) || (reply[2] == 0x05)) || (reply_len != 4))
  {
    if((reply[0] != 0x90) || ((reply[1] >> 4) != 0x05) || (reply[2] != 0xFF))
      PrintPacket("SonyEVID30::CancelCommand(): unexpected response",reply,
                      reply_len);
    //puts("CancelCommand(): calling Receive()");
    if((reply_len = Receive(reply)) <= 0)
      return(reply_len);
  }
  if(socket == 1)
    command_pending1 = false;
  else if(socket == 2)
    command_pending2 = false;
  return(0);
}

int 
SonyEVID30::SendCommand(unsigned char* str, int len)
{
  unsigned char reply[MAX_PTZ_PACKET_LENGTH];
  int reply_len;

  if(command_pending1 && command_pending2)
  {
    if((command_pending1 && CancelCommand(1)) || 
       (command_pending2 && CancelCommand(2)))
      return(-1);
  }

  if(command_pending1 && command_pending2)
  {
    puts("2 commands still pending. wait");
    return(-1);
  }

  if((reply_len = Send(str, len, reply)) <= 0)
    return(reply_len);
  
  // wait for the ACK
  while((reply[0] != 0x90) || ((reply[1] >> 4) != 0x04) || (reply_len != 3))
  {
    if((reply[0] != 0x90) || ((reply[1] >> 4) != 0x05) || (reply_len != 3))
    {
      PrintPacket("SonyEVID30::SendCommand(): expected ACK, but got", 
                      reply,reply_len);
    }
    //puts("SendCommand(): calling Receive()");
    if((reply_len = Receive(reply)) <= 0)
      return(reply_len);
  }
  
  if((reply[1] & 0x0F) == 0x01)
    command_pending1 = true;
  else if((reply[1] & 0x0F) == 0x02)
    command_pending2 = true;
  else
    fprintf(stderr,"SonyEVID30::SendCommand():got ACK for socket %d\n",
                    reply[1] & 0x0F);

  return(0);
}


int 
SonyEVID30::SendRequest(unsigned char* str, int len, unsigned char* reply)
{
  int reply_len;

  if((reply_len = Send(str, len, reply)) <= 0)
    return(reply_len);
  
  // check that it's an information return
  while((reply[0] != 0x90) || (reply[1] != 0x50))
  {
    if((reply[0] != 0x90) || ((reply[1] >> 4) != 0x05) || (reply_len != 3))
    {
      PrintPacket("SonyEVID30::SendCommand(): expected information return, but got", 
                    reply,reply_len);
    }
    //puts("SendRequest(): calling Receive()");
    if((reply_len = Receive(reply)) <= 0)
      return(reply_len);
  }

  return(reply_len);
}

int
SonyEVID30::SendAbsPanTilt(short pan, short tilt)
{
  unsigned char command[MAX_PTZ_MESSAGE_LENGTH];
  short convpan,convtilt;

  if (abs(pan)>(short)PTZ_PAN_MAX) 
  {
    if(pan<(short)-PTZ_PAN_MAX)
      pan=(short)-PTZ_PAN_MAX;
    else if(pan>(short)PTZ_PAN_MAX)
      pan=(short)PTZ_PAN_MAX;
    puts("Camera pan angle thresholded");
  }

  if(abs(tilt)>(short)PTZ_TILT_MAX) 
  {
    if(tilt<(short)-PTZ_TILT_MAX)
      tilt=(short)-PTZ_TILT_MAX;
    else if(tilt>(short)PTZ_TILT_MAX)
      tilt=(short)PTZ_TILT_MAX;
    puts("Camera tilt angle thresholded");
  }

  convpan = (short)(pan*PTZ_PAN_CONV_FACTOR);
  convtilt = (short)(tilt*PTZ_TILT_CONV_FACTOR);

  command[0] = 0x01;  // absolute position command
  command[1] = 0x06;  // absolute position command
  command[2] = 0x02;  // absolute position command
  command[3] = 0x18;  // MAX pan speed
  command[4] = 0x14;  // MAX tilt speed
  // pan position
  command[5] =  (unsigned char)((convpan & 0xF000) >> 12); 
  command[6] = (unsigned char)((convpan & 0x0F00) >> 8);
  command[7] = (unsigned char)((convpan & 0x00F0) >> 4);
  command[8] = (unsigned char)(convpan & 0x000F); 
  // tilt position
  command[9] = (unsigned char)((convtilt & 0xF000) >> 12); 
  command[10] = (unsigned char)((convtilt & 0x0F00) >> 8);
  command[11] = (unsigned char)((convtilt & 0x00F0) >> 4);
  command[12] = (unsigned char)(convtilt & 0x000F); 

  return(SendCommand(command, 13));
}

int
SonyEVID30::GetAbsPanTilt(short* pan, short* tilt)
{
  unsigned char command[MAX_PTZ_MESSAGE_LENGTH];
  unsigned char reply[MAX_PTZ_PACKET_LENGTH];
  int reply_len;
  short convpan, convtilt;

  command[0] = 0x09;
  command[1] = 0x06;
  command[2] = 0x12;
  
  if((reply_len = SendRequest(command,3,reply)) <= 0)
    return(reply_len);

  // first two bytes are header (0x90 0x50)
  
  // next 4 are pan
  convpan = reply[5];
  convpan |= (reply[4] << 4);
  convpan |= (reply[3] << 8);
  convpan |= (reply[2] << 12);

  *pan = (short)(convpan / PTZ_PAN_CONV_FACTOR);
  
  // next 4 are tilt
  convtilt = reply[9];
  convtilt |= (reply[8] << 4);
  convtilt |= (reply[7] << 8);
  convtilt |= (reply[6] << 12);

  *tilt = (short)(convtilt / PTZ_TILT_CONV_FACTOR);

  return(0);
}

int
SonyEVID30::GetAbsZoom(short* zoom)
{
  unsigned char command[MAX_PTZ_MESSAGE_LENGTH];
  unsigned char reply[MAX_PTZ_PACKET_LENGTH];
  int reply_len;

  command[0] = 0x09;
  command[1] = 0x04;
  command[2] = 0x47;

  if((reply_len = SendRequest(command,3,reply)) <= 0)
    return(reply_len);

  // first two bytes are header (0x90 0x50)
  // next 4 are zoom
  *zoom = reply[5];
  *zoom |= (reply[4] << 4);
  *zoom |= (reply[3] << 8);
  *zoom |= (reply[2] << 12);

  return(0);
}

int
SonyEVID30::SendAbsZoom(short zoom)
{
  unsigned char command[MAX_PTZ_MESSAGE_LENGTH];

  if(zoom<0) {
    zoom=0;
    //puts("Camera zoom thresholded");
  }
  else if(zoom>1023){
    zoom=1023;
    //puts("Camera zoom thresholded");
  }

  command[0] = 0x01;  // absolute position command
  command[1] = 0x04;  // absolute position command
  command[2] = 0x47;  // absolute position command
  
  // zoom position
  command[3] =  (unsigned char)((zoom & 0xF000) >> 12); 
  command[4] = (unsigned char)((zoom & 0x0F00) >> 8);
  command[5] = (unsigned char)((zoom & 0x00F0) >> 4);
  command[6] = (unsigned char)(zoom & 0x000F); 

  return(SendCommand(command, 7));
}

void
SonyEVID30::PrintPacket(char* str, unsigned char* cmd, int len)
{
  printf("%s: ", str);
  for(int i=0;i<len;i++)
    printf(" %.2x", cmd[i]);
  puts("");
}

// this function will be run in a separate thread
void 
SonyEVID30::Main()
{
  player_ptz_data_t data;
  player_ptz_cmd_t command;
  short pan,tilt,zoom;
  short pandemand=0, tiltdemand=0, zoomdemand=0;
  bool newpantilt=true, newzoom=true;

  while(1) 
  {
    pthread_testcancel();
    GetCommand((unsigned char*)&command, sizeof(player_ptz_cmd_t));
    pthread_testcancel();

    if(pandemand != (short)ntohs((unsigned short)(command.pan)))
    {
      pandemand = (short)ntohs((unsigned short)(command.pan));
      newpantilt = true;
    }
    if(tiltdemand != (short)ntohs((unsigned short)(command.tilt)))
    {
      tiltdemand = (short)ntohs((unsigned short)(command.tilt));
      newpantilt = true;
    }
    if(zoomdemand != (short)ntohs((unsigned short)(command.zoom)))
    {
      zoomdemand = (short) ntohs((unsigned short)(command.zoom));
      newzoom = true;
    }

    // Do some coordinate transformatiopns.  Pan value must be
    // negated.  The zoom value must be converted from a field of view
    // (in degrees) into arbitrary Sony PTZ units.
    pandemand = -pandemand;
    zoomdemand = (1024 * (zoomdemand - this->maxfov)) / (this->minfov - this->maxfov);

    if(newpantilt)
    {
      if(SendAbsPanTilt(pandemand,tiltdemand))
      {
        fputs("SonyEVID30:Main():SendAbsPanTilt() errored. bailing.\n", stderr);
        pthread_exit(NULL);
      }
    }
    if(newzoom)
    {
      if(SendAbsZoom(zoomdemand))
      {
        fputs("SonyEVID30:Main():SendAbsZoom() errored. bailing.\n", stderr);
        pthread_exit(NULL);
      }
    }

    /* get current state */
    if(GetAbsPanTilt(&pan,&tilt))
    {
      fputs("SonyEVID30:Main():GetAbsPanTilt() errored. bailing.\n", stderr);
      pthread_exit(NULL);
    }
    /* get current state */
    if(GetAbsZoom(&zoom))
    {
      fputs("SonyEVID30:Main():GetAbsZoom() errored. bailing.\n", stderr);
      pthread_exit(NULL);
    }

    // Do the necessary coordinate conversions.  Camera's natural pan
    // coordinates increase clockwise; we want them the other way, so
    // we negate pan here.  Zoom values are converted from arbitrary
    // units to a field of view (in degrees).
    pan = -pan;
    zoom = this->maxfov + (zoom * (this->minfov - this->maxfov)) / 1024; 

    // Copy the data.
    data.pan = htons((unsigned short)pan);
    data.tilt = htons((unsigned short)tilt);
    data.zoom = htons((unsigned short)zoom);

    /* test if we are supposed to cancel */
    pthread_testcancel();
    PutData((unsigned char*)&data, sizeof(player_ptz_data_t),0,0);

    newpantilt = false;
    newzoom = false;
    
    usleep(PTZ_SLEEP_TIME_USEC);
  }
}

