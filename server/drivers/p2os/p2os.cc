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
#include <unistd.h>
#include <math.h>
#include <stdlib.h>  /* for abs() */
#include <netinet/in.h>
#include <termios.h>

#include <p2os.h>
#include <packet.h>

#include <playertime.h>
extern PlayerTime* GlobalTime;

// so we can access the deviceTable and extract pointers to the sonar
// and position objects
#include <devicetable.h>
extern CDeviceTable* deviceTable;
extern int global_playerport; // used to get at devices

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

/* these are necessary to make the static fields visible to the linker */
extern pthread_t       P2OS::thread;
extern struct timeval  P2OS::timeBegan_tv;
extern bool            P2OS::direct_wheel_vel_control;
extern int             P2OS::psos_fd; 
extern char            P2OS::psos_serial_port[];
extern int             P2OS::radio_modemp;
extern bool            P2OS::initdone;
extern char            P2OS::num_loops_since_rvel;
extern SIP*           P2OS::sippacket;
extern int             P2OS::param_idx;
extern pthread_mutex_t P2OS::p2os_accessMutex;
extern pthread_mutex_t P2OS::p2os_setupMutex;
extern int             P2OS::p2os_subscriptions;
extern player_p2os_data_t*  P2OS::data;
extern player_p2os_cmd_t*  P2OS::command;
extern unsigned char*    P2OS::reqqueue;
extern unsigned char*    P2OS::repqueue;


P2OS::P2OS(char* interface, ConfigFile* cf, int section)
{
  int reqqueuelen = 1;
  int repqueuelen = 1;

  if(!initdone)
  {
    // build the table of robot parameters.
    initialize_robot_params();
  
    // also, install default parameter values.
    strncpy(psos_serial_port,DEFAULT_P2OS_PORT,sizeof(psos_serial_port));
    psos_fd = -1;
    radio_modemp = 0;
  
    data = new player_p2os_data_t;
    command = new player_p2os_cmd_t;

    reqqueue = (unsigned char*)(new playerqueue_elt_t[reqqueuelen]);
    repqueue = (unsigned char*)(new playerqueue_elt_t[repqueuelen]);

    SetupBuffers((unsigned char*)data, sizeof(player_p2os_data_t),
                 (unsigned char*)command, sizeof(player_p2os_cmd_t),
                 reqqueue, reqqueuelen,
                 repqueue, repqueuelen);

    ((player_p2os_cmd_t*)device_command)->position.xspeed = 0;
    ((player_p2os_cmd_t*)device_command)->position.yawspeed = 0;

    ((player_p2os_cmd_t*)device_command)->gripper.cmd = GRIPstore;
    ((player_p2os_cmd_t*)device_command)->gripper.arg = 0x00;

    p2os_subscriptions = 0;

    pthread_mutex_init(&p2os_accessMutex,NULL);
    pthread_mutex_init(&p2os_setupMutex,NULL);

    initdone = true; 
  }
  else
  {
    // every sub-device gets its own queue object (but they all point to the
    // same chunk of memory)
    
    // every sub-device needs to get its various pointers set up
    SetupBuffers((unsigned char*)data, sizeof(player_p2os_data_t),
                 (unsigned char*)command, sizeof(player_p2os_cmd_t),
                 reqqueue, reqqueuelen,
                 repqueue, repqueuelen);
  }



  strncpy(psos_serial_port,
          cf->ReadString(section, "port", psos_serial_port),
          sizeof(psos_serial_port));
  radio_modemp = cf->ReadInt(section, "radio", radio_modemp);

  // zero the subscription counter.
  subscriptions = 0;
}

void P2OS::Lock()
{
  pthread_mutex_lock(&p2os_accessMutex);
}
void P2OS::Unlock()
{
  pthread_mutex_unlock(&p2os_accessMutex);
}

