/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_bumpersafe bumpersafe

This is a low level safety 'driver' that temporarily disables
velocity commands if a bumper is pressed. It sits on top of @ref
player_interface_bumper and @ref player_interface_position devices.

The general concept of this device is to not do much, but to provide
a last line of defense in the case that higher level drivers or client
software fails in its object avoidance.

@par Compile-time dependencies

- none

@par Provides

- @ref player_interface_position

@par Requires

- @ref player_interface_position : the underlying robot to be controlled
- @ref player_interface_bumper : the bumper to read from

@par Configuration requests

- PLAYER_POSITION_MOTOR_POWER_REQ : if motor is switched on then we
  reset the 'safe state' so robot can move with a bump panel active
- all other requests are just passed on to the underlying @ref
  player_interface_position device

@par Configuration file options

- none

@par Example 

@verbatim
driver
(
  name "bumpersafe"
  provides ["position:0"]
  requires ["position:1" "bumper:0"]
)
@endverbatim

@par Authors

Toby Collett

*/
/** @} */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#include "player.h"
#include "error.h"
#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"
#include "clientdata.h"
#include "clientmanager.h"

class BumperSafe : public Driver 
{
  public:
    // Constructor
    BumperSafe( ConfigFile* cf, int section);

    // Destructor
    virtual ~BumperSafe();

    // Setup/shutdown routines.
    virtual int Setup();
    virtual int Shutdown();

    // Underlying devices
    int SetupPosition();
    int ShutdownPosition();
	//int GetPosition();
	
    int SetupBumper();
    int ShutdownBumper();
	//int GetBumper();


    // Main function for device thread.
    virtual void Main();


  private:
    // Write the pose data (the data going back to the client).
    //void PutPose();

    // Send commands to underlying position device
    void PutPositionCommand();

    // Check for new commands from server
    //void GetCommand();
    
	// Process incoming messages from clients 
	int ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, int * resp_len);
	
    // state info
    bool Blocked;
    player_bumper_data_t CurrentState;
    player_bumper_data_t SafeState;

    // Position device info
    Driver *position;
    player_device_id_t position_id;
    int speed,turnrate;
    player_position_cmd_t cmd;
    player_position_data_t data;
    double position_time;
	bool position_subscribed;

    // Bumper device info
    Driver *bumper;
    player_device_id_t bumper_id;
    double bumper_time;
    player_bumper_geom_t bumper_geom;
	bool bumper_subscribed;
	
	ClientDataInternal BaseClient;
		
};

// Initialization function
Driver* BumperSafe_Init( ConfigFile* cf, int section) 
{
  return ((Driver*) (new BumperSafe( cf, section)));
} 

// a driver registration function
void BumperSafe_Register(DriverTable* table)
{ 
  table->AddDriver("bumper_safe",  BumperSafe_Init);
  return;
} 

