/*  Player - One Hell of a Robot Server
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
 *   This file modified from the original P2OS device to communicate directly
 *   with the RFLEX on an RWI robot - specifically all testing was done on
 *   an ATRV-Junior, although except for encoder conversion factors, 
 *   everything else should be the same.
 *
 *   the RFLEX device.  it's the parent device for all the RFLEX 'sub-devices',
 *   like position, sonar, etc.  there's a thread here that
 *   actually interacts with RFLEX via the serial line.  the other
 *   "devices" communicate with this thread by putting into and getting
 *   data out of shared buffers.
 */

/* notes:
 * the Main thread continues running when no-one is connected
 * this we retain our odometry data, whether anyone is connected or not
 */

/* Modified by Toby Collett, University of Auckland 2003-02-25
 * Added support for bump sensors for RWI b21r robot, uses DIO
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

#include "rflex.h"
#include "rflex_configs.h"

//rflex communications stuff
#include "rflex_commands.h"
#include "rflex-io.h"

#include <playertime.h>
extern PlayerTime* GlobalTime;

// so we can access the deviceTable and extract pointers to the sonar
// and position objects
#include <devicetable.h>
extern CDeviceTable* deviceTable;
extern int global_playerport; // used to get at devices

/* these are necessary to make the static fields visible to the linker */
//odometry in mm/rad
extern double            RFLEX::mm_odo_x;
extern double            RFLEX::mm_odo_y;
extern double            RFLEX::rad_odo_theta;
//spun off thread
extern pthread_t         RFLEX::thread;
extern struct timeval    RFLEX::timeBegan_tv;
extern int               RFLEX::rflex_fd; 
extern char              RFLEX::rflex_serial_port[];
extern bool              RFLEX::initdone;
extern pthread_mutex_t   RFLEX::rflex_accessMutex;
extern pthread_mutex_t   RFLEX::rflex_setupMutex;
extern int               RFLEX::rflex_subscriptions;
extern player_rflex_data_t*  RFLEX::data;
extern player_rflex_cmd_t*   RFLEX::command;
extern unsigned char*    RFLEX::reqqueue;
extern unsigned char*    RFLEX::repqueue;
extern int               RFLEX::joy_control;

CDevice *		RFLEX::PositionDev = NULL;
CDevice *		RFLEX::SonarDev = NULL;
CDevice *		RFLEX::IrDev = NULL;
CDevice *		RFLEX::BumperDev = NULL;
CDevice *		RFLEX::PowerDev = NULL;
CDevice *		RFLEX::AIODev = NULL;
CDevice *		RFLEX::DIODev = NULL;

//NOTE - this is accessed as an extern variable by the other RFLEX objects
rflex_config_t rflex_configs;

RFLEX::RFLEX(char* interface, ConfigFile* cf, int section)
{
  int reqqueuelen = 1;
  int repqueuelen = 1;

  if(!initdone)
  {  
    //only occurs on first instantiation (this is constructor isinherited)

    //just sets stuff to zero
    set_config_defaults();
	
	// joystick override
	joy_control = 0;

    //get serial port: everyone needs it (and we dont' want them fighting)
    strncpy(rflex_configs.serial_port,
          cf->ReadString(section, "rflex_serial_port", rflex_configs.serial_port),
          sizeof(rflex_configs.serial_port));

    rflex_fd = -1;
  
    data = new player_rflex_data_t;
    command = new player_rflex_cmd_t;

    reqqueue = (unsigned char*)(new playerqueue_elt_t[reqqueuelen]);
    repqueue = (unsigned char*)(new playerqueue_elt_t[repqueuelen]);

    SetupBuffers((unsigned char*)data, sizeof(player_rflex_data_t),
                 (unsigned char*)command, sizeof(player_rflex_cmd_t),
                 reqqueue, reqqueuelen,
                 repqueue, repqueuelen);

    ((player_rflex_cmd_t*)device_command)->position.xspeed = 0;
    ((player_rflex_cmd_t*)device_command)->position.yawspeed = 0;

    rflex_subscriptions = 0;

    pthread_mutex_init(&rflex_accessMutex,NULL);
    pthread_mutex_init(&rflex_setupMutex,NULL);

    initdone = true; 
  }
  else
  {
    // every sub-device gets its own queue object (but they all point to the
    // same chunk of memory)
    
    // every sub-device needs to get its various pointers set up
    SetupBuffers((unsigned char*)data, sizeof(player_rflex_data_t),
                 (unsigned char*)command, sizeof(player_rflex_cmd_t),
                 reqqueue, reqqueuelen,
                 repqueue, repqueuelen);
  }
  
  // zero the subscription counter.
  subscriptions = 0;
  //start RFLEX thread - this differs from the player standard
  //as we do not reinitialize anything if all clients unsubscibe
}

