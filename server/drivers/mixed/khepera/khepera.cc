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

/* Copyright (C) 2004
 *   Toby Collett, University of Auckland Robotics Group
 *
 * Header for the khepera robot.  The
 * architecture is similar to the P2OS device, in that the position, IR and
 * power services all need to go through a single serial port and
 * base device class.  So this code was copied from p2osdevice and
 * modified to taste.
 * 
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
#include <netinet/in.h>
#include <ctype.h>

#include <khepera.h>


#include <playertime.h>
extern PlayerTime* GlobalTime;

// so we can access the deviceTable and extract pointers to the sonar
// and position objects
#include <devicetable.h>
extern CDeviceTable* deviceTable;
extern int global_playerport; // used to get at devices

// we need to debug different things at different times
//#define DEBUG_POS
//#define DEBUG_SERIAL
#define DEBUG_CONFIG

// useful macros
#define DEG2RAD(x) (((double)(x))*0.01745329251994)
#define RAD2DEG(x) (((double)(x))*57.29577951308232)

//#define DEG2RAD_FIX(x) ((x) * 17453)
//#define RAD2DEG_FIX(x) ((x) * 57295780)
#define DEG2RAD_FIX(x) ((x) * 174)
#define RAD2DEG_FIX(x) ((x) * 572958)

/* these are necessary to make the static fields visible to the linker */
player_khepera_data_t * Khepera::data;
player_khepera_geom_t * Khepera::geometry;
player_khepera_cmd_t * 	Khepera::command;
double			Khepera::x;
double			Khepera::y;
double			Khepera::yaw;
pthread_t		Khepera::thread;
struct timeval		Khepera::timeBegan_tv;
int			Khepera::khepera_fd; 
char			Khepera::khepera_serial_port[];
bool			Khepera::initdone;
int			Khepera::param_index;
pthread_mutex_t		Khepera::khepera_accessMutex;
pthread_mutex_t		Khepera::khepera_setupMutex;
int			Khepera::khepera_subscriptions;
int			Khepera::ir_subscriptions;
int			Khepera::pos_subscriptions;
unsigned char*		Khepera::reqqueue;
unsigned char*		Khepera::repqueue;
struct timeval		Khepera::last_position;
bool			Khepera::refresh_last_position;
int			Khepera::last_lpos;
int			Khepera::last_rpos;
int			Khepera::last_x_f;
int			Khepera::last_y_f;
double			Khepera::last_theta;
bool			Khepera::motors_enabled;
bool			Khepera::velocity_mode;
bool			Khepera::direct_velocity_control;
short			Khepera::desired_heading;
int			Khepera::locks;
int			Khepera::slocks;
struct pollfd		Khepera::write_pfd;
struct pollfd		Khepera::read_pfd;

