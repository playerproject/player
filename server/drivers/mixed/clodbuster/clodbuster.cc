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
 *   the clodbuster device.  there's a thread here that
 *   actually interacts with grasp board via the serial line.  the other
 *   "devices" communicate with this thread by putting into and getting
 *   data out of shared buffers.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h> // POSIX file i/o
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <math.h>
#include <stdlib.h>  /* for abs() */
#include <netinet/in.h> // socket things...
#include <termios.h> // serial port things

#include <clodbuster.h>
#include <packet.h> // What's this for?

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
 
 #define PTZ_PAN_MAX 100.0
 #define PTZ_TILT_MAX 25.0
 #define PTZ_PAN_CONV_FACTOR (0x370 / PTZ_PAN_MAX)
 #define PTZ_TILT_CONV_FACTOR (0x12C / PTZ_TILT_MAX)
*/

/* these are necessary to make the static fields visible to the linker 
   extern pthread_t       P2OS::thread;
   extern struct timeval  P2OS::timeBegan_tv;
   extern bool            P2OS::direct_wheel_vel_control;
   extern int             P2OS::motor_max_speed;
   extern int             P2OS::motor_max_turnspeed;
   extern bool            P2OS::use_vel_band;
   extern bool            P2OS::initdone;
   extern pthread_mutex_t P2OS::p2os_accessMutex;
   extern pthread_mutex_t P2OS::p2os_setupMutex;
   extern int             P2OS::p2os_subscriptions;
   extern player_p2os_data_t*  P2OS::data;
   extern player_p2os_cmd_t*  P2OS::command;
   extern unsigned char*    P2OS::reqqueue;
   extern unsigned char*    P2OS::repqueue;
*/ // Don't know what i need of this .. so nothing included for now.

 extern int            ClodBuster::clodbuster_subscriptions;
extern bool            ClodBuster::direct_command_control;
extern struct timeval  ClodBuster::timeBegan_tv;
extern bool            ClodBuster::initdone;
extern int             ClodBuster::clodbuster_fd;
extern char            ClodBuster::clodbuster_serial_port[];
extern pthread_t       ClodBuster::thread;
extern pthread_mutex_t ClodBuster::clodbuster_accessMutex;
extern pthread_mutex_t ClodBuster::clodbuster_setupMutex;
extern unsigned char*    ClodBuster::reqqueue;
extern unsigned char*    ClodBuster::repqueue;
  extern player_clodbuster_data_t*  ClodBuster::data;
   extern player_clodbuster_cmd_t*  ClodBuster::command;

// initialization function
CDevice* ClodBuster_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
    {
      PLAYER_ERROR1("driver \"clodbuster\" does not support interface "
		    "\"%s\"\n", interface);
      return(NULL);
    }
  else
    return((CDevice*)(new ClodBuster(interface, cf, section)));
}

// a driver registration function
void 
ClodBuster_Register(DriverTable* table)
{
  table->AddDriver("clodbuster", PLAYER_ALL_MODE, ClodBuster_Init);
}


