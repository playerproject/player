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

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_nomad_position nomad_position

The nomad_position driver controls the wheelbase of the Nomadics
NOMAD200 robot.  This driver is a thin wrapper that exchanges data and
commands with the @ref player_driver_nomad driver; look there for more
information and an example.

@par Compile-time dependencies

- none

@par Provides

- @ref player_interface_position

@par Requires

- @ref player_interface_nomad

@par Configuration requests

- PLAYER_POSITION_GET_GEOM_REQ
  
@par Configuration file options

- none


@par Authors

Richard Vaughan, Pawel Zebrowski

*/
/** @} */


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
#include "error.h"
#include "nomad.h"

class NomadPosition:public Driver 
{
  public:

  NomadPosition( ConfigFile* cf, int section);
  virtual ~NomadPosition();
  
  /* the main thread */
  virtual void Main();
  
  virtual int Setup();
  virtual int Shutdown();
  virtual void Update();
  
protected:
  Driver* nomad;
  player_device_id_t nomad_id;
  
};

// a factory creation function
Driver* NomadPosition_Init( ConfigFile* cf, int section)
{
  return((Driver*)(new NomadPosition( cf, section)));
}

// a driver registration function
void NomadPosition_Register(DriverTable* table)
{
  table->AddDriver( "nomad_position",  NomadPosition_Init);
}




NomadPosition::NomadPosition( ConfigFile* cf, int section)
  : Driver(cf, section,  PLAYER_POSITION_CODE, PLAYER_ALL_MODE,
           sizeof(player_position_data_t), 
           sizeof(player_position_cmd_t), 1, 1 )
{
  // Must have a nomad
  if (cf->ReadDeviceId(&this->nomad_id, section, "requires",
                       PLAYER_NOMAD_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);    
    return;
  }
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
  this->nomad = deviceTable->GetDriver(nomad_id);

  if(!this->nomad)
  {
    PLAYER_ERROR("unable to find nomad device");
    return(-1);
  }
  
  else printf( " OK.\n" );
 
  // Subscribe to the nomad device, but fail if it fails
  if(this->nomad->Subscribe(this->nomad_id) != 0)
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
  this->nomad->Unsubscribe(this->nomad_id);

  puts("NomadPosition has been shutdown");
  return(0);
}

void NomadPosition::Update()
{
  unsigned char config[NOMAD_CONFIG_BUFFER_SIZE];
  
  void* client;
  size_t config_size = 0;
  
  // first, check if there is a new config command
  if((config_size = GetConfig(&client, (void*)config, sizeof(config),NULL)))
    {
      switch(config[0])
	{
	case PLAYER_POSITION_GET_GEOM_REQ:
	  {
	    /* Return the robot geometry. */
	    if(config_size != 1)
	      {
		puts("Arg get robot geom is wrong size; ignoring");
		if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL))
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
	    
	    if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, 
                        &geom, sizeof(geom),NULL))
	      PLAYER_ERROR("failed to PutReply");
	    break;
	  }
	default:
	  puts("Position got unknown config request");
	  if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL))
	    PLAYER_ERROR("failed to PutReply");
	  break;
	}
    }
  
  /* read the latest Player client commands */
  player_position_cmd_t command;
  if( GetCommand((void*)&command, sizeof(command),NULL) )
    {
      // consume the command
      this->ClearCommand();
      
      // convert from the generic position interface to the
      // Nomad-specific command
      player_nomad_cmd_t cmd;
      memset( &cmd, 0, sizeof(cmd) );
      cmd.vel_trans = (command.xspeed);
      cmd.vel_steer = (command.yawspeed);
      cmd.vel_turret = (command.yspeed);
      
      // command the Nomad device
      this->nomad->PutCommand(this->device_id, (void*)&cmd, sizeof(cmd),NULL); 
    }
}


void 
NomadPosition::Main()
{  
  player_nomad_data_t nomad_data;
  //struct timeval tv;

  for(;;)
    {
      // Wait for new data from the Nomad driver
      this->nomad->Wait();
      
      // Get the Nomad data.
      size_t len = this->nomad->GetData(this->nomad_id,(void*)&nomad_data, 
                                        sizeof(nomad_data), NULL);
      
      assert( len == sizeof(nomad_data) );
      
      // extract the position data from the Nomad packet
      player_position_data_t pos;
      memset(&pos,0,sizeof(pos));
      
      pos.xpos = nomad_data.x;
      pos.ypos = nomad_data.y;
      pos.yaw = nomad_data.a;
      pos.xspeed = nomad_data.vel_trans;
      pos.yawspeed = nomad_data.vel_steer;
      
      PutData((void*)&pos, sizeof(pos), NULL);
    }
  pthread_exit(NULL);
}


