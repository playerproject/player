/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  Brian Gerkey gerkey@usc.edu
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "segwayio.h"

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"


// copied from can.h
#define CAN_MSG_LENGTH 8

struct canmsg_t 
{       
  short           flags;
  int             cob;
  unsigned long   id;
  unsigned long   timestamp;
  unsigned int    length;
  char            data[CAN_MSG_LENGTH];
} __attribute__ ((packed));



// Driver for robotic Segway
class SegwayRMP : public CDevice
{
  public: 
    // Constructor	  
    SegwayRMP(char* interface, ConfigFile* cf, int section);
  ~SegwayRMP();

    // Setup/shutdown routines.
    virtual int Setup();
    virtual int Shutdown();

  private: 

  SegwayIO *segway;
  
  bool motor_enabled;

  // Main function for device thread.
  virtual void Main();


};

// Initialization function
CDevice* SegwayRMP_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_POSITION_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"segwayrmp\" does not support interface \"%s\"\n",
                  interface);
    return (NULL);
  }
  return ((CDevice*) (new SegwayRMP(interface, cf, section)));
}


// a driver registration function
void SegwayRMP_Register(DriverTable* table)
{
  table->AddDriver("segwayrmp", PLAYER_ALL_MODE, SegwayRMP_Init);
}

SegwayRMP::SegwayRMP(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_position_data_t), 
              sizeof(player_position_cmd_t), 10, 10)
{
  segway = new SegwayIO();
}

SegwayRMP::~SegwayRMP()
{
  delete segway;
}

int
SegwayRMP::Setup()
{
  // setup the CAN stuff
  int ret;
  if ((ret = segway->Init()) < 0) {
    PLAYER_ERROR1("SEGWAY: error on segway init (%d)\n", ret);
    return -1;
  }

  StartThread();
  return(0);
}

int
SegwayRMP::Shutdown()
{
  int ret;
  if ((ret = segway->Shutdown()) < 0) {
    PLAYER_ERROR1("SEGWAY: error on canio shutdown (%d)\n", ret);
    return -1;
  }

  StopThread();

  return(0);
}