RFLEX::~RFLEX()
{
  Shutdown();
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
  }
  if(command)
  {
    delete command;
    command = NULL;
  }
}

void RFLEX::Lock()
{
  pthread_mutex_lock(&rflex_accessMutex);
}
void RFLEX::Unlock()
{
  pthread_mutex_unlock(&rflex_accessMutex);
}

int RFLEX::Setup(){
  /* now spawn reading thread */
  StartThread();
  return(0);
}

int RFLEX::Shutdown()
{
  if(rflex_fd == -1)
  {
    return 0;
  }
  StopThread();

  //make sure it doesn't go anywhere
  rflex_stop_robot(rflex_fd,(int) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration));
  //kill that infernal clicking
  rflex_sonars_off(rflex_fd);

  return 0;
}

int RFLEX::Subscribe(void *client)
{
  static int x=0;
  if(x==0){
    Setup();
    x=1;
  }
  rflex_subscriptions++;  // increment the static rflex-wide subscr counter
  subscriptions++;       // increment the per-device subscr counter
  return 0;
}

int RFLEX::Unsubscribe(void *client)
{
  if(rflex_subscriptions == 0) 
  {  
    //shouldn't happen - but why not stop the 'bot just in case
    rflex_stop_robot(rflex_fd,(int) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration));
    rflex_brake_on(rflex_fd);
    rflex_sonars_off(rflex_fd);
    return -1;
  }
  else if(rflex_subscriptions == 1) 
  {
    rflex_stop_robot(rflex_fd,(int) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration));
    rflex_brake_on(rflex_fd);   
    rflex_sonars_off(rflex_fd);
    rflex_subscriptions--;  // decrement the static rflex-wide subscr counter
    subscriptions--;       // decrement the per-device subscr counter
    return 0;
  }
  else 
  {
    rflex_subscriptions--;  // decrement the static rflex-wide subscr counter
    subscriptions--;       // decrement the per-device subscr counter
    return 0;
  }
}


