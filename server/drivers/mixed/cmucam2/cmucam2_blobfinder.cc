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


class Cmucam2blobfinder:public Driver 
{
  public:

  Cmucam2blobfinder( ConfigFile* cf, int section);
  virtual ~Cmucam2blobfinder();
  
  /* the main thread */
  virtual void Main();
  
  virtual int Setup();
  virtual int Shutdown();
  virtual void Update();
  
protected:
  Driver* cmucam2;
  player_device_id_t cmucam2_id;
  
};

// a factory creation function
Driver* Cmucam2blobfinder_Init( ConfigFile* cf, int section)
{
  if(strcmp( PLAYER_BLOBFINDER_STRING))
    {
      PLAYER_ERROR1("driver \"cmucam2_blobfinder\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else
    return((Driver*)(new Cmucam2blobfinder( cf, section)));
}

// a driver registration function
void Cmucam2blobfinder_Register(DriverTable* table)
{
  table->AddDriver( "cmucam2_blobfinder",  Cmucam2blobfinder_Init);
}




Cmucam2blobfinder::Cmucam2blobfinder( ConfigFile* cf, int section)
  : Driver(cf, section,  sizeof(player_blobfinder_data_t), 0, 1, 1 )
{
  this->cmucam2_id.code = PLAYER_CMUCAM2_CODE;
  this->cmucam2_id.port = cf->ReadInt(section, "cmucam2_port", 0 );
  this->cmucam2_id.index = cf->ReadInt(section, "cmucam2_index", 0 );
}

Cmucam2blobfinder::~Cmucam2blobfinder()
{
  puts( "Destroying Cmucam2blobfinder driver" );
}

int Cmucam2blobfinder::Setup()
{
  printf("Cmucam2blobfinder Setup.. ");
  fflush(stdout);
  
  // if we didn't specify a port for the cmucam2, use the same port as
  // this device
  if( this->cmucam2_id.port == 0 )
    this->cmucam2_id.port = device_id.port;
  
  printf( "finding Cmucam2 (%d:%d:%d).. ",
	  this->cmucam2_id.port,
	  this->cmucam2_id.code,
	  this->cmucam2_id.index ); fflush(stdout);

  // get the pointer to the Cmucam2
  this->cmucam2 = deviceTable->GetDriver(cmucam2_id);

  if(!this->cmucam2)
  {
    PLAYER_ERROR("unable to find cmucam2 device");
    return(-1);
  }
  
  else printf( " OK.\n" );
 
  // Subscribe to the cmucam2 device, but fail if it fails
  if(this->cmucam2->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to cmucam2 device");
    return(-1);
  }

  /* now spawn reading thread */
  StartThread();

  puts( "Cmucam2blobfinder setup done" );
  return(0);
}

int Cmucam2blobfinder::Shutdown()
{
  StopThread(); 

  // Unsubscribe from the laser device
  this->cmucam2->Unsubscribe(this);

  puts("Cmucam2blobfinder has been shutdown");
  return(0);
}

void Cmucam2blobfinder::Update()
{
  unsigned char config[128];
  
  void* client;
  player_device_id_t id;
  size_t config_size = 0;
  
  // first, check if there is a new config command
  if((config_size = GetConfig(&id, &client, (void*)config, sizeof(config))))
  {
      switch(config[0])
	{
	default:
	  puts("Cmucam2_blobfinder got unknown config request");
	  if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK,
		      NULL, NULL, 0))
	    PLAYER_ERROR("failed to PutReply");
	  break;
	}
   }
  
  // no commands to the blobfinder
}


void 
Cmucam2blobfinder::Main()
{  
  player_cmucam2_data_t cmucam2_data; // we get one of these
  player_blobfinder_data_t player_data; // and extract the blob data, copying it into here
  //player_blobfinder_blob_elt_t blob;

  player_data.width = htons((uint16_t)166);
  player_data.height = htons((uint16_t)143);
  player_data.header[0].index = 0;
  player_data.header[0].num = htons((uint16_t)1);

  for(;;)
    {
      // Wait for new data from the Cmucam2 driver
      this->cmucam2->Wait();
      
      // Get the Cmucam2 data.
      size_t len = this->cmucam2->
	GetData(this, (uint8_t*)&cmucam2_data, sizeof(cmucam2_data), NULL, NULL );
      
      assert( len == sizeof(cmucam2_data) );
      
      // extract the blob data from the Cmucam2 packet
      memcpy( &player_data, &cmucam2_data.blob, sizeof(cmucam2_data.blob));         
      //memcpy( &player_data.blobs[0], &blob, sizeof(blob) );
      PutData((uint8_t*)&player_data, sizeof(player_data), 0,0 );
    }
  pthread_exit(NULL);
}