int P2OS::Setup()
{
  unsigned char sonarcommand[4];
  P2OSPacket sonarpacket; 
  int i;

  struct termios term;
  unsigned char command;
  P2OSPacket packet, receivedpacket;
  int flags;
  bool sent_close = false;
  enum
  {
    NO_SYNC,
    AFTER_FIRST_SYNC,
    AFTER_SECOND_SYNC,
    READY
  } psos_state;
    
  psos_state = NO_SYNC;

  char name[20], type[20], subtype[20];
  int cnt;

  printf("P2OS connection initializing (%s)...",psos_serial_port);
  fflush(stdout);

  if((psos_fd = open(psos_serial_port, 
                     O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 )
  {
    perror("P2OS::Setup():open():");
    return(1);
  }  
 
  if( tcgetattr( psos_fd, &term ) < 0 )
  {
    perror("P2OS::Setup():tcgetattr():");
    close(psos_fd);
    psos_fd = -1;
    return(1);
  }
  
#if HAVE_CFMAKERAW
  cfmakeraw( &term );
#endif
  cfsetispeed( &term, B9600 );
  cfsetospeed( &term, B9600 );
  
  if( tcsetattr( psos_fd, TCSAFLUSH, &term ) < 0 )
  {
    perror("P2OS::Setup():tcsetattr():");
    close(psos_fd);
    psos_fd = -1;
    return(1);
  }

  if( tcflush( psos_fd, TCIOFLUSH ) < 0 )
  {
    perror("P2OS::Setup():tcflush():");
    close(psos_fd);
    psos_fd = -1;
    return(1);
  }

  if((flags = fcntl(psos_fd, F_GETFL)) < 0)
  {
    perror("P2OS::Setup():fcntl()");
    close(psos_fd);
    psos_fd = -1;
    return(1);
  }

  // radio modem initialization code, courtesy of Kim Jinsuck 
  //   <jinsuckk@cs.tamu.edu>
  if(radio_modemp)
  {
    puts("Initializing radio modem...");
    write(psos_fd, "WMS2\r", 5);

    usleep(50000);
    char modem_buf[40];
    read(psos_fd, modem_buf, 5);          // get "WMS2"
    printf("wireless modem response = %s\n", modem_buf);

    usleep(10000);
    // get "\n\rConnecting..." --> \n\r is my guess
    read(psos_fd, modem_buf, 14); 
    printf("wireless modem response = %s\n", modem_buf);

    // wait until get "Connected to address 2"
    int modem_connect_try = 10;
    while (modem_buf[12] != 't') 
    {
      usleep(300000);
      read(psos_fd, modem_buf, 40);
      printf("wireless modem response = %s\n", modem_buf);
      // if "Partner busy!"
      if(modem_buf[2] == 'P') 
      {
        printf("Please reset partner modem and try again\n");
        return(1);
      }
      // if "\n\rPartner not found!"
      if(modem_buf[0] == 'P') 
      {
        printf("Please check partner modem and try again\n");
        return(1);
      }
      if(modem_connect_try-- == 0) 
      {
        printf("Failed to connect radio modem\n");
        return(1);
      }
    }
  }

  int num_sync_attempts = 5;
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
          perror("P2OS::Setup():fcntl()");
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
        puts("P2OS::Setup():shouldn't be here...");
        break;
    }
    usleep(P2OS_CYCLETIME_USEC);

    if(receivedpacket.Receive( psos_fd ))
    {
      if((psos_state == NO_SYNC) && (num_sync_attempts >= 0))
      {
        num_sync_attempts--;
        usleep(P2OS_CYCLETIME_USEC);
        continue;
      }
      else
      {
        printf("Couldn't synchronize with P2OS.\n"  
               "  Most likely because the robot is not connected to %s\n", 
               psos_serial_port);
        close(psos_fd);
        psos_fd = -1;
        return(1);
      }
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


  command = OPEN;
  packet.Build( &command, 1);
  packet.Send( psos_fd );
  usleep(P2OS_CYCLETIME_USEC);

  command = PULSE;
  packet.Build( &command, 1);
  packet.Send( psos_fd );
  usleep(P2OS_CYCLETIME_USEC);

  printf("Done.\n   Connected to %s, a %s %s\n", name, type, subtype);

  // now, based on robot type, find the right set of parameters
  for(i=0;i<PLAYER_NUM_ROBOT_TYPES;i++)
  {
    if(!strcasecmp(PlayerRobotParams[i].Class,type) && 
       !strcasecmp(PlayerRobotParams[i].Subclass,subtype))
    {
      param_idx = i;
      break;
    }
  }
  if(i == PLAYER_NUM_ROBOT_TYPES)
  {
    fputs("P2OS: Warning: couldn't find parameters for this robot; "
            "using defaults\n",stderr);
    param_idx = 0;
  }
  
  direct_wheel_vel_control = true;
  num_loops_since_rvel = 2;
  //pthread_mutex_unlock(&serial_mutex);

  // first, receive a packet so we know we're connected.
  if(!sippacket)
    sippacket = new SIP(param_idx);
  SendReceive((P2OSPacket*)NULL);//,false);

  // turn off the sonars at first
  sonarcommand[0] = SONAR;
  sonarcommand[1] = 0x3B;
  sonarcommand[2] = 0;
  sonarcommand[3] = 0;
  sonarpacket.Build(sonarcommand, 4);
  SendReceive(&sonarpacket);


  /* now spawn reading thread */
  StartThread();
  return(0);
}

int P2OS::Shutdown()
{
  unsigned char command[20],buffer[20];
  P2OSPacket packet; 

  memset(buffer,0,20);

  if(psos_fd == -1)
  {
    return(0);
  }

  StopThread();

  command[0] = STOP;
  packet.Build(command, 1);
  packet.Send(psos_fd);
  usleep(P2OS_CYCLETIME_USEC);

  command[0] = CLOSE;
  packet.Build( command, 1);
  packet.Send( psos_fd );
  usleep(P2OS_CYCLETIME_USEC);

  close(psos_fd);
  psos_fd = -1;
  puts("P2OS has been shutdown");
  delete sippacket;
  sippacket = NULL;

  return(0);
}

int P2OS::Subscribe(void *client)
{
  int setupResult;

  if(p2os_subscriptions == 0) 
  {
    setupResult = Setup();
    if (setupResult == 0 ) 
    {
      p2os_subscriptions++;  // increment the static p2os-wide subscr counter
      subscriptions++;       // increment the per-device subscr counter
    }
  }
  else 
  {
    p2os_subscriptions++;  // increment the static p2os-wide subscr counter
    subscriptions++;       // increment the per-device subscr counter
    setupResult = 0;
  }
  
  return( setupResult );
}

int P2OS::Unsubscribe(void *client)
{
  int shutdownResult;

  if(p2os_subscriptions == 0) 
  {
    shutdownResult = -1;
  }
  else if(p2os_subscriptions == 1) 
  {
    shutdownResult = Shutdown();
    if (shutdownResult == 0 ) 
    { 
      p2os_subscriptions--;  // decrement the static p2os-wide subscr counter
      subscriptions--;       // decrement the per-device subscr counter
    }
    /* do we want to unsubscribe even though the shutdown went bad? */
  }
  else 
  {
    p2os_subscriptions--;  // decrement the static p2os-wide subscr counter
    subscriptions--;       // decrement the per-device subscr counter
    shutdownResult = 0;
  }
  
  return( shutdownResult );
}


void P2OS::PutData( unsigned char* src, size_t maxsize,
                         uint32_t timestamp_sec, uint32_t timestamp_usec)
{
  Lock();

  *((player_p2os_data_t*)device_data) = *((player_p2os_data_t*)src);

  if(timestamp_sec == 0)
  {
    struct timeval curr;
    GlobalTime->GetTime(&curr);
    timestamp_sec = curr.tv_sec;
    timestamp_usec = curr.tv_usec;
  }

  data_timestamp_sec = timestamp_sec;
  data_timestamp_usec = timestamp_usec;
  
  // need to fill in the timestamps on all P2OS devices, both so that they
  // can read it, but also because other devices may want to read it
  player_device_id_t id;
  id.port = global_playerport;
  id.index = 0;

  id.code = PLAYER_SONAR_CODE;
  CDevice* sonarp = deviceTable->GetDevice(id);
  if(sonarp)
  {
    sonarp->data_timestamp_sec = this->data_timestamp_sec;
    sonarp->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_POWER_CODE;
  CDevice* powerp = deviceTable->GetDevice(id);
  if(powerp)
  {
    powerp->data_timestamp_sec = this->data_timestamp_sec;
    powerp->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_BUMPER_CODE;
  CDevice* bumperp = deviceTable->GetDevice(id);
  if(bumperp)
  {
    bumperp->data_timestamp_sec = this->data_timestamp_sec;
    bumperp->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_AIO_CODE;
  CDevice* aio = deviceTable->GetDevice(id);
  if(aio)
  {
    aio->data_timestamp_sec = this->data_timestamp_sec;
    aio->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_DIO_CODE;
  CDevice* dio = deviceTable->GetDevice(id);
  if(dio)
  {
    dio->data_timestamp_sec = this->data_timestamp_sec;
    dio->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_POSITION_CODE;
  CDevice* positionp = deviceTable->GetDevice(id);
  if(positionp)
  {
    positionp->data_timestamp_sec = this->data_timestamp_sec;
    positionp->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_GRIPPER_CODE;
  CDevice* gripperp = deviceTable->GetDevice(id);
  if(gripperp)
  {
    gripperp->data_timestamp_sec = this->data_timestamp_sec;
    gripperp->data_timestamp_usec = this->data_timestamp_usec;
  }

  Unlock();
}

void 
P2OS::Main()
{
  player_p2os_cmd_t command;
  unsigned char config[P2OS_CONFIG_BUFFER_SIZE];
  unsigned char motorcommand[4];
  unsigned char gripcommand[4];
  P2OSPacket motorpacket; 
  P2OSPacket grippacket; 
  short speedDemand=0, turnRateDemand=0;
  char gripperCmd=0,gripperArg=0;
  bool newmotorspeed, newmotorturn;
  bool newgrippercommand;
  double leftvel, rightvel;
  double rotational_term;
  unsigned short absspeedDemand, absturnRateDemand;

  int config_size;

  int last_sonar_subscrcount;
  int last_position_subscrcount;

  player_device_id_t id;
  id.port = global_playerport;
  id.index = 0;

  id.code = PLAYER_SONAR_CODE;
  CDevice* sonarp = deviceTable->GetDevice(id);
  id.code = PLAYER_POSITION_CODE;
  CDevice* positionp = deviceTable->GetDevice(id);

  last_sonar_subscrcount = 0;
  last_position_subscrcount = 0;

  GlobalTime->GetTime(&timeBegan_tv);

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

  // request the current configuration
  /*
    configreq[0] = 18;
    configreq[1] = 0x3B;
    configpacket.Build(configreq,2);
    puts("sending CONFIGpac");
    SendReceive(&configpacket);
  */

  for(;;)
  {
    // we want to turn on the sonars if someone just subscribed, and turn
    // them off if the last subscriber just unsubscribed.
    if(sonarp)
    {
      if(!last_sonar_subscrcount && sonarp->subscriptions)
      {
        motorcommand[0] = SONAR;
        motorcommand[1] = 0x3B;
        motorcommand[2] = 1;
        motorcommand[3] = 0;
        motorpacket.Build(motorcommand, 4);
        SendReceive(&motorpacket);
      }
      else if(last_sonar_subscrcount && !(sonarp->subscriptions))
      {
        motorcommand[0] = SONAR;
        motorcommand[1] = 0x3B;
        motorcommand[2] = 0;
        motorcommand[3] = 0;
        motorpacket.Build(motorcommand, 4);
        SendReceive(&motorpacket);
      }
      
      last_sonar_subscrcount = sonarp->subscriptions;
    }
    
    // we want to reset the odometry and enable the motors if the first 
    // client just subscribed to the position device, and we want to stop 
    // and disable the motors if the last client unsubscribed.
    if(positionp)
    {
      if(!last_position_subscrcount && positionp->subscriptions)
      {
        // disable motor power
        motorcommand[0] = ENABLE;
        motorcommand[1] = 0x3B;
        motorcommand[2] = 0;
        motorcommand[3] = 0;
        motorpacket.Build(motorcommand, 4);
        SendReceive(&motorpacket);//,false);

        // reset odometry
        ResetRawPositions();
      }
      else if(last_position_subscrcount && !(positionp->subscriptions))
      {
        // command motors to stop
        motorcommand[0] = VEL2;
        motorcommand[1] = 0x3B;
        motorcommand[2] = 0;
        motorcommand[3] = 0;

        // overwrite existing motor commands to be zero
        player_position_cmd_t position_cmd;
        position_cmd.xspeed = 0;
        position_cmd.yawspeed = 0;
        positionp->PutCommand((unsigned char*)(&position_cmd), 
                              sizeof(position_cmd));
      
        // disable motor power
        motorcommand[0] = ENABLE;
        motorcommand[1] = 0x3B;
        motorcommand[2] = 0;
        motorcommand[3] = 0;
        motorpacket.Build(motorcommand, 4);
        SendReceive(&motorpacket);//,false);
      }

      last_position_subscrcount = positionp->subscriptions;
    }

    
    void* client;
    player_device_id_t id;
    // first, check if there is a new config command
    if((config_size = GetConfig(&id, &client, (void*)config, sizeof(config))))
    {
      switch(id.code)
      {
        case PLAYER_SONAR_CODE:
          switch(config[0])
          {
            case PLAYER_SONAR_POWER_REQ:
              /*
               * 1 = enable sonars
               * 0 = disable sonar
               */
              if(config_size != sizeof(player_sonar_power_config_t))
              {
                puts("Arg to sonar state change request wrong size; ignoring");
                if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, 
                            NULL, NULL, 0))
                  PLAYER_ERROR("failed to PutReply");
                break;
              }
              player_sonar_power_config_t sonar_config;
              sonar_config = *((player_sonar_power_config_t*)config);

              motorcommand[0] = SONAR;
              motorcommand[1] = 0x3B;
              motorcommand[2] = sonar_config.value;
              motorcommand[3] = 0;
              motorpacket.Build(motorcommand, 4);
              SendReceive(&motorpacket);

              if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
                PLAYER_ERROR("failed to PutReply");
              break;

            case PLAYER_SONAR_GET_GEOM_REQ:
              /* Return the sonar geometry. */
              if(config_size != 1)
              {
                puts("Arg get sonar geom is wrong size; ignoring");
                if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, 
                            NULL, NULL, 0))
                  PLAYER_ERROR("failed to PutReply");
                break;
              }

              player_sonar_geom_t geom;
              geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;
              geom.pose_count = htons(PlayerRobotParams[param_idx].SonarNum);
              for (int i = 0; i < PLAYER_SONAR_MAX_SAMPLES; i++)
              {
                sonar_pose_t pose = PlayerRobotParams[param_idx].sonar_pose[i];
                geom.poses[i][0] = htons((short) (pose.x));
                geom.poses[i][1] = htons((short) (pose.y));
                geom.poses[i][2] = htons((short) (pose.th));
              }

              if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, 
                          sizeof(geom)))
                PLAYER_ERROR("failed to PutReply");
              break;
            default:
              puts("Sonar got unknown config request");
              if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK,
                          NULL, NULL, 0))
                PLAYER_ERROR("failed to PutReply");
              break;
          }
          break;
        case PLAYER_POSITION_CODE:
          switch(config[0])
          {
            case PLAYER_POSITION_MOTOR_POWER_REQ:
              /* motor state change request 
               *   1 = enable motors
               *   0 = disable motors (default)
               */
              if(config_size != sizeof(player_position_power_config_t))
              {
                puts("Arg to motor state change request wrong size; ignoring");
                if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, 
                            NULL, NULL, 0))
                  PLAYER_ERROR("failed to PutReply");
                break;
              }

              player_position_power_config_t power_config;
              power_config = *((player_position_power_config_t*)config);

              motorcommand[0] = ENABLE;
              motorcommand[1] = 0x3B;
              motorcommand[2] = power_config.value;
              motorcommand[3] = 0;
              motorpacket.Build(motorcommand, 4);
              SendReceive(&motorpacket);

              if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
                PLAYER_ERROR("failed to PutReply");
              break;

            case PLAYER_POSITION_VELOCITY_MODE_REQ:
              /* velocity control mode:
               *   0 = direct wheel velocity control (default)
               *   1 = separate translational and rotational control
               */
              if(config_size != sizeof(player_position_velocitymode_config_t))
              {
                puts("Arg to velocity control mode change request is wrong "
                     "size; ignoring");
                if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, 
                            NULL, NULL, 0))
                  PLAYER_ERROR("failed to PutReply");
                break;
              }

              player_position_velocitymode_config_t velmode_config;
              velmode_config = 
                      *((player_position_velocitymode_config_t*)config);

              if(velmode_config.value)
                direct_wheel_vel_control = false;
              else
                direct_wheel_vel_control = true;

              if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
                PLAYER_ERROR("failed to PutReply");
              break;

            case PLAYER_POSITION_RESET_ODOM_REQ:
              /* reset position to 0,0,0: no args */
              if(config_size != sizeof(player_position_resetodom_config_t))
              {
                puts("Arg to reset position request is wrong size; ignoring");
                if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, 
                            NULL, NULL, 0))
                  PLAYER_ERROR("failed to PutReply");
                break;
              }
              ResetRawPositions();

              if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
                PLAYER_ERROR("failed to PutReply");
              break;

            case PLAYER_POSITION_GET_GEOM_REQ:
            {
              /* Return the robot geometry. */
              if(config_size != 1)
              {
                puts("Arg get robot geom is wrong size; ignoring");
                if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, 
                            NULL, NULL, 0))
                  PLAYER_ERROR("failed to PutReply");
                break;
              }

              // TODO : get values from somewhere.
              player_position_geom_t geom;
              geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
              geom.pose[0] = htons((short) (-100));
              geom.pose[1] = htons((short) (0));
              geom.pose[2] = htons((short) (0));
              geom.size[0] = htons((short) (2 * 250));
              geom.size[1] = htons((short) (2 * 225));

              if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, 
                          sizeof(geom)))
                PLAYER_ERROR("failed to PutReply");
              break;
            }
            default:
              puts("Position got unknown config request");
              if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK,
                          NULL, NULL, 0))
                PLAYER_ERROR("failed to PutReply");
              break;
          }
          break;

        default:
          printf("RunPsosThread: got unknown config request \"%c\"\n",
                 config[0]);

          if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }

    /* read the clients' commands from the common buffer */
    GetCommand((unsigned char*)&command, sizeof(command));

    newmotorspeed = false;
    if( speedDemand != (short) ntohs(command.position.xspeed));
    newmotorspeed = true;
    speedDemand = (short) ntohs(command.position.xspeed);

    newmotorturn = false;
    if(turnRateDemand != (short) ntohs(command.position.yawspeed));
    newmotorturn = true;
    turnRateDemand = (short) ntohs(command.position.yawspeed);

    newgrippercommand = false;
    if(gripperCmd != command.gripper.cmd || 
       gripperArg != command.gripper.arg)
    {
      newgrippercommand = true;
    }
    gripperCmd = command.gripper.cmd;
    gripperArg = command.gripper.arg;

    /* NEXT, write commands */
    if(direct_wheel_vel_control)
    {
      // do direct wheel velocity control here
      //printf("speedDemand: %d\t turnRateDemand: %d\n",
      //speedDemand, turnRateDemand);
      rotational_term = (M_PI/180.0) * turnRateDemand /
        PlayerRobotParams[param_idx].DiffConvFactor;
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
      motorcommand[2] = (char)(rightvel /
                               PlayerRobotParams[param_idx].Vel2Divisor);
      motorcommand[3] = (char)(leftvel /
                               PlayerRobotParams[param_idx].Vel2Divisor);
    }
    else
    {
      // do separate trans and rot vels
      //
      // if trans vel is new, write it;
      // if just rot vel is new, write it;
      // if neither are new, write trans.
      if((newmotorspeed || !newmotorturn) && (num_loops_since_rvel < 2))
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
    SendReceive(&motorpacket);//,false);

    if(newgrippercommand)
    {
      //puts("sending gripper command");
      // gripper command 
      gripcommand[0] = GRIPPER;
      gripcommand[1] = 0x3B;
      *(unsigned short*)&gripcommand[2] = gripperCmd;
      grippacket.Build( gripcommand, 4);
      SendReceive(&grippacket);

      // pass extra value to gripper if needed 
      if(gripperCmd == GRIPpress || gripperCmd == LIFTcarry ) 
      {
        gripcommand[0] = GRIPPERVAL;
        gripcommand[1] = 0x3B;
        *(unsigned short*)&gripcommand[2] = (unsigned short)gripperArg;
        grippacket.Build( gripcommand, 4);
        SendReceive(&grippacket);
      }
    }

  }
  pthread_exit(NULL);
}

