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

#include <error.h>
#include <driver.h>
#include <drivertable.h>

#include <playertime.h>
extern PlayerTime* GlobalTime;

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


/* initialize the driver.
 *
 * returns: pointer to new REBIR object
 */
Driver*
Khepera_Init(ConfigFile *cf, int section)
{
  return (Driver *) new Khepera( cf, section);
}

/* register the Khepera IR driver in the drivertable
 *
 * returns: 
 */
void
Khepera_Register(DriverTable *table) 
{
  table->AddDriver("khepera", Khepera_Init);
}

Khepera::Khepera(ConfigFile *cf, int section) : Driver(cf, section)
{
  // zero ids, so that we'll know later which interfaces were requested
  memset(&this->position_id, 0, sizeof(player_device_id_t));
  memset(&this->ir_id, 0, sizeof(player_device_id_t));

  this->position_subscriptions = this->ir_subscriptions = 0;

  // Do we create a robot position interface?
  if(cf->ReadDeviceId(&(this->position_id), section, "provides", 
                      PLAYER_POSITION_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->position_id, PLAYER_ALL_MODE,
                          sizeof(player_position_data_t),
                          sizeof(player_position_cmd_t), 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create an ir interface?
  if(cf->ReadDeviceId(&(this->ir_id), section, "provides", 
                      PLAYER_IR_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->ir_id, PLAYER_READ_MODE,
                          sizeof(player_ir_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Read config file options
  geometry = new player_khepera_geom_t;
  geometry->PortName= NULL;
  geometry->scale = 0;

#if 0
  ((player_khepera_cmd_t*)device_command)->position.xspeed = 0;
  ((player_khepera_cmd_t*)device_command)->position.yawspeed = 0;
#endif

  //set up the poll parameters... used for the comms
  // over the serial port to the Kam
  //write_pfd.events = POLLOUT;
  //read_pfd.events = POLLIN;

  // now we have to look up our parameters.  this should be given as an argument
  if (geometry->PortName == NULL)
    geometry->PortName = strdup(cf->ReadString(section, "port", KHEPERA_DEFAULT_SERIAL_PORT));
  if (geometry->scale == 0)
    geometry->scale = cf->ReadFloat(section, "scale_factor", KHEPERA_DEFAULT_SCALE);

  // set sub type here
  geometry->position.subtype = PLAYER_POSITION_GET_GEOM_REQ;

  geometry->encoder_res = cf->ReadFloat(section,"encoder_res", KHEPERA_DEFAULT_ENCODER_RES);

  // Load position config
  geometry->position.pose[0] = khtons(static_cast<unsigned short> (cf->ReadTupleFloat(section,"position_pose",0,0)));
  geometry->position.pose[1] = khtons(static_cast<unsigned short> (cf->ReadTupleFloat(section,"position_pose",1,0)));
  geometry->position.pose[2] = htons(static_cast<unsigned short> (cf->ReadTupleFloat(section,"position_pose",2,0)));

  // load dimension of the base
  geometry->position.size[0] = khtons(static_cast<unsigned short> (cf->ReadTupleFloat(section,"position_size",0,57)));
  geometry->position.size[1] = khtons(static_cast<unsigned short> (cf->ReadTupleFloat(section,"position_size",1,57)));

  // load ir geometry config
  geometry->ir.pose_count = (cf->ReadInt(section,"ir_pose_count", 8));
  if (geometry->ir.pose_count == 8 && cf->ReadTupleFloat(section,"ir_poses",0,-1) == -1)
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
      geometry->ir.poses[i][0] = khtons(static_cast<short> (cf->ReadTupleFloat(section,"ir_poses",3*i,0)));
      geometry->ir.poses[i][1] = khtons(static_cast<short> (cf->ReadTupleFloat(section,"ir_poses",3*i+1,0)));
      geometry->ir.poses[i][2] = htons(static_cast<short> (cf->ReadTupleFloat(section,"ir_poses",3*i+2,0)));
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

  // zero position counters
  last_lpos = 0;
  last_rpos = 0;
  last_x_f=0;
  last_y_f=0;
  last_theta = 0.0;
}

short Khepera::khtons(short in)
{
  return htons(static_cast<unsigned short> (in * geometry->scale));
}

short Khepera::ntokhs(short in)
{
  return static_cast<short> (ntohs(static_cast<unsigned short> (in)) / geometry->scale);
}

int 
Khepera::Subscribe(player_device_id_t id)
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
      case PLAYER_IR_CODE:
        this->ir_subscriptions++;
        break;
    }
  }

  return(setupResult);
}

int 
Khepera::Unsubscribe(player_device_id_t id)
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
      case PLAYER_IR_CODE:
        assert(--this->ir_subscriptions >= 0);
        break;
    }
  }

  return(shutdownResult);
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

  player_position_cmd_t zero;
  memset(&zero,0,sizeof(player_position_cmd_t));
  this->PutCommand(this->position_id,(void*)&zero,
                   sizeof(player_position_cmd_t),NULL);

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

  return(0);
}