// Main function for device thread.
void 
SegwayRMP::Main()
{
  char buffer[256];
  player_position_data_t data;
  player_position_cmd_t cmd;
  uint16_t rmp_cmd,rmp_val;
  player_rmp_config_t *rmp;
  void *client;

  while (1) {
    
    // check for config requests
    if (GetConfig(&client, (void *)buffer, sizeof(buffer))) {
      switch (buffer[0]) {
      case PLAYER_POSITION_MOTOR_POWER_REQ:
	// just set a flag telling us whether we should
	// act on motor commands
	// set the commands to 0... think it will automatically
	// do this for us.  
	if (buffer[1]) {
	  motor_enabled = true;
	} else {
	  motor_enabled = false;
	}

	if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		     NULL, NULL, 0)) {
	  PLAYER_ERROR("Failed to PutReply in segwayrmp\n");
	}
	break;

      case PLAYER_POSITION_RESET_ODOM_REQ:
	// we'll reset all the integrators

	segway->StatusCommand((uint16_t)RMP_CAN_CMD_RST_INT, 
			      (uint16_t)RMP_CAN_RST_ALL);

	if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		     NULL, NULL, 0)) {
	  PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
	}
	break;

      case PLAYER_POSITION_RMP_VELOCITY_SCALE:
	rmp_cmd = RMP_CAN_CMD_MAX_VEL;
	rmp = (player_rmp_config_t *)buffer;
	rmp_val = ntohs(rmp->value);

	printf("SEGWAYRMP: velocity scale %d\n", rmp->value);
	
	segway->StatusCommand(rmp_cmd, rmp_val);

	if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		     NULL, NULL, 0)) {
	  PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
	}

	break;

      case PLAYER_POSITION_RMP_ACCEL_SCALE:
	rmp_cmd = RMP_CAN_CMD_MAX_ACCL;
	rmp = (player_rmp_config_t *)buffer;
	rmp_val = ntohs(rmp->value);

	segway->StatusCommand(rmp_cmd, rmp_val);

	if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		     NULL, NULL, 0)) {
	  PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
	}

	break;

      case PLAYER_POSITION_RMP_TURN_SCALE:
	rmp_cmd = RMP_CAN_CMD_MAX_TURN;
	rmp = (player_rmp_config_t *)buffer;
	rmp_val = ntohs(rmp->value);

	segway->StatusCommand(rmp_cmd, rmp_val);

	if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		     NULL, NULL, 0)) {
	  PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
	}

	break;

      case PLAYER_POSITION_RMP_GAIN_SCHEDULE:
	rmp_cmd = RMP_CAN_CMD_GAIN_SCHED;
	rmp = (player_rmp_config_t *)buffer;
	rmp_val = ntohs(rmp->value);

	segway->StatusCommand(rmp_cmd, rmp_val);

	if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		     NULL, NULL, 0)) {
	  PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
	}

	break;

      case PLAYER_POSITION_RMP_CURRENT_LIMIT:
	rmp_cmd = RMP_CAN_CMD_CURR_LIMIT;
	rmp = (player_rmp_config_t *)buffer;
	rmp_val = ntohs(rmp->value);

	segway->StatusCommand(rmp_cmd, rmp_val);

	if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		     NULL, NULL, 0)) {
	  PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
	}

	break;

      case PLAYER_POSITION_RMP_RST_INTEGRATORS:
	rmp_cmd = RMP_CAN_CMD_RST_INT;
	rmp = (player_rmp_config_t *)buffer;
	rmp_val = ntohs(rmp->value);

	segway->StatusCommand(rmp_cmd, rmp_val);

	if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		     NULL, NULL, 0)) {
	  PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
	}

	break;
	
      default:
	printf("segwayrmp received unknown config request %d\n", 
	       buffer[0]);
	if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK, 
		     NULL, NULL, 0)) {
	  PLAYER_ERROR("Failed to PutReply in segwayrmp\n");
	}
	break;
      }
    }

    // Get a new command
    GetCommand((unsigned char *)&cmd, sizeof(cmd));

    if(motor_enabled) {
      // do a motor command
      // segway wants it in host order
      cmd.xspeed = ntohl(cmd.xspeed);

      if (cmd.xspeed > RMP_MAX_TRANS_VEL_MM_S) {
	cmd.xspeed = RMP_MAX_TRANS_VEL_MM_S;
      } else if (cmd.xspeed < -RMP_MAX_TRANS_VEL_MM_S) {
	cmd.xspeed = -RMP_MAX_TRANS_VEL_MM_S;
      }

      cmd.yawspeed = ntohl(cmd.yawspeed);

      if (cmd.yawspeed > RMP_MAX_ROT_VEL_DEG_S) {
	cmd.yawspeed = RMP_MAX_ROT_VEL_DEG_S;
      } else if (cmd.yawspeed < -RMP_MAX_ROT_VEL_DEG_S) {
	cmd.yawspeed = -RMP_MAX_ROT_VEL_DEG_S;
      }

    } else {
      cmd.xspeed = 0;
      cmd.yawspeed = 0;
    }

    // write a velocity command... could be zero
    segway->VelocityCommand(cmd);

    // get new RMP data
    segway->GetData(&data);

    data.xpos = htonl(data.xpos);
    data.ypos = htonl(data.ypos);
    data.yaw = htonl(data.yaw);
    data.xspeed = htonl(data.xspeed);
    data.yspeed = htonl(data.yspeed);
    data.yawspeed = htonl(data.yawspeed);
    
    // now give this data to clients
    PutData((unsigned char *)&data, sizeof(data), 0, 0);
  }

  // not reached
  pthread_exit(NULL);
}
