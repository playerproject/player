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

/* Copyright (C) 2002
 *   John Sweeney, UMASS, Amherst, Laboratory for Perceptual Robotics
 *
 * $Id$
 *
 *   The REB device.  This controls the K-Team Kameleon 376SBC with
 * the Robotics Extension Board (REB).  (Technically the REB doesn't control
 * anything, it just provides the analog I/Os, H-bridges, etc but we
 * thought REB was a good acronym...)  The REB/Kameleon board has the motor
 * drivers and sensor I/O, and we communicate with it via a serial port.
 * So the overall architecture is similar to the p2osdevice, where this class
 * handles the data gathering tasks for the Position, IR and power devices.
 *
 * Our robots use a StrongARM SA110 for the compute power, so we have
 * to minimize the use of floating point, since the ARM can only emulate
 * it.
 * 
 * Note that this device came from copying p2osdevice.cc and then editing
 * to taste, so thanks to the writers of p2osdevice.cc!
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

#include <reb.h>

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

#define DEG2RAD_FIX(x) ((x) * 17453)
#define RAD2DEG_FIX(x) ((x) * 57295780)

/* these are necessary to make the static fields visible to the linker */
extern pthread_t		REB::thread;
extern struct timeval		REB::timeBegan_tv;
extern int			REB::reb_fd; 
extern char			REB::reb_serial_port[];
extern bool			REB::initdone;
extern int			REB::param_index;
extern pthread_mutex_t		REB::reb_accessMutex;
extern pthread_mutex_t		REB::reb_setupMutex;
extern int			REB::reb_subscriptions;
extern int			REB::ir_subscriptions;
extern int			REB::pos_subscriptions;
extern int			REB::power_subscriptions;
extern player_reb_data_t*	REB::data;
extern player_reb_cmd_t*	REB::command;
extern unsigned char*		REB::reqqueue;
extern unsigned char*		REB::repqueue;
extern struct timeval		REB::last_position;
extern bool			REB::refresh_last_position;
extern bool			REB::motors_enabled;
extern bool			REB::velocity_mode;
extern bool			REB::direct_velocity_control;
extern int			REB::ir_sequence;
extern struct timeval		REB::last_ir;
extern short			REB::desired_heading;
extern struct timeval		REB::last_pos_update;
extern struct timeval		REB::last_ir_update;
extern struct timeval		REB::last_power_update;
extern int			REB::pos_update_period;
extern int			REB::locks;
extern int			REB::slocks;

REB::REB(char *interface, ConfigFile *cf, int section)
{
  int reqqueuelen = 1;
  int repqueuelen = 1;

  if(!initdone)
  {
    locks = slocks =0;
    // build the table of robot parameters.
    initialize_reb_params();
  
    // also, install default parameter values.
    strncpy(this->reb_serial_port,REB_DEFAULT_SERIAL_PORT,sizeof(this->reb_serial_port));
    reb_fd = -1;
  
    data = new player_reb_data_t;
    command = new player_reb_cmd_t;

    reqqueue = (unsigned char*)(new playerqueue_elt_t[reqqueuelen]);
    repqueue = (unsigned char*)(new playerqueue_elt_t[repqueuelen]);

    SetupBuffers((unsigned char*)data, sizeof(player_reb_data_t), 
                 (unsigned char*)command, sizeof(player_reb_cmd_t),
                 reqqueue, reqqueuelen,
                 repqueue, repqueuelen);

    ((player_reb_cmd_t*)device_command)->position.xspeed = 0;
    ((player_reb_cmd_t*)device_command)->position.yawspeed = 0;

    this->reb_subscriptions = 0;
    this->ir_subscriptions = 0;
    this->pos_subscriptions = 0;
    this->power_subscriptions = 0;

    GlobalTime->GetTime(&this->last_pos_update);

    // we want to stagger our writes to the serial port
    // so we are doing some rudimentary scheduling
    this->last_ir_update = this->last_pos_update;
    if (this->last_ir_update.tv_usec < 
	this->last_ir_update.tv_usec+REB_IR_UPDATE_PERIOD*1000) {
      this->last_ir_update.tv_usec += REB_IR_UPDATE_PERIOD*1000;
    } else {
      this->last_ir_update.tv_sec++;
      this->last_ir_update.tv_usec += REB_IR_UPDATE_PERIOD*1000;
    }
    this->last_power_update = this->last_pos_update;

    pthread_mutex_init(&reb_accessMutex,NULL);
    pthread_mutex_init(&reb_setupMutex,NULL);

    initdone = true; 
  }
  else
  {
    // every sub-device gets its own queue object (but they all point to the
    // same chunk of memory)
    
    // every sub-device needs to get its various pointers set up
    SetupBuffers((unsigned char*)data, sizeof(player_reb_data_t),
                 (unsigned char*)command, sizeof(player_reb_cmd_t),
                 reqqueue, reqqueuelen,
                 repqueue, repqueuelen);
  }

  this->param_index = 0;   
  this->refresh_last_position = true;

  strncpy(reb_serial_port, cf->ReadString(section, "port", reb_serial_port),
	  sizeof(reb_serial_port));
  
  // zero the subscription counter.
  subscriptions = 0;
}

void REB::Lock()
{
  // keep track of our locks cuz we seem to lose one
  // somewhere somehow
  locks++;
  pthread_mutex_lock(&reb_accessMutex);
}

void REB::Unlock()
{
  locks--;
  pthread_mutex_unlock(&reb_accessMutex);
}

void REB::SetupLock()
{
  slocks++;
  pthread_mutex_lock(&reb_setupMutex);
}
void REB::SetupUnlock()
{
  slocks--;
  pthread_mutex_unlock(&reb_setupMutex);
}

int REB::Setup()
{
  struct termios oldtio;
  struct termios params;

  // open and initialize the serial port from the ARM -> REB  
  printf("REB: connection initializing (%s)...\n", this->reb_serial_port);
  fflush(stdout);

  if ((this->reb_fd = open(this->reb_serial_port, O_RDWR | O_NOCTTY)) == -1) {
    perror("REB::Setup():open()");
    return(1);
  }	
  
  bzero(&params, sizeof(params));  
  tcgetattr(this->reb_fd, &oldtio); /* save current serial port settings */
  params.c_cflag = REB_BAUDRATE | CS8 | CLOCAL | CREAD | CSTOPB;
  params.c_iflag = 0; 
  params.c_oflag = 0;
  params.c_lflag = ICANON; 
   
  params.c_cc[VMIN] = 0;
  params.c_cc[VTIME] = 0;
   
  tcflush(this->reb_fd, TCIFLUSH);
  tcsetattr(this->reb_fd, TCSANOW, &params);
   
  // restart the control software on the REB
  printf("REB: sending restart...");
  fflush(stdout);
  write_serial(REB_RESTART_COMMAND, strlen(REB_RESTART_COMMAND));

  // we need to read 4 complete lines from REB
  char buf[256];
  read_serial_until(buf, sizeof(buf), CRLF, strlen(CRLF));
  read_serial_until(buf, sizeof(buf), CRLF, strlen(CRLF));
  read_serial_until(buf, sizeof(buf), CRLF, strlen(CRLF));
  read_serial_until(buf, sizeof(buf), CRLF, strlen(CRLF));
  printf("done\n");
  
  // so no IRs firing
  StopIR();

  this->param_index =0;
  this->motors_enabled = false;
  this->velocity_mode = true;
  this->direct_velocity_control = false;
  this->refresh_last_position = false;

  this->pos_update_period = REB_POS_UPDATE_PERIOD_VEL;

  this->desired_heading = 0;

  /* now spawn reading thread */
  StartThread();
  return(0);
}