Khepera::Khepera(char *interface, ConfigFile *cf, int section)
{
  int reqqueuelen = 1;
  int repqueuelen = 1;

  if(!initdone)
  {

    locks = slocks =0;

    data = new player_khepera_data_t;
    command = new player_khepera_cmd_t;
    geometry = new player_khepera_geom_t;
    geometry->PortName= NULL;
    geometry->scale = 0;

    reqqueue = (unsigned char*)(new playerqueue_elt_t[reqqueuelen]);
    repqueue = (unsigned char*)(new playerqueue_elt_t[repqueuelen]);

    SetupBuffers((unsigned char*)data, sizeof(player_khepera_data_t), 
                 (unsigned char*)command, sizeof(player_khepera_cmd_t),
                 reqqueue, reqqueuelen,
                 repqueue, repqueuelen);

    ((player_khepera_cmd_t*)device_command)->position.xspeed = 0;
    ((player_khepera_cmd_t*)device_command)->position.yawspeed = 0;

    khepera_subscriptions = 0;
    ir_subscriptions = 0;
    pos_subscriptions = 0;

    //set up the poll parameters... used for the comms
    // over the serial port to the Kam
    //write_pfd.events = POLLOUT;
    //read_pfd.events = POLLIN;

    pthread_mutex_init(&khepera_accessMutex,NULL);
    pthread_mutex_init(&khepera_setupMutex,NULL);

    
    initdone = true; 
  }
  else
  {
    // every sub-device gets its own queue object (but they all point to the
    // same chunk of memory)
    
    // every sub-device needs to get its various pointers set up
    SetupBuffers((unsigned char*)data, sizeof(player_khepera_data_t),
                 (unsigned char*)command, sizeof(player_khepera_cmd_t),
                 reqqueue, reqqueuelen,
                 repqueue, repqueuelen);
  }


	// now we have to look up our parameters.  this should be given as an argument
	if (geometry->PortName == NULL)
		geometry->PortName = strdup(cf->ReadString(section, "port", KHEPERA_DEFAULT_SERIAL_PORT));
	if (geometry->scale == 0)
		geometry->scale = cf->ReadFloat(section, "scale_factor", KHEPERA_DEFAULT_SCALE);
  
  	if (!strcmp(interface,PLAYER_POSITION_STRING))
  	{
		// set sub type here
		geometry->position.subtype = PLAYER_POSITION_GET_GEOM_REQ;
		
		geometry->encoder_res = cf->ReadFloat(section,"encoder_res", KHEPERA_DEFAULT_ENCODER_RES);
		
  		// Load position config
		geometry->position.pose[0] = khtons(static_cast<unsigned short> (cf->ReadTupleFloat(section,"pose",0,0)));
		geometry->position.pose[1] = khtons(static_cast<unsigned short> (cf->ReadTupleFloat(section,"pose",1,0)));
		geometry->position.pose[2] = htons(static_cast<unsigned short> (cf->ReadTupleFloat(section,"pose",2,0)));

		// load dimension of the base
		geometry->position.size[0] = khtons(static_cast<unsigned short> (cf->ReadTupleFloat(section,"size",0,57)));
		geometry->position.size[1] = khtons(static_cast<unsigned short> (cf->ReadTupleFloat(section,"size",1,57)));
	}
	else if (!strcmp(interface,PLAYER_IR_STRING))
	{
		// load ir geometry config
		geometry->ir.pose_count = (cf->ReadInt(section,"pose_count", 8));
		if (geometry->ir.pose_count == 8 && cf->ReadTupleFloat(section,"poses",0,-1) == -1)
		{
			// load the default ir geometry
			geometry->ir.poses[0][0] = khtons(10);
			geometry->ir.poses[0][1] = khtons(24);
			geometry->ir.poses[0][2] = htons(90);

			geometry->ir.poses[1][0] = khtons(19);
			geometry->ir.poses[1][1] = khtons(17);
			geometry->ir.poses[1][2] = htons(45);

			geometry->ir.poses[2][0] = khtons(25);
			geometry->ir.poses[2][1] = khtons(6);
			geometry->ir.poses[2][2] = htons(0);

			geometry->ir.poses[3][0] = khtons(25);
			geometry->ir.poses[3][1] = khtons(-6);
			geometry->ir.poses[3][2] = htons(0);

			geometry->ir.poses[4][0] = khtons(19);
			geometry->ir.poses[4][1] = khtons(-17);
			geometry->ir.poses[4][2] = htons(static_cast<unsigned short> (-45));

			geometry->ir.poses[5][0] = khtons(10);
			geometry->ir.poses[5][1] = khtons(-24);
			geometry->ir.poses[5][2] = htons(static_cast<unsigned short> (-90));

			geometry->ir.poses[6][0] = khtons(-24);
			geometry->ir.poses[6][1] = khtons(-10);
			geometry->ir.poses[6][2] = htons(180);

			geometry->ir.poses[7][0] = khtons(-24);
			geometry->ir.poses[7][1] = khtons(10);
			geometry->ir.poses[7][2] = htons(180);
		}
		else
		{
			// laod geom from config file
			for (int i = 0; i < geometry->ir.pose_count; ++i)
			{
				geometry->ir.poses[i][0] = khtons(static_cast<short> (cf->ReadTupleFloat(section,"poses",3*i,0)));
				geometry->ir.poses[i][1] = khtons(static_cast<short> (cf->ReadTupleFloat(section,"poses",3*i+1,0)));
				geometry->ir.poses[i][2] = htons(static_cast<short> (cf->ReadTupleFloat(section,"poses",3*i+2,0)));
			}				
		}
		// laod ir calibration from config file
		geometry->ir_calib_a = new double[geometry->ir.pose_count];
		geometry->ir_calib_b = new double[geometry->ir.pose_count];
		for (int i = 0; i < geometry->ir.pose_count; ++i)
		{
			geometry->ir_calib_a[i] = cf->ReadTupleFloat(section,"ir_calib_a", i, KHEPERA_DEFAULT_IR_CALIB_A);
			geometry->ir_calib_b[i] = cf->ReadTupleFloat(section,"ir_calib_b", i, KHEPERA_DEFAULT_IR_CALIB_B);
		}
		geometry->ir.pose_count = htons(geometry->ir.pose_count);
	}
  // zero position counters
  last_lpos = 0;
  last_rpos = 0;
  last_x_f=0;
  last_y_f=0;
  last_theta = 0.0;

  // zero the subscription counter.
  subscriptions = 0;
}

