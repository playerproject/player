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
 * Stage device that connects to a Stage server and interacts with it
 */

#include <string.h> // for memcpy()
#include <stdlib.h> // for rand()
#include <playercommon.h> // For length-specific types, error macros, etc
#include <device.h>
#include <configfile.h>
#include <sio.h> // from the Stage installation
#include <netinet/in.h> // for byte swapping macros

#include <playerpacket.h>

#include <stagetime.h>
#include <playertime.h>
extern PlayerTime* GlobalTime;

const int PLAYER_STAGEDEVICE_MAX = 1000;

class StageDevice : public CDevice
{
private:
  // the following vars are static because they concern the connection to
  // Stage, of which there is only one, for all Stage devices.
  static int stage_conn;  // fd to stage
  static bool initdone; // did we initialize the static data yet?
  static pthread_mutex_t stage_conn_mutex; // protect access to the socket
  
  // we map connection numbers to StageDevice pointers with this array
  static StageDevice *stageDeviceMap[ PLAYER_STAGEDEVICE_MAX ]; 
  
  static int HandleProperty(int conn, double timestamp, 
			    void* data, size_t len,
			    stage_buffer_t* replies);
  static int HandleLostConnection(int conn);
  static int HandleLostConnection2(int conn);

  // this structure is sent to Stage to request a new model and
  // returned with its id field set. we use the id to refer to this
  // model subsequently.
  stage_model_t stage_model;
  
protected:
  // grabs the mutex, writes the propery buffer, gets replies,
  // releases the mutex.
  int SendProperties( stage_buffer_t* props, stage_buffer_t* reply );

public:
  StageDevice(int parent, char* interface,
	      size_t datasize, size_t commandsize, 
	      int reqqueuelen, int repqueuelen);
  
  stage_model_t GetModel() { return(stage_model); }
  
  // Initialise the device
  //
  virtual int Setup();
  
  // Terminate the device
  //
  virtual int Shutdown();
  
  // Read data from the device
  //
  virtual size_t GetData(void* client,unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec,
			 uint32_t* timestamp_usec);
  
  // Write a command to the device
  //
  virtual void PutCommand(void* client, unsigned char * , size_t maxsize);
  
  // Main function for device thread
  //
  virtual void Main();
};

extern int StageDevice::stage_conn;  // fd to stage
extern bool StageDevice::initdone; // did we initialize the static data yet?
extern pthread_mutex_t StageDevice::stage_conn_mutex; // protect access to the socket

// TODO - make this storage dynamic
extern StageDevice* StageDevice::stageDeviceMap[ PLAYER_STAGEDEVICE_MAX ];

StageDevice::StageDevice(int parent, char* interface,
                         size_t datasize, size_t commandsize, 
                         int reqqueuelen, int repqueuelen) :
  CDevice(datasize,commandsize,reqqueuelen,repqueuelen)
{
  // connect to Stage
  if(!initdone)
  {
    // TODO: pass the hostname:port from somewhere
    // connect to Stage
    if((stage_conn = SIOInitClient(0,NULL)) == -1)
    {
      PLAYER_ERROR("unable to connect to Stage");
      return;
    }
    
    // replace Player's wall clock with our clock
    if( GlobalTime ) delete GlobalTime;
    assert(GlobalTime = (PlayerTime*)(new StageTime()));
    
    // init the device-to-id map
    memset( stageDeviceMap, 0, PLAYER_STAGEDEVICE_MAX * sizeof(StageDevice*) );
    
    // create a GUI
    stage_gui_config_t gui;
    strcpy( gui.token, "rtk" ); 
    gui.width = 600;
    gui.height = 600;
    gui.ppm = 40;
    gui.originx = 0;//300;
    gui.originy = 0;//300; 
    gui.showsubscribedonly = 0;
    gui.showgrid = 1;
    gui.showdata = 1;
    
    
    stage_buffer_t* guireq = SIOCreateBuffer();
    
    SIOBufferProperty( guireq, 0, STG_PROP_ROOT_GUI,
		       &gui, sizeof(gui), STG_NOREPLY );
    
    SIOPropertyUpdate( stage_conn, 0.0, guireq, NULL );
    
    SIOFreeBuffer( guireq );
    
    pthread_mutex_init(&stage_conn_mutex,NULL);
    StartThread();
    initdone = true;
  }

  // acquire the lock on the socket
  pthread_mutex_lock(&stage_conn_mutex);
  
  // add ourselves into the world
  stage_model.parent_id = parent;

  // TODO: convert from Player interface name to Stage token name
  strncpy(stage_model.token, interface, STG_TOKEN_MAX);
  
  // sends the request to Stage, gets a confirmation reply and fills
  // in the model's id field correctly.
  SIOCreateModels(stage_conn, 0.0, &stage_model, 1 );
  
  // release the lock on the socket
  pthread_mutex_unlock(&stage_conn_mutex);
  
  // store a mapping from the model id to this object so we can recover
  // the device context in static methods
  stageDeviceMap[ stage_model.id ] = this;

  printf( "STAGEDEVICE: stage model %s created id %d parent %d\n",
	  stage_model.token, stage_model.id, stage_model.parent_id );
}