ClodBuster::ClodBuster(char* interface, ConfigFile* cf, int section):
  CDevice(sizeof(player_position_data_t),sizeof(player_position_cmd_t),1,1)
{
  int reqqueuelen = 1;
  int repqueuelen = 1;

  if(!initdone)
    {
      // build the table of robot parameters.
      //   initialize_robot_params(); // THis is specific to the P2OS coz of the Saphira 
  
      // also, install default parameter values.
      strncpy(clodbuster_serial_port,DEFAULT_CLODBUSTER_PORT,sizeof(clodbuster_serial_port));
      clodbuster_fd = -1;
      //  radio_modemp = 0;
      //joystickp = 0;
      //cmucam = 0;		// If p2os_cmucam is used, this will be overridden
  
      data = new player_clodbuster_data_t;
      command = new player_clodbuster_cmd_t;
      ResetRawPositions();

      reqqueue = (unsigned char*)(new playerqueue_elt_t[reqqueuelen]);
      repqueue = (unsigned char*)(new playerqueue_elt_t[repqueuelen]);

      SetupBuffers((unsigned char*)data, sizeof(player_clodbuster_data_t),
		   (unsigned char*)command, sizeof(player_clodbuster_cmd_t),
		   reqqueue, reqqueuelen,
		   repqueue, repqueuelen);

      ((player_clodbuster_cmd_t*)device_command)->position.xspeed = 0;
      ((player_clodbuster_cmd_t*)device_command)->position.yawspeed = 0;

      clodbuster_subscriptions = 0;

      pthread_mutex_init(&clodbuster_accessMutex,NULL);
      pthread_mutex_init(&clodbuster_setupMutex,NULL);

      initdone = true; 
    }
  else
    {
      // every sub-device gets its own queue object (but they all point to the
      // same chunk of memory)
    
      // every sub-device needs to get its various pointers set up
      SetupBuffers((unsigned char*)data, sizeof(player_clodbuster_data_t),
		   (unsigned char*)command, sizeof(player_clodbuster_cmd_t),
		   reqqueue, reqqueuelen,
		   repqueue, repqueuelen);
    }



  strncpy(clodbuster_serial_port,
          cf->ReadString(section, "port", clodbuster_serial_port),
          sizeof(clodbuster_serial_port));

  // zero the subscription counter.
  subscriptions = 0;

  // set parameters
  CountsPerRev = 408;
  WheelRadius = 0.076;
  WheelBase = .2921;
  WheelSeparation = .275;
  Kenc = 2*M_PI*WheelRadius/CountsPerRev;
  LoopFreq = 5;

  // set PID gains
  //  Kv = new PIDGains(-1.0,-1.5,0.0,LoopFreq);
  //  Kw = new PIDGains(-0.5112,-1.5,0.0,LoopFreq);
  Kv = new PIDGains(-10,-20.0,0.0,LoopFreq);
  Kw = new PIDGains(-5,-20.0,0.0,LoopFreq);
  

}

ClodBuster::~ClodBuster()
{
  if(reqqueue)
    {
      delete[] reqqueue;
      reqqueue=NULL;
    }
  if(repqueue)
    {
      delete[] repqueue;
      repqueue=NULL;
    }
  if(data)
    {
      delete data;
      data = NULL;
      device_data = NULL;
    }
  if(command)
    {
      delete command;
      command = NULL;
      device_command = NULL;
    }
  delete Kv;
  delete Kw;
}

void ClodBuster::Lock()
{
  pthread_mutex_lock(&clodbuster_accessMutex);
}
void ClodBuster::Unlock()
{
  pthread_mutex_unlock(&clodbuster_accessMutex);
}

int ClodBuster::Setup()
{
  //  int i;
  // this is the order in which we'll try the possible baud rates. we try 9600
  // first because most robots use it, and because otherwise the radio modem
  // connection code might not work (i think that the radio modems operate at
  // 9600).
  //int baud = B38400;
  //  int numbauds = sizeof(bauds);
  // int currbaud = 0;

  struct termios term;
  //unsigned char command;
  // GRASPPacket packet, receivedpacket;
  int flags;
  //bool sent_close = false;
  
  printf("clodbuster connection initializing (%s)...",clodbuster_serial_port);
  fflush(stdout);

  if((clodbuster_fd = open(clodbuster_serial_port, 
			   O_RDWR | O_SYNC, S_IRUSR | S_IWUSR )) < 0 ) // O_NONBLOCK later..
    {
      perror("ClodBuster::Setup():open():");
      return(1);
    }  
 
  if( tcgetattr( clodbuster_fd, &term ) < 0 )
    {
      perror("ClodBuster::Setup():tcgetattr():");
      close(clodbuster_fd);
      clodbuster_fd = -1;
      return(1);
    }
  
#if HAVE_CFMAKERAW
  cfmakeraw( &term );
  printf("Used MakeRaw\n");
#else
  /* Set the terminal input stream into raw mode, and disable all special
     characters. Also set input to one byte at a time, blocking */
  term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  term.c_cflag &= ~(CSIZE | PARENB);
  term.c_cflag |= (CS8   | CLOCAL | CREAD);
  //      if(RTSCTS_flag) 
  //        term.c_cflag |= CRTSCTS;  
  term.c_oflag &= ~(OPOST);
#endif
  term.c_cc[VTIME] = 10; /* wait 1 second on port A */
  term.c_cc[VMIN] = 0;  

  cfsetispeed(&term, B38400);
  cfsetospeed(&term, B38400);
  
  if( tcsetattr( clodbuster_fd, TCSAFLUSH, &term ) < 0 )
    {
      perror("ClodBuster::Setup():tcsetattr():");
      close(clodbuster_fd);
      clodbuster_fd = -1;
      return(1);
    }

  if( tcflush( clodbuster_fd, TCIOFLUSH ) < 0 )
    {
      perror("ClodBuster::Setup():tcflush():");
      close(clodbuster_fd);
      clodbuster_fd = -1;
      return(1);
    }

  if((flags = fcntl(clodbuster_fd, F_GETFL)) < 0)
    {
      perror("ClodBuster::Setup():fcntl()");
      close(clodbuster_fd);
      clodbuster_fd = -1;
      return(1);
    }
  /* turn on blocking mode */
  flags &= ~O_NONBLOCK;
  fcntl(clodbuster_fd, F_SETFL, flags);

  
  usleep(CLODBUSTER_CYCLETIME_USEC);
  
  
  direct_command_control = true;
  
  /* now spawn reading thread */
  StartThread();
  return(0);
}

