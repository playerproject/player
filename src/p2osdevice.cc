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
 *   the P2OS device.  it's the parent device for all the P2 'sub-devices',
 *   like gripper, position, sonar, etc.  there's a thread here that
 *   actually interacts with P2OS via the serial line.  the other
 *   "devices" communicate with this thread by putting into and getting
 *   data out of shared buffers.
 */

#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>  /* for abs() */
#include <signal.h>  /* for sigblock */
#include <netinet/in.h>

#include <p2osdevice.h>
#include <sip.h>
#include <packet.h>

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

/* these are from personal experience */
#define MOTOR_MAX_SPEED 500
#define MOTOR_MAX_TURNRATE 100

//extern bool experimental;

/* these are necessary to make the static fields visible to the linker */
extern pthread_t      CP2OSDevice::thread;
extern unsigned char* CP2OSDevice::data;
extern unsigned char* CP2OSDevice::command;
extern unsigned char* CP2OSDevice::config;
extern int            CP2OSDevice::config_size;
extern CLock          CP2OSDevice::lock;
extern struct timeval CP2OSDevice::timeBegan_tv;
extern bool           CP2OSDevice::direct_wheel_vel_control;
extern int            CP2OSDevice::psos_fd; 
extern char           CP2OSDevice::psos_serial_port[];
extern char           CP2OSDevice::num_loops_since_rvel;
extern pthread_mutex_t CP2OSDevice::serial_mutex;
extern CSIP*           CP2OSDevice::sippacket;
extern bool           CP2OSDevice::arena_initialized_data_buffer;
extern bool           CP2OSDevice::arena_initialized_command_buffer;

void *RunPsosThread( void *p2osdevice );

CP2OSDevice::CP2OSDevice(char *port) 
{
  //puts("CP2OSDevice::CP2OSDevice()");
  data = new unsigned char[P2OS_DATA_BUFFER_SIZE];
  command = new unsigned char[P2OS_COMMAND_BUFFER_SIZE];
  config = new unsigned char[P2OS_CONFIG_BUFFER_SIZE];

  config_size = 0;
  arena_initialized_command_buffer = false;
  arena_initialized_data_buffer = false;

  *(short*)&command[POSITION_COMMAND_OFFSET] = 
          (short)htons((unsigned short)0);
  *(short*)&command[POSITION_COMMAND_OFFSET+sizeof(short)] = 
          (short)htons((unsigned short)0);

  command[GRIPPER_COMMAND_OFFSET] = GRIPstore;
  command[GRIPPER_COMMAND_OFFSET+1] = 0x00;

  psos_fd = -1;
  strcpy( psos_serial_port, port );

  pthread_mutex_init(&serial_mutex,NULL);
}

CP2OSDevice::~CP2OSDevice()
{
  GetLock()->Shutdown( this );
  pthread_mutex_destroy(&serial_mutex);
}