// initialization function
CDevice* 
Stage_Init(char* interface, ConfigFile* cf, int section)
{
  // TODO: get config file args out and store them somewhere
  // TODO: figure out how much space is actually required for these buffers
  return((CDevice*)(new StageDevice(cf->entities[section].parent, interface,
                                    PLAYER_MAX_PAYLOAD_SIZE,
                                    PLAYER_MAX_PAYLOAD_SIZE,
                                    1,1)));
}

int StageDevice::SendProperties( stage_buffer_t* props, stage_buffer_t* reply )
{
  int retval = 0;
  
  pthread_mutex_lock(&stage_conn_mutex);
  
  if( SIOPropertyUpdate( stage_conn, 0.0, props, reply ) == -1 )
    {
      PRINT_ERR( "property update failed" );
      retval = -1;
    }
  else
    retval = 0;
  
  pthread_mutex_unlock(&stage_conn_mutex);
  return retval;
}

///////////////////////////////////////////////////////////////////////////
// Initialise the device
//
int StageDevice::Setup()
{
  PRINT_WARN( "shutdown" );

  // subscribe to the sonar's data, pose, size and rects.
  stage_buffer_t* props;
  assert(props = SIOCreateBuffer());

  stage_subscription_t sub;
  sub.property = STG_PROP_ENTITY_DATA;
  sub.flag = STG_SUBSCRIBED;
  
  SIOBufferProperty(props, stage_model.id, STG_PROP_ENTITY_SUBSCRIBE, 
		    &sub, sizeof(sub), STG_NOREPLY );

  if( stage_model.id == 1 )
    {
      stage_position_cmd_t cmd;
      cmd.x = cmd.xdot = 0.2;
      cmd.y = cmd.ydot = 0.0;
      cmd.a = cmd.adot = 0.1;
      
      SIOBufferProperty(props, stage_model.id, STG_PROP_ENTITY_COMMAND, 
			&cmd, sizeof(cmd), STG_NOREPLY );
    }
  
  SendProperties( props, NULL );
  SIOFreeBuffer( props );

  return(0);
}


///////////////////////////////////////////////////////////////////////////
// Terminate the device
//
int StageDevice::Shutdown()
{
  // unsubscribe
  stage_buffer_t* props;
  assert(props = SIOCreateBuffer());
  
  // ask root to destroy this model
  SIOBufferProperty(props, 0, STG_PROP_ROOT_DESTROY, 
		    &this->stage_model, sizeof(this->stage_model), 
		    STG_NOREPLY );
  
  SendProperties( props, NULL );
  SIOFreeBuffer( props );
  
  PRINT_ERR1( "destroying model %d", stage_model.id );

  return 0;
};


int
StageDevice::HandleProperty(int conn, double timestamp, 
			    void* data, size_t len, 
			    stage_buffer_t* replies)
{
  stage_property_t* prop = (stage_property_t*)data;
  
  printf( "Received %d bytes  property (%d,%s,%d) on connection %d\n",
          (int)len, prop->id, SIOPropString(prop->property), (int)prop->len, 
          stage_conn);

  // convert from double seconds to timeval
  struct timeval time;
  SIOPackTimeval( &time, timestamp );

  // set the master clock
  ((StageTime*)GlobalTime)->SetTime(&time);

  // find the device object for the model with this id
  StageDevice* sdev = stageDeviceMap[ prop->id ];
  assert( sdev );
  
  // package data into Player packets
  
  // shift the data pointer past the property header
  ((char*)data) += sizeof(stage_property_t); 
  
  // reduce the data length by the same amount;
  len -= sizeof(stage_property_t);
  
  if( strcmp( sdev->stage_model.token, "position" ) == 0 )
    {                              
      if( len != sizeof( stage_position_data_t ) )
	{
	  PRINT_ERR2( "position data is %lu not %lu bytes",
		      (unsigned long)len, 
		      (unsigned long)sizeof(stage_position_data_t) );

	  return -1;
	}
      
      stage_position_data_t* spd =  (stage_position_data_t*)data;
      //(stage_position_data_t*)(((char*)data)+sizeof(stage_property_t));
      
      player_position_data_t ppd;
      
      printf( "time %lu sec %lu usec\n",
	      time.tv_sec, time.tv_usec );
      
      printf( "stage position %.2f,%.2f,%.2f  %.2f,%.2f,%.2f\n",
	      spd->x, spd->y, spd->a, spd->xdot, spd->ydot, spd->adot );
      
      // TODO - stall bit
      PlayerPositionData( &ppd, spd );
           
      // Make data available
      sdev->PutData((uint8_t*)&ppd, sizeof(ppd), time.tv_sec, time.tv_usec);
    }
  else if( strcmp( sdev->stage_model.token, "sonar" ) == 0 )
    {
      player_sonar_data_t psd;
      PlayerSonarData( &psd, (double*)data, len );
      sdev->PutData((uint8_t*)&psd, sizeof(psd), time.tv_sec, time.tv_usec);         }
  else
    PRINT_ERR1( "don't know how to translate data for %s",
		sdev->stage_model.token );

  return(0); //success
}