int ClodBuster::Shutdown()
{
  GRASPPacket packet; 

  if(clodbuster_fd == -1)
    {
      return(0);
    }

  StopThread();

  packet.Build(SET_SLEEP_MODE,SLEEP_MODE_OFF);
  packet.Send(clodbuster_fd);
  usleep(CLODBUSTER_CYCLETIME_USEC);

  close(clodbuster_fd);
  clodbuster_fd = -1;
  puts("ClodBuster has been shutdown");
 
  return(0);
}

int ClodBuster::Subscribe(void *client)
{
  int setupResult;

  if(clodbuster_subscriptions == 0) 
    {
      setupResult = Setup();
      if (setupResult == 0 ) 
	{
	  clodbuster_subscriptions++;  // increment the static p2os-wide subscr counter
	  subscriptions++;       // increment the per-device subscr counter
	}
    }
  else 
    {
      clodbuster_subscriptions++;  // increment the static p2os-wide subscr counter
      subscriptions++;       // increment the per-device subscr counter
      setupResult = 0;
    }
  
  return( setupResult );
}

int ClodBuster::Unsubscribe(void *client)
{
  int shutdownResult;

  if(clodbuster_subscriptions == 0) 
    {
      shutdownResult = -1;
    }
  else if(clodbuster_subscriptions == 1) 
    {
      shutdownResult = Shutdown();
      if (shutdownResult == 0 ) 
	{ 
	  clodbuster_subscriptions--;  // decrement the static p2os-wide subscr counter
	  subscriptions--;       // decrement the per-device subscr counter
	}
      /* do we want to unsubscribe even though the shutdown went bad? */
    }
  else 
    {
      clodbuster_subscriptions--;  // decrement the static p2os-wide subscr counter
      subscriptions--;       // decrement the per-device subscr counter
      shutdownResult = 0;
    }
  
  return( shutdownResult );
}


void ClodBuster::PutData( unsigned char* src, size_t maxsize,
			  uint32_t timestamp_sec, uint32_t timestamp_usec)
{
  Lock();

  *((player_clodbuster_data_t*)device_data) = *((player_clodbuster_data_t*)src);

  if(timestamp_sec == 0)
    {
      struct timeval curr;
      GlobalTime->GetTime(&curr);
      timestamp_sec = curr.tv_sec;
      timestamp_usec = curr.tv_usec;
    }

  data_timestamp_sec = timestamp_sec;
  data_timestamp_usec = timestamp_usec;
  
  // need to fill in the timestamps on all ClodBuster devices, both so that they
  // can read it, but also because other devices may want to read it
  player_device_id_t id = device_id;


  id.code = PLAYER_POSITION_CODE;
  CDevice* positionp = deviceTable->GetDevice(id);
  if(positionp)
    {
      positionp->data_timestamp_sec = this->data_timestamp_sec;
      positionp->data_timestamp_usec = this->data_timestamp_usec;
    }


  Unlock();
}

