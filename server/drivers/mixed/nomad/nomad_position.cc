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


// so we can access the deviceTable and extract pointers to the sonar
// and position objects
#include <drivertable.h>
#include <devicetable.h>
extern CDeviceTable* deviceTable;

class NomadPosition:public CDevice 
{
  public:

  NomadPosition(char* interface, ConfigFile* cf, int section);
  virtual ~NomadPosition();
  
  /* the main thread */
  virtual void Main();
  
  virtual int Setup();
  virtual int Shutdown();
  virtual void Update();
  
protected:
  CDevice* nomad;
  player_device_id_t nomad_id;
  
};

// a factory creation function
CDevice* NomadPosition_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
    {
      PLAYER_ERROR1("driver \"nomad_position\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else
    return((CDevice*)(new NomadPosition(interface, cf, section)));
}

// a driver registration function
void NomadPosition_Register(DriverTable* table)
{
  table->AddDriver( "nomad_position", PLAYER_ALL_MODE, NomadPosition_Init);
}




NomadPosition::NomadPosition(char* interface, ConfigFile* cf, int section)
  : CDevice( sizeof(player_position_data_t), 
	     sizeof(player_position_cmd_t), 1, 1 )
{
  this->nomad_id.code = PLAYER_NOMAD_CODE;
  this->nomad_id.port = cf->ReadInt(section, "nomad_port", 0 );
  this->nomad_id.index = cf->ReadInt(section, "nomad_index", 0 );
}

NomadPosition::~NomadPosition()
{
  puts( "Destroying NomadPosition driver" );
}

int NomadPosition::Setup()
{
  printf("NomadPosition Setup.. ");
  fflush(stdout);
  
  // if we didn't specify a port for the nomad, use the same port as
  // this device
  if( this->nomad_id.port == 0 )
    this->nomad_id.port = device_id.port;
  
  printf( "finding Nomad (%d:%d:%d).. ",
	  this->nomad_id.port,
	  this->nomad_id.code,
	  this->nomad_id.index ); fflush(stdout);

  // get the pointer to the Nomad
  this->nomad = deviceTable->GetDevice(nomad_id);

  if(!this->nomad)
  {
    PLAYER_ERROR("unable to find nomad device");
    return(-1);
  }
  
  else printf( " OK.\n" );
 
  // Subscribe to the nomad device, but fail if it fails
  if(this->nomad->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to nomad device");
    return(-1);
  }

  /* now spawn reading thread */
  StartThread();

  puts( "NomadPosition setup done" );
  return(0);
}

int NomadPosition::Shutdown()
{
  StopThread(); 

  // Unsubscribe from the laser device
  this->nomad->Unsubscribe(this);

  puts("NomadPosition has been shutdown");
  return(0);
}

void NomadPosition::Update()
{
  unsigned char config[NOMAD_CONFIG_BUFFER_SIZE];
  
  void* client;
  player_device_id_t id;
  size_t config_size = 0;
  
  // first, check if there is a new config command
  if((config_size = GetConfig(&id, &client, (void*)config, sizeof(config))))
    {
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
    }
  
  /* read the latest Player client commands */
  player_position_cmd_t command;
  if( GetCommand((unsigned char*)&command, sizeof(command)) )
    {
      // consume the command
      device_used_commandsize = 0;
      
      // convert from the generic position interface to the
      // Nomad-specific command
      player_nomad_cmd_t cmd;
      memset( &cmd, 0, sizeof(cmd) );
      cmd.vel_trans = ntohl(command.xspeed);
      cmd.vel_steer = ntohl(command.yawspeed);
      cmd.vel_turret = ntohl(command.yspeed);
      
      printf( "nomad command: trans:%d steer:%d turret:%d\n", 
	      cmd.vel_trans, cmd.vel_steer, cmd.vel_turret ); 
      
      // command the Nomad device
      // TODO: is this the right syntax?
      this->nomad->PutCommand( this, (unsigned char*)&cmd, sizeof(cmd) ); 
    }
}


void 
NomadPosition::Main()
{  
  player_nomad_data_t nomad_data;
  struct timeval tv;

  for(;;)
    {
      // Wait for new data from the Nomad driver
      this->nomad->Wait();
      
      // Get the Nomad data.
      size_t len = this->nomad->
	GetData(this, (uint8_t*)&nomad_data, sizeof(nomad_data), NULL, NULL );
      
      assert( len == sizeof(nomad_data) );
      
      // extract the position data from the Nomad packet
      player_position_data_t pos;
      memset(&pos,0,sizeof(pos));
      
      pos.xpos = nomad_data.x;
      pos.ypos = nomad_data.y;
      pos.yaw = nomad_data.a;
      pos.xspeed = nomad_data.vel_trans;
      pos.yawspeed = nomad_data.vel_steer;
      
      PutData((uint8_t*)&pos, sizeof(pos), 0,0 );
    }
  pthread_exit(NULL);
}


