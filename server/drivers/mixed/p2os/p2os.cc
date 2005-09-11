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


/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_p2os p2os

Many robots made by ActivMedia, such as the Pioneer series and the
AmigoBot, are controlled by a microcontroller that runs a special embedded
operating system called P2OS (aka AROS, PSOS).  The host computer
talks to the P2OS microcontroller over a standard RS232 serial line.
This driver offer access to the various P2OS-mediated devices, logically
splitting up the devices' functionality.

@par Compile-time dependencies

- none

@par Provides

The p2os driver provides the following device interfaces, some of
them named:

- "odometry" @ref player_interface_position
  - This interface returns odometry data, and accepts velocity commands.

- "compass" @ref player_interface_position
  - This interface returns compass data (if equipped).

- "gyro" @ref player_interface_position
  - This interface returns gyroscope data (if equipped).

- @ref player_interface_power
  - Returns the current battery voltage (12 V when fully charged).

- @ref player_interface_sonar
  - Returns data from sonar arrays (if equipped)

- @ref player_interface_aio
  - Returns data from analog I/O ports (if equipped)

- @ref player_interface_dio
  - Returns data from digital I/O ports (if equipped)

- @ref player_interface_gripper
  - Controls gripper (if equipped)

- @ref player_interface_bumper
  - Returns data from bumper array (if equipped)

- @ref player_interface_blobfinder
  - Controls a CMUCam2 connected to the AUX port on the P2OS board
    (if equipped).

- @ref player_interface_sound
  - Controls the sound system of the AmigoBot, which can play back
    recorded wav files.

@par Supported configuration requests

- "odometry" @ref player_interface_position:
  - PLAYER_POSITION_SET_ODOM_REQ
  - PLAYER_POSITION_MOTOR_POWER_REQ
  - PLAYER_POSITION_RESET_ODOM_REQ
  - PLAYER_POSITION_GET_GEOM_REQ
  - PLAYER_POSITION_VELOCITY_MODE_REQ
- @ref player_interface_sonar:
  - PLAYER_SONAR_POWER_REQ
  - PLAYER_SONAR_GET_GEOM_REQ
- @ref player_interface_blobfinder
  - PLAYER_BLOBFINDER_SET_COLOR_REQ
  - PLAYER_BLOBFINDER_SET_IMAGER_PARAMS_REQ

@par Configuration file options

- port (string)
  - Default: "/dev/ttyS0"
  - Serial port used to communicate with the robot.
- radio (integer)
  - Default: 0
  - Nonzero if a radio modem is being used; zero for a direct serial link.
- bumpstall (integer)
  - Default: -1
  - Determine whether a bumper-equipped robot stalls when its bumpers are 
    pressed.  Allowed values are:
      - -1 : Don't change anything; the bumper-stall behavior will
             be determined by the BumpStall value stored in the robot's
             FLASH.
      - 0 : Don't stall.
      - 1 : Stall on front bumper contact.
      - 2 : Stall on rear bumper contact.
      - 3 : Stall on either bumper contact.
- joystick (integer)
  - Default: 0
  - Use direct joystick control
- direct_wheel_vel_control (integer)
  - Default: 1
  - Send direct wheel velocity commands to P2OS (as opposed to sending
    translational and rotational velocities and letting P2OS smoothly
    achieve them).
- max_xspeed (length)
  - Default: 0.5 m/s
  - Maximum translational velocity
- max_yawspeed (angle)
  - Default: 100 deg/s
  - Maximum rotational velocity
- max_xaccel (length)
  - Default: 0
  - Maximum translational acceleration, in length/sec/sec; nonnegative.
    Zero means use the robot's default value.
- max_xdecel (length)
  - Default: 0
  - Maximum translational deceleration, in length/sec/sec; nonpositive.  
    Zero means use the robot's default value.
- max_yawaccel (angle)
  - Default: 0
  - Maximum rotational acceleration, in angle/sec/sec; nonnegative.  
    Zero means use the robot's default value.
- max_yawdecel (angle)
  - Default: 0
  - Maximum rotational deceleration, in angle/sec/sec; nonpositive.  
    Zero means use the robot's default value.