int REB::Shutdown()
{
  printf("REB: SHUTDOWN\n");
  SetSpeed(REB_MOTOR_LEFT, 0);
  SetSpeed(REB_MOTOR_RIGHT, 0);

  StopIR();
  // zero these out or we may have problems
  // next time we connect
  player_reb_cmd_t cmd;

  cmd.position.xspeed = 0;
  cmd.position.yawspeed = 0;
  cmd.position.yaw = 0;

  //  PutCommand((uint8_t *)&cmd, sizeof(cmd));

  StopThread();

  if (locks > 0) {
    printf("REB: %d LOCKS STILL EXIST\n", locks);
    while (locks) {
      Unlock();
    }
  }
  
  close(this->reb_fd);
  this->reb_fd = -1;
  return(0);
}

int REB::Subscribe(void *client)
{
  int setupResult;

  SetupLock();

  if(reb_subscriptions == 0) 
  {
    setupResult = Setup();
    if (setupResult == 0 ) 
    {
      reb_subscriptions++;  // increment the static reb-wide subscr counter
      subscriptions++;       // increment the per-device subscr counter
    }
  }
  else 
  {
    reb_subscriptions++;  // increment the static reb-wide subscr counter
    subscriptions++;       // increment the per-device subscr counter
    setupResult = 0;
  }
  
  SetupUnlock();
  return( setupResult );
}

int REB::Unsubscribe(void *client)
{
  int shutdownResult;

  SetupLock();
  
  if(reb_subscriptions == 0) 
  {
    shutdownResult = -1;
  }
  else if(reb_subscriptions == 1) 
  {
    shutdownResult = Shutdown();
    if (shutdownResult == 0 ) 
    { 
      reb_subscriptions--;  // decrement the static reb-wide subscr counter
      subscriptions--;       // decrement the per-device subscr counter
    }
    /* do we want to unsubscribe even though the shutdown went bad? */
  }
  else 
  {
    reb_subscriptions--;  // decrement the static reb-wide subscr counter
    subscriptions--;       // decrement the per-device subscr counter
    shutdownResult = 0;
  }
  
  SetupUnlock();

  return( shutdownResult );
}