short Khepera::khtons(short in)
{
	return htons(static_cast<unsigned short> (in * geometry->scale));

}

short Khepera::ntokhs(short in)
{
	return static_cast<short> (ntohs(static_cast<unsigned short> (in)) / geometry->scale);

}

void 
Khepera::Lock()
{
  // keep track of our locks cuz we seem to lose one
  // somewhere somehow
  locks++;
  pthread_mutex_lock(&khepera_accessMutex);
}

void 
Khepera::Unlock()
{
  locks--;
  pthread_mutex_unlock(&khepera_accessMutex);
}

void 
Khepera::SetupLock()
{
  slocks++;
  pthread_mutex_lock(&khepera_setupMutex);
}

void 
Khepera::SetupUnlock()
{
  slocks--;
  pthread_mutex_unlock(&khepera_setupMutex);
}

/* called the first time a client connects
 *
 * returns: 0 on success
 */
int 
Khepera::Setup()
{
	// open and initialize the serial to -> Khepera  
	printf("Khepera: connection initializing (%s)...", this->khepera_serial_port);
	fflush(stdout);
	Serial = new KheperaSerial(geometry->PortName,KHEPERA_BAUDRATE);
	if (Serial == NULL || !Serial->Open())
	{
		return 1;
	}
	printf("Done\n");

	refresh_last_position = false;
	motors_enabled = false;
	velocity_mode = true;
	direct_velocity_control = false;

	desired_heading = 0;

	/* now spawn reading thread */
	StartThread();
	return(0);
}


int 
Khepera::Shutdown()
{
  	printf("Khepera: SHUTDOWN\n");
	StopThread();

	// Killing the thread seems to leave out serial
	// device in a bad state, need to fix this,
	// till them we just dont stop the robot
	// which is theoretically quite bad...but this is the khepera...
	// it cant do that much damage its only 7cm across
  	//SetSpeed(0,0);
  	delete Serial;
  	Serial = NULL;


  	// zero these out or we may have problems
  	// next time we connect
  	player_khepera_cmd_t cmd;
	
  	cmd.position.xspeed = 0;
  	cmd.position.yawspeed = 0;
  	cmd.position.yaw = 0;

  	if (locks > 0) 
	{
    		printf("Khepera: %d LOCKS STILL EXIST\n", locks);
    		while (locks) 
			Unlock();
  	}
  
  	return(0);
}

int 
Khepera::Subscribe(void *client)
{
  int setupResult;

  SetupLock();

  if(khepera_subscriptions == 0) 
  {
    setupResult = Setup();
    if (setupResult == 0 ) 
    {
      khepera_subscriptions++;  // increment the static reb-wide subscr counter
      subscriptions++;       // increment the per-device subscr counter
    }
  }
  else 
  {
    khepera_subscriptions++;  // increment the static reb-wide subscr counter
    subscriptions++;       // increment the per-device subscr counter
    setupResult = 0;
  }
  
  SetupUnlock();
  return( setupResult );
}