int CP2OSDevice::Setup()
{
  struct termios term;
  unsigned char command;
  CPacket packet, receivedpacket;
  pthread_attr_t attr;
  int flags;
  bool sent_close = false;
  enum
  {
    NO_SYNC,
    AFTER_FIRST_SYNC,
    AFTER_SECOND_SYNC,
    READY
  } psos_state;
    
  //puts("CP2OSDevice::Setup()");

  psos_state = NO_SYNC;

  char name[20], type[20], subtype[20];
  int cnt;

  if((psos_fd = open( psos_serial_port, O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 ) {
    perror("CP2OSDevice::Setup():open():");
    return(1);
  }  
 
  if( tcgetattr( psos_fd, &term ) < 0 )
  {
    perror("CP2OSDevice::Setup():tcgetattr():");
    close(psos_fd);
    psos_fd = -1;
    return(1);
  }
  
  cfmakeraw( &term );
  cfsetispeed( &term, B9600 );
  cfsetospeed( &term, B9600 );
  
  if( tcsetattr( psos_fd, TCSAFLUSH, &term ) < 0 )
  {
    perror("CP2OSDevice::Setup():tcsetattr():");
    close(psos_fd);
    psos_fd = -1;
    return(1);
  }

  if( tcflush( psos_fd, TCIOFLUSH ) < 0 )
  {
    perror("CP2OSDevice::Setup():tcflush():");
    close(psos_fd);
    psos_fd = -1;
    return(1);
  }

  if((flags = fcntl(psos_fd, F_GETFL)) < 0)
  {
    perror("CP2OSDevice::Setup():fcntl()");
    close(psos_fd);
    psos_fd = -1;
    return(1);
  }

  while(psos_state != READY)
  {
    //printf("psos_state: %d\n", psos_state);
    switch(psos_state)
    {
      case NO_SYNC:
        command = SYNC0;
        packet.Build( &command, 1);
        packet.Send( psos_fd );
        usleep(P2OS_CYCLETIME_USEC);
        break;
      case AFTER_FIRST_SYNC:
        if(fcntl(psos_fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
        {
          perror("CP2OSDevice::Setup():fcntl()");
          close(psos_fd);
          psos_fd = -1;
          return(1);
        }
        command = SYNC1;
        packet.Build( &command, 1);
        packet.Send( psos_fd );
        break;
      case AFTER_SECOND_SYNC:
        command = SYNC2;
        packet.Build( &command, 1);
        packet.Send( psos_fd );
        break;
      default:
        puts("CP2OSDevice::Setup():shouldn't be here...");
        break;
    }
    usleep(P2OS_CYCLETIME_USEC);

    if(receivedpacket.Receive( psos_fd ))
    {
      printf("Couldn't synchronize with P2OS most likely because the robot\n"
                      "is not connected or is connected not to %s\n", 
                      psos_serial_port);
      close(psos_fd);
      psos_fd = -1;
      return(1);
    }
    //receivedpacket.PrintHex();
    switch(receivedpacket.packet[3])
    {
      case SYNC0:
        psos_state = AFTER_FIRST_SYNC;
        break;
      case SYNC1:
        psos_state = AFTER_SECOND_SYNC;
        break;
      case SYNC2:
        psos_state = READY;
        break;
      default:
        // maybe P2OS is still running from last time.  let's try to CLOSE 
        // and reconnect
        if(!sent_close)
        {
          //puts("sending CLOSE");
          command = CLOSE;
          packet.Build( &command, 1);
          packet.Send( psos_fd );
          sent_close = true;
          usleep(2*P2OS_CYCLETIME_USEC);
          tcflush(psos_fd,TCIFLUSH);
          psos_state = NO_SYNC;
        }
        break;
    }
    usleep(P2OS_CYCLETIME_USEC);
  }

  cnt = 4;
  cnt += sprintf(name, "%s", &receivedpacket.packet[cnt]);
  cnt++;
  cnt += sprintf(type, "%s", &receivedpacket.packet[cnt]);
  cnt++;
  cnt += sprintf(subtype, "%s", &receivedpacket.packet[cnt]);
  cnt++;

  printf("Connected to %s, a %s %s\n", name, type, subtype);

  command = OPEN;
  packet.Build( &command, 1);
  packet.Send( psos_fd );
  usleep(P2OS_CYCLETIME_USEC);

  command = PULSE;
  packet.Build( &command, 1);
  packet.Send( psos_fd );
  usleep(P2OS_CYCLETIME_USEC);

  puts("P2OS connection ready");
  direct_wheel_vel_control = true;
  num_loops_since_rvel = 2;
  pthread_mutex_unlock(&serial_mutex);

  // first, receive a packet so we know we're connected.
  sippacket = new CSIP();
  SendReceive((CPacket*)NULL,false);

  /* now spawn reading thread */
  if(pthread_attr_init(&attr) ||
     pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) ||
     pthread_create(&thread, &attr, &RunPsosThread, this))
  {
    fputs("CP2OSDevice::Setup(): pthread creation messed up\n",stderr);
    return(1);
  }
  return(0);
}

int CP2OSDevice::Shutdown()
{
  unsigned char command[20],buffer[20];
  CPacket packet; 

  memset(buffer,0,20);

  if(psos_fd == -1)
  {
    return(0);
  }

  if(pthread_cancel(thread))
  {
    fputs("CP2OSDevice::Shutdown(): WARNING: pthread_cancel() on psos "
          "reading thread failed\n",stderr);
  }

  command[0] = STOP;
  packet.Build( command, 1);
  packet.Send( psos_fd );
  usleep(P2OS_CYCLETIME_USEC);

  command[0] = CLOSE;
  packet.Build( command, 1);
  packet.Send( psos_fd );
  usleep(P2OS_CYCLETIME_USEC);

  close(psos_fd);
  psos_fd = -1;
  puts("P2OS Connection closed");
  pthread_mutex_unlock(&serial_mutex);
  delete sippacket;
  return(0);
}

int CP2OSDevice::GetData( unsigned char* dest)
{
  return(0);
}
void CP2OSDevice::PutData( unsigned char* src)
{
  memcpy( data, src, P2OS_DATA_BUFFER_SIZE);
}

void CP2OSDevice::GetCommand( unsigned char* dest)
{
  memcpy( dest, command, P2OS_COMMAND_BUFFER_SIZE);
}
void CP2OSDevice::PutCommand( unsigned char* src, int size)
{
}
int CP2OSDevice::GetConfig( unsigned char* dest)
{
  if(config_size)
  {
    memcpy(dest, config, config_size);
  }
  return(config_size);
}
void CP2OSDevice::PutConfig( unsigned char* src, int size)
{
  if(size > P2OS_CONFIG_BUFFER_SIZE)
    puts("CP2OSDevice::PutConfig(): config request too big. ignoring");
  else
  {
    memcpy(config, src, size);
    config_size = size;
  }
}

void EmptySigHanndler( int dummy ) {
  printf("EmptySigHanndler: got %d\n", dummy);
}

void *RunPsosThread( void *p2osdevice ) 
{
  CP2OSDevice* pd = (CP2OSDevice*) p2osdevice;

  unsigned char command[P2OS_COMMAND_BUFFER_SIZE];
  unsigned char config[P2OS_CONFIG_BUFFER_SIZE];
  unsigned char motorcommand[4];
  //unsigned char gripcommand[4];
  CPacket motorpacket; 
  //CPacket grippacket; 
  short speedDemand=0, turnRateDemand=0;
  short gripperDemand;
  int cnt; 
  bool newmotorspeed, newmotorturn;
  bool newgrippercommand;
  double leftvel, rightvel;
  double rotational_term;
  unsigned short absspeedDemand, absturnRateDemand;

  int config_size;

  /*
   * in this order:
   *   ints: time X Y
   *   shorts: heading, forwardvel, turnrate, compass
   *   chars: stalls
   *   shorts: 16 sonars
   *   chars: gripstate,gripbeams
   *   chars: bumpers,voltage
   */
  //unsigned char data[P2OS_DATA_BUFFER_SIZE];
  sigblock(SIGINT);
  sigblock(SIGALRM);

  gettimeofday(&pd->timeBegan_tv, NULL);

  for(;;)
  {
    // first, check if there is a new config command
    if((config_size = pd->GetLock()->GetConfig(pd,config)))
    {
      switch(config[0])
      {
        case 'm':
          /* motor state change request 
           *   1 = enable motors
           *   0 = disable motors (default)
           */
	  if(config_size-1 != 1)
          {
	    puts("Arg to motor state change request is wrong size; ignoring");
	    break;
	  }
          motorcommand[0] = ENABLE;
          motorcommand[1] = 0x3B;
          motorcommand[2] = config[1];
          motorcommand[3] = 0;
          motorpacket.Build(motorcommand, 4);
          pd->SendReceive(&motorpacket,false);
          break;
        case 'v':
          /* velocity control mode:
           *   0 = direct wheel velocity control (default)
           *   1 = separate translational and rotational control
           */
	  if(config_size-1 != sizeof(char))
          {
	    puts("Arg to velocity control mode change request is wrong "
                            "size; ignoring");
	    break;
	  }
          if(config[1])
            pd->direct_wheel_vel_control = false;
          else
            pd->direct_wheel_vel_control = true;
          break;
        case 'R':
          /* reset position to 0,0,0: no args */
	  if(config_size-1 != 0)
          {
	    puts("Arg to reset position request is wrong size; ignoring");
	    break;
	  }
          pd->ResetRawPositions();
          break;
        default:
          printf("RunPsosThread: got unknown config request \"%c\"\n",
                          config[0]);
          break;
      }

      // set size to zero, so we'll know not to read it next time unless
      // PutConfig() was called
      pd->config_size = 0;
    }

    /* read the clients' commands from the common buffer */
    pd->GetLock()->GetCommand(pd,command);

    cnt = POSITION_COMMAND_OFFSET;
    newmotorspeed = false;
    if( speedDemand != (short) ntohs( *(short *) &command[cnt]))
      newmotorspeed = true;
    speedDemand = (short) ntohs( *(short *) &command[cnt]);

    cnt+=sizeof(short);
    newmotorturn = false;
    if(turnRateDemand != (short) ntohs( *(short *) &command[cnt]))
      newmotorturn = true;
    turnRateDemand = (short) ntohs( *(short *) &command[cnt]);

    cnt = GRIPPER_COMMAND_OFFSET;
    newgrippercommand = false;
    if(gripperDemand != (short) ntohs( *(short *) &command[cnt]))
      newgrippercommand = true;
    gripperDemand = (short) ntohs( *(short *) &command[cnt]);

    /* NEXT, write commands */

    if(pd->direct_wheel_vel_control)
    {
      // do direct wheel velocity control here
      //printf("speedDemand: %d\t turnRateDemand: %d\n",
                      //speedDemand, turnRateDemand);
      rotational_term = ((M_PI/180.0)*turnRateDemand*RobotAxleLength)/2.0;
      leftvel = (speedDemand - rotational_term);
      rightvel = (speedDemand + rotational_term);
      if(fabs(leftvel) > MOTOR_MAX_SPEED)
      {
        if(leftvel > 0)
        {
          leftvel = MOTOR_MAX_SPEED;
          rightvel *= MOTOR_MAX_SPEED/leftvel;
          puts("Left wheel velocity threshholded!");
        }
        else
        {
          leftvel = -MOTOR_MAX_SPEED;
          rightvel *= -MOTOR_MAX_SPEED/leftvel;
        }
      }
      if(fabs(rightvel) > MOTOR_MAX_SPEED)
      {
        if(rightvel > 0)
        {
          rightvel = MOTOR_MAX_SPEED;
          leftvel *= MOTOR_MAX_SPEED/rightvel;
          puts("Right wheel velocity threshholded!");
        }
        else
        {
          rightvel = -MOTOR_MAX_SPEED;
          leftvel *= -MOTOR_MAX_SPEED/rightvel;
        }
      }

      // left and right velocities are in 2cm/sec 
      //printf("leftvel: %d\trightvel:%d\n", 
      //(char)(leftvel/20.0), (char)(rightvel/20.0));
      motorcommand[0] = VEL2;
      motorcommand[1] = 0x3B;
      motorcommand[2] = (char)(rightvel/20.0);
      motorcommand[3] = (char)(leftvel/20.0);
    }
    else
    {
      // do separate trans and rot vels
      //
      // if trans vel is new, write it;
      // if just rot vel is new, write it;
      // if neither are new, write trans.
      if((newmotorspeed || !newmotorturn) && (pd->num_loops_since_rvel < 2))
      {
        motorcommand[0] = VEL;
        if(speedDemand >= 0)
          motorcommand[1] = 0x3B;
        else
          motorcommand[1] = 0x1B;

        absspeedDemand = (unsigned short)abs(speedDemand);
        if(absspeedDemand < MOTOR_MAX_SPEED)
          *(unsigned short*)&motorcommand[2] = absspeedDemand;
        else
        {
          puts("Speed demand threshholded!");
          *(unsigned short*)&motorcommand[2] = (unsigned short)MOTOR_MAX_SPEED;
        }
      }
      else
      {
        motorcommand[0] = RVEL;
        if(turnRateDemand >= 0)
          motorcommand[1] = 0x3B;
        else
          motorcommand[1] = 0x1B;

        absturnRateDemand = (unsigned short)abs(turnRateDemand);
        if(absturnRateDemand < MOTOR_MAX_TURNRATE)
          *(unsigned short*)&motorcommand[2] = absturnRateDemand;
        else
        {
          puts("Turn rate demand threshholded!");
          *(unsigned short*)&motorcommand[2] = 
                  (unsigned short)MOTOR_MAX_TURNRATE;
        }
      }
    }
    //printf("motorpacket[0]: %d\n", motorcommand[0]);
    motorpacket.Build( motorcommand, 4);
    pd->SendReceive(&motorpacket,false);

    /*
    if(newgrippercommand)
    {
      // gripper command 
      gripcommand[0] = GRIPPER;
      gripcommand[1] = 0x3B;
      *(unsigned short*)&gripcommand[2] = gripperDemand >> 8;
      grippacket.Build( gripcommand, 4);
      grippacket.Send(pd->psos_fd);

      // pass extra value to gripper if needed 
      if(gripperDemand >> 8 == GRIPpress || gripperDemand >> 8 == LIFTcarry ) 
      {
        gripcommand[0] = GRIPPERVAL;
        gripcommand[1] = 0x3B;
        *(unsigned short*)&gripcommand[2] = gripperDemand & 0x0F;
        grippacket.Build( gripcommand, 4);
        grippacket.Send(pd->psos_fd);
      }
    }
    */
  }
  pthread_exit(NULL);
}

/* send the packet, then receive and parse an SIP */
int
CP2OSDevice::SendReceive(CPacket* pkt, bool already_have_lock)
{
  CPacket packet;
  //static CSIP sippacket;
  unsigned char data[P2OS_DATA_BUFFER_SIZE];

  if((psos_fd >= 0) && sippacket)
  {
    //printf("psos_fd: %d\n", psos_fd);
    if(!already_have_lock)
      pthread_mutex_lock(&serial_mutex);
    if(pkt)
    {
      if(!direct_wheel_vel_control)
      {
        if(pkt->packet[3] == RVEL)
          num_loops_since_rvel = 0;
        else
          num_loops_since_rvel++;
      }
      pkt->Send(psos_fd);
    }

    /* receive a packet */
    pthread_testcancel();
    if(packet.Receive(psos_fd))
    {
      puts("RunPsosThread(): Receive errored");
      // should probably just exit, although we could try to
      // reconnect...
      //tcflush(pd->psos_fd,TCIFLUSH);
      //close(pd->psos_fd);
      //pd->psos_fd = -1;
      //usleep(2*P2OS_CYCLETIME_USEC);
      //pd->Setup();
      pthread_exit(NULL);
    }
    pthread_testcancel();

    if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB && 
                    (packet.packet[3] == 0x32 || packet.packet[3] == 0x33)) 
    {
      /* It is a sip packet */
      sippacket->Parse( &packet.packet[3] );
      sippacket->Fill(data, timeBegan_tv );

      GetLock()->PutData(this,data);
    }
    else 
    {
      puts("got non-SIP packet");
    }

    pthread_mutex_unlock(&serial_mutex);
  }
  return(0);
}

void
CP2OSDevice::ResetRawPositions()
{
  CPacket pkt;
  unsigned char p2oscommand[4];

  if(sippacket)
  {
    pthread_mutex_lock(&serial_mutex);
    sippacket->rawxpos = 0;
    sippacket->rawypos = 0;
    sippacket->xpos = 0;
    sippacket->ypos = 0;
    p2oscommand[0] = SETO;
    p2oscommand[1] = 0x3B;
    pkt.Build(p2oscommand, 2);
    SendReceive(&pkt,true);
  }
}