void REB::PutData( unsigned char* src, size_t maxsize,
                         uint32_t timestamp_sec, uint32_t timestamp_usec)
{
  Lock();

  *((player_reb_data_t*)device_data) = *((player_reb_data_t*)src);


  if(timestamp_sec == 0)
  {
    struct timeval curr;
    GlobalTime->GetTime(&curr);
    timestamp_sec = curr.tv_sec;
    timestamp_usec = curr.tv_usec;
  }

  data_timestamp_sec = timestamp_sec;
  data_timestamp_usec = timestamp_usec;
  
  // need to fill in the timestamps on all REB devices, both so that they
  // can read it, but also because other devices may want to read it
  player_device_id_t id = device_id;

  id.code = PLAYER_IR_CODE;
  CDevice* ir = deviceTable->GetDevice(id);
  if(ir)
  {
    ir->data_timestamp_sec = this->data_timestamp_sec;
    ir->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_POWER_CODE;
  CDevice* power = deviceTable->GetDevice(id);
  if(power)
  {
    power->data_timestamp_sec = this->data_timestamp_sec;
    power->data_timestamp_usec = this->data_timestamp_usec;
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
REB::Main()
{
  player_reb_cmd_t cmd;

  short last_trans_command=0, last_rot_command=0;
  int leftvel=0, rightvel=0;
  int leftpos=0, rightpos=0;

  //  static struct timeval pmot={0,0};
  //  struct timeval ds, de, da={0,0};
  // first get pointers to all the devices we control
  player_device_id_t id = device_id;

  id.code = PLAYER_IR_CODE;
  CDevice *ir = deviceTable->GetDevice(id);

  id.code = PLAYER_POSITION_CODE;
  CDevice *pos = deviceTable->GetDevice(id);

  id.code = PLAYER_POWER_CODE;
  CDevice *power = deviceTable->GetDevice(id);

  this->pos_subscriptions = 0;
  this->ir_subscriptions = 0;
  this->power_subscriptions = 0;

  GlobalTime->GetTime(&timeBegan_tv);

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

  while (1) {
    // we want to turn on the IR if someone just subscribed, and turn
    // them off if the last subscriber just unsubscribed.
    if(ir) {
      if(!this->ir_subscriptions && ir->subscriptions) {
	// then someone just subbed to IR
	StartIR();
	
      } else if(this->ir_subscriptions && !(ir->subscriptions)) {
	// then last person stopped sub from IR..
	StopIR();
      }
      
      this->ir_subscriptions = ir->subscriptions;
    }
    
    // we want to reset the odometry and enable the motors if the first 
    // client just subscribed to the position device, and we want to stop 
    // and disable the motors if the last client unsubscribed.
    if(pos) {
      if(!this->pos_subscriptions && pos->subscriptions) {
	printf("REB: first pos sub. turn off and reset\n");
	// then first sub for pos, so turn off motors and reset odom
	SetSpeed(REB_MOTOR_LEFT, 0);
	SetSpeed(REB_MOTOR_RIGHT, 0);
	
	SetOdometry(0,0,0);
      } else if (this->pos_subscriptions && !(pos->subscriptions)) {
	// last sub just unsubbed
	printf("REB: last pos sub gone\n");
	SetSpeed(REB_MOTOR_LEFT, 0);
	SetSpeed(REB_MOTOR_RIGHT, 0);
	
        // overwrite existing motor commands to be zero
        player_position_cmd_t position_cmd;
        position_cmd.xspeed = 0;
        position_cmd.yawspeed = 0;
	position_cmd.yaw = 0;
        pos->PutCommand((unsigned char*)(&position_cmd), 
			sizeof(position_cmd));
      }
      
      this->pos_subscriptions = pos->subscriptions;
    } 

    if (power) {
      if (!this->power_subscriptions && power->subscriptions) {
	printf("REB: POWER SUBSCRIPTION\n");
	this->power_subscriptions = power->subscriptions;
      }
    }

    // get configuration commands (ioctls)
    ReadConfig();

    /* read the clients' commands from the common buffer */
    GetCommand((unsigned char*)&cmd, sizeof(cmd));

    bool newtrans = false, newrot = false, newheading=false;
    bool newposcommand=false;
    short trans_command, rot_command, heading_command;
    
    if ((trans_command = (short)ntohs(cmd.position.xspeed)) != 
	last_trans_command) {
      newtrans = true;
      last_trans_command = trans_command;
    }
    
    if ((rot_command = (short) ntohs(cmd.position.yawspeed)) != 
	last_rot_command) {
      newrot = true;
      last_rot_command = rot_command;
    }
    
    if ((heading_command = (short) ntohs(cmd.position.yaw)) != 
	this->desired_heading) {
      newheading = true;
      this->desired_heading = heading_command;
    }
    
    if (this->pos_subscriptions) {
      if (this->velocity_mode) {
	// then we are in velocity mode
	
	if (!this->direct_velocity_control) {
	  // then we are doing my velocity based heading PD controller
	  
	  // calculate difference between desired and current
	  unsigned short current_theta = (short) ntohs(this->data->position.yaw);
	  int diff = this->desired_heading - current_theta;
	  
	  // this will make diff the shortest angle between command and current
	  if (diff > 180) {
	    diff += -360; 
	  } else if (diff < -180) {
	    diff += 360; 
	  }
	  
	  int trans_long = (int) trans_command;
	  int rot_long = (int) rot_command;

	  // lets try to do this in fixed point
	  // max angle error is 180, so get a ratio
	  int err_ratio = diff*REB_FIXED_FACTOR / 180;
	  //double err_ratio = (double) diff / 180.0;

	  // choose trans speed inverse proportional to heading error
	  trans_long = (REB_FIXED_FACTOR - ABS(err_ratio))*trans_long;

	  // try doing squared error?
	  //	  trans_long = (REB_FIXED_FACTOR - err_ratio*err_ratio)*trans_long;

	  // now divide by factor to get regular value
	  trans_long /= REB_FIXED_FACTOR;
	  	  
	  // now we have to make a rotational velocity proportional to
	  // heading error with a damping term
	  
	  // there is a gain in here that maybe should be configurable
	  rot_long = err_ratio*3*rot_long;
	  rot_long /= REB_FIXED_FACTOR;
	  //rot_command = (short) (err_ratio*2.0*(double)rot_command);
	  
	  // make sure we stay within given limits

	  trans_command = (short)trans_long;
	  rot_command = (short) rot_long;

#ifdef DEBUG_POS
	  printf("REB: PD: diff=%d err=%d des=%d curr=%d trans=%d rot=%d\n", 
		 diff, err_ratio, this->desired_heading, current_theta, 
		 trans_command, rot_command);
#endif
	  
	  if (ABS(last_trans_command) - ABS(trans_command) < 0) {
	    // then we have to clip the new desired trans to given
	    // multiply by the sign just to take care of some crazy case
	    trans_command = SGN(trans_command)* last_trans_command;
	  }
	  
	  if (ABS(last_rot_command) - ABS(rot_command) < 0) {
	    rot_command = SGN(rot_command)*last_rot_command;
	  }
	}
	
	// so now we need to figure out left and right wheel velocities
	// to achieve the given trans and rot velocitties of the ubot
	long rot_term_fixed = rot_command * 
	  PlayerUBotRobotParams[this->param_index].RobotAxleLength / 2;

	rot_term_fixed = DEG2RAD_FIX(rot_term_fixed);

	leftvel = trans_command*REB_FIXED_FACTOR - rot_term_fixed;
	rightvel = trans_command*REB_FIXED_FACTOR + rot_term_fixed;
	
	leftvel /= REB_FIXED_FACTOR;
	rightvel /= REB_FIXED_FACTOR;

	int max_trans = PlayerUBotRobotParams[this->param_index].MaxVelocity;
	
	if (ABS(leftvel) > max_trans) {
	  if (leftvel > 0) {
	    leftvel = max_trans;
	    rightvel *= max_trans/leftvel;
	  } else {
	    leftvel = -max_trans;
	    rightvel *= -max_trans/leftvel;
	  }
	  
	  fprintf(stderr, "REB: left wheel velocity clipped\n");
	}
	
	if (ABS(rightvel) > max_trans) {
	  if (rightvel > 0) {
	    rightvel = max_trans;
	    leftvel *= max_trans/rightvel;
	  } else {
	    rightvel = -max_trans;
	    leftvel *= -max_trans/rightvel;
	  }

	  fprintf(stderr, "REB: right wheel velocity clipped\n");
	}
	
	// we have to convert from mm/s to pulse/10ms
	//	double left_velocity = (double) leftvel;
	//	double right_velocity = (double) rightvel;

	// add the RFF/2 for rounding
	long lvf = leftvel * PlayerUBotRobotParams[this->param_index].PulsesPerMMMSF + 
	  (REB_FIXED_FACTOR/2);
	long rvf = -(rightvel * PlayerUBotRobotParams[this->param_index].PulsesPerMMMSF +
		     (REB_FIXED_FACTOR/2));

	lvf /= REB_FIXED_FACTOR;
	rvf /= REB_FIXED_FACTOR;
	//	printf("REB: POS: lvel=%d rvel=%d ", leftvel, rightvel);
	//	left_velocity *= PlayerUBotRobotParams[this->param_index].PulsesPerMMMS;
	//	right_velocity *= PlayerUBotRobotParams[this->param_index].PulsesPerMMMS;
	
	//	right_velocity = -right_velocity;
	
	//	leftvel = (int) rint(left_velocity);
	//	rightvel = (int) rint(right_velocity);
	leftvel = lvf;
	rightvel = rvf;
	//	printf("REB: POS: %g [%d] lv=%d [%d] rv=%d [%d]\n", PlayerUBotRobotParams[this->param_index].PulsesPerMMMS,
	//	       PlayerUBotRobotParams[this->param_index].PulsesPerMMMSF,leftvel, lvf/REB_FIXED_FACTOR, rightvel, rvf/REB_FIXED_FACTOR);
#ifdef DEBUG_POS
	printf("REB: [%sABLED] VEL %s: lv=%d rv=%d trans=%d rot=%d\n", 
	       this->motors_enabled ? "EN" : "DIS",
	       this->direct_velocity_control ?
	       "DIRECT" : "PD", leftvel, rightvel, trans_command, rot_command);
#endif
	
	// now we set the speed
	if (this->motors_enabled) {
	  SetSpeed(REB_MOTOR_LEFT, leftvel);
	  SetSpeed(REB_MOTOR_RIGHT, rightvel);
	} else {
	  SetSpeed(REB_MOTOR_LEFT, 0);
	  SetSpeed(REB_MOTOR_RIGHT, 0);
	}
      } else {
	// we are in position mode....
	// we only do a translation or a rotation
	double lp, rp;

	// set this to false so that we catch the first on_target
	//	this->pos_mode_odom_update = false;

	newposcommand = false;
	// this will skip translation if command is 0
	// or if no new command
	if (newtrans) {
	  // then the command is a translation in mm
	  lp = (double) trans_command;
	  lp *= PlayerUBotRobotParams[this->param_index].PulsesPerMM;
	  leftpos = (int)rint(lp);

	  rp = (double) trans_command;
	  rp *= PlayerUBotRobotParams[this->param_index].PulsesPerMM;
	  rightpos = (int) rint(rp);

	  newposcommand = true;
	} else if (newrot) {
	  // then new rotation instead
	  // this rot command is in degrees
	  lp = -DEG2RAD((double)rot_command)*
	    PlayerUBotRobotParams[this->param_index].RobotAxleLength/2.0 *
	    PlayerUBotRobotParams[this->param_index].PulsesPerMM;
	  rp = -lp;
	  
	  leftpos = (int) rint(lp);
	  rightpos = (int) rint(rp);

	  newposcommand = true;
	}

#ifdef DEBUG_POS	
	printf("REB: [%sABLED] POSITION leftpos=%d rightpos=%d\n",
	       this->motors_enabled ? "EN" : "DIS", leftpos, rightpos);
#endif

	// now leftpos and rightpos are the right positions to reach
	// reset the counters first???? FIX  
	// we have to return the position command status now FIX
	if (this->motors_enabled && newposcommand) {
	  printf("REB: SENDING POS COMMAND\n");
	  // we need to reset counters to 0 for odometry to work
	  SetPosCounter(REB_MOTOR_LEFT, 0);
	  SetPosCounter(REB_MOTOR_RIGHT, 0);
	  SetPos(REB_MOTOR_LEFT, leftpos);
	  SetPos(REB_MOTOR_RIGHT, -rightpos);
	}
      }
    }
    
    // now lets get new data...
    UpdateData();

    pthread_testcancel();
    
  }
  pthread_exit(NULL);
}

/* start a thread that will invoke Main() */
void 
REB::StartThread()
{
  pthread_create(&thread, NULL, &DummyMain, this);
}

/* cancel (and wait for termination) of the thread */
void 
REB::StopThread()
{
  void* dummy;
  pthread_cancel(thread);
  if(pthread_join(thread,&dummy))
    perror("REB::StopThread:pthread_join()");
}

/* this will read a new config command and interpret it
 *
 * returns: 
 */
void
REB::ReadConfig()
{
  int config_size;
  unsigned char config_buffer[REB_CONFIG_BUFFER_SIZE];
  player_device_id_t id;
  void *client;

  if ((config_size = GetConfig(&id, &client, 
			       (void *)config_buffer, sizeof(config_buffer)))) {
    
    // figure out which device it's for
    switch(id.code) {
      
      // REB_IR IOCTLS /////////////////
    case PLAYER_IR_CODE:
      
#ifdef DEBUG_CONFIG
      printf("REB: IR CONFIG\n");
#endif

      // figure out which command
      switch(config_buffer[0]) {
      case PLAYER_IR_POWER_REQ: {
	// request to change IR state
	// 1 means turn on
	// 0 is off
	if (config_size != sizeof(player_ir_power_req_t)) {
	  fprintf(stderr, "REB: argument to IR power req wrong size (%d)\n", config_size);
	  if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	    PLAYER_ERROR("REB: failed to reply");
	    break;
	  }
	}

	player_ir_power_req_t *powreq = (player_ir_power_req_t *)config_buffer;	
#ifdef DEBUG_CONFIG
	printf("REB: IR_POWER_REQ: %d\n", powreq->state);
#endif

	if (powreq->state) {
	  StartIR();
	} else {
	  StopIR();
	}
	
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) {
	  PLAYER_ERROR("REB: failed to reply");
	}
      }
      break;
      
      case PLAYER_IR_POSE_REQ: {
	// request the pose of the IR sensors in robot-centric coords
	if (config_size != sizeof(player_ir_pose_req_t)) {
	  fprintf(stderr, "REB: argument to IR pose req wrong size (%d)\n", config_size);
	  if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	    PLAYER_ERROR("REB: failed to put reply");
	    break;
	  }
	}
	
#ifdef DEBUG_CONFIG
	printf("REB: IR_POSE_REQ\n");
#endif

	player_ir_pose_t irpose;
	for (int i =0; i < PLAYER_IR_MAX_SAMPLES; i++) {
	  ir_pose_t irp = PlayerUBotRobotParams[param_index].ir_pose[i];
	  
	  irpose.poses[i][0] = htons((short) irp.ir_x);
	  irpose.poses[i][1] = htons((short) irp.ir_y);
	  irpose.poses[i][2] = htons((short) irp.ir_theta);
	}
	
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &irpose,
		     sizeof(irpose))) {
	  PLAYER_ERROR("REB: failed to put reply");
	  break;
	}
      }
      break;

      default:
	fprintf(stderr, "REB: IR got unknown config\n");
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	  PLAYER_ERROR("REB: failed to put reply");
	}
	break;
      }
      break;
      
      // END REB_IR IOCTLS //////////////

      // POSITION IOCTLS ////////////////
    case PLAYER_POSITION_CODE:
#ifdef DEBUG_CONFIG
      printf("REB: POSITION CONFIG\n");
#endif
      switch (config_buffer[0]) {
      case PLAYER_POSITION_GET_GEOM_REQ: {
	// get geometry of robot
	if (config_size != sizeof(player_position_geom_t)) {
	  fprintf(stderr, "REB: get geom req is wrong size (%d)\n", config_size);
	  PLAYER_ERROR("REB: failed to put reply");
	  break;
	}

#ifdef DEBUG_CONFIG
	printf("REB: POSITION_GET_GEOM_REQ\n");
#endif

	player_position_geom_t geom;
	geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
	geom.pose[0] = htons(0);
	geom.pose[1] = htons(0);
	geom.pose[2] = htons(0);
	geom.size[0] = geom.size[1] = 
	  htons( (short) (2 * PlayerUBotRobotParams[this->param_index].RobotRadius));
	
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom,
		     sizeof(geom))) {
	  PLAYER_ERROR("REB: failed to put reply");
	}
      }
      break;

      case PLAYER_POSITION_MOTOR_POWER_REQ: {
	// change motor state
	// 1 for on 
	// 0 for off

	if (config_size != sizeof(player_position_power_config_t)) {
	  fprintf(stderr, "REB: pos motor power req got wrong size (%d)\n", config_size);
	  if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	    PLAYER_ERROR("REB: failed to put reply");
	    break;
	  }
	}

	player_position_power_config_t *mpowreq = (player_position_power_config_t *)config_buffer;
 