int 
Khepera::Unsubscribe(void *client)
{
  int shutdownResult;

  SetupLock();
  
  if(khepera_subscriptions == 0) 
  {
    shutdownResult = -1;
  }
  else if(khepera_subscriptions == 1) 
  {
    shutdownResult = Shutdown();
    if (shutdownResult == 0 ) 
    { 
      khepera_subscriptions--;  // decrement the static reb-wide subscr counter
      subscriptions--;       // decrement the per-device subscr counter
    }
    /* do we want to unsubscribe even though the shutdown went bad? */
  }
  else 
  {
    khepera_subscriptions--;  // decrement the static reb-wide subscr counter
    subscriptions--;       // decrement the per-device subscr counter
    shutdownResult = 0;
  }
  
  SetupUnlock();

  return( shutdownResult );
}


void 
Khepera::PutData( unsigned char* src, size_t maxsize,
	      uint32_t timestamp_sec, uint32_t timestamp_usec)
{
  Lock();

  *((player_khepera_data_t*)device_data) = *((player_khepera_data_t*)src);


  if(timestamp_sec == 0)
  {
    struct timeval curr;
    GlobalTime->GetTime(&curr);
    timestamp_sec = curr.tv_sec;
    timestamp_usec = curr.tv_usec;
  }

  data_timestamp_sec = timestamp_sec;
  data_timestamp_usec = timestamp_usec;
  
  // need to fill in the timestamps on all Khepera devices, both so that they
  // can read it, but also because other devices may want to read it
  player_device_id_t id = device_id;

  id.code = PLAYER_IR_CODE;
  CDevice* ir = deviceTable->GetDevice(id);
  if(ir)
  {
    ir->data_timestamp_sec = this->data_timestamp_sec;
    ir->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_POSITION_CODE;
  CDevice* position = deviceTable->GetDevice(id);
  if(position)
  {
    position->data_timestamp_sec = this->data_timestamp_sec;
    position->data_timestamp_usec = this->data_timestamp_usec;
  }

  Unlock();
}

void 
Khepera::Main()
{
	player_khepera_cmd_t cmd;

	// first get pointers to all the devices we control
	player_device_id_t id = device_id;

	id.code = PLAYER_IR_CODE;
	CDevice *ir = deviceTable->GetDevice(id);

	id.code = PLAYER_POSITION_CODE;
	CDevice *pos = deviceTable->GetDevice(id);

	this->pos_subscriptions = 0;
	this->ir_subscriptions = 0;

	GlobalTime->GetTime(&timeBegan_tv);

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

	while (1) 
	{
		// we want to turn on the IR if someone just subscribed, and turn
		// them off if the last subscriber just unsubscribed.
		if(ir) 
		{
      			if(!this->ir_subscriptions && ir->subscriptions) 
			{
				// zero out ranges in IR data so proxy knows
				// to do regression
				for (int i =0 ; i < PLAYER_IR_MAX_SAMPLES; i++) 
				 	data->ir.ranges[i] = 0;
			}
			this->ir_subscriptions = ir->subscriptions;
    		}
    
		// we want to reset the odometry and enable the motors if the first 
		// client just subscribed to the position device, and we want to stop 
		// and disable the motors if the last client unsubscribed.
		if(pos) 
		{
			if(!this->pos_subscriptions && pos->subscriptions) 
			{
				printf("Khepera: first pos sub. turn off and reset\n");
				// then first sub for pos, so turn off motors and reset odom
				SetSpeed(0,0);
				ResetOdometry();

      			} 
			else if (this->pos_subscriptions && !(pos->subscriptions)) 
			{
				// last sub just unsubbed
				printf("Khepera: last pos sub gone\n");
				SetSpeed(0,0);
	
        			// overwrite existing motor commands to be zero
        			player_position_cmd_t position_cmd;
        			position_cmd.xspeed = 0;
        			position_cmd.yawspeed = 0;
				position_cmd.yaw = 0;
        			// TODO: who should really be the client here?
        			pos->PutCommand(this,(unsigned char*)(&position_cmd), sizeof(position_cmd));
			}
      			this->pos_subscriptions = pos->subscriptions;
    		} 

    		// get configuration commands (ioctls)
		ReadConfig();

		// read the clients' commands from the common buffer 
		GetCommand((unsigned char*)&cmd, sizeof(cmd));
    
		if (this->pos_subscriptions) 
		{
      			if (this->velocity_mode) 
			{
				// then we are in velocity mode

				// need to calculate the left and right velocities
				int transvel = static_cast<int> (static_cast<int> (ntohl(cmd.position.xspeed)) * geometry->encoder_res);
				int rotvel = static_cast<int> (static_cast<int> (ntohl(cmd.position.yawspeed)) * geometry->encoder_res * M_PI * ntokhs(geometry->position.size[0])/360.0);
				int leftvel = transvel + rotvel;
				int rightvel = transvel - rotvel;
	
				// now we set the speed
				if (this->motors_enabled) 
					SetSpeed(leftvel,rightvel);
				else 
					SetSpeed(0,0);
      			} 
	  	}
    		pthread_testcancel();

    		// now lets get new data...
    		UpdateData();

    		pthread_testcancel();
	}
  	pthread_exit(NULL);
}

/* start a thread that will invoke Main() */
void 
Khepera::StartThread()
{
  pthread_create(&thread, NULL, &DummyMain, this);
}

/* cancel (and wait for termination) of the thread */
void 
Khepera::StopThread()
{
  void* dummy;
  pthread_cancel(thread);
  if(pthread_join(thread,&dummy))
    perror("Khepera::StopThread:pthread_join()");
}


/* this will read a new config command and interpret it
 *
 * returns: 
 */
void
Khepera::ReadConfig()
{
  int config_size;
  unsigned char config_buffer[KHEPERA_CONFIG_BUFFER_SIZE];
  player_device_id_t id;
  void *client;

  if ((config_size = GetConfig(&id, &client, 
			       (void *)config_buffer, sizeof(config_buffer)))) {
    
    	// figure out which device it's for
	switch(id.code) 
	{
      
      	// Khepera_IR IOCTLS /////////////////
    	case PLAYER_IR_CODE:
      
#ifdef DEBUG_CONFIG
      	printf("Khepera: IR CONFIG\n");
#endif

      	// figure out which command
      	switch(config_buffer[0]) 
	{
		case PLAYER_IR_POSE_REQ: 
		{
			// request the pose of the IR sensors in robot-centric coords
			if (config_size != 1) 
			{
	  			fprintf(stderr, "Khepera: argument to IR pose req wrong size (%d) should be (%d)\n", config_size,sizeof(player_ir_pose_req_t));
	  			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) 
				{
	    				PLAYER_ERROR("Khepera: failed to put reply");
	    				break;
	  			}
			}
	
#ifdef DEBUG_CONFIG
			printf("Khepera: IR_POSE_REQ\n");
#endif

			player_ir_pose_req_t irpose;
			irpose.subtype = PLAYER_IR_POSE_REQ;
			irpose.poses = geometry->ir;
			
			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &irpose, sizeof(irpose))) 
			{
	  			PLAYER_ERROR("Khepera: failed to put reply");
	  			break;
			}
      		}
      		break;

      		default:
			fprintf(stderr, "Khepera: IR got unknown config\n");
			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) 
			{
	  			PLAYER_ERROR("Khepera: failed to put reply");
			}
		break;
      	}
      	break;
      
      	// END Khepera_IR IOCTLS //////////////

      	// POSITION IOCTLS ////////////////
	case PLAYER_POSITION_CODE:
#ifdef DEBUG_CONFIG
	printf("Khepera: POSITION CONFIG\n");
#endif
	switch (config_buffer[0]) 
	{
		case PLAYER_POSITION_GET_GEOM_REQ: 
		{
			// get geometry of robot
			if (config_size != 1) 
			{
	  			fprintf(stderr, "Khepera: get geom req is wrong size (%d)\n", config_size);
	  			PLAYER_ERROR("Khepera: failed to put reply");
	  			break;
			}

#ifdef DEBUG_CONFIG
			printf("Khepera: POSITION_GET_GEOM_REQ\n");
#endif

	
			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geometry->position,
		     		sizeof(geometry->position))) 
			{
	  			PLAYER_ERROR("Khepera: failed to put reply");
			}
      		}
      		break;

      		case PLAYER_POSITION_MOTOR_POWER_REQ: 
		{
			// change motor state
			// 1 for on 
			// 0 for off

			if (config_size != sizeof(player_position_power_config_t)) 
			{
	  			fprintf(stderr, "Khepera: pos motor power req got wrong size (%d)\n", config_size);
	  			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) 
				{
	    				PLAYER_ERROR("Khepera: failed to put reply");
	    				break;
	  			}
			}

			player_position_power_config_t *mpowreq = (player_position_power_config_t *)config_buffer;
 
