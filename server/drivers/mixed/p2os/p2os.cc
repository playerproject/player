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

#if HAVE_CONFIG_H
  #include <config.h>
#endif

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

Driver*
P2OS_Init(ConfigFile* cf, int section)
{
  return (Driver*)(new P2OS(cf,section));
}

void P2OS_Register(DriverTable* table)
{
  table->AddDriver("p2os", P2OS_Init);
}

P2OS::P2OS(ConfigFile* cf, int section) : Driver(cf,section)
{
  player_device_id_t* ids;
  int num_ids;

  // zero ids, so that we'll know later which interfaces were requested
  memset(&this->position_id, 0, sizeof(player_device_id_t));
  memset(&this->sonar_id, 0, sizeof(player_device_id_t));
  memset(&this->aio_id, 0, sizeof(player_device_id_t));
  memset(&this->dio_id, 0, sizeof(player_device_id_t));
  memset(&this->gripper_id, 0, sizeof(player_device_id_t));
  memset(&this->bumper_id, 0, sizeof(player_device_id_t));
  memset(&this->power_id, 0, sizeof(player_device_id_t));
  memset(&this->compass_id, 0, sizeof(player_device_id_t));
  memset(&this->gyro_id, 0, sizeof(player_device_id_t));
  memset(&this->blobfinder_id, 0, sizeof(player_device_id_t));
  memset(&this->sound_id, 0, sizeof(player_device_id_t));

  this->position_subscriptions = this->sonar_subscriptions = 0;

  // Parse devices section
  if((num_ids = cf->ParseDeviceIds(section,&ids)) < 0)
  {
    this->SetError(-1);    
    return;
  }

  // Do we create a robot position interface?
  if(cf->ReadDeviceId(&(this->position_id), ids, 
                      num_ids, PLAYER_POSITION_CODE,0) == 0)
  {
    if(this->AddInterface(this->position_id, PLAYER_ALL_MODE,
                          sizeof(player_position_data_t),
                          sizeof(player_position_cmd_t), 1, 1) != 0)
    {
      this->SetError(-1);    
      free(ids);
      return;
    }
  }

  // Do we create a compass position interface?
  if(cf->ReadDeviceId(&(this->compass_id), ids, 
                      num_ids, PLAYER_POSITION_CODE,1) == 0)
  {
    if(this->AddInterface(this->compass_id, PLAYER_READ_MODE,
                          sizeof(player_position_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);
      free(ids);
      return;
    }
  }

  // Do we create a gyro position interface?
  if(cf->ReadDeviceId(&(this->gyro_id), ids, 
                      num_ids, PLAYER_POSITION_CODE,2) == 0)
  {
    if(this->AddInterface(this->gyro_id, PLAYER_READ_MODE,
                          sizeof(player_position_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);
      free(ids);
      return;
    }
  }


  // Do we create a sonar interface?
  if(cf->ReadDeviceId(&(this->sonar_id), ids, 
                      num_ids, PLAYER_SONAR_CODE, 0) == 0)
  {
    if(this->AddInterface(this->sonar_id, PLAYER_READ_MODE,
                          sizeof(player_sonar_data_t),0,1,1) != 0)
    {
      this->SetError(-1);    
      free(ids);
      return;
    }
  }


  // Do we create an aio interface?
  if(cf->ReadDeviceId(&(this->aio_id), ids, 
                      num_ids, PLAYER_AIO_CODE, 0) == 0)
  {
    if(this->AddInterface(this->aio_id, PLAYER_READ_MODE,
                          sizeof(player_aio_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);    
      free(ids);
      return;
    }
  }

  // Do we create a dio interface?
  if(cf->ReadDeviceId(&(this->dio_id), ids, 
                      num_ids, PLAYER_DIO_CODE, 0) == 0)
  {
    if(this->AddInterface(this->dio_id, PLAYER_READ_MODE,
                          sizeof(player_dio_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);    
      free(ids);
      return;
    }
  }

  // Do we create a gripper interface?
  if(cf->ReadDeviceId(&(this->gripper_id), ids, 
                      num_ids, PLAYER_GRIPPER_CODE, 0) == 0)
  {
    if(this->AddInterface(this->gripper_id, PLAYER_ALL_MODE,
                          sizeof(player_gripper_data_t), 
                          sizeof(player_gripper_cmd_t), 0, 0) != 0)
    {
      this->SetError(-1);
      free(ids);
      return;
    }
  }

  // Do we create a bumper interface?
  if(cf->ReadDeviceId(&(this->bumper_id), ids, 
                      num_ids, PLAYER_BUMPER_CODE, 0) == 0)
  {
    if(this->AddInterface(this->bumper_id, PLAYER_READ_MODE,
                          sizeof(player_bumper_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);
      free(ids);
      return;
    }
  }

  // Do we create a power interface?
  if(cf->ReadDeviceId(&(this->power_id), ids, 
                      num_ids, PLAYER_POWER_CODE, 0) == 0)
  {
    if(this->AddInterface(this->power_id, PLAYER_READ_MODE,
                          sizeof(player_power_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);
      free(ids);
      return;
    }
  }

  // Do we create a blobfinder interface?
  if(cf->ReadDeviceId(&(this->blobfinder_id), ids, 
                      num_ids, PLAYER_BLOBFINDER_CODE, 0) == 0)
  {
    if(this->AddInterface(this->blobfinder_id, PLAYER_READ_MODE,
                          sizeof(player_blobfinder_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);
      free(ids);
      return;
    }
  }

  // Do we create a sound interface?
  if(cf->ReadDeviceId(&(this->sound_id), ids, 
                      num_ids, PLAYER_SOUND_CODE, 0) == 0)
  {
    if(this->AddInterface(this->sound_id, PLAYER_WRITE_MODE,
                          0, sizeof(player_sound_cmd_t), 0, 0) != 0)
    {
      this->SetError(-1);
      free(ids);
      return;
    }
  }

  // check for unused ids
  if(cf->UnusedIds(section,ids,num_ids))
  {
    this->SetError(-1);
    free(ids);
    return;
  }

  // we're done with the list of ids now.
  free(ids);

  // build the table of robot parameters.
  ::initialize_robot_params();
  
  // Read config file options
  this->psos_serial_port = cf->ReadString(section,"port",DEFAULT_P2OS_PORT);
  this->radio_modemp = cf->ReadInt(section, "radio", radio_modemp);
  this->joystickp = cf->ReadInt(section, "joystick", joystickp);
  this->direct_wheel_vel_control = 
          cf->ReadInt(section, "direct_wheel_vel_control", 1);
  this->motor_max_speed = cf->ReadInt(section, "max_xspeed", 
                                      MOTOR_DEF_MAX_SPEED);
  this->motor_max_turnspeed = cf->ReadInt(section, "max_yawspeed", 
                                          MOTOR_DEF_MAX_TURNSPEED);
  this->use_vel_band = cf->ReadInt(section, "use_vel_band", 0);

  this->psos_fd = -1;

  this->sent_gripper_cmd = false;
}

int P2OS::Setup()
{
  int i;
  // this is the order in which we'll try the possible baud rates. we try 9600
  // first because most robots use it, and because otherwise the radio modem
  // connection code might not work (i think that the radio modems operate at
  // 9600).
  int bauds[] = {B9600, B38400, B19200, B115200, B57600};
  int numbauds = sizeof(bauds);
  int currbaud = 0;

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

  printf("P2OS connection initializing (%s)...",this->psos_serial_port);
  fflush(stdout);

  if((this->psos_fd = open(this->psos_serial_port, 
                     O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 )
  {
    perror("P2OS::Setup():open():");
    return(1);
  }  
 
  if(tcgetattr( this->psos_fd, &term ) < 0 )
  {
    perror("P2OS::Setup():tcgetattr():");
    close(this->psos_fd);
    this->psos_fd = -1;
    return(1);
  }

#if HAVE_CFMAKERAW
  cfmakeraw( &term );
#endif
  cfsetispeed(&term, bauds[currbaud]);
  cfsetospeed(&term, bauds[currbaud]);
  
  if(tcsetattr(this->psos_fd, TCSAFLUSH, &term ) < 0)
  {
    perror("P2OS::Setup():tcsetattr():");
    close(this->psos_fd);
    this->psos_fd = -1;
    return(1);
  }

  if(tcflush(this->psos_fd, TCIOFLUSH ) < 0)
  {
    perror("P2OS::Setup():tcflush():");
    close(this->psos_fd);
    this->psos_fd = -1;
    return(1);
  }

  if((flags = fcntl(this->psos_fd, F_GETFL)) < 0)
  {
    perror("P2OS::Setup():fcntl()");
    close(this->psos_fd);
    this->psos_fd = -1;
    return(1);
  }

  // radio modem initialization code, courtesy of Kim Jinsuck 
  //   <jinsuckk@cs.tamu.edu>
  if(this->radio_modemp)
  {
    puts("Initializing radio modem...");
    write(this->psos_fd, "WMS2\r", 5);

    usleep(50000);
    char modem_buf[50];
    int buf_len = read(this->psos_fd, modem_buf, 5);          // get "WMS2"
    modem_buf[buf_len]='\0';
    printf("wireless modem response = %s\n", modem_buf);

    usleep(10000);
    // get "\n\rConnecting..." --> \n\r is my guess
    buf_len = read(this->psos_fd, modem_buf, 14); 
    modem_buf[buf_len]='\0';
    printf("wireless modem response = %s\n", modem_buf);

    // wait until get "Connected to address 2"
    int modem_connect_try = 10;
    while(strstr(modem_buf, "ected to addres") == NULL)
    {
      usleep(300000);
      buf_len = read(this->psos_fd, modem_buf, 40);
      modem_buf[buf_len]='\0';
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
        puts("Failed to connect radio modem, Trying direct connection...");
        break;
      }
    }
  }

  int num_sync_attempts = 3;
  while(psos_state != READY)
  {
    switch(psos_state)
    {
      case NO_SYNC:
        command = SYNC0;
        packet.Build(&command, 1);
        packet.Send(this->psos_fd);
        usleep(P2OS_CYCLETIME_USEC);
        break;
      case AFTER_FIRST_SYNC:
        if(fcntl(this->psos_fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
        {
          perror("P2OS::Setup():fcntl()");
          close(this->psos_fd);
          this->psos_fd = -1;
          return(1);
        }
        command = SYNC1;
        packet.Build(&command, 1);
        packet.Send(this->psos_fd);
        break;
      case AFTER_SECOND_SYNC:
        command = SYNC2;
        packet.Build(&command, 1);
        packet.Send(this->psos_fd);
        break;
      default:
        puts("P2OS::Setup():shouldn't be here...");
        break;
    }
    usleep(P2OS_CYCLETIME_USEC);

    if(receivedpacket.Receive(this->psos_fd))
    {
      if((psos_state == NO_SYNC) && (num_sync_attempts >= 0))
      {
        num_sync_attempts--;
        usleep(P2OS_CYCLETIME_USEC);
        continue;
      }
      else
      {
        // couldn't connect; try different speed.
        if(++currbaud < numbauds)
        {
          cfsetispeed(&term, bauds[currbaud]);
          cfsetospeed(&term, bauds[currbaud]);
          if( tcsetattr(this->psos_fd, TCSAFLUSH, &term ) < 0 )
          {
            perror("P2OS::Setup():tcsetattr():");
            close(this->psos_fd);
            this->psos_fd = -1;
            return(1);
          }

          if(tcflush(this->psos_fd, TCIOFLUSH ) < 0 )
          {
            perror("P2OS::Setup():tcflush():");
            close(this->psos_fd);
            this->psos_fd = -1;
            return(1);
          }
          num_sync_attempts = 3;
          continue;
        }
        else
        {
          // tried all speeds; bail
          break;
        }
      }
    }
    
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
          packet.Send(this->psos_fd);
          sent_close = true;
          usleep(2*P2OS_CYCLETIME_USEC);
          tcflush(this->psos_fd,TCIFLUSH);
          psos_state = NO_SYNC;
        }
        break;
    }
    usleep(P2OS_CYCLETIME_USEC);
  }

  if(psos_state != READY)
  {
    printf("Couldn't synchronize with P2OS.\n"  
           "  Most likely because the robot is not connected to %s\n", 
           this->psos_serial_port);
    close(this->psos_fd);
    this->psos_fd = -1;
    return(1);
  }

  cnt = 4;
  cnt += sprintf(name, "%s", &receivedpacket.packet[cnt]);
  cnt++;
  cnt += sprintf(type, "%s", &receivedpacket.packet[cnt]);
  cnt++;
  cnt += sprintf(subtype, "%s", &receivedpacket.packet[cnt]);
  cnt++;


  command = OPEN;
  packet.Build(&command, 1);
  packet.Send(this->psos_fd);
  usleep(P2OS_CYCLETIME_USEC);

  command = PULSE;
  packet.Build(&command, 1);
  packet.Send(this->psos_fd);
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
  
  // first, receive a packet so we know we're connected.
  if(!this->sippacket)
    this->sippacket = new SIP(param_idx);

  this->sippacket->x_offset = 0;
  this->sippacket->y_offset = 0;
  this->sippacket->angle_offset = 0;

  SendReceive((P2OSPacket*)NULL);

  // turn off the sonars at first
  this->ToggleSonarPower(0);

  if(this->joystickp)
  {
    // enable joystick control
    P2OSPacket js_packet;
    unsigned char js_command[4];
    js_command[0] = JOYDRIVE;
    js_command[1] = ARGINT;
    js_command[2] = 1;
    js_command[3] = 0;
    js_packet.Build(js_command, 4);
    this->SendReceive(&js_packet);
  }

  if(this->blobfinder_id.code)
    CMUcamReset();

  if(this->gyro_id.code)
  {
    // request that gyro data be sent each cycle
    P2OSPacket gyro_packet;
    unsigned char gyro_command[4];
    gyro_command[0] = GYRO;
    gyro_command[1] = ARGINT;
    gyro_command[2] = 1;
    gyro_command[3] = 0;
    gyro_packet.Build(gyro_command, 4);
    this->SendReceive(&gyro_packet);
  }

  // zero position command buffer
  player_position_cmd_t zero;
  memset(&zero,0,sizeof(player_position_cmd_t));
  this->PutCommand(this->position_id,(void*)&zero,
                   sizeof(player_position_cmd_t),NULL);
  
  /* now spawn reading thread */
  this->StartThread();
  return(0);
}

int P2OS::Shutdown()
{
  unsigned char command[20],buffer[20];
  P2OSPacket packet; 

  memset(buffer,0,20);

  if(this->psos_fd == -1)
    return(0);

  this->StopThread();

  command[0] = STOP;
  packet.Build(command, 1);
  packet.Send(this->psos_fd);
  usleep(P2OS_CYCLETIME_USEC);

  command[0] = CLOSE;
  packet.Build(command, 1);
  packet.Send(this->psos_fd);
  usleep(P2OS_CYCLETIME_USEC);

  close(this->psos_fd);
  this->psos_fd = -1;
  puts("P2OS has been shutdown");
  delete this->sippacket;
  this->sippacket = NULL;

  return(0);
}

int 
P2OS::Subscribe(player_device_id_t id)
{
  int setupResult;

  // do the subscription
  if((setupResult = Driver::Subscribe(id)) == 0)
  {
    // also increment the appropriate subscription counter
    switch(id.code)
    {
      case PLAYER_POSITION_CODE:
        this->position_subscriptions++;
        break;
      case PLAYER_SONAR_CODE:
        this->sonar_subscriptions++;
        break;
    }
  }

  return(setupResult);
}

int 
P2OS::Unsubscribe(player_device_id_t id)
{
  int shutdownResult;

  // do the unsubscription
  if((shutdownResult = Driver::Unsubscribe(id)) == 0)
  {
    // also decrement the appropriate subscription counter
    switch(id.code)
    {
      case PLAYER_POSITION_CODE:
        assert(--this->position_subscriptions >= 0);
        break;
      case PLAYER_SONAR_CODE:
        assert(--this->sonar_subscriptions >= 0);
        break;
	/*
      default:
        PLAYER_ERROR1("got unsubscription for unknown interface %d", id.code);
        assert(false);
	*/
    }
  }

  return(shutdownResult);
}

void 
P2OS::PutData(void)
{
  // TODO: something smarter about timestamping.

  // put position data
  Driver::PutData(this->position_id, 
                  (void*)&(this->p2os_data.position), 
                  sizeof(player_position_data_t),
                  NULL);

  // put sonar data
  Driver::PutData(this->sonar_id, 
                  (void*)&(this->p2os_data.sonar), 
                  sizeof(player_sonar_data_t),
                  NULL);
  
  // put aio data
  Driver::PutData(this->aio_id, 
                  (void*)&(this->p2os_data.aio), 
                  sizeof(player_aio_data_t),
                  NULL);

  // put dio data
  Driver::PutData(this->dio_id, 
                  (void*)&(this->p2os_data.dio), 
                  sizeof(player_dio_data_t),
                  NULL);

  // put gripper data
  Driver::PutData(this->gripper_id, 
                  (void*)&(this->p2os_data.gripper), 
                  sizeof(player_gripper_data_t),
                  NULL);

  // put bumper data
  Driver::PutData(this->bumper_id, 
                  (void*)&(this->p2os_data.bumper), 
                  sizeof(player_bumper_data_t),
                  NULL);

  // put power data
  Driver::PutData(this->power_id, 
                  (void*)&(this->p2os_data.power), 
                  sizeof(player_power_data_t),
                  NULL);

  // put compass data
  Driver::PutData(this->compass_id, 
                  (void*)&(this->p2os_data.compass), 
                  sizeof(player_position_data_t),
                  NULL);

  // put gyro data
  Driver::PutData(this->gyro_id, 
                  (void*)&(this->p2os_data.gyro), 
                  sizeof(player_position_data_t),
                  NULL);

  // put blobfinder data
  Driver::PutData(this->blobfinder_id, 
                  (void*)&(this->p2os_data.blobfinder), 
                  sizeof(player_blobfinder_data_t),
                  NULL);
}

void 
P2OS::Main()
{
  int last_sonar_subscrcount=0;
  int last_position_subscrcount=0;

  for(;;)
  {
    // we want to turn on the sonars if someone just subscribed, and turn
    // them off if the last subscriber just unsubscribed.
    if(!last_sonar_subscrcount && this->sonar_subscriptions)
      this->ToggleSonarPower(1);
    else if(last_sonar_subscrcount && !(this->sonar_subscriptions))
      this->ToggleSonarPower(0);
    last_sonar_subscrcount = this->sonar_subscriptions;
    
    // we want to reset the odometry and enable the motors if the first 
    // client just subscribed to the position device, and we want to stop 
    // and disable the motors if the last client unsubscribed.
    if(!last_position_subscrcount && this->position_subscriptions)
    {
      this->ToggleMotorPower(0);
      this->ResetRawPositions();
    }
    else if(last_position_subscrcount && !(this->position_subscriptions))
    {
      // overwrite existing motor commands to be zero
      player_position_cmd_t position_cmd;
      position_cmd.xspeed = 0;
      position_cmd.yawspeed = 0;
      this->PutCommand(this->position_id,(void*)(&position_cmd), 
                       sizeof(player_position_cmd_t),NULL);

      // enable motor power
      this->ToggleMotorPower(1);
    }
    last_position_subscrcount = this->position_subscriptions;

    // The Amigo board seems to drop commands once in a while.  This is
    // a hack to restart the serial reads if that happens.
    if(this->blobfinder_id.code)
    {
      struct timeval now_tv;
      GlobalTime->GetTime(&now_tv);
      if (now_tv.tv_sec > lastblob_tv.tv_sec) 
      {
        P2OSPacket cam_packet;
        unsigned char cam_command[4];

        cam_command[0] = GETAUX2;
        cam_command[1] = ARGINT;
        cam_command[2] = 0;
        cam_command[3] = 0;
        cam_packet.Build(cam_command, 4);
        SendReceive(&cam_packet);

        cam_command[0] = GETAUX2;
        cam_command[1] = ARGINT;
        cam_command[2] = CMUCAM_MESSAGE_LEN * 2 -1;
        cam_command[3] = 0;
        cam_packet.Build(cam_command, 4);
        SendReceive(&cam_packet);
        GlobalTime->GetTime(&lastblob_tv);	// Reset last blob packet time
      }
    }
    
    // handle pending config requests
    this->HandleConfig();

    // process latest commands
    this->GetCommand();
  }
  pthread_exit(NULL);
}

/* send the packet, then receive and parse an SIP */
int
P2OS::SendReceive(P2OSPacket* pkt)
{
  P2OSPacket packet;

  // zero the combined data buffer.  it will be filled with the latest data
  // by SIP::Fill()
  memset(&(this->p2os_data),0,sizeof(player_p2os_data_t));

  if((this->psos_fd >= 0) && this->sippacket)
  {
    if(pkt)
      pkt->Send(this->psos_fd);

    /* receive a packet */
    pthread_testcancel();
    if(packet.Receive(this->psos_fd))
    {
      puts("RunPsosThread(): Receive errored");
      pthread_exit(NULL);
    }

    if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB && 
       (packet.packet[3] == 0x30 || packet.packet[3] == 0x31) ||
       (packet.packet[3] == 0x32 || packet.packet[3] == 0x33) ||
       (packet.packet[3] == 0x34))
    {

      /* It is a server packet, so process it */
      this->sippacket->Parse( &packet.packet[3] );
      this->sippacket->Fill(&(this->p2os_data));

      this->PutData();
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB &&
            packet.packet[3] == SERAUX)
    {
       // This is an AUX serial packet
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB &&
            packet.packet[3] == SERAUX2)
    {
      // This is an AUX2 serial packet
      if(blobfinder_id.code)
      {
        /* It is an extended SIP (blobfinder) packet, so process it */
        /* Be sure to pass data size too (packet[2])! */
        this->sippacket->ParseSERAUX( &packet.packet[2] );
        this->sippacket->Fill(&(this->p2os_data));

        this->PutData();

        P2OSPacket cam_packet;
        unsigned char cam_command[4];

        /* We cant get the entire contents of the buffer,
        ** and we cant just have P2OS send us the buffer on a regular basis.
        ** My solution is to flush the buffer and then request exactly
        ** CMUCAM_MESSAGE_LEN * 2 -1 bytes of data.  This ensures that
        ** we will get exactly one full message, and it will be "current"
        ** within the last 2 messages.  Downside is that we end up pitching
        ** every other CMUCAM message.  Tradeoffs... */
        // Flush
        cam_command[0] = GETAUX2;
        cam_command[1] = ARGINT;
        cam_command[2] = 0;
        cam_command[3] = 0;
        cam_packet.Build(cam_command, 4);
        this->SendReceive(&cam_packet);

        // Reqest next packet
        cam_command[0] = GETAUX2;
        cam_command[1] = ARGINT;
        // Guarantee exactly 1 full message
        cam_command[2] = CMUCAM_MESSAGE_LEN * 2 -1;
        cam_command[3] = 0;
        cam_packet.Build(cam_command, 4);
        this->SendReceive(&cam_packet);
        GlobalTime->GetTime(&lastblob_tv);	// Reset last blob packet time
      }
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB && 
            (packet.packet[3] == 0x50 || packet.packet[3] == 0x80) ||
//            (packet.packet[3] == 0xB0 || packet.packet[3] == 0xC0) ||
            (packet.packet[3] == 0xC0) ||
            (packet.packet[3] == 0xD0 || packet.packet[3] == 0xE0))
    {
      /* It is a vision packet from the old Cognachrome system*/

      /* we don't understand these yet, so ignore */
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB &&
            packet.packet[3] == GYROPAC)
    {
      if(this->gyro_id.code)
      {
        /* It's a set of gyro measurements */
        this->sippacket->ParseGyro(&packet.packet[2]);
        this->sippacket->Fill(&(this->p2os_data));
        this->PutData();

        /* Now, the manual says that we get one gyro packet each cycle,
         * right before the standard SIP.  So, we'll call SendReceive() 
         * again (with no packet to send) to get the standard SIP.  There's 
         * a definite danger of infinite recursion here if the manual
         * is wrong.
         */
        this->SendReceive(NULL);
      }
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB && 
            (packet.packet[3] == 0x20))
    {
      //printf("got a CONFIGpac:%d\n",packet.size);
    }
    else 
    {
      PLAYER_WARN("got unknown packet:");
      packet.PrintHex();
    }
  }
  return(0);
}

void
P2OS::ResetRawPositions()
{
  P2OSPacket pkt;
  unsigned char p2oscommand[4];

  if(this->sippacket)
  {
    this->sippacket->rawxpos = 0;
    this->sippacket->rawypos = 0;
    this->sippacket->xpos = 0;
    this->sippacket->ypos = 0;
    p2oscommand[0] = SETO;
    p2oscommand[1] = ARGINT;
    pkt.Build(p2oscommand, 2);
    this->SendReceive(&pkt);
  }
}


/****************************************************************
** Reset the CMUcam.  This includes flushing the buffer and
** setting interface output mode to raw.  It also restarts
** tracking output (current mode)
****************************************************************/
void P2OS::CMUcamReset()
{
  CMUcamStopTracking();	// Stop the current tracking.

  P2OSPacket cam_packet;
  unsigned char cam_command[8];

  printf("Resetting the CMUcam...\n");
  cam_command[0] = TTY3;
  cam_command[1] = ARGSTR;
  sprintf((char*)&cam_command[3], "RS\r");
  cam_command[2] = strlen((char *)&cam_command[3]);
  cam_packet.Build(cam_command, (int)cam_command[2]+3);
  this->SendReceive(&cam_packet);

  // Set for raw output + no ACK/NACK
  printf("Setting raw mode...\n");
  cam_command[0] = TTY3;
  cam_command[1] = ARGSTR;
  sprintf((char*)&cam_command[3], "RM 3\r");
  cam_command[2] = strlen((char *)&cam_command[3]);
  cam_packet.Build(cam_command, (int)cam_command[2]+3);
  this->SendReceive(&cam_packet);
  usleep(100000);

  printf("Flushing serial buffer...\n");
  cam_command[0] = GETAUX2;
  cam_command[1] = ARGINT;
  cam_command[2] = 0;
  cam_command[3] = 0;
  cam_packet.Build(cam_command, 4);
  this->SendReceive(&cam_packet);

  sleep(1);
  // (Re)start tracking
  this->CMUcamTrack();
}


/****************************************************************
** Start CMUcam blob tracking.  This method can be called 3 ways:
**   1) with a set of 6 color arguments (RGB min and max)
**   2) with auto tracking (-1 argument)
**   3) with current values (0 or no arguments)
****************************************************************/
void P2OS::CMUcamTrack(int rmin, int rmax,
                       int gmin, int gmax,
                       int bmin, int bmax)
{
  this->CMUcamStopTracking();	// Stop the current tracking.

  P2OSPacket cam_packet;
  unsigned char cam_command[50];

  if (!rmin && !rmax && !gmin && !gmax && !bmin && !bmax)
  {
    // Then start it up with current values.
    cam_command[0] = TTY3;
    cam_command[1] = ARGSTR;
    sprintf((char*)&cam_command[3], "TC\r");
    cam_command[2] = strlen((char *)&cam_command[3]);
    cam_packet.Build(cam_command, (int)cam_command[2]+3);
    this->SendReceive(&cam_packet);
  }
  else if (rmin<0 || rmax<0 || gmin<0 || gmax<0 || bmin<0 || bmax<0)
  {
    printf("Activating CMUcam color tracking (AUTO-mode)...\n");
    cam_command[0] = TTY3;
    cam_command[1] = ARGSTR;
    sprintf((char*)&cam_command[3], "TW\r");
    cam_command[2] = strlen((char *)&cam_command[3]);
    cam_packet.Build(cam_command, (int)cam_command[2]+3);
    this->SendReceive(&cam_packet);
  }
  else
  {
    printf("Activating CMUcam color tracking (MANUAL-mode)...\n");
    //printf("      RED: %d %d    GREEN: %d %d    BLUE: %d %d\n",
    //                   rmin, rmax, gmin, gmax, bmin, bmax);
    cam_command[0] = TTY3;
    cam_command[1] = ARGSTR;
    sprintf((char*)&cam_command[3], "TC %d %d %d %d %d %d\r", 
             rmin, rmax, gmin, gmax, bmin, bmax);
    cam_command[2] = strlen((char *)&cam_command[3]);
    cam_packet.Build(cam_command, (int)cam_command[2]+3);
    this->SendReceive(&cam_packet);
  }

  cam_command[0] = GETAUX2;
  cam_command[1] = ARGINT;
  cam_command[2] = CMUCAM_MESSAGE_LEN * 2 -1;	// Guarantee 1 full message
  cam_command[3] = 0;
  cam_packet.Build(cam_command, 4);
  this->SendReceive(&cam_packet);
}


/****************************************************************
** Stop Tracking - This should be done before any new command
** are issued to the CMUcam.
****************************************************************/
void P2OS::CMUcamStopTracking()
{
  P2OSPacket cam_packet;
  unsigned char cam_command[50];

  // First we must STOP tracking.  Just send a return.
  cam_command[0] = TTY3;
  cam_command[1] = ARGSTR;
  sprintf((char*)&cam_command[3], "\r");
  cam_command[2] = strlen((char *)&cam_command[3]);
  cam_packet.Build(cam_command, (int)cam_command[2]+3);
  this->SendReceive(&cam_packet);
}

/* toggle sonars on/off, according to val */
void
P2OS::ToggleSonarPower(unsigned char val)
{
  unsigned char command[4];
  P2OSPacket packet; 

  command[0] = SONAR;
  command[1] = ARGINT;
  command[2] = val;
  command[3] = 0;
  packet.Build(command, 4);
  SendReceive(&packet);
}

/* toggle motors on/off, according to val */
void
P2OS::ToggleMotorPower(unsigned char val)
{
  unsigned char command[4];
  P2OSPacket packet; 

  command[0] = ENABLE;
  command[1] = ARGINT;
  command[2] = val;
  command[3] = 0;
  packet.Build(command, 4);
  SendReceive(&packet);
}

void
P2OS::HandleConfig(void)
{
  void* client;
  unsigned char config[PLAYER_MAX_REQREP_SIZE];
  int config_size;

  // check for position config requests
  if((config_size = this->GetConfig(this->position_id, &client, 
                                    (void*)config, sizeof(config),NULL)) > 0)
  {
    switch(config[0])
    {
      case PLAYER_POSITION_SET_ODOM_REQ:
        if(config_size != sizeof(player_position_set_odom_req_t))
        {
          PLAYER_WARN("Arg to odometry set requests wrong size; ignoring");
          this->PutReply(this->position_id, client,
                         PLAYER_MSGTYPE_RESP_NACK, NULL);
        }
        else
        {
          player_position_set_odom_req_t set_odom_req;
          set_odom_req = *((player_position_set_odom_req_t*)config);

          this->sippacket->x_offset = ((int)ntohl(set_odom_req.x)) -
                  this->sippacket->xpos;
          this->sippacket->y_offset = ((int)ntohl(set_odom_req.y)) -
                  this->sippacket->ypos;
          this->sippacket->angle_offset = ((int)ntohl(set_odom_req.theta)) -
                  this->sippacket->angle;

          this->PutReply(this->position_id, client, 
                         PLAYER_MSGTYPE_RESP_ACK, NULL);
        }
        break;

      case PLAYER_POSITION_MOTOR_POWER_REQ:
        /* motor state change request 
         *   1 = enable motors
         *   0 = disable motors (default)
         */
        if(config_size != sizeof(player_position_power_config_t))
        {
          PLAYER_WARN("Arg to motor state change request wrong size; ignoring");
          this->PutReply(this->position_id, client, 
                         PLAYER_MSGTYPE_RESP_NACK, NULL);
        }
        else
        {
          player_position_power_config_t power_config;
          power_config = *((player_position_power_config_t*)config);
          this->ToggleMotorPower(power_config.value);

          this->PutReply(this->position_id, client, 
                         PLAYER_MSGTYPE_RESP_ACK, NULL);
        }
        break;

      case PLAYER_POSITION_RESET_ODOM_REQ:
        /* reset position to 0,0,0: no args */
        if(config_size != sizeof(player_position_resetodom_config_t))
        {
          PLAYER_WARN("Arg to reset position request is wrong size; ignoring");
          this->PutReply(this->position_id, client, 
                         PLAYER_MSGTYPE_RESP_NACK, NULL);
        }
        else
        {
          ResetRawPositions();

          this->PutReply(this->position_id, client, 
                         PLAYER_MSGTYPE_RESP_ACK, NULL);
        }
        break;

      case PLAYER_POSITION_GET_GEOM_REQ:
        /* Return the robot geometry. */
        if(config_size != 1)
        {
          PLAYER_WARN("Arg get robot geom is wrong size; ignoring");
          this->PutReply(this->position_id, client, 
                   PLAYER_MSGTYPE_RESP_NACK, NULL);
        }
        else
        {
          player_position_geom_t geom;
          geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
          // TODO: Figure out this rotation offset somehow; it's not 
          //       given in the Saphira parameters.  For now, -100 is 
          //       about right for a Pioneer 2DX.
          geom.pose[0] = htons((short) (-100));
          geom.pose[1] = htons((short) (0));
          geom.pose[2] = htons((short) (0));
          // get dimensions from the parameter table
          //geom.size[0] = htons((short) (2 * 250));
          //geom.size[1] = htons((short) (2 * 225));
          geom.size[0] = 
                  htons((short)PlayerRobotParams[param_idx].RobotLength);
          geom.size[1] = 
                  htons((short)PlayerRobotParams[param_idx].RobotWidth);

          this->PutReply(this->position_id, client, 
                         PLAYER_MSGTYPE_RESP_ACK, &geom, sizeof(geom), NULL);
        }
        break;
      default:
        PLAYER_WARN("Position got unknown config request");
        this->PutReply(this->position_id, client, 
                       PLAYER_MSGTYPE_RESP_NACK, NULL);
        break;
    }
  }

  // check for sonar config requests
  if((config_size = this->GetConfig(this->sonar_id, &client, 
                                    (void*)config, sizeof(config),NULL)) > 0)
  {
    switch(config[0])
    {
      case PLAYER_SONAR_POWER_REQ:
        /*
         * 1 = enable sonars
         * 0 = disable sonar
         */
        if(config_size != sizeof(player_sonar_power_config_t))
        {
          PLAYER_WARN("Arg to sonar state change request wrong size; ignoring");
          this->PutReply(this->sonar_id, client, 
                         PLAYER_MSGTYPE_RESP_NACK, NULL);
        }
        else
        {
          player_sonar_power_config_t sonar_config;
          sonar_config = *((player_sonar_power_config_t*)config);
          this->ToggleSonarPower(sonar_config.value);

          this->PutReply(this->sonar_id, client, 
                         PLAYER_MSGTYPE_RESP_ACK, NULL);
        }
        break;

      case PLAYER_SONAR_GET_GEOM_REQ:
        /* Return the sonar geometry. */
        if(config_size != 1)
        {
          PLAYER_WARN("Arg get sonar geom is wrong size; ignoring");
          this->PutReply(this->sonar_id, client, 
                         PLAYER_MSGTYPE_RESP_NACK, NULL);
        }
        else
        {
          player_sonar_geom_t geom;
          geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;
          geom.pose_count = htons(PlayerRobotParams[param_idx].SonarNum);
          for(int i = 0; i < PLAYER_SONAR_MAX_SAMPLES; i++)
          {
            sonar_pose_t pose = PlayerRobotParams[param_idx].sonar_pose[i];
            geom.poses[i][0] = htons((short) (pose.x));
            geom.poses[i][1] = htons((short) (pose.y));
            geom.poses[i][2] = htons((short) (pose.th));
          }

          this->PutReply(this->sonar_id, client, 
                         PLAYER_MSGTYPE_RESP_ACK, &geom, sizeof(geom),NULL);
        }
        break;

      default:
        PLAYER_WARN("Sonar got unknown config request");
        this->PutReply(this->sonar_id, client, 
                       PLAYER_MSGTYPE_RESP_NACK, NULL);
        break;
    }
  }

  // check for blobfinder requests
  if((config_size = this->GetConfig(this->blobfinder_id, &client, 
                                    (void*)config, sizeof(config),NULL)) > 0)
  {
    switch(config[0])
    {
      case PLAYER_BLOBFINDER_SET_COLOR_REQ:
        // Set the tracking color (RGB max/min values)

        if(config_size != sizeof(player_blobfinder_color_config_t))
        {
          puts("Arg to blobfinder color request wrong size; ignoring");
          if(PutReply(this->blobfinder_id, client, 
                      PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
        }
        player_blobfinder_color_config_t color_config;
        color_config = *((player_blobfinder_color_config_t*)config);

        CMUcamTrack( (short)ntohs(color_config.rmin),
                     (short)ntohs(color_config.rmax), 
                     (short)ntohs(color_config.gmin),
                     (short)ntohs(color_config.gmax),
                     (short)ntohs(color_config.bmin),
                     (short)ntohs(color_config.bmax) );

        //printf("Color Tracking parameter updated.\n");

        if(PutReply(this->blobfinder_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
          PLAYER_ERROR("failed to PutReply");
        break;

      case PLAYER_BLOBFINDER_SET_IMAGER_PARAMS_REQ:
        // Set the imager control params
        if(config_size != sizeof(player_blobfinder_imager_config_t))
        {
          puts("Arg to blobfinder imager request wrong size; ignoring");
          if(PutReply(this->blobfinder_id, client, 
                      PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
        }
        player_blobfinder_imager_config_t imager_config;
        imager_config = *((player_blobfinder_imager_config_t*)config);

        P2OSPacket cam_packet;
        unsigned char cam_command[50];
        int np;

        np=3;

        CMUcamStopTracking();	// Stop the current tracking.

        cam_command[0] = TTY3;
        cam_command[1] = ARGSTR;
        np += sprintf((char*)&cam_command[np], "CR ");

        if ((short)ntohs(imager_config.brightness) >= 0)
          np += sprintf((char*)&cam_command[np], " 6 %d",
                        ntohs(imager_config.brightness));

        if ((short)ntohs(imager_config.contrast) >= 0)
          np += sprintf((char*)&cam_command[np], " 5 %d",
                        ntohs(imager_config.contrast));

        if (imager_config.autogain >= 0)
          if (imager_config.autogain == 0)
            np += sprintf((char*)&cam_command[np], " 19 32");
          else
            np += sprintf((char*)&cam_command[np], " 19 33");

        if (imager_config.colormode >= 0)
          if (imager_config.colormode == 3)
            np += sprintf((char*)&cam_command[np], " 18 36");
          else if (imager_config.colormode == 2)
            np += sprintf((char*)&cam_command[np], " 18 32");
          else if (imager_config.colormode == 1)
            np += sprintf((char*)&cam_command[np], " 18 44");
          else
            np += sprintf((char*)&cam_command[np], " 18 40");

        if (np > 6)
        {
          sprintf((char*)&cam_command[np], "\r");
          cam_command[2] = strlen((char *)&cam_command[3]);
          cam_packet.Build(cam_command, (int)cam_command[2]+3);
          SendReceive(&cam_packet);

          printf("Blobfinder imager parameters updated.\n");
          printf("       %s\n", &cam_command[3]);
        } else
          printf("Blobfinder imager parameters NOT updated.\n");

        CMUcamTrack(); 	// Restart tracking

        if(PutReply(this->blobfinder_id, client, 
                    PLAYER_MSGTYPE_RESP_ACK, NULL))
          PLAYER_ERROR("failed to PutReply");
        break;

      default:
        PLAYER_WARN("unknown config request to blobfinder interface");
        if(PutReply(this->blobfinder_id, client, 
                    PLAYER_MSGTYPE_RESP_NACK, NULL))
          PLAYER_ERROR("failed to PutReply");
        break;
    }
  }
}

void
P2OS::HandlePositionCommand(player_position_cmd_t position_cmd)
{
  int speedDemand, turnRateDemand;
  double leftvel, rightvel;
  double rotational_term;
  unsigned short absspeedDemand, absturnRateDemand;
  unsigned char motorcommand[4];
  P2OSPacket motorpacket; 

  speedDemand = (int)ntohl(position_cmd.xspeed);
  turnRateDemand = (int) ntohl(position_cmd.yawspeed);

  if(this->direct_wheel_vel_control)
  {
    // convert xspeed and yawspeed into wheelspeeds
    rotational_term = (M_PI/180.0) * turnRateDemand /
            PlayerRobotParams[param_idx].DiffConvFactor;
    leftvel = (speedDemand - rotational_term);
    rightvel = (speedDemand + rotational_term);

    // Apply wheel speed bounds
    if(fabs(leftvel) > this->motor_max_speed)
    {
      if(leftvel > 0)
      {
        leftvel = this->motor_max_speed;
        rightvel *= this->motor_max_speed/leftvel;
        puts("Left wheel velocity threshholded!");
      }
      else
      {
        leftvel = -this->motor_max_speed;
        rightvel *= -this->motor_max_speed/leftvel;
      }
    }
    if(fabs(rightvel) > this->motor_max_speed)
    {
      if(rightvel > 0)
      {
        rightvel = this->motor_max_speed;
        leftvel *= this->motor_max_speed/rightvel;
        puts("Right wheel velocity threshholded!");
      }
      else
      {
        rightvel = -this->motor_max_speed;
        leftvel *= -this->motor_max_speed/rightvel;
      }
    }

    // Apply control band bounds
    if(this->use_vel_band)
    {
      // This band prevents the wheels from turning in opposite
      // directions
      if (leftvel * rightvel < 0)
      {
        if (leftvel + rightvel >= 0)
        {
          if (leftvel < 0)
            leftvel = 0;
          if (rightvel < 0)
            rightvel = 0;
        }
        else
        {
          if (leftvel > 0)
            leftvel = 0;
          if (rightvel > 0)
            rightvel = 0;
        }
      }
    }

    // Apply byte range bounds
    if (leftvel / PlayerRobotParams[param_idx].Vel2Divisor > 126)
      leftvel = 126 * PlayerRobotParams[param_idx].Vel2Divisor;
    if (leftvel / PlayerRobotParams[param_idx].Vel2Divisor < -126)
      leftvel = -126 * PlayerRobotParams[param_idx].Vel2Divisor;
    if (rightvel / PlayerRobotParams[param_idx].Vel2Divisor > 126)
      rightvel = 126 * PlayerRobotParams[param_idx].Vel2Divisor;
    if (rightvel / PlayerRobotParams[param_idx].Vel2Divisor < -126)
      rightvel = -126 * PlayerRobotParams[param_idx].Vel2Divisor;

    // send the speed command
    motorcommand[0] = VEL2;
    motorcommand[1] = ARGINT;
    motorcommand[2] = (char)(rightvel /
                             PlayerRobotParams[param_idx].Vel2Divisor);
    motorcommand[3] = (char)(leftvel /
                             PlayerRobotParams[param_idx].Vel2Divisor);

    motorpacket.Build(motorcommand, 4);
    this->SendReceive(&motorpacket);
  }
  else
  {
    // do separate trans and rot vels

    motorcommand[0] = VEL;
    if(speedDemand >= 0)
      motorcommand[1] = ARGINT;
    else
      motorcommand[1] = ARGNINT;

    absspeedDemand = (unsigned short)abs(speedDemand);
    if(absspeedDemand < this->motor_max_speed)
    {
      motorcommand[2] = absspeedDemand & 0x00FF;
      motorcommand[3] = (absspeedDemand & 0xFF00) >> 8;
    }
    else
    {
      puts("Speed demand threshholded!");
      motorcommand[2] = this->motor_max_speed & 0x00FF;
      motorcommand[3] = (this->motor_max_speed & 0xFF00) >> 8;
    }
    motorpacket.Build(motorcommand, 4);
    this->SendReceive(&motorpacket);

    motorcommand[0] = RVEL;
    if(turnRateDemand >= 0)
      motorcommand[1] = ARGINT;
    else
      motorcommand[1] = ARGNINT;

    absturnRateDemand = (unsigned short)abs(turnRateDemand);
    if(absturnRateDemand < this->motor_max_turnspeed)
    {
      motorcommand[2] = absturnRateDemand & 0x00FF;
      motorcommand[3] = (absturnRateDemand & 0xFF00) >> 8;
    }
    else
    {
      puts("Turn rate demand threshholded!");
      motorcommand[2] = this->motor_max_turnspeed & 0x00FF;
      motorcommand[3] = (this->motor_max_turnspeed & 0xFF00) >> 8;
    }

    motorpacket.Build(motorcommand, 4);
    this->SendReceive(&motorpacket);

  }
}

void
P2OS::HandleGripperCommand(player_gripper_cmd_t gripper_cmd)
{
  bool newgrippercommand;
  unsigned char gripcommand[4];
  P2OSPacket grippacket;

  if(!this->sent_gripper_cmd)
    newgrippercommand = true;
  else
  {
    newgrippercommand = false;
    if(gripper_cmd.cmd != this->last_gripper_cmd.cmd || 
       gripper_cmd.arg != this->last_gripper_cmd.arg)
    {
      newgrippercommand = true;
    }
  }

  if(newgrippercommand)
  {
    //puts("sending gripper command");
    // gripper command 
    gripcommand[0] = GRIPPER;
    gripcommand[1] = ARGINT;
    gripcommand[2] = gripper_cmd.cmd & 0x00FF;
    gripcommand[3] = (gripper_cmd.cmd & 0xFF00) >> 8;
    grippacket.Build(gripcommand, 4);
    SendReceive(&grippacket);

    // pass extra value to gripper if needed 
    if(gripper_cmd.cmd == GRIPpress || gripper_cmd.cmd == LIFTcarry ) 
    {
      gripcommand[0] = GRIPPERVAL;
      gripcommand[1] = ARGINT;
      gripcommand[2] = gripper_cmd.arg & 0x00FF;
      gripcommand[3] = (gripper_cmd.arg & 0xFF00) >> 8;
      grippacket.Build(gripcommand, 4);
      SendReceive(&grippacket);
    }

    this->sent_gripper_cmd = true;
    this->last_gripper_cmd = gripper_cmd;
  }
}

void
P2OS::HandleSoundCommand(player_sound_cmd_t sound_cmd)
{
  unsigned char soundcommand[4];
  P2OSPacket soundpacket;
  unsigned short soundindex;

  soundindex = ntohs(sound_cmd.index);

  if(!this->sent_sound_cmd || (soundindex != this->last_sound_cmd.index))
  {
    soundcommand[0] = SOUND;
    soundcommand[1] = ARGINT;
    soundcommand[2] = soundindex & 0x00FF;
    soundcommand[3] = (soundindex & 0xFF00) >> 8;
    soundpacket.Build(soundcommand,4);
    SendReceive(&soundpacket);
    fflush(stdout);

    this->last_sound_cmd.index = soundindex;
  }
}

void
P2OS::GetCommand(void)
{
  // get and send the latest motor command
  player_position_cmd_t position_cmd;
  if(Driver::GetCommand(this->position_id,(void*)&position_cmd,
                        sizeof(player_position_cmd_t),NULL) > 0)
  {
    this->HandlePositionCommand(position_cmd);
  }

  // get and send the latest gripper command, if it's new
  player_gripper_cmd_t gripper_cmd;
  if(Driver::GetCommand(this->gripper_id,(void*)&gripper_cmd,
                        sizeof(player_gripper_cmd_t),NULL) > 0)
  {
    this->HandleGripperCommand(gripper_cmd);
  }

  // get and send the latest sound command, if it's new
  player_sound_cmd_t sound_cmd;
  if(Driver::GetCommand(this->sound_id,(void*)&sound_cmd,
                        sizeof(player_sound_cmd_t),NULL) > 0)
  {
    this->HandleSoundCommand(sound_cmd);
  }
}