void 
ClodBuster::Main()
{
  player_clodbuster_cmd_t command;
  unsigned char config[CLODBUSTER_CONFIG_BUFFER_SIZE];
  GRASPPacket packet; 
  
  short speedDemand=0, turnRateDemand=0;
  bool newmotorspeed, newmotorturn;
  
   int config_size;

  int last_position_subscrcount;
 
  player_device_id_t id;
  id.port = global_playerport;
  id.index = 0;

  id.code = PLAYER_POSITION_CODE;
  CDevice* positionp = deviceTable->GetDevice(id);

  last_position_subscrcount = 0;

  GlobalTime->GetTime(&timeBegan_tv);
  GetGraspBoardParams();

  // memory for PID
  int uVmax=max_limits[SET_SERVO_THROTTLE];//-center_limits[SET_SERVO_THROTTLE];
  int uVmin=min_limits[SET_SERVO_THROTTLE];//-center_limits[SET_SERVO_THROTTLE];
  int uV0=center_limits[SET_SERVO_THROTTLE];
  int uWmax=max_limits[SET_SERVO_FRONTSTEER];//-center_limits[SET_SERVO_FRONTSTEER];
  int uWmin=min_limits[SET_SERVO_FRONTSTEER];//-center_limits[SET_SERVO_FRONTSTEER];
  int uW0=center_limits[SET_SERVO_FRONTSTEER];
  float uV = uV0, uW=uW0, errV[3]={0}, errW[3]={0}, uVlast=uV0, uWlast=uW0,V;

  printf("V max min centre %d %d %d\n",uVmax,uVmin,uV0);
  printf("W max min centre %d %d %d\n",uWmax,uWmin,uW0);

  for(;;)
    {
    
      // we want to reset the odometry and enable the motors if the first 
      // client just subscribed to the position device, and we want to stop 
      // and disable the motors if the last client unsubscribed.
      if(positionp)
	{
	  if(!last_position_subscrcount && positionp->subscriptions)
	    {
	      // disable motor power
	      packet.Build(SET_SLEEP_MODE,SLEEP_MODE_OFF);
	      packet.Send(clodbuster_fd);
	      // reset odometry
	      ResetRawPositions();
	    }
	  else if(last_position_subscrcount && !(positionp->subscriptions))
	    {
	      packet.Build(SET_SLEEP_MODE,SLEEP_MODE_OFF);
	      packet.Send(clodbuster_fd);
 
	      // overwrite existing motor commands to be zero
	      player_position_cmd_t position_cmd;
	      position_cmd.xspeed = 0;
	      position_cmd.yawspeed = 0;
	      // TODO: who should really be the client here?
	      positionp->PutCommand(this,(unsigned char*)(&position_cmd), 
				    sizeof(position_cmd));
            }

	  last_position_subscrcount = positionp->subscriptions;
	}

    
      /** New configuration commands **/
      void* client;
      player_device_id_t id;
      // first, check if there is a new config command
      if((config_size = GetConfig(&id, &client, (void*)config, sizeof(config))))
	{
	  switch(id.code)
	    {
	      
	    case PLAYER_POSITION_CODE:
	      switch(config[0])
		{ 
		case PLAYER_POSITION_SET_ODOM_REQ:
		  if(config_size != sizeof(player_position_set_odom_req_t))
		    {
		      puts("Arg to odometry set requests wrong size; ignoring");
		      if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, 
				  NULL, NULL, 0))
			PLAYER_ERROR("failed to PutReply");
		      break;
		    }
		  player_position_set_odom_req_t set_odom_req;
		  set_odom_req = *((player_position_set_odom_req_t*)config);
		  
		  this->data->position.xpos=(set_odom_req.x);
		  this->data->position.ypos=(set_odom_req.y);
		  this->data->position.yaw= set_odom_req.theta>>8;
		 
		  printf("command yaw: %d , actual %d",set_odom_req.theta,data->position.yaw);
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
		  if(power_config.value==1)
		    packet.Build(SET_SLEEP_MODE,SLEEP_MODE_OFF);
		  else 
		    packet.Build(SET_SLEEP_MODE,SLEEP_MODE_ON);
		  
		  packet.Send(clodbuster_fd);
		  
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
		    direct_command_control = false;
		  else
		    direct_command_control = true;

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

	
		case PLAYER_POSITION_SPEED_PID_REQ: 
		  // set up the velocity PID on the CB
		  // kp, ki, kd are used
		  if (config_size != sizeof(player_position_speed_pid_req_t)) {
		    fprintf(stderr, "CB: pos speed PID req got wrong size (%d)\n", config_size);
		    if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
		      PLAYER_ERROR("CB: failed to put reply");
		      break;
		    }
		  }
		  
		  player_position_speed_pid_req_t pid =*((player_position_speed_pid_req_t *)config);
		  kp = ntohl(pid.kp);
		  ki = ntohl(pid.ki);
		  kd = ntohl(pid.kd);
		
		  if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK,
			      NULL, NULL, 0))
		    PLAYER_ERROR("failed to PutReply");
		  break;
		  
		default:
		  puts("Position got unknown config request");
		  printf("The config command was %d\n",config[0]);
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
      if( speedDemand != (int) ntohl(command.position.xspeed));
      newmotorspeed = true;
      speedDemand = (int) ntohl(command.position.xspeed);
      
      newmotorturn = false;
      if(turnRateDemand != (int) ntohl(command.position.yawspeed));
      newmotorturn = true;
      turnRateDemand = (int) ntohl(command.position.yawspeed);
      
      // read encoders & update pose and velocity
      encoder_measurement = ReadEncoders();
      
      DifferenceEncoders();
      IntegrateEncoders();
      
      // remember old values
      old_encoder_measurement = encoder_measurement;
      
      PutData((unsigned char*)data,sizeof(player_clodbuster_data_t),0,0);
      //      printf("left: %d , right %d count: %u\n",encoder_measurement.left,encoder_measurement.right,encoder_measurement.time_count);
      //      printf("left: %d , right %d\n",encoder_measurement.left-encoder_offset.left,encoder_measurement.right-encoder_offset.right);


      // do control
      /* NEXT, write commands */
      if(!direct_command_control) // fix this it's reversed for playerv
	{
	  // do direct wheel velocity control here
	  //printf("speedDemand: %d\t turnRateDemand: %d\n",
	  //speedDemand, turnRateDemand);
	  unsigned char v,w;
	  v = SetServo(SET_SERVO_THROTTLE,speedDemand);
	  w = SetServo(SET_SERVO_FRONTSTEER,turnRateDemand);
	  printf("The vel/turn command numbers : v:%d (%u) w:%d (%u)\n",center_limits[SET_SERVO_THROTTLE]+speedDemand,v,center_limits[SET_SERVO_FRONTSTEER]+turnRateDemand,w);
    
	  
	}
      else
	{
	     //	  printf("Do PID !\n");
	     // find tracking errors
	     errV[0] = EncV - speedDemand*1e-3;
	     errW[0] = EncOmega - turnRateDemand*M_PI/180.0;
	     
	     // find actions
	     uV = uVlast + Kv->K1()*errV[0] + + Kv->K2()*errV[1] + Kv->K3()*errV[2];
	     if (uV > uVmax)
		  {
		       uV = uVmax;
		       printf("+V control saturated!\n");
		  }
	     if (uV < uVmin)
		  {
		       uV = uVmin;
		       printf("-V control saturated!\n");
		  }
	     printf("V loop err: %f, u = %f, r = %f, x = %f\n",errV[0], uV,speedDemand*1e-3,EncV);
	     //	     if (speedDemand*1e-3 > .0125) 
	     if (fabs(EncV) > .0125) 
		  {
		    if (fabs(EncV) <.1)
		      if (EncV <.1)
			V = -.1;
		      else
			V = .1;
		    else
		      V = EncV;
		    // NB "-" sign for wrong convention +ve -> left 
		    uW = uWlast - WheelBase/V*(Kw->K1()*errW[0] + + Kw->K2()*errW[1] + Kw->K3()*errW[2]);
		    //   uW = uWlast - 1e3*WheelBase/speedDemand*(Kw->K1()*errW[0] + + Kw->K2()*errW[1] + Kw->K3()*errW[2]);
		  }
	     else
		  {
		       // set it to zero (center)
		       uW = uW0;
		       uWlast = uW0;
		       for (int i=0;i<3;i++)
			    errW[i] = 0.;
		  } 
	     if (uW > uWmax)
		  {
		       uW = uWmax;
		       printf("+W control saturated!\n");
		  }
	     if (uW < uWmin)
		  {
		       uW = uWmin;
		       printf("-W control saturated!\n");
		  }
	     printf("W loop err: %f, u = %f, r = %f, x = %f\n",errW[0], uW,turnRateDemand*M_PI/180.0,EncOmega);
	     
	     // Write actions to control board
	     SetServo(SET_SERVO_THROTTLE,(unsigned char) uV);
	     SetServo(SET_SERVO_FRONTSTEER,(unsigned char) uW);
	     
	     for (int i=1;i<3;i++)
	       {
		 errW[i] = errW[i-1];
		 errV[i] = errV[i-1];
	       }
	     uVlast = uV;
	     uWlast = uW;

	}
      usleep(200000);
    }
  pthread_exit(NULL);
}