- use_vel_band (integer)
  - Default: 0
  - Use velocity bands

  
@par Example 

@verbatim
driver
(
  name "p2os"
  provides ["odometry::position:0" "compass::position:1" "sonar:0" "power:0"]
)
@endverbatim

@par Authors

Brian Gerkey, Kasper Stoy, James McKenna
*/
/** @} */

#ifdef HAVE_CONFIG_H
  #include "config.h"
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

#include "p2os.h"

Driver*
P2OS_Init(ConfigFile* cf, int section)
{
  return (Driver*)(new P2OS(cf,section));
}

void P2OS_Register(DriverTable* table)
{
  table->AddDriver("p2os", P2OS_Init);
}

P2OS::P2OS(ConfigFile* cf, int section) 
        : Driver(cf,section,true,PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  // zero ids, so that we'll know later which interfaces were requested
  memset(&this->position_id, 0, sizeof(player_devaddr_t));
  memset(&this->sonar_id, 0, sizeof(player_devaddr_t));
  memset(&this->aio_id, 0, sizeof(player_devaddr_t));
  memset(&this->dio_id, 0, sizeof(player_devaddr_t));
  memset(&this->gripper_id, 0, sizeof(player_devaddr_t));
  memset(&this->bumper_id, 0, sizeof(player_devaddr_t));
  memset(&this->power_id, 0, sizeof(player_devaddr_t));
  memset(&this->compass_id, 0, sizeof(player_devaddr_t));
  memset(&this->gyro_id, 0, sizeof(player_devaddr_t));
  memset(&this->blobfinder_id, 0, sizeof(player_devaddr_t));
  memset(&this->sound_id, 0, sizeof(player_devaddr_t));

  memset(&this->last_position_cmd, 0, sizeof(player_position2d_cmd_t));

  this->position_subscriptions = this->sonar_subscriptions = 0;

  // Do we create a robot position interface?
  if(cf->ReadDeviceAddr(&(this->position_id), section, "provides",
                        PLAYER_POSITION2D_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->position_id) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a compass position interface?
  if(cf->ReadDeviceAddr(&(this->compass_id), section, "provides", 
                        PLAYER_POSITION2D_CODE, -1, "compass") == 0)
  {
    if(this->AddInterface(this->compass_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a gyro position interface?
  if(cf->ReadDeviceAddr(&(this->gyro_id), section, "provides", 
                        PLAYER_POSITION2D_CODE, -1, "gyro") == 0)
  {
    if(this->AddInterface(this->gyro_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }


  // Do we create a sonar interface?
  if(cf->ReadDeviceAddr(&(this->sonar_id), section, "provides", 
                      PLAYER_SONAR_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->sonar_id) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }


  // Do we create an aio interface?
  if(cf->ReadDeviceAddr(&(this->aio_id), section, "provides", 
                      PLAYER_AIO_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->aio_id) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a dio interface?
  if(cf->ReadDeviceAddr(&(this->dio_id), section, "provides", 
                      PLAYER_DIO_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->dio_id) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a gripper interface?
  if(cf->ReadDeviceAddr(&(this->gripper_id), section, "provides", 
                      PLAYER_GRIPPER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->gripper_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a bumper interface?
  if(cf->ReadDeviceAddr(&(this->bumper_id), section, "provides", 
                      PLAYER_BUMPER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->bumper_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a power interface?
  if(cf->ReadDeviceAddr(&(this->power_id), section, "provides", 
                      PLAYER_POWER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->power_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a blobfinder interface?
  if(cf->ReadDeviceAddr(&(this->blobfinder_id), section, "provides", 
                      PLAYER_BLOBFINDER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->blobfinder_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a sound interface?
  if(cf->ReadDeviceAddr(&(this->sound_id), section, "provides", 
                      PLAYER_SOUND_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->sound_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // build the table of robot parameters.
  ::initialize_robot_params();
  
  // Read config file options
  this->bumpstall = cf->ReadInt(section,"bumpstall",-1);
  this->psos_serial_port = cf->ReadString(section,"port",DEFAULT_P2OS_PORT);
  this->radio_modemp = cf->ReadInt(section, "radio", 0);
  this->joystickp = cf->ReadInt(section, "joystick", 0);
  this->direct_wheel_vel_control = 
          cf->ReadInt(section, "direct_wheel_vel_control", 1);
  this->motor_max_speed = (int)rint(1e3 * cf->ReadLength(section,
                                                         "max_xspeed",
                                                         MOTOR_DEF_MAX_SPEED));
  this->motor_max_turnspeed = (int)rint(RTOD(cf->ReadAngle(section, 
                                                         "max_yawspeed", 
                                                         MOTOR_DEF_MAX_TURNSPEED)));
  this->motor_max_trans_accel = (short)rint(1e3 * 
                                            cf->ReadLength(section, 
                                                           "max_xaccel", 0));
  this->motor_max_trans_decel = (short)rint(1e3 *
                                            cf->ReadLength(section, 
                                                           "max_xdecel", 0));
  this->motor_max_rot_accel = (short)rint(RTOD(cf->ReadAngle(section, 
                                                             "max_yawaccel", 
                                                             0)));
  this->motor_max_rot_decel = (short)rint(RTOD(cf->ReadAngle(section, 
                                                             "max_yawdecel", 
                                                             0)));

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

  cfmakeraw( &term );
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

  SendReceive((P2OSPacket*)NULL,false);

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
    this->SendReceive(&js_packet,false);
  }

  if(this->blobfinder_id.interf)
    CMUcamReset();

  if(this->gyro_id.interf)
  {
    // request that gyro data be sent each cycle
    P2OSPacket gyro_packet;
    unsigned char gyro_command[4];
    gyro_command[0] = GYRO;
    gyro_command[1] = ARGINT;
    gyro_command[2] = 1;
    gyro_command[3] = 0;
    gyro_packet.Build(gyro_command, 4);
    this->SendReceive(&gyro_packet,false);
  }

  // if requested, set max accel/decel limits
  P2OSPacket accel_packet;
  unsigned char accel_command[4];
  if(this->motor_max_trans_accel > 0)
  {
    accel_command[0] = SETA;
    accel_command[1] = ARGINT;
    accel_command[2] = this->motor_max_trans_accel & 0x00FF;
    accel_command[3] = (this->motor_max_trans_accel & 0x00FF) >> 8;
    accel_packet.Build(accel_command, 4);
    this->SendReceive(&accel_packet,false);
  }
  if(this->motor_max_trans_decel < 0)
  {
    accel_command[0] = SETA;
    accel_command[1] = ARGNINT;
    accel_command[2] = abs(this->motor_max_trans_decel) & 0x00FF;
    accel_command[3] = (abs(this->motor_max_trans_decel) & 0x00FF) >> 8;
    accel_packet.Build(accel_command, 4);
    this->SendReceive(&accel_packet,false);
  }
  if(this->motor_max_rot_accel > 0)
  {
    accel_command[0] = SETRA;
    accel_command[1] = ARGINT;
    accel_command[2] = this->motor_max_rot_accel & 0x00FF;
    accel_command[3] = (this->motor_max_rot_accel & 0x00FF) >> 8;
    accel_packet.Build(accel_command, 4);
    this->SendReceive(&accel_packet,false);
  }
  if(this->motor_max_rot_decel < 0)
  {
    accel_command[0] = SETRA;
    accel_command[1] = ARGNINT;
    accel_command[2] = abs(this->motor_max_rot_decel) & 0x00FF;
    accel_command[3] = (abs(this->motor_max_rot_decel) & 0x00FF) >> 8;
    accel_packet.Build(accel_command, 4);
    this->SendReceive(&accel_packet,false);
  }

  // if requested, change bumper-stall behavior
  // 0 = don't stall
  // 1 = stall on front bumper contact
  // 2 = stall on rear bumper contact
  // 3 = stall on either bumper contact
  if(this->bumpstall >= 0)
  {
    if(this->bumpstall > 3)
      PLAYER_ERROR1("ignoring bumpstall value %d; should be 0, 1, 2, or 3",
                    this->bumpstall);
    else
    {
      PLAYER_MSG1(1, "setting bumpstall to %d", this->bumpstall);
      P2OSPacket bumpstall_packet;;
      unsigned char bumpstall_command[4];
      bumpstall_command[0] = BUMP_STALL;
      bumpstall_command[1] = ARGINT;
      bumpstall_command[2] = (unsigned char)this->bumpstall;
      bumpstall_command[3] = 0;
      bumpstall_packet.Build(bumpstall_command, 4);
      this->SendReceive(&bumpstall_packet,false);
    }
  }
  

  // TODO: figure out what the right behavior here is
#if 0
  // zero position command buffer
  player_position_cmd_t zero;
  memset(&zero,0,sizeof(player_position_cmd_t));
  this->PutCommand(this->position_id,(void*)&zero,
                   sizeof(player_position_cmd_t),NULL);
#endif
  
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
P2OS::Subscribe(player_devaddr_t id)
{
  int setupResult;

  // do the subscription
  if((setupResult = Driver::Subscribe(id)) == 0)
  {
    // also increment the appropriate subscription counter
    if(Device::MatchDeviceAddress(id, this->position_id))
      this->position_subscriptions++;
    else if(Device::MatchDeviceAddress(id, this->sonar_id))
      this->sonar_subscriptions++;
  }

  return(setupResult);
}

int 
P2OS::Unsubscribe(player_devaddr_t id)
{
  int shutdownResult;

  // do the unsubscription
  if((shutdownResult = Driver::Unsubscribe(id)) == 0)
  {
    // also decrement the appropriate subscription counter
    if(Device::MatchDeviceAddress(id, this->position_id))
      assert(--this->position_subscriptions >= 0);
    else if(Device::MatchDeviceAddress(id, this->sonar_id))
      assert(--this->sonar_subscriptions >= 0);
  }

  return(shutdownResult);
}

void 
P2OS::PutData(void)
{
  // TODO: something smarter about timestamping.

  // put odometry data
  this->Publish(this->position_id, NULL, 
                PLAYER_MSGTYPE_DATA,
                PLAYER_POSITION2D_DATA_STATE,
                (void*)&(this->p2os_data.position), 
                sizeof(player_position2d_data_t),
                NULL);

  // put sonar data
  this->Publish(this->sonar_id, NULL, 
                PLAYER_MSGTYPE_DATA,
                PLAYER_SONAR_DATA_RANGES,
                (void*)&(this->p2os_data.sonar), 
                sizeof(player_sonar_data_t),
                NULL);
  
  // put aio data
  this->Publish(this->aio_id, NULL, 
                PLAYER_MSGTYPE_DATA,
                PLAYER_AIO_DATA_VALUES,
                (void*)&(this->p2os_data.aio), 
                sizeof(player_aio_data_t),
                NULL);

  // put dio data
  this->Publish(this->dio_id, NULL, 
                PLAYER_MSGTYPE_DATA,
                PLAYER_DIO_DATA_VALUES,
                (void*)&(this->p2os_data.dio), 
                sizeof(player_dio_data_t),
                NULL);

  // put gripper data
  this->Publish(this->gripper_id, NULL, 
                PLAYER_MSGTYPE_DATA,
                PLAYER_GRIPPER_DATA_STATE,
                (void*)&(this->p2os_data.gripper), 
                sizeof(player_gripper_data_t),
                NULL);

  // put bumper data
  this->Publish(this->bumper_id, NULL, 
                PLAYER_MSGTYPE_DATA,
                PLAYER_BUMPER_DATA_STATE,
                (void*)&(this->p2os_data.bumper), 
                sizeof(player_bumper_data_t),
                NULL);

  // put power data
  this->Publish(this->power_id, NULL, 
                PLAYER_MSGTYPE_DATA,
                PLAYER_POWER_DATA_VOLTAGE,
                (void*)&(this->p2os_data.power), 
                sizeof(player_power_data_t),
                NULL);

  // put compass data
  this->Publish(this->compass_id, NULL, 
                PLAYER_MSGTYPE_DATA,
                PLAYER_POSITION2D_DATA_STATE,
                (void*)&(this->p2os_data.compass), 
                sizeof(player_position2d_data_t),
                NULL);

  // put gyro data
  this->Publish(this->gyro_id, NULL, 
                PLAYER_MSGTYPE_DATA,
                PLAYER_POSITION2D_DATA_STATE,
                (void*)&(this->p2os_data.gyro), 
                sizeof(player_position2d_data_t),
                NULL);

  // put blobfinder data
  this->Publish(this->blobfinder_id, NULL, 
                PLAYER_MSGTYPE_DATA,
                PLAYER_BLOBFINDER_DATA_STATE,
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
    pthread_testcancel();

    // we want to turn on the sonars if someone just subscribed, and turn
    // them off if the last subscriber just unsubscribed.
    this->Lock();
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
      // enable motor power
      this->ToggleMotorPower(1);
    }
    last_position_subscrcount = this->position_subscriptions;
    this->Unlock();

    // The Amigo board seems to drop commands once in a while.  This is
    // a hack to restart the serial reads if that happens.
    if(this->blobfinder_id.interf)
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

    // handle pending messages
    if(!this->InQueue->Empty())
      ProcessMessages();
    else
    {
      // if no pending msg, resend the last position cmd.
      this->HandlePositionCommand(this->last_position_cmd);
    }
  }
}

/* send the packet, then receive and parse an SIP */
int
P2OS::SendReceive(P2OSPacket* pkt, bool publish_data)
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

      if(publish_data)
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
      if(blobfinder_id.interf)
      {
        /* It is an extended SIP (blobfinder) packet, so process it */
        /* Be sure to pass data size too (packet[2])! */
        this->sippacket->ParseSERAUX( &packet.packet[2] );
        this->sippacket->Fill(&(this->p2os_data));

        if(publish_data)
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
      if(this->gyro_id.interf)
      {
        /* It's a set of gyro measurements */
        this->sippacket->ParseGyro(&packet.packet[2]);
        this->sippacket->Fill(&(this->p2os_data));
        if(publish_data)
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
    this->SendReceive(&pkt,false);
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
  this->SendReceive(&cam_packet,false);

  // Set for raw output + no ACK/NACK
  printf("Setting raw mode...\n");
  cam_command[0] = TTY3;
  cam_command[1] = ARGSTR;
  sprintf((char*)&cam_command[3], "RM 3\r");
  cam_command[2] = strlen((char *)&cam_command[3]);
  cam_packet.Build(cam_command, (int)cam_command[2]+3);
  this->SendReceive(&cam_packet,false);
  usleep(100000);

  printf("Flushing serial buffer...\n");
  cam_command[0] = GETAUX2;
  cam_command[1] = ARGINT;
  cam_command[2] = 0;
  cam_command[3] = 0;
  cam_packet.Build(cam_command, 4);
  this->SendReceive(&cam_packet,false);

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
  SendReceive(&packet,false);
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
  SendReceive(&packet,false);
}

int 
P2OS::ProcessMessage(MessageQueue * resp_queue, 
                     player_msghdr * hdr, 
                     void * data)
{
  // Look for configuration requests
  if(hdr->type == PLAYER_MSGTYPE_REQ)
    return(this->HandleConfig(resp_queue,hdr,data));
  else if(hdr->type == PLAYER_MSGTYPE_CMD)
    return(this->HandleCommand(hdr,data));
  else
    return(-1);
}

int
P2OS::HandleConfig(MessageQueue* resp_queue,
                   player_msghdr * hdr,
                   void * data)
{
  // check for position config requests
  if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                           PLAYER_POSITION2D_REQ_SET_ODOM, 
                           this->position_id))
  {
    if(hdr->size != sizeof(player_position2d_set_odom_req_t))
    {
      PLAYER_WARN("Arg to odometry set requests wrong size; ignoring");
      return(-1);
    }
    player_position2d_set_odom_req_t* set_odom_req = 
            (player_position2d_set_odom_req_t*)data;

    this->sippacket->x_offset = ((int)rint(set_odom_req->pose.px*1e3)) -
            this->sippacket->xpos;
    this->sippacket->y_offset = ((int)rint(set_odom_req->pose.py*1e3)) -
            this->sippacket->ypos;
    this->sippacket->angle_offset = ((int)rint(RTOD(set_odom_req->pose.pa))) -
            this->sippacket->angle;

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_SET_ODOM);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_POSITION2D_REQ_MOTOR_POWER, 
                                this->position_id))
  {
    /* motor state change request 
     *   1 = enable motors
     *   0 = disable motors (default)
     */
    if(hdr->size != sizeof(player_position2d_power_config_t))
    {
      PLAYER_WARN("Arg to motor state change request wrong size; ignoring");
      return(-1);
    }
    player_position2d_power_config_t* power_config =
            (player_position2d_power_config_t*)data;
    this->ToggleMotorPower(power_config->state);

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_MOTOR_POWER);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_POSITION2D_REQ_RESET_ODOM, 
                                this->position_id))
  {
    /* reset position to 0,0,0: no args */
    if(hdr->size != 0)
    {
      PLAYER_WARN("Arg to reset position request is wrong size; ignoring");
      return(-1);
    }
    ResetRawPositions();

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_RESET_ODOM);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_POSITION2D_REQ_GET_GEOM, 
                                this->position_id))
  {
    /* Return the robot geometry. */
    if(hdr->size != 0)
    {
      PLAYER_WARN("Arg get robot geom is wrong size; ignoring");
      return(-1);
    }
    player_position2d_geom_t geom;
    // TODO: Figure out this rotation offset somehow; it's not 
    //       given in the Saphira parameters.  For now, -0.1 is 
    //       about right for a Pioneer 2DX.
    geom.pose.px = -0.1;
    geom.pose.py = 0.0;
    geom.pose.pa = 0.0;
    // get dimensions from the parameter table
    geom.size.sl = PlayerRobotParams[param_idx].RobotLength / 1e3;
    geom.size.sw = PlayerRobotParams[param_idx].RobotWidth / 1e3;

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_POSITION2D_REQ_GET_GEOM,
                  (void*)&geom, sizeof(geom), NULL);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_POSITION2D_REQ_VELOCITY_MODE, 
                                this->position_id))
  {
    /* velocity control mode:
     *   0 = direct wheel velocity control (default)
     *   1 = separate translational and rotational control
     */
    if(hdr->size != sizeof(player_position2d_velocity_mode_config_t))
    {
      PLAYER_WARN("Arg to velocity control mode change request is wrong "
                  "size; ignoring");
      return(-1);
    }
    player_position2d_velocity_mode_config_t* velmode_config = 
            (player_position2d_velocity_mode_config_t*)data;

    if(velmode_config->value)
      direct_wheel_vel_control = false;
    else
      direct_wheel_vel_control = true;

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_VELOCITY_MODE);
    return(0);
  }
  // check for sonar config requests
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_SONAR_REQ_POWER, 
                                this->sonar_id))
  {
    /*
     * 1 = enable sonars
     * 0 = disable sonar
     */
    if(hdr->size != sizeof(player_sonar_power_config_t))
    {
      PLAYER_WARN("Arg to sonar state change request wrong size; ignoring");
      return(-1);
    }
    player_sonar_power_config_t* sonar_config = 
            (player_sonar_power_config_t*)data;
    this->ToggleSonarPower(sonar_config->state);

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_SONAR_REQ_POWER);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_SONAR_REQ_GET_GEOM, 
                                this->sonar_id))
  {
    /* Return the sonar geometry. */
    if(hdr->size != 0)
    {
      PLAYER_WARN("Arg get sonar geom is wrong size; ignoring");
      return(-1);
    }
    player_sonar_geom_t geom;
    geom.poses_count = PlayerRobotParams[param_idx].SonarNum;
    for(int i = 0; i < PlayerRobotParams[param_idx].SonarNum; i++)
    {
      sonar_pose_t pose = PlayerRobotParams[param_idx].sonar_pose[i];
      geom.poses[i].px = pose.x / 1e3;
      geom.poses[i].py = pose.y / 1e3;
      geom.poses[i].pa = DTOR(pose.th);
    }

    this->Publish(this->sonar_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_SONAR_REQ_GET_GEOM,
                  (void*)&geom, sizeof(geom), NULL);
    return(0);
  }
  // check for blobfinder requests
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                             PLAYER_BLOBFINDER_REQ_SET_COLOR, 
                             this->blobfinder_id))
  {
    // Set the tracking color (RGB max/min values)

    if(hdr->size != sizeof(player_blobfinder_color_config_t))
    {
      puts("Arg to blobfinder color request wrong size; ignoring");
      return(-1);
    }
    player_blobfinder_color_config_t* color_config = 
            (player_blobfinder_color_config_t*)data;

    CMUcamTrack(color_config->rmin,
                color_config->rmax, 
                color_config->gmin,
                color_config->gmax,
                color_config->bmin,
                color_config->bmax);

    this->Publish(this->blobfinder_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_BLOBFINDER_REQ_SET_COLOR);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_BLOBFINDER_REQ_SET_IMAGER_PARAMS, 
                                this->blobfinder_id))
  {
    // Set the imager control params
    if(hdr->size != sizeof(player_blobfinder_imager_config_t))
    {
      puts("Arg to blobfinder imager request wrong size; ignoring");
      return(-1);
    }
    player_blobfinder_imager_config_t* imager_config = 
            (player_blobfinder_imager_config_t*)data;

    P2OSPacket cam_packet;
    unsigned char cam_command[50];
    int np;

    np=3;

    CMUcamStopTracking();	// Stop the current tracking.

    cam_command[0] = TTY3;
    cam_command[1] = ARGSTR;
    np += sprintf((char*)&cam_command[np], "CR ");

    if (imager_config->brightness >= 0)
      np += sprintf((char*)&cam_command[np], " 6 %d",
                    imager_config->brightness);

    if (imager_config->contrast >= 0)
      np += sprintf((char*)&cam_command[np], " 5 %d",
                    imager_config->contrast);

    if (imager_config->autogain >= 0)
      if (imager_config->autogain == 0)
        np += sprintf((char*)&cam_command[np], " 19 32");
      else
        np += sprintf((char*)&cam_command[np], " 19 33");

    if (imager_config->colormode >= 0)
      if (imager_config->colormode == 3)
        np += sprintf((char*)&cam_command[np], " 18 36");
      else if (imager_config->colormode == 2)
        np += sprintf((char*)&cam_command[np], " 18 32");
      else if (imager_config->colormode == 1)
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

    this->Publish(this->blobfinder_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, 
                  PLAYER_BLOBFINDER_REQ_SET_IMAGER_PARAMS);
    return(0);
  }
  else
  {
    PLAYER_WARN("unknown config request to p2os driver");
    return(-1);
  }
}

