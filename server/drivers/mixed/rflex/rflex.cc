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

#include <driver.h>
#include <error.h>
#include <drivertable.h>
#include <playertime.h>
extern PlayerTime* GlobalTime;

extern int               RFLEX::joy_control;

//NOTE - this is accessed as an extern variable by the other RFLEX objects
rflex_config_t rflex_configs;

/* initialize the driver.
 *
 * returns: pointer to new REBIR object
 */
Driver*
RFLEX_Init(ConfigFile *cf, int section)
{
  return (Driver *) new RFLEX( cf, section);
}

/* register the Khepera IR driver in the drivertable
 *
 * returns: 
 */
void
RFLEX_Register(DriverTable *table) 
{
  table->AddDriver("rflex", RFLEX_Init);
}

RFLEX::RFLEX(ConfigFile* cf, int section)
        : Driver(cf,section)
{
  // zero ids, so that we'll know later which interfaces were requested
  memset(&this->position_id, 0, sizeof(player_device_id_t));
  memset(&this->sonar_id, 0, sizeof(player_device_id_t));
  memset(&this->sonar_id_2, 0, sizeof(player_device_id_t));
  memset(&this->ir_id, 0, sizeof(player_device_id_t));
  memset(&this->bumper_id, 0, sizeof(player_device_id_t));
  memset(&this->power_id, 0, sizeof(player_device_id_t));
  memset(&this->aio_id, 0, sizeof(player_device_id_t));
  memset(&this->dio_id, 0, sizeof(player_device_id_t));

  this->position_subscriptions = 0;
  this->sonar_subscriptions = 0;
  this->ir_subscriptions = 0;
  this->bumper_subscriptions = 0;

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

  // Do we create a sonar interface?
  if(cf->ReadDeviceId(&(this->sonar_id), section, "provides", 
                      PLAYER_SONAR_CODE, -1, "sonar") == 0)
  {
    if(this->AddInterface(this->sonar_id, PLAYER_READ_MODE,
                          sizeof(player_sonar_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a second sonar interface?
  if(cf->ReadDeviceId(&(this->sonar_id_2), section, "provides", 
                      PLAYER_SONAR_CODE, -1, "sonar2") == 0)
  {
    if(this->AddInterface(this->sonar_id_2, PLAYER_READ_MODE,
                          sizeof(player_sonar_data_t), 0, 1, 1) != 0)
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

  // Do we create a bumper interface?
  if(cf->ReadDeviceId(&(this->bumper_id), section, "provides", 
                      PLAYER_BUMPER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->bumper_id, PLAYER_READ_MODE,
                          sizeof(player_bumper_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a power interface?
  if(cf->ReadDeviceId(&(this->power_id), section, "provides", 
                      PLAYER_POWER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->power_id, PLAYER_READ_MODE,
                          sizeof(player_power_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create an aio interface?
  if(cf->ReadDeviceId(&(this->aio_id), section, "provides", 
                      PLAYER_AIO_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->aio_id, PLAYER_READ_MODE,
                          sizeof(player_aio_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a dio interface?
  if(cf->ReadDeviceId(&(this->dio_id), section, "provides", 
                      PLAYER_DIO_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->dio_id, PLAYER_READ_MODE,
                          sizeof(player_dio_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  //just sets stuff to zero
  set_config_defaults();

  // joystick override
  joy_control = 0;

  //get serial port: everyone needs it (and we dont' want them fighting)
  strncpy(rflex_configs.serial_port,
          cf->ReadString(section, "rflex_serial_port", 
                         rflex_configs.serial_port),
          sizeof(rflex_configs.serial_port));

  ////////////////////////////////////////////////////////////////////////
  // Position-related options

  //length
  rflex_configs.mm_length=
    cf->ReadFloat(section, "mm_length",0.5);
  //width
  rflex_configs.mm_width=
    cf->ReadFloat(section, "mm_width",0.5);
  //distance conversion
  rflex_configs.odo_distance_conversion=
    cf->ReadFloat(section, "odo_distance_conversion", 0.0);
  //angle conversion
  rflex_configs.odo_angle_conversion=
    cf->ReadFloat(section, "odo_angle_conversion", 0.0);
  //default trans accel
  rflex_configs.mmPsec2_trans_acceleration=
    cf->ReadFloat(section, "default_trans_acceleration",0.1);
  //default rot accel
  rflex_configs.radPsec2_rot_acceleration=
    cf->ReadFloat(section, "default_rot_acceleration",0.1);

  // use rflex joystick for position
  rflex_configs.use_joystick |= cf->ReadInt(section, "rflex_joystick",0);
  rflex_configs.joy_pos_ratio = cf->ReadFloat(section, "rflex_joy_pos_ratio",0);
  rflex_configs.joy_ang_ratio = cf->ReadFloat(section, "rflex_joy_ang_ratio",0);

  ////////////////////////////////////////////////////////////////////////
  // Sonar-related options
  int x;

  rflex_configs.range_distance_conversion=
          cf->ReadFloat(section, "range_distance_conversion",1);
  rflex_configs.max_num_sonars=
          cf->ReadInt(section, "max_num_sonars",64);
  rflex_configs.num_sonars=
          cf->ReadInt(section, "num_sonars",24);
  rflex_configs.sonar_age=
          cf->ReadInt(section, "sonar_age",1);
  rflex_configs.num_sonar_banks=
          cf->ReadInt(section, "num_sonar_banks",8);
  rflex_configs.num_sonars_possible_per_bank=
          cf->ReadInt(section, "num_sonars_possible_per_bank",16);
  rflex_configs.num_sonars_in_bank=(int *) malloc(rflex_configs.num_sonar_banks*sizeof(double));
  for(x=0;x<rflex_configs.num_sonar_banks;x++)
    rflex_configs.num_sonars_in_bank[x]=
            (int) cf->ReadTupleFloat(section, "num_sonars_in_bank",x,8);
  rflex_configs.sonar_echo_delay=
          cf->ReadInt(section, "sonar_echo_delay",3000);
  rflex_configs.sonar_ping_delay=
          cf->ReadInt(section, "sonar_ping_delay",0);
  rflex_configs.sonar_set_delay=
          cf->ReadInt(section, "sonar_set_delay", 0);
  rflex_configs.mmrad_sonar_poses=(sonar_pose_t *) malloc(rflex_configs.num_sonars*sizeof(sonar_pose_t));
  for(x=0;x<rflex_configs.num_sonars;x++)
  {
    rflex_configs.mmrad_sonar_poses[x].x=
            cf->ReadTupleFloat(section, "mmrad_sonar_poses",3*x+1,0.0);
    rflex_configs.mmrad_sonar_poses[x].y=
            cf->ReadTupleFloat(section, "mmrad_sonar_poses",3*x+2,0.0);
    rflex_configs.mmrad_sonar_poses[x].t=
            cf->ReadTupleFloat(section, "mmrad_sonar_poses",3*x,0.0);
  }
  rflex_configs.sonar_2nd_bank_start=cf->ReadInt(section, "sonar_2nd_bank_start", 0);
  rflex_configs.sonar_1st_bank_end=rflex_configs.sonar_2nd_bank_start>0?rflex_configs.sonar_2nd_bank_start:rflex_configs.num_sonars;
  ////////////////////////////////////////////////////////////////////////
  // IR-related options

  int pose_count=cf->ReadInt(section, "pose_count",8);
  rflex_configs.ir_base_bank=cf->ReadInt(section, "rflex_base_bank",0);
  rflex_configs.ir_bank_count=cf->ReadInt(section, "rflex_bank_count",0);
  rflex_configs.ir_min_range=cf->ReadInt(section,"ir_min_range",100);
  rflex_configs.ir_max_range=cf->ReadInt(section,"ir_max_range",800); 
  rflex_configs.ir_count=new int[rflex_configs.ir_bank_count];
  rflex_configs.ir_a=new double[pose_count];
  rflex_configs.ir_b=new double[pose_count];
  rflex_configs.ir_poses.pose_count=pose_count;
  int RunningTotal = 0;
  for(int i=0; i < rflex_configs.ir_bank_count; ++i)
    RunningTotal += (rflex_configs.ir_count[i]=(int) cf->ReadTupleFloat(section, "rflex_banks",i,0));

  // posecount is actually unnecasary, but for consistancy will juse use it for error checking :)
  if (RunningTotal != rflex_configs.ir_poses.pose_count)
  {
    fprintf(stderr,"Error in config file, pose_count not equal to total poses in bank description\n");  
    rflex_configs.ir_poses.pose_count = RunningTotal;
  }		

  //  rflex_configs.ir_poses.poses=new int16_t[rflex_configs.ir_poses.pose_count];
  for(x=0;x<rflex_configs.ir_poses.pose_count;x++)
  {
    rflex_configs.ir_poses.poses[x][0]=(int) cf->ReadTupleFloat(section, "poses",x*3,0);
    rflex_configs.ir_poses.poses[x][1]=(int) cf->ReadTupleFloat(section, "poses",x*3+1,0);
    rflex_configs.ir_poses.poses[x][2]=(int) cf->ReadTupleFloat(section, "poses",x*3+2,0);

    // Calibration parameters for ir in form range=(a*voltage)^b
    rflex_configs.ir_a[x] = cf->ReadTupleFloat(section, "rflex_ir_calib",x*2,1);
    rflex_configs.ir_b[x] = cf->ReadTupleFloat(section, "rflex_ir_calib",x*2+1,1);	
  }

  ////////////////////////////////////////////////////////////////////////
  // Bumper-related options
  rflex_configs.bumper_count = cf->ReadInt(section, "bumper_count",0);
  rflex_configs.bumper_def = new player_bumper_define_t[rflex_configs.bumper_count];
  for(x=0;x<rflex_configs.bumper_count;++x)
  {
    rflex_configs.bumper_def[x].x_offset = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x,0)); //mm
    rflex_configs.bumper_def[x].y_offset = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+1,0)); //mm
    rflex_configs.bumper_def[x].th_offset = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+2,0)); //deg
    rflex_configs.bumper_def[x].length = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+3,0)); //mm
    rflex_configs.bumper_def[x].radius = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+4,0));	//mm  	
  }
  rflex_configs.bumper_address = cf->ReadInt(section, "rflex_bumper_address",DEFAULT_RFLEX_BUMPER_ADDRESS);


  const char *bumperStyleStr = cf->ReadString(section, "rflex_bumper_style",DEFAULT_RFLEX_BUMPER_STYLE);

  if(strcmp(bumperStyleStr,RFLEX_BUMPER_STYLE_BIT) == 0)
  {
    rflex_configs.bumper_style = BUMPER_BIT;
  }
  else if(strcmp(bumperStyleStr,RFLEX_BUMPER_STYLE_ADDR) == 0)
  {
    rflex_configs.bumper_style = BUMPER_ADDR;
  }
  else
  {
    //Invalid value
    rflex_configs.bumper_style = BUMPER_ADDR;
  }

  ////////////////////////////////////////////////////////////////////////
  // Power-related options
  rflex_configs.power_offset = cf->ReadInt(section, "rflex_power_offset",DEFAULT_RFLEX_POWER_OFFSET);

  rflex_fd = -1;
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

int 
RFLEX::Subscribe(player_device_id_t id)
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
      case PLAYER_BUMPER_CODE:
        this->bumper_subscriptions++;
        break;
      case PLAYER_IR_CODE:
        this->ir_subscriptions++;
        break;
    }
  }

  return(setupResult);
}

int 
RFLEX::Unsubscribe(player_device_id_t id)
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
      case PLAYER_BUMPER_CODE:
        assert(--this->bumper_subscriptions >= 0);
        break;
      case PLAYER_IR_CODE:
        assert(--this->ir_subscriptions >= 0);
        break;
    }
  }

  return(shutdownResult);
}

void 
RFLEX::Main()
{
  printf("Rflex Thread Started\n");

  //sets up connection, and sets defaults
  //configures sonar, motor acceleration, etc.
  if(initialize_robot()<0){
    fprintf(stderr,"ERROR, no connection to RFLEX established\n");
    exit(1);
  }
  reset_odometry();


  player_position_cmd_t command;
  unsigned char config[RFLEX_CONFIG_BUFFER_SIZE];

  static double mmPsec_speedDemand=0.0, radPsec_turnRateDemand=0.0;
  bool newmotorspeed, newmotorturn;

  int config_size;
  int i;
  int last_sonar_subscrcount = 0;
  int last_position_subscrcount = 0;
  int last_ir_subscrcount = 0;

  while(1)
  {
    // we want to turn on the sonars if someone just subscribed, and turn
    // them off if the last subscriber just unsubscribed.
    if(!last_sonar_subscrcount && this->sonar_subscriptions)
        rflex_sonars_on(rflex_fd);
    else if(last_sonar_subscrcount && !(this->sonar_subscriptions))
      rflex_sonars_off(rflex_fd);
    last_sonar_subscrcount = this->sonar_subscriptions;

    // we want to turn on the ir if someone just subscribed, and turn
    // it off if the last subscriber just unsubscribed.
    if(!last_ir_subscrcount && this->ir_subscriptions)
      rflex_ir_on(rflex_fd);
    else if(last_ir_subscrcount && !(this->ir_subscriptions))
      rflex_ir_off(rflex_fd);
    last_ir_subscrcount = this->ir_subscriptions;

    // we want to reset the odometry and enable the motors if the first 
    // client just subscribed to the position device, and we want to stop 
    // and disable the motors if the last client unsubscribed.

    //first user logged in
    if(!last_position_subscrcount && this->position_subscriptions)
    {
      //set drive defaults
      rflex_motion_set_defaults(rflex_fd);

      //make sure robot doesn't go anywhere
      rflex_stop_robot(rflex_fd,(int) (MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration)));

      //clear the buffers
      player_position_cmd_t position_cmd;
      memset(&position_cmd,0,sizeof(player_position_cmd_t));
      PutCommand(this->position_id,(unsigned char*)(&position_cmd), 
                 sizeof(position_cmd), NULL);
    }
    //last user logged out
    else if(last_position_subscrcount && !(this->position_subscriptions))
    {
      //make sure robot doesn't go anywhere
      rflex_stop_robot(rflex_fd,(int) (MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration)));
      // disable motor power
      rflex_brake_on(rflex_fd);
    }
    last_position_subscrcount = this->position_subscriptions;
    
    void* client;

    // check if there is a new sonar config
    if((config_size = GetConfig(this->sonar_id, &client, 
                                (void*)config, sizeof(config), NULL)))
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
            puts("Arg to sonar state change request wrong size; ignoring");
            if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_sonar_power_config_t sonar_config;
          sonar_config = *((player_sonar_power_config_t*)config);

          if(sonar_config.value==0)
            rflex_sonars_off(rflex_fd);
          else
            rflex_sonars_on(rflex_fd);

          if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        case PLAYER_SONAR_GET_GEOM_REQ:
          /* Return the sonar geometry. */

          if(config_size != 1)
          {
            puts("Arg get sonar geom is wrong size; ignoring");
            if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_sonar_geom_t geom;
          geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;
          geom.pose_count = htons((short) rflex_configs.sonar_1st_bank_end);
          for (i = 0; i < rflex_configs.sonar_1st_bank_end; i++)
          {
            geom.poses[i][0] = htons((short) rflex_configs.mmrad_sonar_poses[i].x);
            geom.poses[i][1] = htons((short) rflex_configs.mmrad_sonar_poses[i].y);
            geom.poses[i][2] = htons((short) RAD2DEG_CONV(rflex_configs.mmrad_sonar_poses[i].t));
          }

          if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_ACK, 
                      &geom, sizeof(geom), NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        default:
          puts("Sonar got unknown config request");
          if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }

    // check if there is a new sonar 2 config
    if((config_size = GetConfig(this->sonar_id_2, &client, 
                                (void*)config, sizeof(config), NULL)))
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
            puts("Arg to sonar state change request wrong size; ignoring");
            if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_sonar_power_config_t sonar_config;
          sonar_config = *((player_sonar_power_config_t*)config);

          if(sonar_config.value==0)
            rflex_sonars_off(rflex_fd);
          else
            rflex_sonars_on(rflex_fd);

          if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        case PLAYER_SONAR_GET_GEOM_REQ:
          /* Return the sonar geometry. */

          if(config_size != 1)
          {
            puts("Arg get sonar geom is wrong size; ignoring");
            if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_sonar_geom_t geom;
          geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;
          geom.pose_count = htons((short) rflex_configs.num_sonars - rflex_configs.sonar_2nd_bank_start);
          for (i = 0; i < rflex_configs.num_sonars - rflex_configs.sonar_2nd_bank_start; i++)
          {
            geom.poses[i][0] = htons((short) rflex_configs.mmrad_sonar_poses[i+rflex_configs.sonar_2nd_bank_start].x);
            geom.poses[i][1] = htons((short) rflex_configs.mmrad_sonar_poses[i+rflex_configs.sonar_2nd_bank_start].y);
            geom.poses[i][2] = htons((short) RAD2DEG_CONV(rflex_configs.mmrad_sonar_poses[i+rflex_configs.sonar_2nd_bank_start].t));
          }

          if(PutReply(this->sonar_id_2, client, PLAYER_MSGTYPE_RESP_ACK, 
                      &geom, sizeof(geom), NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        default:
          puts("Sonar got unknown config request");
          if(PutReply(this->sonar_id_2, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }


    // check if there is a new bumper config
    if((config_size = GetConfig(this->bumper_id, &client, 
                                (void*)config, sizeof(config), NULL)))
    {
      switch(config[0])
      {
        case PLAYER_BUMPER_GET_GEOM_REQ:
          /* Return the bumper geometry. */
          if(config_size != 1)
          {
            puts("Arg get bumper geom is wrong size; ignoring");
            if(PutReply(this->bumper_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          // Assemble geometry structure for sending
          player_bumper_geom_t geom;
          geom.subtype = PLAYER_BUMPER_GET_GEOM_REQ;
          geom.bumper_count = htons((short) rflex_configs.bumper_count);
          for (i = 0; i < rflex_configs.bumper_count; i++)
          {
            geom.bumper_def[i].x_offset = htons((short) rflex_configs.bumper_def[i].x_offset); //mm
            geom.bumper_def[i].y_offset = htons((short) rflex_configs.bumper_def[i].y_offset); //mm
            geom.bumper_def[i].th_offset = htons((short) rflex_configs.bumper_def[i].th_offset); //deg
            geom.bumper_def[i].length = htons((short) rflex_configs.bumper_def[i].length); //mm
            geom.bumper_def[i].radius = htons((short) rflex_configs.bumper_def[i].radius); //mm
          }

          // Send
          if(PutReply(this->bumper_id, client, PLAYER_MSGTYPE_RESP_ACK, 
                      &geom, sizeof(geom), NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

          // Arent any request other than geometry
        default:
          puts("Bumper got unknown config request");
          if(PutReply(this->bumper_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }

    // check if there is a new ir config
    if((config_size = GetConfig(this->ir_id, &client, 
                                (void*)config, sizeof(config), NULL)))
    {
      switch(config[0])
      {
        case PLAYER_IR_POSE_REQ:
          /* Return the ir geometry. */
          if(config_size != 1)
          {
            puts("Arg get bumper geom is wrong size; ignoring");
            if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
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
          if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_ACK, 
                      &geom, sizeof(geom), NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
        case PLAYER_IR_POWER_REQ:
          /* Return the ir geometry. */
          if(config_size != 1)
          {
            puts("Arg get ir geom is wrong size; ignoring");
            if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          if (config[1] == 0)
            rflex_ir_off(rflex_fd);
          else
            rflex_ir_on(rflex_fd);

          // Send
          if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

          // Arent any request other than geometry and power
        default:
          puts("Ir got unknown config request");
          if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }

    // check if there is a new position config
    if((config_size = GetConfig(this->position_id, &client, 
                                (void*)config, sizeof(config), NULL)))
    {
      switch(config[0])
      {
        case PLAYER_POSITION_SET_ODOM_REQ:
          if(config_size != sizeof(player_position_set_odom_req_t))
          {
            puts("Arg to odometry set requests wrong size; ignoring");
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_position_set_odom_req_t set_odom_req;
          set_odom_req = *((player_position_set_odom_req_t*)config);
          //in mm
          set_odometry((long) ntohl(set_odom_req.x),(long) ntohl(set_odom_req.y),(short) ntohs(set_odom_req.theta));

          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_ACK, NULL))
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
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_position_power_config_t power_config;
          power_config = *((player_position_power_config_t*)config);

          if(power_config.value==0)
            rflex_brake_on(rflex_fd);
          else
            rflex_brake_off(rflex_fd);

          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_ACK, NULL))
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
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_position_velocitymode_config_t velmode_config;
          velmode_config = *((player_position_velocitymode_config_t*)config);

          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        case PLAYER_POSITION_RESET_ODOM_REQ:
          /* reset position to 0,0,0: no args */
          if(config_size != sizeof(player_position_resetodom_config_t))
          {
            puts("Arg to reset position request is wrong size; ignoring");
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }
          reset_odometry();

          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        case PLAYER_POSITION_GET_GEOM_REQ:
          /* Return the robot geometry. */
          if(config_size != 1)
          {
            puts("Arg get robot geom is wrong size; ignoring");
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
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

          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_ACK, &geom, sizeof(geom), NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
        default:
          puts("Position got unknown config request");
          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }


    if(this->position_subscriptions || rflex_configs.use_joystick)
    {
      /* read the clients' commands from the common buffer */
      GetCommand(this->position_id,(unsigned char*)&command, 
                 sizeof(command), NULL);

      newmotorspeed = false;
      newmotorturn = false;
      //the long casts are necicary (ntohl returns unsigned - we need signed)
      if(mmPsec_speedDemand != (long) ntohl(command.xspeed))
      {
        newmotorspeed = true;
        mmPsec_speedDemand = (long) ntohl(command.xspeed);
      }
      if(radPsec_turnRateDemand != DEG2RAD_CONV((long) ntohl(command.yawspeed)))
      {
        newmotorturn = true;
        radPsec_turnRateDemand = DEG2RAD_CONV((long) ntohl(command.yawspeed));
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
      else if (command.type == 0)
      {
        rflex_set_velocity(rflex_fd,(long) MM2ARB_ODO_CONV(mmPsec_speedDemand),(long) RAD2ARB_ODO_CONV(radPsec_turnRateDemand),(long) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration));    
        command.type = 255;
        PutCommand(this->position_id,(unsigned char *)&command, 
                   sizeof(command), NULL);
      }
    }
    else
      rflex_stop_robot(rflex_fd,(long) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration));

    /* Get data from robot */
    player_rflex_data_t rflex_data;
    memset(&rflex_data,0,sizeof(player_rflex_data_t));
    update_everything(&rflex_data);
    pthread_testcancel();

    PutData(this->position_id,
            (void*)&rflex_data.position,
            sizeof(player_position_data_t),
            NULL);
    PutData(this->sonar_id,
            (void*)&rflex_data.sonar,
            sizeof(player_sonar_data_t),
            NULL);
    PutData(this->sonar_id_2,
            (void*)&rflex_data.sonar2,
            sizeof(player_sonar_data_t),
            NULL);
    PutData(this->ir_id,
            (void*)&rflex_data.ir,
            sizeof(player_ir_data_t),
            NULL);
    PutData(this->bumper_id,
            (void*)&rflex_data.bumper,
            sizeof(player_bumper_data_t),
            NULL);
    PutData(this->power_id,
            (void*)&rflex_data.power,
            sizeof(player_power_data_t),
            NULL);
    PutData(this->aio_id,
            (void*)&rflex_data.aio,
            sizeof(player_aio_data_t),
            NULL);
    PutData(this->dio_id,
            (void*)&rflex_data.dio,
            sizeof(player_dio_data_t),
            NULL);

    pthread_testcancel();
  }
  pthread_exit(NULL);
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

void RFLEX::update_everything(player_rflex_data_t* d)
{

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
  if(this->sonar_subscriptions)
  {
    // TODO - currently bad sonar data is sent back to clients 
    // (not enough data buffered, so sonar sent in wrong order - missing intermittent sonar values - fix this
    a_num_sonars=rflex_configs.num_sonars;

    pthread_testcancel();
    rflex_update_sonar(rflex_fd, a_num_sonars,
		       arb_ranges);
    pthread_testcancel();
    d->sonar.range_count=htons(rflex_configs.sonar_1st_bank_end);
    for (i = 0; i < rflex_configs.sonar_1st_bank_end; i++){
      d->sonar.ranges[i] = htons((uint16_t) ARB2MM_RANGE_CONV(arb_ranges[i]));
    }
    d->sonar2.range_count=htons(rflex_configs.num_sonars - rflex_configs.sonar_2nd_bank_start);
    for (i = 0; i < rflex_configs.num_sonars - rflex_configs.sonar_2nd_bank_start; i++){
      d->sonar2.ranges[i] = htons((uint16_t) ARB2MM_RANGE_CONV(arb_ranges[rflex_configs.sonar_2nd_bank_start+i]));
    }
  }

  // if someone is subscribed to bumpers copy internal data to device
  if(this->bumper_subscriptions)
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
  if(this->ir_subscriptions)
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
}