#ifdef DEBUG_CONFIG
	printf("REB: MOTOR_POWER_REQ %d\n", mpowreq->value);
#endif
	
	if (mpowreq->value) {
	  this->motors_enabled = true;
	} else {
	  this->motors_enabled = false;
	}
	
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) {
	  PLAYER_ERROR("REB: failed to put reply");
	}

	printf("REB: put MOTOR POWER REQ\n");
      }
      break;

      case PLAYER_POSITION_VELOCITY_MODE_REQ: {
	// select method of velocity control
	// 0 for direct velocity control (trans and rot applied directly)
	// 1 for builtin velocity based heading PD controller
	if (config_size != sizeof(player_position_velocitymode_config_t)) {
	  fprintf(stderr, "REB: pos vel control req got wrong size (%d)\n", config_size);
	  if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	    PLAYER_ERROR("REB: failed to put reply");
	    break;
	  }
	}

	player_position_velocitymode_config_t *velcont = 
	  (player_position_velocitymode_config_t *) config_buffer;

#ifdef DEBUG_CONFIG
	printf("REB: VELOCITY_MODE_REQ %d\n", velcont->value);
#endif
	
	if (!velcont->value) {
	  this->direct_velocity_control = true;
	} else {
	  this->direct_velocity_control = false;
	}

	// also set up not to use position mode!
	this->velocity_mode = true;
	this->pos_update_period = REB_POS_UPDATE_PERIOD_VEL;
	
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) {
	  PLAYER_ERROR("REB: failed to put reply");
	}
      }
      break;

      case PLAYER_POSITION_RESET_ODOM_REQ: {
	// reset the odometry
	if (config_size != sizeof(player_position_resetodom_config_t)) {
	  fprintf(stderr, "REB: pos reset odom req got wrong size (%d)\n", config_size);
	  if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	    PLAYER_ERROR("REB: failed to put reply");
	    break;
	  }
	}

#ifdef DEBUG_CONFIG
	printf("REB: RESET_ODOM_REQ\n");
#endif
	
	SetOdometry(0,0,0);
	
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) {
	  PLAYER_ERROR("REB: failed to put reply");
	}
      }
      break;
      // END POSITION IOCTLS ////////////

      case PLAYER_POSITION_POSITION_MODE_REQ: {
	// select velocity or position mode
	// 0 for velocity mode
	// 1 for position mode
	if (config_size != sizeof(player_position_position_mode_req_t)) {
	  fprintf(stderr, "REB: pos vel mode req got wrong size (%d)\n", config_size);
	  if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	    PLAYER_ERROR("REB: failed to put reply");
	    break;
	  }
	}

	player_position_position_mode_req_t *posmode = 
                (player_position_position_mode_req_t *)config_buffer;
#ifdef DEBUG_CONFIG
	printf("REB: POSITION_MODE_REQ %d\n", posmode->state);
#endif
	
	if (posmode->state) {
	  this->velocity_mode = false;
	  this->pos_update_period = REB_POS_UPDATE_PERIOD_POS;
	} else {
	  this->velocity_mode = true;
	  this->pos_update_period = REB_POS_UPDATE_PERIOD_VEL;
	}
	
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) {
	  PLAYER_ERROR("REB: failed to put reply");
	}
      }
      break;
      
      case PLAYER_POSITION_SET_ODOM_REQ: {
	// set the odometry to a given position
	if (config_size != sizeof(player_position_set_odom_req_t)) {
	  fprintf(stderr, "REB: pos set odom req got wrong size (%d)\n",
		  config_size);
	  if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	    PLAYER_ERROR("REB: failed to put reply");
	    break;
	  }
	}

	player_position_set_odom_req_t *req = 
                (player_position_set_odom_req_t *)config_buffer;

#ifdef DEBUG_CONFIG
	int x,y;
	short theta;
	x = ntohl(req->x);
	y = ntohl(req->y);
	theta = ntohs(req->theta);

	printf("REB: SET_ODOM_REQ x=%d y=%d theta=%d\n", x, y, theta);
#endif
	SetOdometry(req->x, req->y, req->theta);

	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) {
	  PLAYER_ERROR("REB: failed to put reply");
	}
      }
      break;

      case PLAYER_POSITION_SPEED_PID_REQ: {
	// set up the velocity PID on the REB
	// kp, ki, kd are used
	if (config_size != sizeof(player_position_speed_pid_req_t)) {
	  fprintf(stderr, "REB: pos speed PID req got wrong size (%d)\n", config_size);
	  if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	    PLAYER_ERROR("REB: failed to put reply");
	    break;
	  }
	}

	player_position_speed_pid_req_t *pid = 
                (player_position_speed_pid_req_t *)config_buffer;
	int kp = ntohl(pid->kp);
	int ki = ntohl(pid->ki);
	int kd = ntohl(pid->kd);

#ifdef DEBUG_CONFIG
	printf("REB: SPEED_PID_REQ kp=%d ki=%d kd=%d\n", kp, ki, kd);
#endif
	
	ConfigSpeedPID(REB_MOTOR_LEFT, kp, ki, kd);
	ConfigSpeedPID(REB_MOTOR_RIGHT, kp, ki, kd);
	
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) {
	  PLAYER_ERROR("REB: failed to put reply");
	}
      }
      break;
      
      case PLAYER_POSITION_POSITION_PID_REQ: {
	// set up the position PID on the REB
	// kp, ki, kd are used
	if (config_size != sizeof(player_position_position_pid_req_t)) {
	  fprintf(stderr, "REB: pos pos PID req got wrong size (%d)\n", config_size);
	  if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	    PLAYER_ERROR("REB: failed to put reply");
	    break;
	  }
	}
	
	player_position_position_pid_req_t *pid = 
                (player_position_position_pid_req_t *)config_buffer;
	int kp = ntohl(pid->kp);
	int ki = ntohl(pid->ki);
	int kd = ntohl(pid->kd);

#ifdef DEBUG_CONFIG
	printf("REB: POS_PID_REQ kp=%d ki=%d kd=%d\n", kp, ki, kd);