void
P2OS::HandlePositionCommand(player_position2d_cmd_t position_cmd)
{
  int speedDemand, turnRateDemand;
  double leftvel, rightvel;
  double rotational_term;
  unsigned short absspeedDemand, absturnRateDemand;
  unsigned char motorcommand[4];
  P2OSPacket motorpacket; 

  speedDemand = (int)rint(position_cmd.vel.px * 1e3);
  turnRateDemand = (int)rint(RTOD(position_cmd.vel.pa));

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

int
P2OS::HandleCommand(player_msghdr * hdr, void* data)
{
  if(Message::MatchMessage(hdr,
                           PLAYER_MSGTYPE_CMD,
                           PLAYER_POSITION2D_CMD_STATE,
                           this->position_id))
  {
    // get and send the latest motor command
    player_position2d_cmd_t position_cmd;
    position_cmd = *(player_position2d_cmd_t*)data;
    this->HandlePositionCommand(position_cmd);
    return(0);
  }
  else if(Message::MatchMessage(hdr,
                                PLAYER_MSGTYPE_CMD,
                                PLAYER_GRIPPER_CMD_STATE,
                                this->gripper_id))
  {
    // get and send the latest gripper command, if it's new
    player_gripper_cmd_t gripper_cmd;
    gripper_cmd = *(player_gripper_cmd_t*)data;
    this->HandleGripperCommand(gripper_cmd);
    return(0);
  }
  else if(Message::MatchMessage(hdr,
                                PLAYER_MSGTYPE_CMD,
                                PLAYER_SOUND_CMD_IDX,
                                this->sound_id))
  {
    // get and send the latest sound command, if it's new
    player_sound_cmd_t sound_cmd;
    sound_cmd = *(player_sound_cmd_t*)data;
    this->HandleSoundCommand(sound_cmd);
    return(0);
  }
  return(-1);
}