void
ClodBuster::ResetRawPositions()
{
  encoder_offset=ReadEncoders();
}

clodbuster_encoder_data_t  ClodBuster::ReadEncoders()
{ GRASPPacket packet,rpacket;
 clodbuster_encoder_data_t temp;

 packet.Build(ECHO_ENCODER_COUNTS_TS);
 packet.Send(clodbuster_fd);

 rpacket.Receive(clodbuster_fd,ECHO_ENCODER_COUNTS_TS);
 rpacket.size=rpacket.retsize;
 rpacket.PrintHex();

 // make 24bit signed int a 32bit signed int
 temp.left = (rpacket.packet[0]<<16)|(rpacket.packet[1]<<8)|(rpacket.packet[2]);
 if (rpacket.packet[0] & 0x80)
   temp.left |= 0xff<<24;
 temp.right = (rpacket.packet[3]<<16)|(rpacket.packet[4]<<8)|(rpacket.packet[5]);
 if (rpacket.packet[3] & 0x80)
   temp.right |= 0xff<<24;
 // temp.left = (((char) rpacket.packet[0])<<16)|(rpacket.packet[1]<<8)|(rpacket.packet[2]);
 // temp.right = (((char) rpacket.packet[3])<<16)|(rpacket.packet[4]<<8)|(rpacket.packet[5]);

 // get timer count 32bit unsigned int
 temp.time_count = (rpacket.packet[6] << 24)|(rpacket.packet[7] <<16)|(rpacket.packet[8]<<8)|(rpacket.packet[9]);

 return temp;

}
/* start a thread that will invoke Main() */
void 
ClodBuster::StartThread()
{
  pthread_create(&thread, NULL, &DummyMain, this);
}

