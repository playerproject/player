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
// and sonar objects
#include <drivertable.h>
#include <devicetable.h>
extern DriverTable* deviceTable;


class NomadSonar:public Driver 
{
  public:

  NomadSonar( ConfigFile* cf, int section);
  virtual ~NomadSonar();
  
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
Driver* NomadSonar_Init( ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_SONAR_STRING))
    {
      PLAYER_ERROR1("driver \"nomad_sonar\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else
    return((Driver*)(new NomadSonar(interface, cf, section)));
}

// a driver registration function
void NomadSonar_Register(DriverTable* table)
{
  table->AddDriver( "nomad_sonar", PLAYER_ALL_MODE, NomadSonar_Init);
}




NomadSonar::NomadSonar( ConfigFile* cf, int section)
  : Driver(cf, section,  sizeof(player_sonar_data_t), 0, 1, 1 )
{
  this->nomad_id.code = PLAYER_NOMAD_CODE;
  this->nomad_id.port = cf->ReadInt(section, "nomad_port", 0 );
  this->nomad_id.index = cf->ReadInt(section, "nomad_index", 0 );
}

NomadSonar::~NomadSonar()
{
  puts( "Destroying NomadSonar driver" );
}

int NomadSonar::Setup()
{
  printf("NomadSonar Setup.. ");
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

  puts( "NomadSonar setup done" );
  return(0);
}

int NomadSonar::Shutdown()
{
  StopThread(); 

  // Unsubscribe from the laser device
  this->nomad->Unsubscribe(this);

  puts("NomadSonar has been shutdown");
  return(0);
}

void NomadSonar::Update()
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
	case PLAYER_SONAR_GET_GEOM_REQ:
	  {
	    /* Return the sonar geometry. */
	    if(config_size != 1)
	      {
		puts("Arg get sonar geom is wrong size; ignoring");
		if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, 
			    NULL, NULL, 0))
		  PLAYER_ERROR("failed to PutReply");
		break;
	      }
	    
	    double interval = (M_PI*2.0)/NOMAD_SONAR_COUNT;
	    double radius = NOMAD_RADIUS_MM;
	    
	    player_sonar_geom_t geom;
	    memset(&geom,0,sizeof(geom));
	    geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;
	    geom.pose_count = htons((uint16_t)NOMAD_SONAR_COUNT);
	    for (int i = 0; i < NOMAD_SONAR_COUNT; i++)
	      {
		double angle = interval * i;
		geom.poses[i][0] = htons((int16_t)rint(radius*cos(angle)));
		geom.poses[i][1] = htons((int16_t)rint(radius*sin(angle)));
		geom.poses[i][2] = htons((int16_t)RTOD(angle));
	      }
	    
	    puts( "returned Nomad sonar geometry" );
	    
	    if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, 
			sizeof(geom)))
	      PLAYER_ERROR("failed to PutReply");
	  }
	  break;
	default:
	  puts("Sonar got unknown config request");
	  if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK,
		      NULL, NULL, 0))
	    PLAYER_ERROR("failed to PutReply");
	  break;
	}
    }
  
  // no commads to the sonar
}


void 
NomadSonar::Main()
{  
  player_nomad_data_t nomad_data;
  
  for(;;)
    {
      // Wait for new data from the Nomad driver
      this->nomad->Wait();
      
      // Get the Nomad data.
      size_t len = this->nomad->
	GetData(this, (uint8_t*)&nomad_data, sizeof(nomad_data), NULL, NULL );
      
      assert( len == sizeof(nomad_data) );
      
      // extract the sonar data from the Nomad packet
      player_sonar_data_t player_data;
      memset(&player_data,0,sizeof(player_data));
      
      player_data.range_count = ntohs((uint16_t)NOMAD_SONAR_COUNT);
  
      memcpy( &player_data.ranges, 
	      &nomad_data.sonar, 
	      NOMAD_SONAR_COUNT * sizeof(uint16_t) );
      
      PutData((uint8_t*)&player_data, sizeof(player_data), 0,0 );
    }
  pthread_exit(NULL);
}


