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
 *
 * Some of this code is borrowed and/or adapted from the 'cerebellum'
 * module of CARMEN; thanks to the authors of that module.
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

#include "trogdor_constants.h"

static void StopRobot(void* trogdordev);

class Trogdor : public CDevice 
{
  private:
    // this function will be run in a separate thread
    virtual void Main();
    
    // bookkeeping
    bool fd_blocking;
    double px, py, pa;  // integrated odometric pose (m,m,rad)
    int last_ltics, last_rtics;
    bool odom_initialized;

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
    int ComputeTickDiff(int from, int to);
    int ChangeMotorState(int state);
    int InitRobot();
    int GetBatteryVoltage(int* voltage);

  public:
    int fd; // device file descriptor
    const char* serial_port; // name of dev file

    // public, so that it can be called from pthread cleanup function
    int SetVelocity(int lvel, int rvel);

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
  this->serial_port = cf->ReadString(section, "port", TROGDOR_DEFAULT_PORT);
}

int
Trogdor::InitRobot()
{
  // initialize the robot
  unsigned char initstr[3];
  initstr[0] = TROGDOR_INIT1;
  initstr[1] = TROGDOR_INIT2;
  initstr[2] = TROGDOR_INIT3;
  unsigned char deinitstr[1];
  deinitstr[0] = TROGDOR_DEINIT;

  if(tcflush(this->fd, TCIOFLUSH) < 0 )
  {
    PLAYER_ERROR1("tcflush() failed: %s", strerror(errno));
    close(this->fd);
    this->fd = -1;
    return(-1);
  }

  if(WriteBuf(initstr,sizeof(initstr)) < 0)
  {
    PLAYER_WARN("failed to initialize robot; i'll try to de-initializate it");
    if(WriteBuf(deinitstr,sizeof(deinitstr)) < 0)
    {
      PLAYER_ERROR("failed on write of de-initialization string");
      return(-1);
    }
    if(WriteBuf(initstr,sizeof(initstr)) < 0)
    {
      PLAYER_ERROR("failed on 2nd write of initialization string; giving up");
      return(-1);
    }
  }
  return(0);
}