void RFLEX::PutData( unsigned char* src, size_t maxsize,
                         uint32_t timestamp_sec, uint32_t timestamp_usec)
{
  Lock();

  *((player_rflex_data_t*)device_data) = *((player_rflex_data_t*)src);

  if(timestamp_sec == 0)
  {
    struct timeval curr;
    pthread_testcancel();
    GlobalTime->GetTime(&curr);
    pthread_testcancel();
    timestamp_sec = curr.tv_sec;
    timestamp_usec = curr.tv_usec;
  }

  data_timestamp_sec = timestamp_sec;
  data_timestamp_usec = timestamp_usec;
  
  // need to fill in the timestamps on all RFLEX devices, both so that they
  // can read it, but also because other devices may want to read it
  player_device_id_t id = device_id;

  id.code = PLAYER_SONAR_CODE;
  pthread_testcancel();
  //CDevice* sonarp = deviceTable->GetDevice(id);
  CDevice* sonarp = SonarDev;
  pthread_testcancel();
  if(sonarp)
  {
    sonarp->data_timestamp_sec = this->data_timestamp_sec;
    sonarp->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_POWER_CODE;
  pthread_testcancel();
  //CDevice* powerp = deviceTable->GetDevice(id);
  CDevice* powerp = PowerDev;
  pthread_testcancel();
  if(powerp)
  {
    powerp->data_timestamp_sec = this->data_timestamp_sec;
    powerp->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_BUMPER_CODE;
  pthread_testcancel();
  //CDevice* bumperp = deviceTable->GetDevice(id);
  CDevice* bumperp = BumperDev;
  pthread_testcancel();
  if(bumperp)
  {
    bumperp->data_timestamp_sec = this->data_timestamp_sec;
    bumperp->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_IR_CODE;
  pthread_testcancel();
  //CDevice* ir = deviceTable->GetDevice(id);
  CDevice* ir = IrDev;
  pthread_testcancel();
  if(ir)
  {
    ir->data_timestamp_sec = this->data_timestamp_sec;
    ir->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_AIO_CODE;
  pthread_testcancel();
  //CDevice* aio = deviceTable->GetDevice(id);
  CDevice* aio = AIODev;
  pthread_testcancel();
  if(aio)
  {
    aio->data_timestamp_sec = this->data_timestamp_sec;
    aio->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_DIO_CODE;
  pthread_testcancel();
  //CDevice* dio = deviceTable->GetDevice(id);
  CDevice* dio = DIODev;
  pthread_testcancel();
  if(dio)
  {
    dio->data_timestamp_sec = this->data_timestamp_sec;
    dio->data_timestamp_usec = this->data_timestamp_usec;
  }

  id.code = PLAYER_POSITION_CODE;
  pthread_testcancel();
  //CDevice* positionp = deviceTable->GetDevice(id);
  CDevice * positionp = PositionDev;
  pthread_testcancel();
  if(positionp)
  {
    positionp->data_timestamp_sec = this->data_timestamp_sec;
    positionp->data_timestamp_usec = this->data_timestamp_usec;
  }

  Unlock();
}

void 
RFLEX::Main()
{
	printf("Waiting for rflex_done=1 in config file...\n");
	while (rflex_configs.run == false) pthread_testcancel();
	printf("Rflex Thread Started\n");

  //sets up connection, and sets defaults
  //configures sonar, motor acceleration, etc.
  if(initialize_robot()<0){
    fprintf(stderr,"ERROR, no connection to RFLEX established\n");
    exit(1);
  }
  reset_odometry();

	
	player_rflex_cmd_t command;
	unsigned char config[RFLEX_CONFIG_BUFFER_SIZE];
	
	static double mmPsec_speedDemand=0.0, radPsec_turnRateDemand=0.0;
	bool newmotorspeed, newmotorturn;
	
	int config_size;
	int i;
	int last_sonar_subscrcount;
	int last_position_subscrcount;
	int last_bumper_subscrcount;
	int last_ir_subscrcount;

/*	player_device_id_t id;

	id.port = global_playerport;
	id.index = 0;


	id.code = PLAYER_SONAR_CODE;
	CDevice* sonarp = deviceTable->GetDevice(id);
	id.code = PLAYER_POSITION_CODE;
	CDevice* positionp = deviceTable->GetDevice(id);
	id.code = PLAYER_BUMPER_CODE;
	CDevice* bumperp = deviceTable->GetDevice(id);
	id.code = PLAYER_IR_CODE;
	CDevice* irp = deviceTable->GetDevice(id);*/
	
	CDevice* sonarp = SonarDev;
	CDevice* positionp = PositionDev;
	CDevice* bumperp = BumperDev;
	CDevice* irp = IrDev;
	
	last_sonar_subscrcount = 0;
	last_position_subscrcount = 0;
	last_bumper_subscrcount = 0;
	last_ir_subscrcount = 0;


	GlobalTime->GetTime(&timeBegan_tv);
	while(1)
	{
		// we want to turn on the sonars if someone just subscribed, and turn
		// them off if the last subscriber just unsubscribed.
		if(sonarp)
		{
			if(!last_sonar_subscrcount && sonarp->subscriptions)
				rflex_sonars_on(rflex_fd);
			else if(last_sonar_subscrcount && !(sonarp->subscriptions))
				rflex_sonars_off(rflex_fd);
			
			last_sonar_subscrcount = sonarp->subscriptions;
		}

		// dont need to do anything with bumpers exactly, but may as well
		// keep track of them ...
		if(bumperp)
		{
			last_bumper_subscrcount = bumperp->subscriptions;
		}

		// we want to turn on the ir if someone just subscribed, and turn
		// it off if the last subscriber just unsubscribed.
		if(irp)
		{
			if(!last_ir_subscrcount && irp->subscriptions)
				rflex_ir_on(rflex_fd);
			else if(last_ir_subscrcount && !(irp->subscriptions))
				rflex_ir_off(rflex_fd);
			
			last_ir_subscrcount = irp->subscriptions;
		}


		// we want to reset the odometry and enable the motors if the first 
		// client just subscribed to the position device, and we want to stop 
		// and disable the motors if the last client unsubscribed.
		if(positionp){
			//first user logged in
			if(!last_position_subscrcount && positionp->subscriptions)
			{
				//set drive defaults
				rflex_motion_set_defaults(rflex_fd);
				
				//make sure robot doesn't go anywhere
				rflex_stop_robot(rflex_fd,(int) (MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration)));
				
				//clear the buffers
				player_position_cmd_t position_cmd;
				position_cmd.xspeed = 0;
				position_cmd.yawspeed = 0;
				
				// TODO: who should really be the client here?
				positionp->PutCommand(this,(unsigned char*)(&position_cmd), sizeof(position_cmd));
			}

			//last user logged out
			else if(last_position_subscrcount && !(positionp->subscriptions))
			{
				//make sure robot doesn't go anywhere
				rflex_stop_robot(rflex_fd,(int) (MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration)));
				// disable motor power
				rflex_brake_on(rflex_fd);
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
						if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
							PLAYER_ERROR("failed to PutReply");
						break;
					}
	
					player_sonar_power_config_t sonar_config;
					sonar_config = *((player_sonar_power_config_t*)config);

					if(sonar_config.value==0)
						rflex_sonars_off(rflex_fd);
					else
						rflex_sonars_on(rflex_fd);
	
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
						PLAYER_ERROR("failed to PutReply");
					break;
	
				case PLAYER_SONAR_GET_GEOM_REQ:
					/* Return the sonar geometry. */

					if(config_size != 1)
					{
						puts("Arg get sonar geom is wrong size; ignoring");
						if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
							PLAYER_ERROR("failed to PutReply");
						break;
					}
	
					player_sonar_geom_t geom;
					geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;
					geom.pose_count = htons((short) rflex_configs.num_sonars);
					for (i = 0; i < rflex_configs.num_sonars; i++){
						geom.poses[i][0] = htons((short) rflex_configs.mmrad_sonar_poses[i].x);
						geom.poses[i][1] = htons((short) rflex_configs.mmrad_sonar_poses[i].y);
						geom.poses[i][2] = htons((short) RAD2DEG_CONV(rflex_configs.mmrad_sonar_poses[i].t));
					}
	
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)))
						PLAYER_ERROR("failed to PutReply");
					
					break;
				default:
					puts("Sonar got unknown config request");
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
						PLAYER_ERROR("failed to PutReply");
					break;
				}
				break;
			case PLAYER_BUMPER_CODE:
				switch(config[0])
				{
				case PLAYER_BUMPER_GET_GEOM_REQ:
					/* Return the bumper geometry. */
					if(config_size != 1)
					{
						puts("Arg get bumper geom is wrong size; ignoring");
						if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
							PLAYER_ERROR("failed to PutReply");
						break;
					}

					// Assemble geometry structure for sending
					player_bumper_geom_t geom;
					geom.subtype = PLAYER_BUMPER_GET_GEOM_REQ;
					geom.bumper_count = htons((short) rflex_configs.bumper_count);
					for (i = 0; i < rflex_configs.bumper_count; i++){
						geom.bumper_def[i].x_offset = htons((short) rflex_configs.bumper_def[i].x_offset); //mm
						geom.bumper_def[i].y_offset = htons((short) rflex_configs.bumper_def[i].y_offset); //mm
						geom.bumper_def[i].th_offset = htons((short) rflex_configs.bumper_def[i].th_offset); //deg
						geom.bumper_def[i].length = htons((short) rflex_configs.bumper_def[i].length); //mm
						geom.bumper_def[i].radius = htons((short) rflex_configs.bumper_def[i].radius); //mm
					}

					// Send
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)))
						PLAYER_ERROR("failed to PutReply");
					break;
	
				// Arent any request other than geometry
				default:
					puts("Bumper got unknown config request");
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
						PLAYER_ERROR("failed to PutReply");
					break;
				}
				break;

			case PLAYER_IR_CODE:
				switch(config[0])
				{
				case PLAYER_IR_POSE_REQ:
					/* Return the ir geometry. */
					if(config_size != 1)
					{
						puts("Arg get bumper geom is wrong size; ignoring");
						if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
							PLAYER_ERROR("failed to PutReply");
						break;
					}

					// Assemble geometry structure for sending
					//printf("sending geometry to client, posecount = %d\n", rflex_configs.ir_poses.pose_count);
					player_ir_pose_req geom;
					geom.subtype = PLAYER_IR_POSE_REQ;
					geom.poses.pose_count = htons((short) rflex_configs.ir_poses.pose_count);
					for (i = 0; i < rflex_configs.ir_poses.pose_count; i++){
						geom.poses.poses[i][0] = htons((short) rflex_configs.ir_poses.poses[i][0]); //mm
						geom.poses.poses[i][1] = htons((short) rflex_configs.ir_poses.poses[i][1]); //mm
						geom.poses.poses[i][2] = htons((short) rflex_configs.ir_poses.poses[i][2]); //deg
					}

					// Send
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)))
						PLAYER_ERROR("failed to PutReply");
					break;
				case PLAYER_IR_POWER_REQ:
					/* Return the ir geometry. */
					if(config_size != 1)
					{
						puts("Arg get ir geom is wrong size; ignoring");
						if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
							PLAYER_ERROR("failed to PutReply");
						break;
					}

					if (config[1] == 0)
						rflex_ir_off(rflex_fd);
					else
						rflex_ir_on(rflex_fd);
										
					// Send
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
						PLAYER_ERROR("failed to PutReply");
					break;
	
				// Arent any request other than geometry and power
				default:
					puts("Ir got unknown config request");
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
						PLAYER_ERROR("failed to PutReply");
					break;
				}
				break;

			case PLAYER_POSITION_CODE:
				switch(config[0])
				{
				case PLAYER_POSITION_SET_ODOM_REQ:
					if(config_size != sizeof(player_position_set_odom_req_t))
					{
						puts("Arg to odometry set requests wrong size; ignoring");
						if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
							PLAYER_ERROR("failed to PutReply");
						break;
					}
	
					player_position_set_odom_req_t set_odom_req;
					set_odom_req = *((player_position_set_odom_req_t*)config);
					//in mm
					set_odometry((long) ntohl(set_odom_req.x),(long) ntohl(set_odom_req.y),(short) ntohs(set_odom_req.theta));
	
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
						PLAYER_ERROR("failed to PutReply");
					break;
	
				case PLAYER_POSITION_MOTOR_POWER_REQ:
					/* motor state change request 
					 *   1 = enable motors
					 *   0 = disable motors (default)
					 */
					if(config_size != sizeof(player_position_power_config_t))
					{
						puts("Arg to motor state change request wrong size; ignoring");
						if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
							PLAYER_ERROR("failed to PutReply");
						break;
					}
		
					player_position_power_config_t power_config;
					power_config = *((player_position_power_config_t*)config);

					if(power_config.value==0)
						rflex_brake_on(rflex_fd);
					else
						rflex_brake_off(rflex_fd);
		
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
						PLAYER_ERROR("failed to PutReply");
					break;
		
				case PLAYER_POSITION_VELOCITY_MODE_REQ:
					/* velocity control mode:
					 *   0 = direct wheel velocity control (default)
					 *   1 = separate translational and rotational control
					 */
					fprintf(stderr,"WARNING!!: only velocity mode supported\n");
					if(config_size != sizeof(player_position_velocitymode_config_t))
					{
						puts("Arg to velocity control mode change request is wrong size; ignoring");
						if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
							PLAYER_ERROR("failed to PutReply");
						break;
					}
		
					player_position_velocitymode_config_t velmode_config;
					velmode_config = *((player_position_velocitymode_config_t*)config);
					
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
						PLAYER_ERROR("failed to PutReply");
					break;

				case PLAYER_POSITION_RESET_ODOM_REQ:
					/* reset position to 0,0,0: no args */
					if(config_size != sizeof(player_position_resetodom_config_t))
					{
						puts("Arg to reset position request is wrong size; ignoring");
						if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
							PLAYER_ERROR("failed to PutReply");
						break;
					}
					reset_odometry();
		
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
						PLAYER_ERROR("failed to PutReply");
					break;
		
				case PLAYER_POSITION_GET_GEOM_REQ:
					/* Return the robot geometry. */
					if(config_size != 1)
					{
						puts("Arg get robot geom is wrong size; ignoring");
						if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
							PLAYER_ERROR("failed to PutReply");
						break;
					}
		
					// TODO : get values from somewhere.
					player_position_geom_t geom;
					geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
					//mm
					geom.pose[0] = htons((short) (0));
					geom.pose[1] = htons((short) (0));
					//radians
					geom.pose[2] = htons((short) (0));
					//mm
					geom.size[0] = htons((short) (rflex_configs.mm_length));
					geom.size[1] = htons((short) (rflex_configs.mm_width));
		
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)))
						PLAYER_ERROR("failed to PutReply");
					break;
				default:
					puts("Position got unknown config request");
					if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
						PLAYER_ERROR("failed to PutReply");
					break;
				}
				break;
	
			default:
				printf("RunRflex Thread: got unknown config request \"%c\"\n", config[0]);

				if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
					PLAYER_ERROR("failed to PutReply");
				break;
			}
		}

		/* read the clients' commands from the common buffer */
		GetCommand((unsigned char*)&command, sizeof(command));
		
		if(positionp && (positionp->subscriptions || rflex_configs.use_joystick))
		{
			newmotorspeed = false;
			newmotorturn = false;
			//the long casts are necicary (ntohl returns unsigned - we need signed)
			if(mmPsec_speedDemand != (long) ntohl(command.position.xspeed))
			{
				newmotorspeed = true;
				mmPsec_speedDemand = (long) ntohl(command.position.xspeed);
			}
			if(radPsec_turnRateDemand != DEG2RAD_CONV((long) ntohl(command.position.yawspeed)))
			{
				newmotorturn = true;
				radPsec_turnRateDemand = DEG2RAD_CONV((long) ntohl(command.position.yawspeed));
			}
			/* NEXT, write commands */
			// rflex has a built in failsafe mode where if no move command is recieved in a 
			// certain interval the robot stops.
			// this is a good thing given teh size of the robot...
			// if network goes down or some such and the user looses control then the robot stops
			// if the robot is running in an autonomous mdoe it is easy enough to simply 
			// resend the command repeatedly

			// allow rflex joystick to overide the player command
			if (joy_control > 0)
				--joy_control;
			// only set new command if type is valid and their is a new command
			else if (command.position.type == 0)
			{
				rflex_set_velocity(rflex_fd,(long) MM2ARB_ODO_CONV(mmPsec_speedDemand),(long) RAD2ARB_ODO_CONV(radPsec_turnRateDemand),(long) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration));    
				command.position.type = 255;
				positionp->PutCommand(this,(unsigned char *)&command.position, sizeof(command.position));
			}
		}
		else
			rflex_stop_robot(rflex_fd,(long) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration));
		
		/* Get data from robot */
		update_everything(data,sonarp,bumperp,irp);
		pthread_testcancel();
		PutData((unsigned char*) data,sizeof(data),0,0);
		pthread_testcancel();
	}
	pthread_exit(NULL);
}