#endif
	
	ConfigPosPID(REB_MOTOR_LEFT, kp, ki, kd);
	ConfigPosPID(REB_MOTOR_RIGHT, kp, ki, kd);
	
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) {
	  PLAYER_ERROR("REB: failed to put reply");
	}
      }
      break;
      
      case PLAYER_POSITION_SPEED_PROF_REQ: {
	// set the speed profile for position mode 
	// speed is max speed
	// acc is max acceleration
	if (config_size != sizeof(player_position_speed_prof_req_t)) {
	  fprintf(stderr, "REB: pos speed prof req got wrong size (%d)\n", config_size);
	  if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0)) {
	    PLAYER_ERROR("REB: failed to put reply");
	    break;
	  }
	}

	player_position_speed_prof_req_t *prof = 
                (player_position_speed_prof_req_t *)config_buffer;	
	int spd = ntohs(prof->speed);
	int acc = ntohs(prof->acc);

#ifdef DEBUG_CONFIG	
	printf("REB: SPEED_PROF_REQ: spd=%d acc=%d  spdu=%g accu=%g\n", spd, acc,
	       spd*PlayerUBotRobotParams[this->param_index].PulsesPerMMMS,
	       acc*PlayerUBotRobotParams[this->param_index].PulsesPerMMMS);
#endif
	// have to convert spd from mm/s to pulse/10ms
	spd = (int) rint((double)spd * 
			 PlayerUBotRobotParams[this->param_index].PulsesPerMMMS);
	
	// have to convert acc from mm/s^2 to pulses/256/(10ms^2)
	acc = (int) rint((double)acc * 
			 PlayerUBotRobotParams[this->param_index].PulsesPerMMMS);
	// now have to turn into pulse/256
	//	acc *= 256;
	
	if (acc > REB_MAX_ACC) {
	  acc = REB_MAX_ACC;
	} else if (acc == 0) {
	  acc = REB_MIN_ACC;
	}
	
#ifdef DEBUG_CONFIG
	printf("REB: SPEED_PROF_REQ: SPD=%d  ACC=%d\n", spd, acc);
	ConfigSpeedProfile(REB_MOTOR_LEFT, spd, acc);
	ConfigSpeedProfile(REB_MOTOR_RIGHT, spd, acc);
#endif
	if (PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0)) {
	  PLAYER_ERROR("REB: failed to put reply");
	}
      }
      break;
      
      default:
	fprintf(stderr, "REB: got unknown position config command\n");
	break;
      }
      break;
      // END REB_POSITION IOCTLS ////////////
      
    default:
      printf("REB: unknown config code %d\n", id.code);
    }
  }
}


/* this will update the data that is sent to clients
 * just call separate functions to take care of it
 *
 * returns:
 */
void
REB::UpdateData()
{
  player_reb_data_t d;
    
  struct timeval curr;
  int timems;
  GlobalTime->GetTime(&curr);

  Lock();
  memcpy(&d, this->data, sizeof(player_reb_data_t));
  Unlock();

  // get time since last ir update in ms
  timems = (curr.tv_sec - last_ir_update.tv_sec)*1000 +
    (curr.tv_usec - last_ir_update.tv_usec)/1000;
  
  // we dont want to update IR during position mode moves
  // because it uses a lot of b/w on the serial port... FIX
  if (this->ir_subscriptions && timems >= REB_IR_UPDATE_PERIOD &&
      this->velocity_mode) {
    Lock();
    UpdateIRData(&d);
    Unlock();
    
    last_ir_update = curr;
  }

  timems = (curr.tv_sec - last_power_update.tv_sec)*1000 +
    (curr.tv_usec - last_power_update.tv_usec)/1000;

  if (this->power_subscriptions && timems >= REB_POWER_UPDATE_PERIOD) {
    Lock();
    UpdatePowerData(&d);
    Unlock();
    last_power_update = curr;
  }

  timems = (curr.tv_sec - last_pos_update.tv_sec)*1000 +
    (curr.tv_usec - last_pos_update.tv_usec)/1000;

  if (this->pos_subscriptions && timems >= this->pos_update_period) {
    Lock();
    UpdatePosData(&d);
    Unlock();

    last_pos_update = curr;
  }
  
  PutData((unsigned char *)&d, sizeof(d), 0 ,0);
}

/* this will update the IR part of the client data
 * it entails reading the currently active IR sensors
 * and then changing their state to off and turning on
 * 2 new IRs.  
 * NOTE: assumes calling function already called Lock()
 *
 * returns:
 */
void
REB::UpdateIRData(player_reb_data_t * d)
{
  // then we can take a reading
  uint16_t volts[4];
  int which;
  struct timeval curr;

  GlobalTime->GetTime(&curr);
  
  if ( (curr.tv_sec - this->last_ir.tv_sec)*1000 + 
       (curr.tv_usec - this->last_ir.tv_usec)/1000 >= 
       REB_IR_UPDATE_PERIOD) {

    // we only use 4 IRs at a time.  so collect their
    // data, then turn them off and turn the others on
    for (int i =0; i < 4; i++) {
      which = this->ir_sequence + 2*i;
      // these are in units of 4 mV
      volts[i] = ReadAD(which);
      
      // now turn into mV units
      volts[i] *= 4;
      d->ir.voltages[which] = htons(volts[i]);
      
      // now turn this IR off
      ConfigAD(which, REB_AD_OFF);
    }
    
    // now get two new IRs
    this->ir_sequence++;
    this->ir_sequence %= 2;
    
    for (int i =0; i < 4; i++) {
      ConfigAD(this->ir_sequence + 2*i, REB_AD_ON);
    }

    last_ir =curr;
  }    
}

/* this will update the POWER data.. basically just the battery level for now
 * NOTE: assumes calling function already called Lock()
 * returns:
 */
void
REB::UpdatePowerData(player_reb_data_t *d)
{
  // read voltage 
  uint16_t volt = (uint16_t)ReadAD(REB_BATTERY_CHANNEL);
  
  // this is in units of 20mV.. change to mV
  volt *= 20;
  d->power.charge = htons(volt);
}
  
/* this will update the position data.  this entails odometry, etc
 * assumes caller already called Lock()
 * It is in the midst of being converted from floating to fixed point...
 * returns:
 */ 
