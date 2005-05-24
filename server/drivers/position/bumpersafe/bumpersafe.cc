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

class BumperSafe : public Driver 
{
  public:
    // Constructor
    BumperSafe( ConfigFile* cf, int section);

    // Destructor
    virtual ~BumperSafe() {};

    // Setup/shutdown routines.
    virtual int Setup();
    virtual int Shutdown();

    // Underlying devices
    int SetupPosition();
    int ShutdownPosition();
	
    int SetupBumper();
    int ShutdownBumper();

    // Process incoming messages from clients 
    int ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, size_t * resp_len);

  private:

    // state info
    bool Blocked;
    player_bumper_data_t CurrentState;
    player_bumper_data_t SafeState;

    // Position device info
    Driver *position;
    player_device_id_t position_id;
    int speed,turnrate;
    double position_time;
    bool position_subscribed;

    // Bumper device info
    Driver *bumper;
    player_device_id_t bumper_id;
    double bumper_time;
    player_bumper_geom_t bumper_geom;
    bool bumper_subscribed;
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
  Blocked = true;
		
  // Initialise the underlying devices.
  if (this->SetupPosition() != 0)
  {
  	PLAYER_ERROR2("Bumber safe failed to connect to undelying position device %d:%d\n",position_id.code, position_id.index);
    return -1;
  }
  if (this->SetupBumper() != 0)
  {
  	PLAYER_ERROR2("Bumber safe failed to connect to undelying bumper device %d:%d\n",bumper_id.code, bumper_id.index);
    return -1;
  }

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

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int BumperSafe::ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, size_t * resp_len)
{
	assert(hdr);
	assert(data);
	assert(resp_data);
	assert(resp_len);
	assert(*resp_len==PLAYER_MAX_MESSAGE_SIZE);

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
	
	if(MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 0, bumper_id))
//	if (hdr->type==PLAYER_MSGTYPE_DATA && hdr->device==bumper_id.code && hdr->device_index==bumper_id.index)
	{
		// we got bumper data, we need to deal with this
		double time = (double) hdr->timestamp_sec + ((double) hdr->timestamp_usec) * 1e-6;

		Lock();
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
			Unlock();
			player_position_cmd_t NullCmd = {0};
			player_msghdr_t NullHdr;
			NullHdr.stx = PLAYER_STXX;
			NullHdr.type = PLAYER_MSGTYPE_CMD;
			NullHdr.subtype = 0;
			NullHdr.device = PLAYER_POSITION_CODE;
			NullHdr.device_index = position_id.index;
			NullHdr.size = sizeof (NullCmd);
			int ret = position->ProcessMessage(BaseClient, &NullHdr, (unsigned char*)&NullCmd, resp_data, resp_len);
		}
		else
		{
			Blocked = false;
			SafeState = CurrentState;
			Unlock();
		}
		return 0;
	}
	
	if (Blocked && MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION_MOTOR_POWER, device_id))
	{
		assert(hdr->size == sizeof(player_position_power_config_t));
		// if motor is switched on then we reset the 'safe state' so robot can move with a bump panel active
  		if (((player_position_power_config_t *) data)->value == 1)
		{
			Lock();
			SafeState = CurrentState;
			Blocked = false;
			/*cmd.xspeed = 0;
			cmd.yspeed = 0;
			cmd.yawspeed = 0;*/
			Unlock();
		}
		*resp_len = 0;
		return PLAYER_MSGTYPE_RESP_ACK;
	}
	
	// set reply to value so the reply for this message goes straight to the given client
	if(hdr->device==device_id.code && hdr->device_index==device_id.index && hdr->type == PLAYER_MSGTYPE_REQ)
	{
//		Lock();
		hdr->device_index = position_id.index;
		int ret = position->ProcessMessage(BaseClient, hdr, data, resp_data, resp_len);
		hdr->device_index = device_id.index;
//		Unlock();
		return ret;
	}
	
	if (MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION_GEOM, position_id))
	{
		assert(hdr->size == sizeof(player_position_geom_t));
		PutMsg(device_id, NULL, PLAYER_MSGTYPE_DATA, PLAYER_POSITION_GEOM, data, sizeof(player_position_geom_t));		
		*resp_len=0;
		return 0;
	}
	if (MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 0, position_id))
	{
		assert(hdr->size == sizeof(player_position_data_t));
		PutMsg(device_id, NULL, PLAYER_MSGTYPE_DATA, 0, data, sizeof(player_position_data_t));		
		*resp_len=0;
		return 0;
	}
	if (MatchMessage(hdr, PLAYER_MSGTYPE_CMD, 0, device_id))
	{
		assert(hdr->size == sizeof(player_position_cmd_t));
		Lock();
//		player_position_cmd_t cmd = *reinterpret_cast<player_position_cmd_t *> (data);
		if (!Blocked)
		{
			Unlock();
			hdr->device_index = position_id.index;
			int ret = position->ProcessMessage(BaseClient, hdr, data, resp_data, resp_len);
			hdr->device_index = device_id.index;
			return ret;
		}
		Unlock();
		*resp_len = 0;
		return 0;
	}

	return -1;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying position device.
int BumperSafe::SetupPosition() 
{
	uint8_t DataBuffer[PLAYER_MAX_MESSAGE_SIZE];
	
	this->position = SubscribeInternal(this->position_id);
	if (!this->position)
	{
		PLAYER_ERROR("unable to locate suitable position device");
		return -1;
	}
  
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying position device.
int BumperSafe::ShutdownPosition() 
{
  UnsubscribeInternal(position_id);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the bumper
int BumperSafe::SetupBumper() {
	this->bumper = SubscribeInternal(this->bumper_id);
	if (!this->bumper)
	{
		PLAYER_ERROR("unable to locate suitable laser device");
		return -1;
	}
	
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the bumper
int BumperSafe::ShutdownBumper() {
	UnsubscribeInternal(bumper_id);
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
BumperSafe::BumperSafe( ConfigFile* cf, int section)
        : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_POSITION_CODE, PLAYER_ALL_MODE)
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