#ifdef DEBUG_CONFIG
			printf("Khepera: MOTOR_POWER_REQ %d\n", mpowreq->value);
#endif
	
			if (mpowreq->value) 
			{
	  			this->motors_enabled = true;
			} 
			else 
			{
	  			this->motors_enabled = false;
			}
	
			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) 
			{
	 	 		PLAYER_ERROR("Khepera: failed to put reply");
			}

			printf("Khepera: put MOTOR POWER REQ\n");
      		}
      		break;

      		case PLAYER_POSITION_VELOCITY_MODE_REQ: 
		{
			// select method of velocity control
			// 0 for direct velocity control (trans and rot applied directly)
			// 1 for builtin velocity based heading PD controller
			if (config_size != sizeof(player_position_velocitymode_config_t)) 
			{
	  			fprintf(stderr, "Khepera: pos vel control req got wrong size (%d)\n", config_size);
	  			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) 
				{
	    				PLAYER_ERROR("Khepera: failed to put reply");
	    				break;
	  			}
			}

			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) 
			{
	  			PLAYER_ERROR("Khepera: failed to put reply");
			}
      		}
      		break;

      		case PLAYER_POSITION_RESET_ODOM_REQ: 
		{
			// reset the odometry
			if (config_size != 1) 
			{
	  			fprintf(stderr, "Khepera: pos reset odom req got wrong size (%d)\n", config_size);
	  			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) 
				{
	    				PLAYER_ERROR("Khepera: failed to put reply");
	    				break;
	  			}
			}

#ifdef DEBUG_CONFIG
			printf("Khepera: RESET_ODOM_REQ\n");
#endif
	
			ResetOdometry();
	
			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) 
			{
	 	 		PLAYER_ERROR("Khepera: failed to put reply");
			}
      		}
      		break;

      		case PLAYER_POSITION_SET_ODOM_REQ: 
		{
			// set the odometry to a given position
			if (config_size != sizeof(player_position_set_odom_req_t)) 
			{
	  			fprintf(stderr, "Khepera: pos set odom req got wrong size (%d)\n", config_size);
	  			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) 
				{
	    				PLAYER_ERROR("Khepera: failed to put reply");
	    				break;
	  			}
			}

			player_position_set_odom_req_t *req = (player_position_set_odom_req_t *)config_buffer;
#ifdef DEBUG_CONFIG
			int x,y;
			short theta;
			x = ntohl(req->x);
			y = ntohl(req->y);
			theta = ntohs(req->theta);

			printf("Khepera: SET_ODOM_REQ x=%d y=%d theta=%d\n", x, y, theta);
#endif
			ResetOdometry();

			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) 
			{
	  			PLAYER_ERROR("Khepera: failed to put reply");
			}
		}
		break;
      		
		default:
		{
			fprintf(stderr, "Khepera: Position got unknown config\n");
			if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) 
			{
	  			PLAYER_ERROR("Khepera: failed to put reply");
			}
		}
		break;
	}
	break;
      
    default:
      printf("Khepera: unknown config code %d\n", id.code);
    }
  }
}


/* this will update the data that is sent to clients
 * just call separate functions to take care of it
 *
 * returns:
 */
void
Khepera::UpdateData()
{
  //player_khepera_data_t d;

  //Lock();
  //memcpy(&d, this->data, sizeof(player_khepera_data_t));
  //Unlock();

  UpdateIRData(data);

  UpdatePosData(data);
  
  PutData((unsigned char *)data, sizeof(*data), 0 ,0);
}