/* start a thread that will invoke Main() */
void 
RFLEX::StartThread()
{
  pthread_create(&thread, NULL, &DummyMain, this);
}

/* cancel (and wait for termination) of the thread */
void 
RFLEX::StopThread()
{
  void* dummy;
  pthread_cancel(thread);
  if(pthread_join(thread,&dummy))
    perror("RFLEX::StopThread:pthread_join()");
}


int RFLEX::initialize_robot(){
#ifdef _REENTRANT
  if (thread_is_running)
    {
      fprintf(stderr,"Tried to connect to the robot while the command loop "
                  "thread is running.\n");
      fprintf(stderr,"This is a bug in the code, and must be fixed.\n");
      return -1;
    }
#endif

  if (rflex_open_connection(rflex_configs.serial_port, &rflex_fd) < 0)
    return -1;
  
  rflex_initialize(rflex_fd, (int) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration),(int) RAD2ARB_ODO_CONV(rflex_configs.radPsec2_rot_acceleration), 0, 0);

  return 0;
}

void RFLEX::reset_odometry() {
  mm_odo_x=0;
  mm_odo_y=0;
  rad_odo_theta= 0.0;
}

void RFLEX::set_odometry(long mm_x, long mm_y, short deg_theta) {
  mm_odo_x=mm_x;
  mm_odo_y=mm_y;
  rad_odo_theta=DEG2RAD_CONV(deg_theta);
}