void
REB::UpdatePosData(player_reb_data_t *d)
{
  // change to fixed point FIX
  double x=0, y=0, theta;
  long x_f, y_f;
  unsigned char target_status =0;
  int lreading=0, rreading=0;
  long mmpp_f = PlayerUBotRobotParams[this->param_index].MMPerPulsesF;

  // check if we have to get a baseline time first
  if (this->refresh_last_position) {
    GlobalTime->GetTime(&(this->last_position));
    this->refresh_last_position = false;
  }

  // get the previous odometry values
  // we know this is from last time, cuz this function
  // is only place to change them
  theta = (double) ((int)ntohs(this->data->position.yaw));

  //covert theta to rad
  theta = DEG2RAD(theta);

  x_f = ntohl(this->data->position.xpos)*REB_FIXED_FACTOR;
  y_f = ntohl(this->data->position.ypos)*REB_FIXED_FACTOR;

  // get the time
  struct timeval curr;
  GlobalTime->GetTime(&curr);

  double v, theta_dot;
  long v_f=0;

  if (this->velocity_mode) {
    int lvel=0,rvel=0;
    lvel = ReadSpeed(REB_MOTOR_LEFT);
    // negate because motor's are facing opposite directions
    rvel = -ReadSpeed(REB_MOTOR_RIGHT); 

    lreading = lvel;
    rreading = rvel;

    // calc time in  sec
    long t_f = (curr.tv_sec - this->last_position.tv_sec)*100 +
      (curr.tv_usec - this->last_position.tv_usec)/10000;

    // this is pulse/10ms
    v_f = (rvel+lvel)/2;
    v_f *= REB_FIXED_FACTOR;

    // rad/pulse
    theta_dot = (rvel- lvel) /
      (PlayerUBotRobotParams[this->param_index].RobotAxleLength *
       PlayerUBotRobotParams[this->param_index].PulsesPerMM);

    theta += theta_dot *t_f;

    // convert from rad/10ms -> rad/s -> deg/s
    theta_dot = theta_dot * 100.0;
    
    // this is pulse/10ms
    long x_dot_f = (long) (v_f * cos(theta));
    long y_dot_f = (long) (v_f * sin(theta));

    // change to deltas mm and add integrate over time
    x_f += (x_dot_f/REB_FIXED_FACTOR) * mmpp_f * t_f;
    y_f += (y_dot_f/REB_FIXED_FACTOR) * mmpp_f * t_f;
    
    x_f /= REB_FIXED_FACTOR;
    y_f /= REB_FIXED_FACTOR;
  } else {
    // in position mode
    int rpos=0, lpos=0;
    uint8_t rtar;
    int mode, error;
    
    v = theta_dot= 0.0;

    // now we read the status of the motion controller
    // DONT ASK ME -- but calling ReadStatus on the LEFT
    // motor seems to cause the REB (the kameleon itself!)
    // to freeze some time after issuing a position mode
    // command -- happens for RIGHT motor too but
    // maybe not as much???
    //  ltar = ReadStatus(REB_MOTOR_LEFT, &mode, &error);
    
    rtar = ReadStatus(REB_MOTOR_RIGHT, &mode, &error);

    target_status = rtar;
    // check for on target so we know to update
    if (!d->position.stall && target_status) {
      // then this is a new one, so do an update

      lpos = ReadPos(REB_MOTOR_LEFT);
      rpos = -ReadPos(REB_MOTOR_RIGHT);

      lreading = lpos;
      rreading = rpos;
      double p;

      // take average pos
      p = (lpos + rpos) /2.0;
      
      // now convert to mm
      p *= PlayerUBotRobotParams[this->param_index].MMPerPulses;
      
      // this should be change in theta in rad
      theta_dot = (rpos - lpos) *
	PlayerUBotRobotParams[this->param_index].MMPerPulses / 
	PlayerUBotRobotParams[this->param_index].RobotAxleLength;
      
      // update our theta
      theta += theta_dot;
      
      // update x & y positions
      x += p * cos(theta);
      y += p * sin(theta);

      x_f = (long) rint(x);
      y_f = (long) rint(y);

      printf("REB: pos mode x=%g y=%g theta=%g\n", x,y, theta);
    } 

  }

  // get integer rounded x,y and theta
  int rx = x_f;
  int ry = y_f;

  int rtheta = (int) rint(RAD2DEG(theta));
  
  // get int rounded angular velocity
  int rtd = (int) rint(RAD2DEG(theta_dot));

  // get int rounded trans velocity (in convert from pulses/10ms -> mm/s)
  
  // need to add the RFF/2 for rounding
  long rv  = (v_f/REB_FIXED_FACTOR) *100 * mmpp_f + (REB_FIXED_FACTOR/2);
  rv/= REB_FIXED_FACTOR;

  // normalize theta
  rtheta %= 360;

  // now make theta positive
  if (rtheta < 0) {
    rtheta += 360;
  }

#ifdef DEBUG_POS
  printf("REB: l%s=%d r%s=%d x=%d y=%d theta=%d trans=%d rot=%d target=%02x\n",
	 this->velocity_mode ? "vel" : "pos", lreading,
	 this->velocity_mode ? "vel" : "pos", rreading, 
	 rx, ry, rtheta,  rv, rtd, target_status);
#endif

  // now write data
  d->position.xpos = htonl(rx);
  d->position.ypos = htonl(ry);
  d->position.yaw = htons( rtheta);
  d->position.xspeed = htons(rv);
  d->position.yawspeed = htons( rtd);
  //  d->position.heading = htons(this->desired_heading);
  d->position.stall = target_status;

  // later we read the torques FIX

  // update last time
  this->last_position = curr;
}
  

/* this will start the IR reading sequence
 *
 * returns:
 */
void
REB::StartIR()
{
  // now lets start the IR sequence..
  // turn on evens
  this->ir_sequence = 0;
  for (int i =0; i < PLAYER_IR_MAX_SAMPLES; i++) {
    if (!i%2) {
      ConfigAD(i,REB_AD_ON);
    } else {
      ConfigAD(i, REB_AD_OFF);
    }
  }
  
  // record last IR reading
  GlobalTime->GetTime(&this->last_ir);
}

/* this will stop the sequence and turn off IRs
 *
 * returns:
 */
void
REB::StopIR()
{
  printf("REB: StopIR\n");
  for (int i =0; i < PLAYER_IR_MAX_SAMPLES; i++) {
    ConfigAD(i, REB_AD_OFF);
  }
  
  this->ir_sequence = -1;
}

/* this will reset odometry -- the counters on the encoders and 
 * the integrated position estimates
 *
 * returns:
 */
/*void
REB::ResetOdometry()
{

    SetPosCounter(REB_MOTOR_LEFT, 0);
  SetPosCounter(REB_MOTOR_RIGHT, 0);

  this->data->position.x = 0;
  this->data->position.y = 0;
  this->data->position.theta = 0;
  
}
*/

/* this will set the odometry to a given position
 * ****NOTE: assumes that the arguments are in network byte order!*****
 *
 * returns: 
 */
void
REB::SetOdometry(int x, int y, short theta)
{

  SetPosCounter(REB_MOTOR_LEFT, 0);
  SetPosCounter(REB_MOTOR_RIGHT, 0);

  // we assume these are already in network byte order!!!!
  this->data->position.xpos = x;
  this->data->position.ypos = y;
  this->data->position.yaw = theta;

}

/* this will write len bytes from buf out to the serial port reb_fd 
 *   
 *  returns: the number of bytes written, -1 on error
 */
