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
 * Driver for the Nomadics Nomad 200 robot. Should be easily adapted for other Nomads.
 * Authors: Richard Vaughan (vaughan@sfu.ca)
 * Based on Brian Gerkey et al's P2OS driver.
 * 
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

#include "mother.h"

#include <playertime.h>
extern PlayerTime* GlobalTime;

// so we can access the deviceTable and extract pointers to the sonar
// and position objects
#include <drivertable.h>
#include <devicetable.h>
extern CDeviceTable* deviceTable;
extern int global_playerport; // used to get at devices

#define NOMAD_DEFAULT_SERIAL_PORT "/dev/ttyS0"
#define NOMAD_SONAR_COUNT 16
#define NOMAD_RADIUS_MM 400 // TODO: measure the Nomad to get this exactly right
#define NOMAD_CONFIG_BUFFER_SIZE 256 // this should be bigger than the biggest config we could receive

class Nomad:public CDevice 
{
  // did we initialize the common data segments yet?
  static bool initdone;

  public:

  Nomad(char* interface, ConfigFile* cf, int section);
  virtual ~Nomad();
  
  /* the main thread */
  virtual void Main();
  
  virtual int Setup();
  virtual int Shutdown();

};

// declare the static members (this is an ugly requirement of C++ )

bool Nomad::initdone = FALSE;

// a factory creation function
CDevice* Nomad_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
    {
      PLAYER_ERROR1("driver \"nomad\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else
    return((CDevice*)(new Nomad(interface, cf, section)));
}

// a driver registration function
void Nomad_Register(DriverTable* table)
{
  table->AddDriver("nomad", PLAYER_ALL_MODE, Nomad_Init);
}




Nomad::Nomad(char* interface, ConfigFile* cf, int section)
  : CDevice( sizeof(player_position_data_t), 
	     sizeof(player_position_cmd_t), 1, 1 )
{
  if(!initdone)
    {
      initdone = TRUE;
    }
  else
    {
      
      
    }
  
}

Nomad::~Nomad()
{
  puts( "Destroying Nomad driver" );
}

int Nomad::Setup()
{
  printf("Nomad Setup..");
  fflush(stdout);
  
  connectToRobot();
  initRobot();

  /* now spawn reading thread */
  StartThread();

	 puts( "done" );
  return(0);
}

int Nomad::Shutdown()
{
  StopThread();
  puts("Nomad has been shutdown");
  return(0);
}


void 
Nomad::Main()
{  
  unsigned char config[NOMAD_CONFIG_BUFFER_SIZE];
  
  for(;;)
    {
      void* client;
      player_device_id_t id;
      size_t config_size = 0;
      //player_position_data_t

      // first, check if there is a new config command
      if((config_size = GetConfig(&id, &client, (void*)config, sizeof(config))))
	{
	  switch(id.code)
	    {
	    case PLAYER_SONAR_CODE:
	      switch(config[0])
		{
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
		  geom.pose_count = NOMAD_SONAR_COUNT;
		  for (int i = 0; i < PLAYER_SONAR_MAX_SAMPLES; i++)
		    {
		      // todo - get the geometry properly
		      geom.poses[i][0] = htons((short) 0);
		      geom.poses[i][1] = htons((short) 0);
		      geom.poses[i][2] = htons((short) 360/NOMAD_SONAR_COUNT * i);
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
		geom.pose[0] = htons((short) (0)); // x offset
		geom.pose[1] = htons((short) (0)); // y offset
		geom.pose[2] = htons((short) (0)); // a offset
		geom.size[0] = htons((short) (2 * NOMAD_RADIUS_MM )); // x size
		geom.size[1] = htons((short) (2 * NOMAD_RADIUS_MM )); // y size
		
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
	      printf("Nomad: got unknown config request \"%c\"\n",
		     config[0]);
	      
	      if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
		PLAYER_ERROR("failed to PutReply");
	      break;
	    }
	}
      
      /* read the latest data from the robot */
      //printf( "read data from robot" );
      readRobot();


      /* read the latest Player client commands */
      player_position_cmd_t command;
      GetCommand((unsigned char*)&command, sizeof(command));
      
      /* write the command to the robot */      
      int v = ntohl(command.xspeed);
      int w = ntohl(command.yawspeed);
      int turret = ntohl(command.yspeed);
      printf( "command: v:%d w:%d turret:%d\n", v,w,turret ); 
      setSpeed( v, w, turret );
      //setSpeed( 0, v );

      player_position_data_t pos;
      memset(&pos,0,sizeof(pos));

      pos.xpos = xPos();
      pos.ypos = yPos();
      pos.yaw = theta();
      pos.xspeed = speed();
      pos.yawspeed = turnrate();
      
      pos.xpos = htonl(pos.xpos);
      pos.ypos = htonl(pos.ypos);
      pos.yaw = htonl(pos.yaw);
      pos.xspeed = htonl(pos.xspeed);
      //pos.yspeed = htonl(pos.yspeed); 
      pos.yawspeed = htonl(pos.yawspeed);

      PutData((uint8_t*)&pos, sizeof(pos), 0,0 );
      
      usleep(1);

    }
  pthread_exit(NULL);
}