void RFLEX::update_everything(player_rflex_data_t* d, CDevice* sonarp, CDevice *bumperp, CDevice * irp) {

  int arb_ranges[PLAYER_SONAR_MAX_SAMPLES];
  char abumper_ranges[PLAYER_BUMPER_MAX_SAMPLES];
  unsigned char air_ranges[PLAYER_IR_MAX_SAMPLES];

  // changed these variable-size array declarations to the 
  // bigger-than-necessary ones above, because older versions of gcc don't
  // support variable-size arrays.
  // int arb_ranges[rflex_configs.num_sonars];
  // char abumper_ranges[rflex_configs.bumper_count];
  // unsigned char air_ranges[rflex_configs.ir_poses.pose_count];

  static int initialized = 0;

  double mm_new_range_position; double rad_new_bearing_position;
  double mmPsec_t_vel;
  double radPsec_r_vel;

  int arb_t_vel, arb_r_vel;
  static int arb_last_range_position;
  static int arb_last_bearing_position;
  int arb_new_range_position;
  int arb_new_bearing_position;
  double mm_displacement;
  short a_num_sonars, a_num_bumpers, a_num_ir;

  int batt,brake;

  int i;
    
  //update status
  rflex_update_status(rflex_fd, &arb_new_range_position,
                             &arb_new_bearing_position, &arb_t_vel,
                             &arb_r_vel);
  mmPsec_t_vel=ARB2MM_ODO_CONV(arb_t_vel);
  radPsec_r_vel=ARB2RAD_ODO_CONV(arb_r_vel);
  mm_new_range_position=ARB2MM_ODO_CONV(arb_new_range_position);
  rad_new_bearing_position=ARB2RAD_ODO_CONV(arb_new_bearing_position);
  
  if (!initialized) {
    initialized = 1;
  } else {
    rad_odo_theta += ARB2RAD_ODO_CONV(arb_new_bearing_position - arb_last_bearing_position);
    rad_odo_theta = normalize_theta(rad_odo_theta);
    mm_displacement = ARB2MM_ODO_CONV(arb_new_range_position - arb_last_range_position);

    //integrate latest motion into odometry
    mm_odo_x += mm_displacement * cos(rad_odo_theta);
    mm_odo_y += mm_displacement * sin(rad_odo_theta);
    d->position.xpos = htonl((long) mm_odo_x);
    d->position.ypos = htonl((long) mm_odo_y);
    while(rad_odo_theta<0)
      rad_odo_theta+=2*M_PI;
    d->position.yaw = htonl((long) RAD2DEG_CONV(rad_odo_theta) %360);

    d->position.xspeed = htonl((long) mmPsec_t_vel);
    d->position.yawspeed = htonl((long) RAD2DEG_CONV(radPsec_r_vel));
    //TODO - get better stall information (battery draw?)
  }
  d->position.stall = false;

  arb_last_range_position = arb_new_range_position;
  arb_last_bearing_position = arb_new_bearing_position;

   //note - sonar mappings are strange - look in rflex_commands.c
  if(sonarp && sonarp->subscriptions){
    // TODO - currently bad sonar data is sent back to clients 
    // (not enough data buffered, so sonar sent in wrong order - missing intermittent sonar values - fix this
    a_num_sonars=rflex_configs.num_sonars;

    pthread_testcancel();
    rflex_update_sonar(rflex_fd, a_num_sonars,
		       arb_ranges);
    pthread_testcancel();
    d->sonar.range_count=htons(a_num_sonars);
    for (i = 0; i < a_num_sonars; i++){
      d->sonar.ranges[i] = htons((uint16_t) ARB2MM_RANGE_CONV(arb_ranges[i]));
    }
  }

  // if someone is subscribed to bumpers copy internal data to device
   if(bumperp && bumperp->subscriptions)
   {
       a_num_bumpers=rflex_configs.bumper_count;

       pthread_testcancel();
	   // first make sure our internal state is up to date
       rflex_update_bumpers(rflex_fd, a_num_bumpers, abumper_ranges);
       pthread_testcancel();

       d->bumper.bumper_count=(a_num_bumpers);
       memcpy(d->bumper.bumpers,abumper_ranges,a_num_bumpers);
   }


  // if someone is subscribed to irs copy internal data to device
   if(irp && irp->subscriptions)
   {
       a_num_ir=rflex_configs.ir_poses.pose_count;

       pthread_testcancel();
	   // first make sure our internal state is up to date
       rflex_update_ir(rflex_fd, a_num_ir, air_ranges);
       pthread_testcancel();

       d->ir.range_count = htons(a_num_ir);
	   for (int i = 0; i < a_num_ir; ++i)
	   {
	   		d->ir.voltages[i] = htons(air_ranges[i]);
			// using power law mapping of form range = (a*voltage)^b
			int range = (int) (pow(rflex_configs.ir_a[i] *((double) air_ranges[i]),rflex_configs.ir_b[i]));
			// check for min and max ranges, < min = 0 > max = max
			range = range < rflex_configs.ir_min_range ? 0 : range;
			range = range > rflex_configs.ir_max_range ? rflex_configs.ir_max_range : range;
	   		d->ir.ranges[i] = htons(range);		
	   }
   }


  //this would get the battery,time, and brake state (if we cared)
  //update system (battery,time, and brake also request joystick data)
  rflex_update_system(rflex_fd,&batt,&brake);
	d->power.charge = htons(static_cast<uint16_t> (batt/10) + rflex_configs.power_offset);
}