// this one's called from the StageDevice constructor, and thus has the
// context of the server read thread.
int
StageDevice::HandleLostConnection2(int conn)
{
  // TODO: this doesn't work, cause we don't have CDevice object context;
  //       need some other mechanism to kill the Stage interaction thread
  // DONE - (but it doesn't seem to work...?) rtv)

  StageDevice* sdev = stageDeviceMap[conn];
  assert( sdev );
  sdev->StopThread();

  return(0);
}

// this one's called from the Stage interaction thread
int
StageDevice::HandleLostConnection(int conn)
{
  pthread_exit(NULL);
  return(0);
}

// Main function for device thread
//
void 
StageDevice::Main()
{
  for(;;)
  {
    // grab a lock on the stage fd
    pthread_mutex_lock(&stage_conn_mutex);

    // tell stage we're done talking so it can update
    SIOWriteMessage(stage_conn, 0.0, STG_HDR_CONTINUE, NULL, 0);
    
    // receive packets from Stage
    SIOServiceConnections( &HandleLostConnection, 
			   NULL,
			   &HandleProperty );

    // release the stage fd
    pthread_mutex_unlock(&stage_conn_mutex);
  }
}


///////////////////////////////////////////////////////////////////////////
// Read data from the device
//
size_t StageDevice::GetData(void* client,unsigned char *data, size_t size,
                        uint32_t* timestamp_sec, 
                        uint32_t* timestamp_usec)
{
  /*
#ifdef DEBUG
  printf( "P: getting (%d,%d,%d) info at %p, data at %p, buffer len %d, %d bytes available, size parameter %d\n", 
          m_info->player_id.port, 
          m_info->player_id.code, 
          m_info->player_id.index, 
          m_info, device_data,
          m_info->data_len,
          m_info->data_avail,
          size );
  fflush( stdout );
#endif

  // See if there is any data
  //
  size_t data_avail = m_info->data_avail;

  // Check for overflows 1
  //
  if (data_avail > PLAYER_MAX_MESSAGE_SIZE )
  {
    printf( "Available data (%d bytes) is larger than Player's"
            " maximum message size (%d bytes)\n", 
            data_avail, PLAYER_MAX_MESSAGE_SIZE );
  }

  // Check for overflows 2
  //
  if (data_avail > device_datasize )
  {
    printf("warning: available data (%d bytes) > buffer size (%d bytes); ignoring data\n", data_avail, device_datasize );
    Unlock();
    return 0;
    //data_avail = m_data_len;
  }
    
  // Check for overflows 3
  //
  if( data_avail > size)
  {
    printf("warning data available (%d bytes) > space in Player packet (%d bytes); ignoring data\n", data_avail, size );
    Unlock();
    return 0;
  }
    
  // Copy the data
  memcpy(data, device_data, data_avail);

  // store the timestamp in the device, because other devices may
  // wish to read it
  data_timestamp_sec = m_info->data_timestamp_sec;
  data_timestamp_usec = m_info->data_timestamp_usec;

  // also return the timestamp to the caller
  if(timestamp_sec)
    *timestamp_sec = data_timestamp_sec;
  if(timestamp_usec)
    *timestamp_usec = data_timestamp_usec;
    
  return data_avail;
  */
  return(0);
}


///////////////////////////////////////////////////////////////////////////
// Write a command to the device
//
void StageDevice::PutCommand(void* client,unsigned char *command, size_t len)
{
  /*
#ifdef DEBUG
  printf( "P: StageDevice::PutCommand (%d,%d,%d) info at %p,"
	  " command at %p\n", 
          m_info->player_id.port, 
          m_info->player_id.code, 
          m_info->player_id.index, 
          m_info, command);
  fflush( stdout );
#endif

  // Check for overflows
  if (len > device_commandsize)
    PLAYER_ERROR("invalid command length; ignoring command");
    
  // Copy the command
  memcpy(device_command, command, len);

  // Set flag to indicate command has been set
  m_info->command_avail = len;

  // set timestamp for this command
  struct timeval tv;
  GlobalTime->GetTime(&tv);

  m_info->command_timestamp_sec = tv.tv_sec;
  m_info->command_timestamp_usec = tv.tv_usec;
  */
}
    