void 
Khepera::Main()
{
  player_position_cmd_t cmd;
  int last_ir_subscrcount=0;
  int last_position_subscrcount=0;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

  while (1) 
  {
    // we want to turn on the IR if someone just subscribed, and turn
    // them off if the last subscriber just unsubscribed.
    if(!last_ir_subscrcount && this->ir_subscriptions)
    {
      // zero out ranges in IR data so proxy knows
      // to do regression
      player_ir_data_t ir_data;
      memset(&ir_data,0,sizeof(player_ir_data_t));
      PutData(this->ir_id,(unsigned char*)&ir_data,
              sizeof(player_ir_data_t),NULL);
    }
    last_ir_subscrcount = this->ir_subscriptions;

    // we want to reset the odometry and enable the motors if the first 
    // client just subscribed to the position device, and we want to stop 
    // and disable the motors if the last client unsubscribed.
    if(!last_position_subscrcount && this->position_subscriptions)
    {
      printf("Khepera: first pos sub. turn off and reset\n");
      // then first sub for pos, so turn off motors and reset odom
      SetSpeed(0,0);
      ResetOdometry();

    } 
    else if(last_position_subscrcount && !(this->position_subscriptions))
    {
      // last sub just unsubbed
      printf("Khepera: last pos sub gone\n");
      SetSpeed(0,0);

      // overwrite existing motor commands to be zero
      player_position_cmd_t position_cmd;
      memset(&position_cmd,0,sizeof(player_position_cmd_t));
      // TODO: who should really be the client here?
      PutCommand(this->position_id,
                 (unsigned char*)(&position_cmd), sizeof(position_cmd),
                 NULL);
    }
    last_position_subscrcount = this->position_subscriptions;

    // get configuration commands (ioctls)
    ReadConfig();

    if(this->position_subscriptions) 
    {
      // get a position command
      GetCommand(this->position_id,(unsigned char*)&cmd, 
                 sizeof(player_position_cmd_t), NULL);

      if (this->velocity_mode) 
      {
        // then we are in velocity mode

        // need to calculate the left and right velocities
        int transvel = static_cast<int> (static_cast<int> (ntohl(cmd.xspeed)) * geometry->encoder_res);
        int rotvel = static_cast<int> (static_cast<int> (ntohl(cmd.yawspeed)) * geometry->encoder_res * M_PI * ntokhs(geometry->position.size[0])/360.0);
        int leftvel = transvel - rotvel;
        int rightvel = transvel + rotvel;

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

/* this will read a new config command and interpret it
 *
 * returns: 
 */
void
Khepera::ReadConfig()
{
  int config_size;
  unsigned char config_buffer[KHEPERA_CONFIG_BUFFER_SIZE];
  void *client;

  // check for IR config requests
  if((config_size = GetConfig(this->ir_id, &client, 
                              (void*)config_buffer, 
                              sizeof(config_buffer),NULL)))
  {
    // Khepera_IR IOCTLS /////////////////

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
            if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
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

          if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_ACK, 
                      &irpose, sizeof(irpose),NULL)) 
          {
            PLAYER_ERROR("Khepera: failed to put reply");
            break;
          }
        }
        break;

      default:
        fprintf(stderr, "Khepera: IR got unknown config\n");
        if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
        {
          PLAYER_ERROR("Khepera: failed to put reply");
        }
        break;
    }

    // END Khepera_IR IOCTLS //////////////
  }

  // check for position config requests
  if((config_size = GetConfig(this->position_id, &client, 
                              (void*)config_buffer, 
                              sizeof(config_buffer),NULL)))
  {
    // POSITION IOCTLS ////////////////

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
            if(PutReply(this->position_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
            {
              PLAYER_ERROR("Khepera: failed to put reply");
            }
            break;
          }

#ifdef DEBUG_CONFIG
          printf("Khepera: POSITION_GET_GEOM_REQ\n");
#endif

          if(PutReply(this->position_id, client, PLAYER_MSGTYPE_RESP_ACK, 
                      &geometry->position, sizeof(geometry->position),NULL)) 
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
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
            {
              PLAYER_ERROR("Khepera: failed to put reply");
            }
            break;
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

          if(PutReply(this->position_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
          {
            PLAYER_ERROR("Khepera: failed to put reply");
          }

          printf("Khepera: put MOTOR POWER REQ\n");
        }
        break;

      case PLAYER_POSITION_VELOCITY_MODE_REQ: 
        // select method of velocity control
        // 0 for direct velocity control (trans and rot applied directly)
        // 1 for builtin velocity based heading PD controller
        if (config_size != sizeof(player_position_velocitymode_config_t)) 
        {
          fprintf(stderr, "Khepera: pos vel control req got wrong size (%d)\n", config_size);
          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_NACK, NULL))
          {
            PLAYER_ERROR("Khepera: failed to put reply");
          }
          break;
        }

        if(PutReply(this->position_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
        {
          PLAYER_ERROR("Khepera: failed to put reply");
        }
        break;

      case PLAYER_POSITION_RESET_ODOM_REQ: 
        // reset the odometry
        if (config_size != 1) 
        {
          fprintf(stderr, "Khepera: pos reset odom req got wrong size (%d)\n", config_size);
          if(PutReply(this->position_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
          {
            PLAYER_ERROR("Khepera: failed to put reply");
          }
          break;
        }

#ifdef DEBUG_CONFIG
        printf("Khepera: RESET_ODOM_REQ\n");
#endif

        ResetOdometry();

        if(PutReply(this->position_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
        {
          PLAYER_ERROR("Khepera: failed to put reply");
        }
        break;

      case PLAYER_POSITION_SET_ODOM_REQ: 
        {
          // set the odometry to a given position
          if (config_size != sizeof(player_position_set_odom_req_t)) 
          {
            fprintf(stderr, "Khepera: pos set odom req got wrong size (%d)\n", config_size);
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
            {
              PLAYER_ERROR("Khepera: failed to put reply");
            }
            break;
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

          if(PutReply(this->position_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
          {
            PLAYER_ERROR("Khepera: failed to put reply");
          }
        }
        break;

      default:
        fprintf(stderr, "Khepera: Position got unknown config\n");
        if(PutReply(this->position_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
        {
          PLAYER_ERROR("Khepera: failed to put reply");
        }
        break;
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
  player_position_data_t position_data;
  player_ir_data_t ir_data;

  UpdatePosData(&position_data);

  // put position data
  PutData(this->position_id, 
          (void*)&position_data,
          sizeof(player_position_data_t),
          NULL);

  UpdateIRData(&ir_data);

  // put ir data
  PutData(this->ir_id, 
          (void*)&ir_data,
          sizeof(player_ir_data_t),
          NULL);
}

/* this will update the IR part of the client data
 * it entails reading the currently active IR sensors
 * and then changing their state to off and turning on
 * 2 new IRs.  
 *
 * returns:
 */
void
Khepera::UpdateIRData(player_ir_data_t * d)
{
  ReadAllIR(d);

  d->range_count = geometry->ir.pose_count;
  for (int i =0; i < ntohs(geometry->ir.pose_count); i++) 
  {
    d->ranges[i] = htons(static_cast<unsigned short> (geometry->scale * geometry->ir_calib_a[i] * pow(d->voltages[i],geometry->ir_calib_b[i])));
    d->voltages[i] = htons(d->voltages[i]);
    //printf("(%d,%d) ",ntohs(d->ir.ranges[i]),ntohs(d->ir.voltages[i]));
  }
  //printf("\n");
}

  
/* this will update the position data.  this entails odometry, etc
 */ 
void
Khepera::UpdatePosData(player_position_data_t *d)
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
  double rad_Theta = DTOR(yaw);
  x+=(dx*cos(rad_Theta) + dy*sin(rad_Theta));
  y+=(dy*cos(rad_Theta) + dx*sin(rad_Theta));
  d->xpos = htonl(static_cast<int> (x));
  d->ypos = ntohl(static_cast<int> (y));
  yaw += Theta;
  while (yaw > 360) yaw -= 360;
  while (yaw < 0) yaw += 360;
  d->yaw = htonl(static_cast<int> (yaw));
  d->xspeed = htonl(static_cast<int> (trans_vel));
  //d->yspeed = 0;
  d->yawspeed = htonl(static_cast<int> (rot_vel_deg));
  //d->stall = 0;
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

  player_position_data_t data;
  memset(&data,0,sizeof(player_position_data_t));
  this->PutData(this->position_id, &data, 
                sizeof(player_position_data_t), NULL);

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
Khepera::ReadAllIR(player_ir_data_t* d)
{
  int Values[PLAYER_IR_MAX_SAMPLES];

  // changed these variable-size array declarations to the 
  // bigger-than-necessary ones above, because older versions of gcc don't
  // support variable-size arrays.
  // int Values[ntohs(geometry->ir.pose_count)];

  if(Serial->KheperaCommand('N',0,NULL,ntohs(geometry->ir.pose_count),Values) < 0)
    return -1;			
  for (int i=0; i< ntohs(geometry->ir.pose_count); ++i)
  {
    d->voltages[i] = static_cast<short> (Values[i]);
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