/* this will update the IR part of the client data
 * it entails reading the currently active IR sensors
 * and then changing their state to off and turning on
 * 2 new IRs.  
 *
 * returns:
 */
void
Khepera::UpdateIRData(player_khepera_data_t * d)
{
	Lock();
	ReadAllIR();
  
  	d->ir.range_count = geometry->ir.pose_count;
	for (int i =0; i < ntohs(geometry->ir.pose_count); i++) 
	{
		d->ir.ranges[i] = htons(static_cast<unsigned short> (geometry->scale * geometry->ir_calib_a[i] * pow(data->ir.voltages[i],geometry->ir_calib_b[i])));
		d->ir.voltages[i] = htons(data->ir.voltages[i]);
		//printf("(%d,%d) ",ntohs(d->ir.ranges[i]),ntohs(d->ir.voltages[i]));
  	}
	Unlock();
		//printf("\n");
}

  
/* this will update the position data.  this entails odometry, etc
 */ 
void
Khepera::UpdatePosData(player_khepera_data_t *d)
{
	// calculate position data
	int pos_left, pos_right;
	Khepera::ReadPos(&pos_left, &pos_right);
	int change_left = pos_left - last_lpos;
	int change_right = pos_right - last_rpos;
	last_lpos = pos_left;
	last_rpos = pos_right;

	double transchange = (change_left + change_right) * geometry->encoder_res / 2;
	double rotchange = (change_left - change_right) * geometry->encoder_res / 2;
	
	double dx,dy,Theta;
	double r = (ntokhs(geometry->position.size[0])/2);
	
	if (transchange == 0)
	{
		Theta = 360 * rotchange/(2 * M_PI * r);	
		dx = dy= 0;
	}
	else if (rotchange == 0)
	{
		dx = transchange;
		dy = 0;
		Theta = 0;
	}
	else
	{
		Theta = 360 * rotchange/(2 * M_PI * r);
		double R = transchange * r / rotchange;
		dy = R - R*cos(Theta*M_PI/180);
		dx = R*sin(Theta*M_PI/180);
	}

	// add code to read in the speed data
	int left_vel, right_vel;
	Khepera::ReadSpeed(&left_vel, &right_vel);
	double lv = left_vel * geometry->encoder_res;
	double rv = right_vel * geometry->encoder_res;
	double trans_vel = 100 * (lv + rv)/2;
	double rot_vel = (lv - rv)/2;
	double rot_vel_deg = 100 * 360 * rot_vel/(2 * M_PI * r);

  	// now write data
	Lock();
	x+=dx;
	y+=dy;
  	d->position.xpos = htonl(static_cast<int> (x));
  	d->position.ypos = ntohl(static_cast<int> (y));
	yaw += Theta;
	while (yaw > 360) yaw -= 360;
	while (yaw < 0) yaw += 360;
  	d->position.yaw = htonl(static_cast<int> (yaw));
  	d->position.xspeed = htonl(static_cast<int> (trans_vel));
	//d->position.yspeed = 0;
  	d->position.yawspeed = htonl(static_cast<int> (rot_vel_deg));
  	//d->position.stall = 0;
	Unlock();
}

/* this will set the odometry to a given position
 * ****NOTE: assumes that the arguments are in network byte order!*****
 *
 * returns: 
 */
int
Khepera::ResetOdometry()
{
	printf("Reset Odometry\n");
	int Values[2];
	Values[0] = 0;
	Values[1] = 0;
	if (Serial->KheperaCommand('G',2,Values,0,NULL) < 0)
		return -1;
		
	last_lpos = 0;
	last_rpos = 0;

	Lock();
	this->data->position.xpos = 0;
	this->data->position.ypos = 0;
	this->data->position.yaw = 0;
	Unlock();
	
	x=y=yaw=0;
	return 0;
}


/* this will read the given AD channel
 *
 * returns: the value of the AD channel
 */
