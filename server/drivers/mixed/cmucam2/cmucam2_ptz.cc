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
 * Driver for the CMUcam2. 
 * Authors: Richard Vaughan (vaughan@sfu.ca), Pouya Bastani(pbastani@cs.sfu.ca)
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

// so we can access the deviceTable and extract pointers to the sonar
// and sonar objects
#include <drivertable.h>
#include <devicetable.h>
extern CDeviceTable* deviceTable;


class Cmucam2ptz:public CDevice 
{
  public:

  Cmucam2ptz(char* interface, ConfigFile* cf, int section);
  virtual ~Cmucam2ptz();
  
  /* the main thread */
  virtual void Main();
  
  virtual int Setup();
  virtual int Shutdown();
  virtual void Update();
  
protected:
  CDevice* cmucam2;
  player_device_id_t cmucam2_id;
  
};

// a factory creation function
CDevice* Cmucam2ptz_Init(char* interface, ConfigFile* cf, int section)
{
/** [Synopsis]
The {\tt ptz} interface is used to control a pan-tilt-zoom unit. */

/** [Constants] */

/** Configuration request codes */
  if(strcmp(interface, PLAYER_PTZ_STRING))
    {
      PLAYER_ERROR1("driver \"cmucam2_ptz\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else
    return((CDevice*)(new Cmucam2ptz(interface, cf, section)));
}

// a driver registration function
void Cmucam2ptz_Register(DriverTable* table)
{
  table->AddDriver( "cmucam2_ptz", PLAYER_ALL_MODE, Cmucam2ptz_Init);
}

Cmucam2ptz::Cmucam2ptz(char* interface, ConfigFile* cf, int section)
  : CDevice( sizeof(player_ptz_data_t), sizeof(player_ptz_cmd_t), 5, 5)
{
  this->cmucam2_id.code = PLAYER_CMUCAM2_CODE;
  this->cmucam2_id.port = cf->ReadInt(section, "cmucam2_port", 0 );
  this->cmucam2_id.index = cf->ReadInt(section, "cmucam2_index", 0 );
}

Cmucam2ptz::~Cmucam2ptz()
{
  puts( "Destroying Cmucam2ptz driver" );
}

int Cmucam2ptz::Setup()
{
  printf("Cmucam2ptz Setup.. ");
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
  this->cmucam2 = deviceTable->GetDevice(cmucam2_id);

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

  puts( "Cmucam2ptz setup done" );
  return(0);
}

int Cmucam2ptz::Shutdown()
{
  StopThread(); 

  // Unsubscribe from the laser device
  this->cmucam2->Unsubscribe(this);

  puts("Cmucam2ptz has been shutdown");
  return(0);
}

void Cmucam2ptz::Update()
{
  unsigned char config[CMUCAM_CONFIG_SIZE];
  
  void* client;
  player_device_id_t id;
  size_t config_size = 0;
  
  // first, check if there is a new config command
  if((config_size = GetConfig(&id, &client, (void*)config, sizeof(config))))
  {   
    switch(config[0])
    {
       case PLAYER_PTZ_AUTOSERVO:
       {
	 player_cmucam2_autoservo_config_t servo;
	 servo = *((player_cmucam2_autoservo_config_t*)config);

	 if(config_size < 0)
	 {
	     if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
	       PLAYER_ERROR("failed to PutReply");
	     return;
	 }      
    
	 if(cmucam2->PutConfig(&id, client, (void*)config, config_size) == -1)
	 {
	   if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
	     PLAYER_ERROR("failed to PutReply");
	 }
	 else
	 {
	   if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
	     PLAYER_ERROR("failed to PutReply");   	   
	 }
	 break;
       }

       default:
       {
	 puts("Cmucam2_ptz got unknown config request");
	 if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
	   PLAYER_ERROR("failed to PutReply");
	 break;
       }
    }
  }
  
  //player_ptz_cmd_t command;  
  player_cmucam2_cmd_t command;
  if( GetCommand((unsigned char*)&command, sizeof(command)) )
  {
      // consume the command
      device_used_commandsize = 0;      
                  
      // CHECK: is this the right syntax?      
      this->cmucam2->PutCommand( this, (unsigned char*)&command, sizeof(command) ); 
   }   
}


void 
Cmucam2ptz::Main()
{  
  player_cmucam2_data_t cmucam2_data; // we get one of these
  player_ptz_data_t player_data;      // and extract the blob data, copying it into here  

  for(;;)
    {
      // Wait for new data from the Cmucam2 driver
      this->cmucam2->Wait();
      
      // Get the Cmucam2 data.
      size_t len = this->cmucam2->
      GetData(this, (uint8_t*)&cmucam2_data, sizeof(cmucam2_data), NULL, NULL );
      
      assert( len == sizeof(cmucam2_data) );
      
      // extract the ptz data from the Cmucam2 packet      
      memcpy( &player_data, &cmucam2_data.ptz_data, sizeof(cmucam2_data.ptz_data) );
      PutData((uint8_t*)&player_data, sizeof(player_data), 0,0 );
    }
  pthread_exit(NULL);
}