int 
Trogdor::Setup()
{
  struct termios term;
  int flags;
  int ltics,rtics,lvel,rvel;
  player_position_cmd_t cmd;
  player_position_data_t data;

  cmd.xpos = cmd.ypos = cmd.yaw = 0;
  cmd.xspeed = cmd.yspeed = cmd.yawspeed = 0;
  data.xpos = data.ypos = data.yaw = 0;
  data.xspeed = data.yspeed = data.yawspeed = 0;

  this->px = this->py = this->pa = 0.0;
  this->odom_initialized = false;

  printf("Botrics Trogdor connection initializing (%s)...", serial_port);
  fflush(stdout);

  // open it.  non-blocking at first, in case there's no robot
  if((this->fd = open(serial_port, O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 )
  {
    PLAYER_ERROR1("open() failed: %s", strerror(errno));
    return(-1);
  }  
 
  if(tcgetattr(this->fd, &term) < 0 )
  {
    PLAYER_ERROR1("tcgetattr() failed: %s", strerror(errno));
    close(this->fd);
    this->fd = -1;
    return(-1);
  }
  
#if HAVE_CFMAKERAW
  cfmakeraw(&term);
#endif
  cfsetispeed(&term, B57600);
  cfsetospeed(&term, B57600);
  
  if(tcsetattr(this->fd, TCSAFLUSH, &term) < 0 )
  {
    PLAYER_ERROR1("tcsetattr() failed: %s", strerror(errno));
    close(this->fd);
    this->fd = -1;
    return(-1);
  }

  fd_blocking = false;

  if(InitRobot() < 0)
  {
    PLAYER_ERROR("failed to initialize robot");
    close(this->fd);
    this->fd = -1;
    return(-1);
  }

  /* try to get current odometry, just to make sure we actually have a robot */
  if(GetOdom(&ltics,&rtics,&lvel,&rvel) < 0)
  {
    PLAYER_ERROR("failed to get odometry");
    close(this->fd);
    this->fd = -1;
    return(-1);
  }

  /* ok, we got data, so now set NONBLOCK, and continue */
  if((flags = fcntl(this->fd, F_GETFL)) < 0)
  {
    PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
    close(this->fd);
    this->fd = -1;
    return(-1);
  }
  if(fcntl(this->fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
  {
    PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
    close(this->fd);
    this->fd = -1;
    return(-1);
  }
  fd_blocking = true;
  puts("Done.");

  if(SendCommand(TROGDOR_SET_ACCELERATIONS,10,10) < 0)
  {
    PLAYER_ERROR("failed to set accelerations on setup");
    close(this->fd);
    this->fd = -1;
    return(-1);
  }

  // zero the command buffer
  PutCommand(this,(unsigned char*)&cmd,sizeof(cmd));
  PutData((unsigned char*)&data,sizeof(data),0,0);

  // start the thread to talk with the robot
  StartThread();

  return(0);
}

int
Trogdor::Shutdown()
{
  unsigned char deinitstr[1];

  if(this->fd == -1)
    return(0);

  StopThread();

  // the robot is stopped by the thread cleanup function StopRobot(), which
  // is called as a result of the above StopThread()
  /*
  if(SetVelocity(0,0) < 0)
    PLAYER_ERROR("failed to stop robot while shutting down");
   */

  usleep(TROGDOR_DELAY_US);
  //if(ChangeMotorState(0) < 0)
    //PLAYER_ERROR("failed to limp motors on shutdown");

  deinitstr[0] = TROGDOR_DEINIT;
  if(WriteBuf(deinitstr,sizeof(deinitstr)) < 0)
    PLAYER_ERROR("failed to deinitialize connection to robot");

  if(close(this->fd))
    PLAYER_ERROR1("close() failed:%s",strerror(errno));
  this->fd = -1;
  puts("Botrics Trogdor has been shutdown");
  return(0);
}

void 
Trogdor::Main()
{
  player_position_cmd_t command;
  player_position_data_t data;
  double lvel_mps, rvel_mps;
  int lvel, rvel;
  int ltics, rtics;
  double rotational_term, command_lvel, command_rvel;
  void* client;
  char config[256];
  int config_size;
  int final_lvel, final_rvel;
  int last_final_lvel, last_final_rvel;

  last_final_rvel = last_final_lvel = 0;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

  // push a pthread cleanup function that stops the robot
  pthread_cleanup_push(StopRobot,this);

  for(;;)
  {
    pthread_testcancel();
    
    GetCommand((unsigned char*)&command, sizeof(player_position_cmd_t));
    command.yawspeed = ntohl(command.yawspeed);
    command.xspeed = ntohl(command.xspeed);

    // convert (tv,rv) to (lv,rv) and send to robot
    rotational_term = DTOR(command.yawspeed) * TROGDOR_AXLE_LENGTH / 2.0;
    command_rvel = (command.xspeed/1e3) + rotational_term;
    command_lvel = (command.xspeed/1e3) - rotational_term;

    // sanity check on per-wheel speeds
    if(fabs(command_lvel) > TROGDOR_MAX_WHEELSPEED)
    {
      if(command_lvel > 0)
      {
        command_lvel = TROGDOR_MAX_WHEELSPEED;
        command_rvel *= TROGDOR_MAX_WHEELSPEED/command_lvel;
      }
      else
      {
        command_lvel = - TROGDOR_MAX_WHEELSPEED;
        command_rvel *= -TROGDOR_MAX_WHEELSPEED/command_lvel;
      }
    }
    if(fabs(command_rvel) > TROGDOR_MAX_WHEELSPEED)
    {
      if(command_rvel > 0)
      {
        command_rvel = TROGDOR_MAX_WHEELSPEED;
        command_lvel *= TROGDOR_MAX_WHEELSPEED/command_rvel;
      }
      else
      {
        command_rvel = - TROGDOR_MAX_WHEELSPEED;
        command_lvel *= -TROGDOR_MAX_WHEELSPEED/command_rvel;
      }
    }

    final_lvel = (int)rint(command_lvel / TROGDOR_MPS_PER_TICK);
    final_rvel = (int)rint(command_rvel / TROGDOR_MPS_PER_TICK);

    /* to account for our bad low-level PID motor controller */
    if(abs(final_rvel) > 0 && abs(final_rvel) < TROGDOR_MIN_WHEELSPEED_TICKS)
    {
      if(final_rvel > 0)
        final_rvel = TROGDOR_MIN_WHEELSPEED_TICKS;
      else
        final_rvel = -TROGDOR_MIN_WHEELSPEED_TICKS;
    }
    if(abs(final_lvel) > 0 && abs(final_lvel) < TROGDOR_MIN_WHEELSPEED_TICKS)
    {
      if(final_lvel > 0)
        final_lvel = TROGDOR_MIN_WHEELSPEED_TICKS;
      else
        final_lvel = -TROGDOR_MIN_WHEELSPEED_TICKS;
    }

    if((final_lvel != last_final_lvel) ||
       (final_rvel != last_final_rvel))
    {
      if(SetVelocity(final_lvel,final_rvel) < 0)
      {
        PLAYER_ERROR("failed to set velocity");
        pthread_exit(NULL);
      }
      last_final_lvel = final_lvel;
      last_final_rvel = final_rvel;
    }

    if(GetOdom(&ltics,&rtics,&lvel,&rvel) < 0)
    {
      PLAYER_ERROR("failed to get odometry");
      pthread_exit(NULL);
    }


    UpdateOdom(ltics,rtics);

    /*
    int volt;
    if(GetBatteryVoltage(&volt) < 0)
      PLAYER_WARN("failed to get voltage");
    printf("volt: %d\n", volt);
    */

    double tmp_angle;
    data.xpos = htonl((int32_t)rint(this->px * 1e3));
    data.ypos = htonl((int32_t)rint(this->py * 1e3));
    if(this->pa < 0)
      tmp_angle = this->pa + 2*M_PI;
    else
      tmp_angle = this->pa;

    data.yaw = htonl((int32_t)floor(RTOD(tmp_angle)));

    data.yspeed = 0;
    lvel_mps = lvel * TROGDOR_MPS_PER_TICK;
    rvel_mps = rvel * TROGDOR_MPS_PER_TICK;
    data.xspeed = htonl((int32_t)rint(1e3 * (lvel_mps + rvel_mps) / 2.0));
    data.yawspeed = htonl((int32_t)rint(RTOD((rvel_mps-lvel_mps) / 
                                             TROGDOR_AXLE_LENGTH)));

    PutData((unsigned char*)&data,sizeof(data),0,0);

    player_position_power_config_t* powercfg;

    // handle configs
    // TODO: break this out into a separate method
    if((config_size = GetConfig(&client,(void*)config,sizeof(config))) > 0)
    {
      switch(config[0])
      {
        case PLAYER_POSITION_GET_GEOM_REQ:
          /* Return the robot geometry. */
          if(config_size != 1)
          {
            PLAYER_WARN("Get robot geom config is wrong size; ignoring");
            if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          // TODO : get values from somewhere.
          player_position_geom_t geom;
          geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
          geom.pose[0] = htons((short) (0));
          geom.pose[1] = htons((short) (0));
          geom.pose[2] = htons((short) (0));
          geom.size[0] = htons((short) (450));
          geom.size[1] = htons((short) (450));

          if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, 
                      sizeof(geom)))
            PLAYER_ERROR("failed to PutReply");
          break;
        case PLAYER_POSITION_MOTOR_POWER_REQ:
          // NOTE: this doesn't seem to actually work
          if(config_size != sizeof(player_position_power_config_t))
          {
            PLAYER_WARN("Motor state change request wrong size; ignoring");
            if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
              PLAYER_ERROR("failed to PutReply");
            break;
          }
          powercfg = (player_position_power_config_t*)config;
          printf("got motor power req: %d\n", powercfg->value);
          if(ChangeMotorState(powercfg->value) < 0)
          {
            if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
              PLAYER_ERROR("failed to PutReply");
          }
          else
          {
            if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
              PLAYER_ERROR("failed to PutReply");
          }
          break;

        default:
          PLAYER_WARN1("received unknown config type %d\n", config[0]);
          PutReply(client, PLAYER_MSGTYPE_RESP_NACK);
      }
    }
    
    usleep(TROGDOR_DELAY_US);
  }
  pthread_cleanup_pop(1);
}

int
Trogdor::ReadBuf(unsigned char* s, size_t len)
{
  int thisnumread;
  size_t numread = 0;
  int loop;
  int maxloops=10;

  loop=0;
  while(numread < len)
  {
    //printf("loop %d of %d\n", loop,maxloops);
    // apparently the underlying PIC gets overwhelmed if we read too fast
    // wait...how can that be?
    if((thisnumread = read(this->fd,s+numread,len-numread)) < 0)
    {
      if(!this->fd_blocking && errno == EAGAIN && ++loop < maxloops)
      {
        usleep(TROGDOR_DELAY_US);
        continue;
      }
      PLAYER_ERROR1("read() failed: %s", strerror(errno));
      return(-1);
    }
    if(thisnumread == 0)
      PLAYER_WARN("short read");
    numread += thisnumread;
  }
  /*
  printf("read: ");
  for(size_t i=0;i<numread;i++)
    printf("%d ", s[i]);
  puts("");
  */
  return(0);
}

int
Trogdor::WriteBuf(unsigned char* s, size_t len)
{
  size_t numwritten;
  int thisnumwritten;
  unsigned char ack[1];

  for(;;)
  {
    numwritten=0;
    while(numwritten < len)
    {
      if((thisnumwritten = write(this->fd,s+numwritten,len-numwritten)) < 0)
      {
        if(!this->fd_blocking && errno == EAGAIN)
        {
          usleep(TROGDOR_DELAY_US);
          continue;
        }
        PLAYER_ERROR1("write() failed: %s", strerror(errno));
        return(-1);
      }
      numwritten += thisnumwritten;
    }

    // get acknowledgement
    if(ReadBuf(ack,1) < 0)
    {
      PLAYER_ERROR("failed to get acknowledgement");
      return(-1);
    }

    // TODO: re-init robot on NACK, to deal with underlying cerebellum reset 
    // problem
    switch(ack[0])
    {
      case TROGDOR_ACK:
        return(0);
      case TROGDOR_NACK:
        PLAYER_WARN("got NACK; reinitializing connection");
        return(-1);
        /*
        if(InitRobot() < 0)
        {
          PLAYER_ERROR("failed to reinitialize");
          return(-1);
        }
        break;
        */
      default:
        PLAYER_WARN1("got unknown value for acknowledgement: %d",ack[0]);
        return(-1);
    }
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

int
Trogdor::GetBatteryVoltage(int* voltage)
{
  unsigned char buf[5];
  buf[0] = TROGDOR_GET_VOLTAGE;

  puts("sending for voltage");
  if(WriteBuf(buf,1) < 0)
  {
    PLAYER_ERROR("failed to send battery voltage command");
    return(-1);
  }

  puts("waiting for voltage");
  if(ReadBuf(buf,5) < 0)
  {
    PLAYER_ERROR("failed to read battery voltage");
    return(-1);
  }

  if(ValidateChecksum(buf,5) < 0)
  {
    PLAYER_ERROR("checksum failed on battery voltage");
    return(-1);
  }

  *voltage = BytesToInt32(buf);
  return(0);
}

void
Trogdor::Int32ToBytes(unsigned char* buf, int i)
{
  buf[0] = (i >> 0)  & 0xFF;
  buf[1] = (i >> 8)  & 0xFF;
  buf[2] = (i >> 16) & 0xFF;
  buf[3] = (i >> 24) & 0xFF;
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
  //usleep(TROGDOR_DELAY_US);
  
  // read 4 int32's, 1 error byte, and 1 checksum
  if(ReadBuf(buf, 18) < 0)
  {
    PLAYER_ERROR("failed to read odometry");
    return(-1);
  }

  if(ValidateChecksum(buf, 18) < 0)
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
  *ltics = BytesToInt32(buf+index);
  index += 4;
  *rtics = BytesToInt32(buf+index);
  index += 4;
  *rvel = BytesToInt32(buf+index);
  index += 4;
  *lvel = BytesToInt32(buf+index);

  //puts("got good odom packet");

  return(0);
}

int 
Trogdor::ComputeTickDiff(int from, int to) 
{
  int diff1, diff2;

  // find difference in two directions and pick shortest
  if(to > from) 
  {
    diff1 = to - from;
    diff2 = (-TROGDOR_MAX_TICS - from) + (to - TROGDOR_MAX_TICS);
  }
  else 
  {
    diff1 = to - from;
    diff2 = (from - TROGDOR_MAX_TICS) + (-TROGDOR_MAX_TICS - to);
  }

  if(abs(diff1) < abs(diff2)) 
    return(diff1);
  else
    return(diff2);
}

void
Trogdor::UpdateOdom(int ltics, int rtics)
{
  int ltics_delta, rtics_delta;
  double l_delta, r_delta, a_delta, d_delta;

  if(!this->odom_initialized)
  {
    last_ltics = ltics;
    last_rtics = rtics;
    this->odom_initialized = true;
    return;
  }

  ltics_delta = ComputeTickDiff(last_ltics,ltics);
  rtics_delta = ComputeTickDiff(last_rtics,rtics);

  l_delta = ltics_delta * TROGDOR_M_PER_TICK;
  r_delta = rtics_delta * TROGDOR_M_PER_TICK;

  a_delta = (r_delta - l_delta) / TROGDOR_AXLE_LENGTH;
  d_delta = (l_delta + r_delta) / 2.0;

  //printf("ltics: %d\n", ltics);
  //printf("rtics: %d\n", rtics);

  // account for transient errors in tick values by ignoring changes that
  // suggest that we've move farther than physically possible (seems that we 
  // sometimes get zeros)
  if(d_delta > 100*(TROGDOR_MAX_WHEELSPEED * (TROGDOR_DELAY_US/1e6)))
  {
    PLAYER_WARN("Invalid odometry change; ignoring");
    return;
  }
  
  // MAJOR HACK! The check above is too strict, for some reason.  Since the
  // problem comes from one or the other encoder returning 0 ticks (always
  // the left, I think), we'll just throw out those readings.  Shouldn't have
  // too much impact.
  if(!ltics || !rtics)
  {
    PLAYER_WARN("Invalid odometry change; ignoring");
    return;
  }

  this->px += d_delta * cos(this->pa);
  this->py += d_delta * sin(this->pa);
  this->pa += a_delta;
  this->pa = NORMALIZE(this->pa);

  this->last_ltics = ltics;
  this->last_rtics = rtics;
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

  //printf("SendCommand: %d %d %d\n", cmd, val1, val2);
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
  printf("sending %d:%d\n", lvel,rvel);
  if(SendCommand(TROGDOR_SET_VELOCITIES,lvel,rvel) < 0)
  {
    PLAYER_ERROR("failed to set velocities");
    return(-1);
  }
  return(0);
}

int 
Trogdor::ChangeMotorState(int state)
{
  unsigned char buf[1];
  if(state)
    buf[0] = TROGDOR_ENABLE_VEL_CONTROL;
  else
    buf[0] = TROGDOR_DISABLE_VEL_CONTROL;
  return(WriteBuf(buf,sizeof(buf)));
}

static void
StopRobot(void* trogdordev)
{
  Trogdor* td = (Trogdor*)trogdordev;

  if(td->SetVelocity(0,0) < 0)
    PLAYER_ERROR("failed to stop robot on thread exit");
}