//default is for ones that don't need any configuration
void RFLEX::GetOptions(ConfigFile *cf,int section,rflex_config_t *configs){
  //do nothing at all
}

//this is so things don't crash if we don't load a device
//(and thus it's settings)
void RFLEX::set_config_defaults(){
  strcpy(rflex_configs.serial_port,"/dev/ttyR0");
  rflex_configs.mm_length=0.0;
  rflex_configs.mm_width=0.0;
  rflex_configs.odo_distance_conversion=0.0;
  rflex_configs.odo_angle_conversion=0.0;
  rflex_configs.range_distance_conversion=0.0;
  rflex_configs.mmPsec2_trans_acceleration=500.0;
  rflex_configs.radPsec2_rot_acceleration=500.0;
  rflex_configs.use_joystick=false;
  rflex_configs.joy_pos_ratio=0;
  rflex_configs.joy_ang_ratio=0;
  
  
  rflex_configs.max_num_sonars=0;
  rflex_configs.num_sonars=0;
  rflex_configs.sonar_age=0;
  rflex_configs.num_sonar_banks=0;
  rflex_configs.num_sonars_possible_per_bank=0;
  rflex_configs.num_sonars_in_bank=NULL;
  rflex_configs.mmrad_sonar_poses=NULL;
  
  rflex_configs.bumper_count = 0;
  rflex_configs.bumper_address = 0;
  rflex_configs.bumper_def = NULL;
  
  rflex_configs.ir_poses.pose_count = 0;
  rflex_configs.ir_base_bank = 0;
  rflex_configs.ir_bank_count = 0;
  rflex_configs.ir_count = NULL;
  rflex_configs.ir_a = NULL;
  rflex_configs.ir_b = NULL;
  
  rflex_configs.run = false; 
}