/*unsigned short
REB::ReadAD(int channel) 
{
  char buf[64];

  sprintf(buf, "I,%d\r", channel);
  write_command(buf, strlen(buf), sizeof(buf));
  
  return atoi(&buf[2]);
}*/

/* reads all the IR values at once.  stores them
 * in the uint16_t array given as arg ir
 *
 * returns: 
 */
int
Khepera::ReadAllIR()
{
	int Values[ntohs(geometry->ir.pose_count)];

	if(Serial->KheperaCommand('N',0,NULL,ntohs(geometry->ir.pose_count),Values) < 0)
		return -1;			
	for (int i=0; i< ntohs(geometry->ir.pose_count); ++i)
	{
		data->ir.voltages[i] = static_cast<short> (Values[i]);
	}
	return 0;
}

/* this will set the desired speed for the given motor mn
 *
 * returns:
 */
int
Khepera::SetSpeed(int speed1, int speed2)
{
	int Values[2];
	Values[0] = speed1;
	Values[1] = speed2;
	return Serial->KheperaCommand('D',2,Values,0,NULL);
}

/* reads the current speed of motor mn
 *
 * returns: the speed of mn
 */
int
Khepera::ReadSpeed(int * left,int * right)
{
	int Values[2];
	if (Serial->KheperaCommand('E',0,NULL,2,Values) < 0)
		return -1;
	*left = Values[0];
	*right = Values[1];
	return 0;
}

/* this sets the desired position motor mn should go to
 *
 * returns:
 */
/*void
REB::SetPos(int mn, int pos)
{
  char buf[64];
  
  sprintf(buf,"C,%c,%d\r", '0'+mn,pos);

  write_command(buf, strlen(buf), sizeof(buf));
}*/

/* this sets the position counter of motor mn to the given value
 *
 * returns:
 */
int
Khepera::SetPosCounter(int pos1, int pos2)
{
	int Values[2];
	Values[0] = pos1;
	Values[1] = pos2;
	return Serial->KheperaCommand('G',2,Values,0,NULL);
}

/* this will read the current value of the position counter
 * for motor mn
 *
 * returns: the current position for mn
 */
int
Khepera::ReadPos(int * pos1, int * pos2)
{
	int Values[2];
	if (Serial->KheperaCommand('H',0,NULL,2,Values) < 0)
		return -1;
	*pos1 = Values[0];
	*pos2 = Values[1];
	return 0;
}

/* this will configure the position PID for motor mn
 * using paramters Kp, Ki, and Kd
 *
 * returns:
 */
/*void
REB::ConfigPosPID(int mn, int kp, int ki, int kd)
{ 
  char buf[64];

  sprintf(buf, "F,%c,%d,%d,%d\r", '0'+mn, kp,ki,kd);
  write_command(buf, strlen(buf), sizeof(buf));
}*/

/* this will configure the speed PID for motor mn
 *
 * returns:
 */
/*void
REB::ConfigSpeedPID(int mn, int kp, int ki, int kd)
{
  char buf[64];
  
  sprintf(buf, "A,%c,%d,%d,%d\r", '0'+mn, kp,ki,kd);
  
  write_command(buf, strlen(buf), sizeof(buf));
}*/

/* this will set the speed profile for motor mn
 * it takes the max velocity and acceleration
 *
 * returns:
 */
/*void
REB::ConfigSpeedProfile(int mn, int speed, int acc)
{
  char buf[64];
  
  sprintf(buf, "J,%c,%d,%d\r", '0'+mn, speed,acc);
  write_command(buf, strlen(buf), sizeof(buf));
}*/

/* this will read the status of the motion controller.
 * mode is set to 1 if in position mode, 0 if velocity mode
 * error is set to the position/speed error
 *
 * returns: target status: 1 is on target, 0 is not on target
 */
/*unsigned char
Khepera::ReadStatus(int mn, int *mode, int *error)
{
  char buf[64];

  sprintf(buf, "K,%c\r", '0'+mn);
  //write_command(buf, strlen(buf), sizeof(buf));

  // buf now has the form "k,target,mode,error"
  int target;

  sscanf(buf, "k,%d,%d,%d", &target, mode, error);

  return (unsigned char)target;
}*/