/* send the packet, then receive and parse an SIP */
int
P2OS::SendReceive(P2OSPacket* pkt) //, bool already_have_lock)
{
  P2OSPacket packet;
  //static SIP sippacket;
  player_p2os_data_t data;

  if((psos_fd >= 0) && sippacket)
  {
    //printf("psos_fd: %d\n", psos_fd);
    //if(!already_have_lock)
      //pthread_mutex_lock(&serial_mutex);
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

    if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB && 
       (packet.packet[3] == 0x30 || packet.packet[3] == 0x31) ||
       (packet.packet[3] == 0x32 || packet.packet[3] == 0x33) ||
       (packet.packet[3] == 0x34))
    {
      /* It is a server packet, so process it */
      sippacket->Parse( &packet.packet[3] );
      //sippacket->Print();
      sippacket->Fill(&data, timeBegan_tv );

      PutData((unsigned char*)&data, sizeof(data),0,0);
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB && 
            (packet.packet[3] == 0x50 || packet.packet[3] == 0x80) ||
            (packet.packet[3] == 0xB0 || packet.packet[3] == 0xC0) ||
            (packet.packet[3] == 0xD0 || packet.packet[3] == 0xE0))
    {
      /* It is a vision packet from the old Cognachrome system*/

      /* we don't understand these yet, so ignore */
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB && 
            (packet.packet[3] == 0x20))
    {
      //printf("got a CONFIGpac:%d\n",packet.size);
    }
    else 
    {
      puts("got unknown packet:");
      packet.PrintHex();
    }

    //pthread_mutex_unlock(&serial_mutex);
  }
  return(0);
}

void
P2OS::ResetRawPositions()
{
  P2OSPacket pkt;
  unsigned char p2oscommand[4];

  if(sippacket)
  {
    //pthread_mutex_lock(&serial_mutex);
    sippacket->rawxpos = 0;
    sippacket->rawypos = 0;
    sippacket->xpos = 0;
    sippacket->ypos = 0;
    p2oscommand[0] = SETO;
    p2oscommand[1] = 0x3B;
    pkt.Build(p2oscommand, 2);
    SendReceive(&pkt);//,true);
  }
}

/* start a thread that will invoke Main() */
void 
P2OS::StartThread()
{
  pthread_create(&thread, NULL, &DummyMain, this);
}

/* cancel (and wait for termination) of the thread */
void 
P2OS::StopThread()
{
  void* dummy;
  pthread_cancel(thread);
  if(pthread_join(thread,&dummy))
    perror("P2OS::StopThread:pthread_join()");
}


