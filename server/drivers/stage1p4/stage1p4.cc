/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
 *
 *  Audiodevice attempted written by Esben Ostergaard
 *
 * This program is free software; you can redistribute it and/or modify
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
#define PLAYER_ENABLE_TRACE 1
#define PLAYER_ENABLE_MSG 1

#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/soundcard.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h> 
#include <unistd.h>

#include <pthread.h>


#include <device.h>
#include <drivertable.h>

#include "stage.h" // from the Stage distro

#define DEFAULT_STG_HOST "localhost"
#define DEFAULT_STG_WORLDFILE "default.world"


// BASE CLASS FOR ALL STAGE-1.4 DRIVERS //////////////////////////////////////////////
// creates a single static Stage client shared by all derived drivers

class Stage1p4:public CDevice
{
public:
  Stage1p4(char* interface, ConfigFile* cf, int section, 
	   size_t datasz, size_t cmdsz, int rqlen, int rplen) :
    CDevice(datasz,cmdsz,rqlen,rplen)
  {
    PLAYER_TRACE1( "Stage1p4 device created for interface %s\n", interface );

    if( this->stage_client == NULL )
      {
	this->world_file = (char*)cf->ReadString(section, "worldfile", DEFAULT_STG_WORLDFILE);
	int stage_port = cf->ReadInt(section, "port", STG_DEFAULT_SERVER_PORT);
	char* stage_host = (char*)cf->ReadString(section, "host", DEFAULT_STG_HOST);

	this->stage_client = this->CreateStageClient(stage_host, stage_port, world_file);
      }
  }
  
  virtual ~Stage1p4()
  {
    if( this->stage_client )
      this->DestroyStageClient( this->stage_client );
  }

  virtual void Main();
  virtual int Setup();
  virtual int Shutdown();

private:
  // this gets called once only for each derived instantiation
  // to create a connection to Stage
  stg_client_t* CreateStageClient(char* host, int port, char* world);
  int DestroyStageClient( stg_client_t* cli );

  static stg_client_t* stage_client;
  static char* world_file;
};

// init static vars
char* Stage1p4::world_file = DEFAULT_STG_WORLDFILE;
stg_client_t* Stage1p4::stage_client = NULL;

// methods start

stg_client_t* Stage1p4::CreateStageClient( char* host, int port, char* world )
{
  PLAYER_MSG2( "Creating client to Stage server on %s:%d", host, port );

  stg_client_t* cli = stg_client_create( host, port );
  assert( cli );

  PLAYER_MSG1( "Uploading world from \"%s\"", world );

  stg_world_create_t world_cfg;
  strncpy( world_cfg.name, "test world", STG_TOKEN_MAX );
  world_cfg.width = 10.0;
  world_cfg.height = 10.0;
  world_cfg.resolution = 0.05; 
  stg_id_t root = stg_world_create( cli, &world_cfg );

  /*strncpy( world_cfg.name, "test world 2", STG_TOKEN_MAX );
  world_cfg.width = 20.0;
  world_cfg.height = 10.0;
  world_cfg.resolution = 0.05; 
  stg_id_t root2 = stg_world_create( cli, &world_cfg );
  */

  /*  
  stg_entity_create_t child;
  strncpy( child.name, "banana", STG_TOKEN_MAX );
  strncpy( child.token, "monkey", STG_TOKEN_MAX );
  strncpy( child.color, "green", STG_TOKEN_MAX );
  child.type = 0;
  child.parent_id = root; // make a new entity on the root 
  stg_id_t banana = stg_model_create( cli, &child );

  sleep(1);
  
  strncpy( child.name, "alien", STG_TOKEN_MAX );
  strncpy( child.token, "monkey", STG_TOKEN_MAX );
  strncpy( child.color, "purple", STG_TOKEN_MAX );
  child.type = 0;
  child.parent_id = banana; // make a new entity on the root 
  stg_id_t alien = stg_model_create( cli, &child );

  stg_size_t sz;
  sz.x = 0.25;
  sz.y = 0.15;
  stg_model_set_size( cli, alien, &sz );

  stg_velocity_t vel;
  vel.x = 0.0;
  vel.y = 0.0;
  vel.a = 0.3;
  stg_model_set_velocity( cli, banana, &vel );

  vel.x = 0.0;
  vel.y = 0.0;
  vel.a = -0.6;

  stg_model_set_velocity( cli, alien, &vel );

  sleep(10);
  
  stg_model_destroy( cli, alien );  

  */

  return cli;
}


int Stage1p4::DestroyStageClient( stg_client_t* cli )
{
  stg_client_free( cli );
  return 0;
}


int 
Stage1p4::Setup()
{
  StartThread();
  return 0;
}

/* main thread */
void 
Stage1p4::Main()
{
    while(1) 
      {
	pthread_testcancel();
	
	sleep(1);
	//PutData(&data, sizeof(data),0,0);
      }
}

int 
Stage1p4::Shutdown()
{
  StopThread();
  return 0;
}

// END BASE CLASS

// DRIVER FOR LASER INTERFACE ////////////////////////////////////////////////////////

class StgLaser:public Stage1p4
{
public:
  StgLaser(char* interface, ConfigFile* cf, int section );
};

StgLaser::StgLaser(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, sizeof(player_laser_data_t), 0, 1, 1 )
{
  PLAYER_TRACE1( "constructing StgLaser with interface %s", interface );
}

CDevice* StgLaser_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_LASER_STRING))
    {
      PLAYER_ERROR1("driver \"stg_laser\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgLaser(interface, cf, section)));
}


void StgLaser_Register(DriverTable* table)
{
  table->AddDriver("stg_laser", PLAYER_ALL_MODE, StgLaser_Init);
}

// DRIVER FOR POSITION INTERFACE //////////////////////////////////////////////////////

class StgPosition:public Stage1p4
{
public:
  StgPosition(char* interface, ConfigFile* cf, int section );
};

StgPosition::StgPosition(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, 
	      sizeof(player_position_data_t), sizeof(player_position_cmd_t), 1, 1 )
{
  PLAYER_TRACE1( "constructing StgPosition with interface %s", interface );
}

CDevice* StgPosition_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
    {
      PLAYER_ERROR1("driver \"stg_position\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgPosition(interface, cf, section)));
}
	      
void StgPosition_Register(DriverTable* table)
{
  table->AddDriver("stg_position", PLAYER_ALL_MODE, StgPosition_Init);
}

// DRIVER FOR SIMULATION INTERFACE //////////////////////////////////////////////////////

class StgSimulation:public Stage1p4
{
public:
  StgSimulation(char* interface, ConfigFile* cf, int section);
};

StgSimulation::StgSimulation(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, 
	      sizeof(player_simulation_data_t), 
	      sizeof(player_simulation_cmd_t), 1, 1 )
  {
    PLAYER_MSG1( "constructing StgSimulation with interface %s", interface );
  }
	      
CDevice* StgSimulation_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_SIMULATION_STRING))
    {
      PLAYER_ERROR1("driver \"stg_simulation\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgSimulation(interface, cf, section)));
}


void StgSimulation_Register(DriverTable* table)
{
  table->AddDriver("stg_simulation", PLAYER_ALL_MODE, StgSimulation_Init);
}
//////////////////////////////////////////////////////////////////////////////////////