int
REB::write_serial(char *buf, int len)
{
  int num, t,i;

#ifdef DEBUG_SERIAL
  printf("WRITE: len=%d: ", len);
  for (i =0; i < len; i++) {
    if (isspace(buf[i])) {
      if (buf[i] == ' ') {
	printf("%c", ' ');
      } else {
	printf("'%02x'", buf[i]);
      }
    } else {
      printf("%c", buf[i]);
    }
  }
  printf("\n");
#endif
  
  num = 0;
  while (num < len) {
    t = write(this->reb_fd, buf+num, len-num);
    if (t < 0) {
      switch(errno) {
      case EBADF:
	fprintf(stderr, "bad file descriptor!!\n");
	break;
      }

      fprintf(stderr, "error writing\n");
      for (i = 0; i < len; i++) {
	fprintf(stderr, "%c (%02x) ", isprint(buf[i]) ? buf[i] : ' ',
	       buf[i]);
      }
      fprintf(stderr, "\n");
      return -1;
    }
    
#ifdef _DEBUG
    printf("WRITES: wrote %d of %d\n", t, len);
#endif
    
    num += t;
  }
  return len;
}

/* this will read upto len bytes from reb_fd or until it reads the
   flag string given by flag (length is flen)

   returns: number of bytes read
*/
int
REB::read_serial_until(char *buf, int len, char *flag, int flen)
{
  int num=0,t;
  
#ifdef DEBUG_SERIAL
  printf("RSU before while flag len=%d len=%d\n", flen, len);
#endif
  
  while (num < len-1) {
    //    t = read(s, buf+num, len-num);
    t = read(this->reb_fd, buf+num, 1);

#ifdef DEBUG_SERIAL
    printf("RSU: %c (%02x)\n", isspace(buf[num]) ? ' ' : buf[num], buf[num]);
#endif
    
    if (t < 0) {
      fprintf(stderr, "error!!!\n");
      switch(errno) {
      case EAGAIN:
	t = 0;
	break;
      case EINTR:
	return -1;
      default:
	return -1;
      }
    }
    
    num ++;
    buf[num] = '\0';
    if (num >= flen) {
      
      if (!strcmp(buf+num-flen, flag)) {
	return 0;
      }
    }
    
    if (num>= 2) {
      buf[num] = '\0';
      if (strcmp(buf+num-2, "\r\n") == 0) {
	num =0;
#ifdef DEBUG_SERIAL
	printf("RSU: MATCHED CRLF\n");
#endif
      }
    }
  }
  
  buf[num] = '\0';
  return num;
}


/* this will take the given buffer, which should have a command in it,
 * and write it to the serial port, then read a response back into the
 * buffer
 *
 * returns: number of bytes read
 */
int
REB::write_command(char *buf, int len, int maxsize)
{
  static int total=0;
  int ret;
 
  total += write_serial(buf, len);
  
  //usleep(200);
  usleep(0);
  ret = read_serial_until(buf, maxsize, CRLF, strlen(CRLF));
  
  total += ret;

  return ret;
}

/* this will configure the AD channel given.
 *  0 == channel OFF
 *  1 == channel ON
 *  2 == toggle channel state
 */
void
REB::ConfigAD(int channel, int action)
{
  char buf[64];

  sprintf(buf, "Q,%d,%c\r", channel, '0'+action);

  write_command(buf, strlen(buf), sizeof(buf));
}

/* this will read the given AD channel
 *
 * returns: the value of the AD channel
 */
unsigned short
REB::ReadAD(int channel) 
{
  char buf[64];

  sprintf(buf, "I,%d\r", channel);
  write_command(buf, strlen(buf), sizeof(buf));

  return atoi(&buf[2]);
}

/* this will set the desired speed for the given motor mn
 *
 * returns:
 */
void
REB::SetSpeed(int mn, int speed)
{
  char buf[64];
  
  sprintf(buf, "D,%c,%d\r", '0'+mn, speed);

  write_command(buf, strlen(buf), sizeof(buf));
}

/* reads the current speed of motor mn
 *
 * returns: the speed of mn
 */
int
REB::ReadSpeed(int mn)
{
  char buf[64];
  
  sprintf(buf, "E,%c\r", '0'+mn);

  write_command(buf, strlen(buf), sizeof(buf));

  return atoi(&buf[2]);
}

/* this sets the desired position motor mn should go to
 *
 * returns:
 */
void
REB::SetPos(int mn, int pos)
{
  char buf[64];
  
  sprintf(buf,"C,%c,%d\r", '0'+mn,pos);

  write_command(buf, strlen(buf), sizeof(buf));
}

/* this sets the position counter of motor mn to the given value
 *
 * returns:
 */
void
REB::SetPosCounter(int mn, int pos)
{
  char buf[64];
 
  sprintf(buf,"G,%c,%d\r", '0'+mn,pos);
  write_command(buf, strlen(buf), sizeof(buf));
}

/* this will read the current value of the position counter
 * for motor mn
 *
 * returns: the current position for mn
 */
int
REB::ReadPos(int mn)
{
  char buf[64];
  
  sprintf(buf, "H,%c\r", '0'+mn);
  write_command(buf, strlen(buf), sizeof(buf));
  
  return atoi(&buf[2]);
}

/* this will configure the position PID for motor mn
 * using paramters Kp, Ki, and Kd
 *
 * returns:
 */
void
REB::ConfigPosPID(int mn, int kp, int ki, int kd)
{ 
  char buf[64];

  sprintf(buf, "F,%c,%d,%d,%d\r", '0'+mn, kp,ki,kd);
  write_command(buf, strlen(buf), sizeof(buf));
}

/* this will configure the speed PID for motor mn
 *
 * returns:
 */
void
REB::ConfigSpeedPID(int mn, int kp, int ki, int kd)
{
  char buf[64];
  
  sprintf(buf, "A,%c,%d,%d,%d\r", '0'+mn, kp,ki,kd);
  
  write_command(buf, strlen(buf), sizeof(buf));
}

/* this will set the speed profile for motor mn
 * it takes the max velocity and acceleration
 *
 * returns:
 */
void
REB::ConfigSpeedProfile(int mn, int speed, int acc)
{
  char buf[64];
  
  sprintf(buf, "J,%c,%d,%d\r", '0'+mn, speed,acc);
  write_command(buf, strlen(buf), sizeof(buf));
}

/* this will read the status of the motion controller.
 * mode is set to 1 if in position mode, 0 if velocity mode
 * error is set to the position/speed error
 *
 * returns: target status: 1 is on target, 0 is not on target
 */
unsigned char
REB::ReadStatus(int mn, int *mode, int *error)
{
  char buf[64];

  sprintf(buf, "K,%c\r", '0'+mn);
  write_command(buf, strlen(buf), sizeof(buf));

  // buf now has the form "k,target,mode,error"
  int target;

  sscanf(buf, "k,%d,%d,%d", &target, mode, error);

  return (unsigned char)target;
}