/* cancel (and wait for termination) of the thread */
void 
ClodBuster::StopThread()
{
  void* dummy;
  pthread_cancel(thread);
  if(pthread_join(thread,&dummy))
    perror("ClodBuster::StopThread:pthread_join()");
}


void ClodBuster::GetGraspBoardParams()
{
  GRASPPacket packet,rpacket;
  packet.Build(ECHO_MAX_SERVO_LIMITS);
  packet.Send(clodbuster_fd);
  printf("Servo Limit Enquiry: ");

  packet.PrintHex();

  rpacket.Receive(clodbuster_fd,ECHO_MAX_SERVO_LIMITS);
  
  memcpy(max_limits,rpacket.packet,8);
  rpacket.size = rpacket.retsize;
  
  rpacket.PrintHex();

  packet.Build(ECHO_MIN_SERVO_LIMITS);
  packet.Send(clodbuster_fd);
   printf("Servo Limit Enquiry: ");

  packet.PrintHex();
 
  rpacket.Receive(clodbuster_fd,ECHO_MIN_SERVO_LIMITS);
  rpacket.size = rpacket.retsize;
  
  rpacket.PrintHex();
  memcpy(min_limits,rpacket.packet,8);
  packet.Build(ECHO_CEN_SERVO_LIMITS);
  packet.Send(clodbuster_fd);
  printf("Servo Limit Enquiry: ");

  packet.PrintHex();

  rpacket.Receive(clodbuster_fd,ECHO_CEN_SERVO_LIMITS);
  
  memcpy(center_limits,rpacket.packet,8);
  rpacket.size = rpacket.retsize;
  
  rpacket.PrintHex();
}