////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int BumperSafe::Setup() 
{
  	player_position_cmd_t cmd = {0};
	Blocked = true;

	clientmanager->AddClient(&BaseClient);
		
  // Initialise the underlying device s.
  if (this->SetupPosition() != 0)
    return -1;
  if (this->SetupBumper() != 0)
    return -1;

  // Start the driver thread.
  this->StartThread();

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int BumperSafe::Shutdown() {
  // Stop the driver thread.
  this->StopThread();

  // Stop the laser
  this->ShutdownPosition();

  // Stop the odom device.
  this->ShutdownBumper();

	clientmanager->RemoveClient(&BaseClient);
	
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int BumperSafe::ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, int * resp_len)
{
	assert(hdr);
	assert(data);
	assert(resp_data);
	assert(resp_len);
	assert(*resp_len==PLAYER_MAX_MESSAGE_SIZE);

	//printf("bumpersafe Got Message: %d %d %d %d %d\n", hdr->type, hdr->device, hdr->device_index, hdr->size, hdr->size ? data[0] : 0);
	
	if (hdr->type==PLAYER_MSGTYPE_SYNCH)
	{	
		*resp_len = 0;
		return 0;
	}
	if (hdr->type==PLAYER_MSGTYPE_RESP_ACK)
	{	
		*resp_len = 0;
		return 0;
	}	
	if (hdr->type==PLAYER_MSGTYPE_DATA && hdr->device==bumper_id.code && hdr->device_index==bumper_id.index)
	{
		// we got bumper data, we need to deal with this
		double time = (double) hdr->timestamp_sec + ((double) hdr->timestamp_usec) * 1e-6;

		// Dont do anything if this is old data.
		if (time - bumper_time < 0.001)
			return 0;
		bumper_time = time;

		CurrentState = *reinterpret_cast<player_bumper_data *> (data);

		unsigned char hash = 0;
		for (int i = 0; i < CurrentState.bumper_count; ++i)
			hash |= CurrentState.bumpers[i] & ~SafeState.bumpers[i];
			
		if (hash)
		{
			Blocked = true;
		}
		else
		{
			Blocked = false;
			SafeState = CurrentState;
		}
		PutPositionCommand();
		return 0;
	}
	
	/*if(hdr->device==position_id.code && hdr->device_index==position_id.index)
	{
		hdr->device_index = device_id.index;
		PutMsg(hdr, NULL, hdr->type, data, hdr->size);
		return 0;
	}*/
	
	if (Blocked)
	{
		MSG(device_id, PLAYER_MSGTYPE_REQ, sizeof(player_position_power_config_t), PLAYER_POSITION_MOTOR_POWER_REQ)
		{
			// if motor is switched on then we reset the 'safe state' so robot can move with a bump panel active
	  		if (((player_position_power_config_t *) data)->value == 1)
			{
				SafeState = CurrentState;
				Blocked = false;
    			cmd.xspeed = 0;
    			cmd.yspeed = 0;
    			cmd.yawspeed = 0;
			}
		}
		*resp_len = 0;
		MSG_END_ACK;
	}
	
	// set reply to value so the reply for this message goes straight to the given client
	if(hdr->device==device_id.code && hdr->device_index==device_id.index && hdr->type == PLAYER_MSGTYPE_REQ)
	{
		//printf("send message for Immediate processing by position device\n");
		hdr->device_index = position_id.index;
		int ret = position->ProcessMessage(&BaseClient, hdr, data, resp_data, resp_len);
		hdr->device_index = device_id.index;
		return ret;
	}
	
	MSG(position_id, PLAYER_MSGTYPE_GEOM, sizeof(player_position_geom_t),0xFF)
	{
		//printf("Got geom message from position device\n");
		PutMsg(device_id, NULL, PLAYER_MSGTYPE_GEOM, data, sizeof(player_position_geom_t));		
		*resp_len=0;
	}
	MSG_END

	MSG(position_id, PLAYER_MSGTYPE_DATA, sizeof(player_position_data_t),0xFF)
	{
		//printf("Got data message from position device\n");
		PutMsg(device_id, NULL, PLAYER_MSGTYPE_DATA, data, sizeof(player_position_data_t));		
		*resp_len=0;
	}
	MSG_END

	MSG(device_id, PLAYER_MSGTYPE_CMD, sizeof(player_position_cmd_t), 0xFF)
	{
		cmd = *reinterpret_cast<player_position_cmd_t *> (data);
		if (!Blocked)
		{
			hdr->device_index = position_id.index;
			int ret = position->ProcessMessage(&BaseClient, hdr, data, resp_data, resp_len);
			hdr->device_index = device_id.index;
			return ret;
		}
	}
	MSG_END_ACK;

	
	return -1;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying position device.
int BumperSafe::SetupPosition() 
{
	printf("Setup POsition called\n");
	uint8_t DataBuffer[PLAYER_MAX_MESSAGE_SIZE];
	
	this->position = deviceTable->GetDriver(this->position_id);
	if (!this->position)
	{
		PLAYER_ERROR("unable to locate suitable position device");
		return -1;
	}

	return BaseClient.Subscribe(position_id);
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying position device.
int BumperSafe::ShutdownPosition() 
{

  // Stop the robot before unsubscribing
  this->speed = 0;
  this->turnrate = 0;
  this->PutPositionCommand();
  
  return BaseClient.Unsubscribe(position_id);
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying position device.
void BumperSafe::PutPositionCommand() 
{
	player_position_cmd_t NullCmd = {0};
	if (Blocked)
		BaseClient.SendMsg(position_id, PLAYER_MSGTYPE_CMD, (uint8_t*)&NullCmd, sizeof(cmd));
	else
		BaseClient.SendMsg(position_id, PLAYER_MSGTYPE_CMD, (uint8_t*)&cmd, sizeof(cmd));
}

////////////////////////////////////////////////////////////////////////////////
// Set up the bumper
int BumperSafe::SetupBumper() {
	printf("Setup bumper called\n");
	this->bumper = deviceTable->GetDriver(this->bumper_id);
	if (!this->bumper)
	{
		PLAYER_ERROR("unable to locate suitable laser device");
		return -1;
	}

	if (BaseClient.Subscribe(bumper_id,'r') != 0)
	{
		PLAYER_ERROR("unable to subscribe to bumper device");
		return -1;
	}
	
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the bumper
int BumperSafe::ShutdownBumper() {
	return BaseClient.Unsubscribe(bumper_id);
}




////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void BumperSafe::Main() 
{
  struct timespec sleeptime;
  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = 1000000L;

  // Wait till we get new odometry data; this may block indefinitely
  // this->GetPosition();
  // this->GetBumper();

	// need to do geom request first
	printf("Main Bumper thread started\n");
  
  while (true)
  {
	// check base client for messages
	BaseClient.Read();	  
	  
    // Process any pending requests.
    this->ProcessMessages();

    // Sleep for 1ms (will actually take longer than this).
    nanosleep(&sleeptime, NULL);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
BumperSafe::BumperSafe( ConfigFile* cf, int section)
        : Driver(cf, section, PLAYER_POSITION_CODE, PLAYER_ALL_MODE), BaseClient(this)
{
	Blocked = false;

  this->position = NULL;
  // Must have a position device
  if (cf->ReadDeviceId(&this->position_id, section, "requires",
                       PLAYER_POSITION_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);    
    return;
  }
  this->position_time = 0.0;
  
  this->bumper = NULL;
  // Must have a bumper device
  if (cf->ReadDeviceId(&this->bumper_id, section, "requires",
                       PLAYER_BUMPER_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  this->bumper_time = 0.0;

  return;
}


BumperSafe::~BumperSafe() {
  return;
}