unsigned char 
ClodBuster::SetServo(unsigned char chan, int value)
{
  GRASPPacket spacket;
  unsigned char cmd;
  int demanded = center_limits[chan]+value/10;

  if (demanded > max_limits[chan])
    cmd = (unsigned char) max_limits[chan];
  else if (demanded < min_limits[chan])
    cmd = (unsigned char) min_limits[chan];
  else
    cmd = (unsigned char)demanded;
  spacket.Build(chan,cmd);
  spacket.Send(clodbuster_fd);
  // spacket.PrintHex();

  return(cmd);
}
void 
ClodBuster::SetServo(unsigned char chan, unsigned char cmd)
{
  GRASPPacket spacket;
  spacket.Build(chan,cmd);
  spacket.Send(clodbuster_fd);
  // spacket.PrintHex();
}

void ClodBuster::IntegrateEncoders()
{
  // assign something to data-> xpos,ypos, theta
  float dEr = encoder_measurement.right-old_encoder_measurement.right; 
  float dEl = encoder_measurement.left-old_encoder_measurement.left; 
  float L = Kenc*(dEr+dEl)*.5;
  float D = Kenc*(dEr-dEl)/WheelSeparation;
  float Phi = M_PI/180.0*data->position.yaw+0.5*D;

  data->position.xpos += (int32_t) (L*cos(Phi)*1.0e3);
  data->position.ypos += (int32_t) (L*sin(Phi)*1.0e3);
  data->position.yaw += (int32_t) (D*180.0/M_PI);
}

void ClodBuster::DifferenceEncoders()
{
  float dEr = encoder_measurement.right-old_encoder_measurement.right; 
  float dEl = encoder_measurement.left-old_encoder_measurement.left; 
  int64_t dtc =  encoder_measurement.time_count-old_encoder_measurement.time_count;
  if (dtc< -2147483648LL) 
       {
	    dtc += 4294967296LL; //(1<<32);
	    printf("encoder timer rollover caught\n");
       }
  float dt = dtc*1.6e-6;
  if (dt < 20e-3)
       printf("dt way too short %f s\n",dt);
  else if  (dt > 2.0/LoopFreq)
       printf("dt way too long %f s\n",dt);
       
  EncV = Kenc*(dEr+dEl)*.5/dt;
  EncOmega = Kenc*(dEr-dEl)/WheelSeparation/dt;     
  EncVleft = dEl/dt;
  EncVright = dEr/dt;
  printf("EncV = %f, EncW = %f, dt = %f\n",EncV,EncOmega*180/M_PI,dt);

}

size_t ClodBuster::GetData(void* client, unsigned char *dest, size_t maxsize,
                                 uint32_t* timestamp_sec, 
                                 uint32_t* timestamp_usec)
{
  Lock();
  *((player_position_data_t*)dest) = 
          ((player_clodbuster_data_t*)device_data)->position;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();
  return( sizeof( player_position_data_t) );
}

void ClodBuster::PutCommand(void* client, unsigned char *src, size_t size ) 
{
  if(size != sizeof( player_position_cmd_t ) )
  {
    puts("ClodBuster::PutCommand(): command wrong size. ignoring.");
    printf("expected %d; got %d\n", sizeof(player_position_cmd_t),size);
  }
  else
    ((player_clodbuster_cmd_t*)device_command)->position = 
            *((player_position_cmd_t*)src);
}

