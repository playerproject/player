/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2009 Goutham Mallapragda, Anthony Cascone, Rich Mattes & Brian Gerkey
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
 Desc: Segway RMP 400 Driver
 Authors: Goutham Mallapragada, Anthony Cascone, Rich Mattes
 Updated: July 21, 2009
*/


/** @ingroup drivers */
/** @{ */
/** @defgroup driver_segwayrmp400 segwayrmp400
 * @brief SegwayRMP400 Mobile Robot

%Device driver for the Segway RMP 400, subscribes to two "segwayrmp" drivers

This driver subscribes to both the front and rear modules of the RMP400 and
provides a common interface to control the unit.  It wraps two "segwayrmp" drivers
into a single interface and provides the same output to both units.  It also averages the
incoming odometry and returns the combined data.

This driver was developed by the Networked Robotics and Sensors Laboratory at
The Pennsylvania State University, University Park, PA 16802.
(http://nrsl.mne.psu.edu).

@par Note Some Dell laptops have trouble talking and staying connected to both rmp modules.
It is necessary to use a USB hub between the rmp units and the laptop.

@par Compile-time dependencies

- none

@par Requires

- @ref interface_position2d
  - This interface is required if you chose to run the RMP400
  from the position2d interface. The RMP400 driver can run in 2d, 3d, or both 2d and 3d modes.
- @ref interface_position3d
  - This interface is required if you chose to run the RMP400
  from the position3d interface. The RMP400 driver can run in 2d, 3d, or both 2d and 3d modes.

@par Provides

- @ref interface_position2d
  - This interface returns odometry data (x,y, and yaw),
   and accepts velocity commands (x vel and yaw vel).

- @ref interface_position3d
  - This interface returns odometry data (x, y and yaw) from the wheel
  encoders, and attitude data (pitch and roll) from the IMU.  The
  driver accepts velocity commands (x vel and yaw vel).

@par Configuration requests

- none

@par Configuration file options

- fullspeed_data
  - Default: 1
  - If set to 0, the driver will only publish every tenth data state.  This can help
  prevent queue overflows in certain situations since the underlying RMP devices
  publish their data at around 100Hz.  When set to 1, all incoming data is published.

@par Example

@verbatim
driver
(
  name "segwayrmp400"
  provides ["position2d:0" "position3d:0"]
  requires ["front:::position3d:1" "back:::position3d:2" "front2d:::position2d:1""back2d:::position2d:2"]
  fullspeed_data 1
)
@endverbatim

@authors Goutham Mallapragada, Anthony Cascone, Rich Mattes

 */

#include "segwayrmp400.h"

////////////////////////////////////////////////////
// Init
Driver* SegwayRMP400_Init(ConfigFile* cf, int section)
{
        return ((Driver*)(new SegwayRMP400(cf, section)));
}

////////////////////////////////////////////////////
// Register with drivertable
void segwayrmp400_Register(DriverTable *table)
{
    table->AddDriver("segwayrmp400", SegwayRMP400_Init);
}

////////////////////////////////////////////////////
// Constructor
SegwayRMP400::SegwayRMP400(ConfigFile* cf, int section): ThreadedDriver(cf,section,true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
	// Start clean
	this->provide_2d = false;
	this->provide_3d = false;

	memset(&this->position2d_id,0,sizeof(player_devaddr_t));
	memset(&this->position3d_id,0,sizeof(player_devaddr_t));
	memset(&this->segwayrmp2d_id[0],0,sizeof(player_devaddr_t));
	memset(&this->segwayrmp2d_id[1],0,sizeof(player_devaddr_t));
	memset(&this->segwayrmp3d_id[0],0,sizeof(player_devaddr_t));
	memset(&this->segwayrmp3d_id[1],0,sizeof(player_devaddr_t));
	
	newfront2d = false;
	newback2d = false;
    	newfront3d = false;
    	newback3d = false;

	// Do we create a position3d interface?
	if( cf->ReadDeviceAddr(&this->position3d_id,section,"provides",PLAYER_POSITION3D_CODE,-1,NULL) == 0 )
	{
    		if( cf->ReadDeviceAddr(&this->segwayrmp3d_id[0], section, "requires", PLAYER_POSITION3D_CODE, -1, "front") != 0)
    		{
			this->SetError(-1);
			PLAYER_ERROR("Front Segway Position3d not present");
			return;
		}
		if( cf->ReadDeviceAddr(&this->segwayrmp3d_id[1], section, "requires", PLAYER_POSITION3D_CODE, -1, "back") != 0 )
		{
			this->SetError(-1);
			PLAYER_ERROR("Back Segway Position2d not present");
			return;
		}

		if(this->AddInterface(this->position3d_id)!=0)
		{
			this->SetError(-1);
			PLAYER_ERROR("Unable Add SegwayRMP400 Position3d Device");
			return;
		}

		this->provide_3d = true;
		PLAYER_MSG0(2, "SegwayRMP400 Providing Position3d Device");
	}

	// Do we create a position2d interface?
	if( cf->ReadDeviceAddr(&this->position2d_id,section,"provides",PLAYER_POSITION2D_CODE,-1,NULL) == 0 )
	{
		if( cf->ReadDeviceAddr(&this->segwayrmp2d_id[0], section, "requires", PLAYER_POSITION2D_CODE, -1, "front2d") != 0)
		{
			this->SetError(-1);
			PLAYER_ERROR("Front Segway Position2d not present");
			return;
		}
		if( cf->ReadDeviceAddr(&this->segwayrmp2d_id[1], section, "requires", PLAYER_POSITION2D_CODE, -1, "back2d") != 0 )
		{
			this->SetError(-1);
			PLAYER_ERROR("Back Segway Position2d not present");
			return;
		}

		if(this->AddInterface(this->position2d_id)!=0)
		{
			this->SetError(-1);
			PLAYER_ERROR("Unable Add SegwayRMP400 Position2d Device");
			return;
		}

		this->provide_2d = true;
		PLAYER_MSG0(2, "SegwayRMP400 Providing Position2d Device");
	}
	counter = 0;
	
	// Check config file for additional options
	this->fullspeed = cf->ReadInt(section, "fullspeed_data", 1) == 0 ? false : true;
	

}

////////////////////////////////////////////////////
// Setup the connections and subscribe to required devices
int SegwayRMP400::MainSetup()
{
    int i;
    PLAYER_MSG0(0,"SegwayRMP400 Initializing ...");

    for( i=0; i<2; i++)
    {
		if (this->provide_3d)
		{
			if(!(this->segwayrmp3d[i] = deviceTable->GetDevice(this->segwayrmp3d_id[i])))
			{
				PLAYER_ERROR1("Unable to locate segwayrmp position3d device[%d]",i);
				return(-1);
			}
		}
		if (this->provide_2d)
		{
			if(!(this->segwayrmp2d[i] = deviceTable->GetDevice(this->segwayrmp2d_id[i])))
			{
				PLAYER_ERROR1("Unable to locate segwayrmp position3d device[%d]",i);
				return(-1);
			}
		}
    }

    for(i=0;i<2;i++)
    {
		if (this->provide_3d)
		{
			if(this->segwayrmp3d[i]->Subscribe(this->InQueue) != 0 )
			{
				PLAYER_ERROR1("Unable to subscribe to host segwayrmp position2d device[%d]",i);
				return(-1);
			}
		}
		if (this->provide_2d)
		{
			if(this->segwayrmp2d[i]->Subscribe(this->InQueue) != 0 )
			{
				PLAYER_ERROR1("Unable to subscribe to host segwayrmp position2d device[%d]",i);
				return(-1);
			}
		}
    }
    PLAYER_MSG0(0, "SegwayRMP Initialized");
    return 0;
}

////////////////////////////////////////////////////
// Shutdown connection to subscribed devices
void SegwayRMP400::MainQuit()
{
    PLAYER_MSG0(0,"Shutting SegwayRMP400 down...");

    if (this->provide_3d)
    {
		for(int i=0;i<2;i++)
			this->segwayrmp3d[i]->Unsubscribe(this->InQueue);
    }
    if (this->provide_2d)
	{
		for(int i=0;i<2;i++)
			this->segwayrmp2d[i]->Unsubscribe(this->InQueue);
	}

    PLAYER_MSG0(0, "SegwayRMP400 has been shutdown");
    return;
}

////////////////////////////////////////////////////
// Main, you get the idea
void SegwayRMP400::Main()
{
	// Setup Thread
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    // Main Loop
    for(;;)
    {
        // Check for time to quit
        pthread_testcancel();

		// Yep, everyone has to do it.
		ProcessMessages();

		// Fill the data structure, and publish position data.
		ProcessData();

		// Sleep for 1 ms, everyone needs some sleep
		usleep(1000);
	}
}

////////////////////////////////////////////////////
//  Process Message because we should
int SegwayRMP400::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{

	// is it new position3d data?
	if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_DATA, PLAYER_POSITION3D_DATA_STATE, this->segwayrmp3d_id[0]))
	{
		assert(hdr->size == sizeof(player_position3d_data_t));
		rmp3d_data[0] = *(player_position3d_data_t *)data;
		newfront3d = true;
	}
	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_DATA, PLAYER_POSITION3D_DATA_STATE, this->segwayrmp3d_id[1]))
	{
		assert(hdr->size == sizeof(player_position3d_data_t));
		rmp3d_data[1] = *(player_position3d_data_t *)data;
		newback3d = true;
	}

	// is it new position3d cmd?
	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_CMD, PLAYER_POSITION3D_CMD_SET_VEL, this->position3d_id))
	{
		return HandlePosition3DCmd((player_position3d_cmd_vel_t*)data);
	}

	// is it new position2d data?
	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, this->segwayrmp2d_id[0]))
	{
		assert(hdr->size == sizeof(player_position2d_data_t));
		rmp2d_data[0] = *(player_position2d_data_t *)data;
		newfront2d = true;
	}
	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, this->segwayrmp2d_id[1]))
	{
		assert(hdr->size == sizeof(player_position2d_data_t));
		rmp2d_data[1] = *(player_position2d_data_t *)data;
		newback2d = true;
	}

	// is it new position2d cmd?
	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, this->position2d_id))
	{
		return HandlePosition2DCmd((player_position2d_cmd_vel_t*)data);
	}

	// is it a geometry request?
	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_POSITION2D_REQ_GET_GEOM,this->position2d_id))
	{
		player_position2d_geom_t geom;
		geom.pose.px = 0;
		geom.pose.py = 0;
		geom.pose.pyaw = 0;
		geom.size.sw = 0.508;
		geom.size.sl = 0.610;

		this->Publish(position2d_id, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_GET_GEOM, &geom, sizeof(geom),NULL);
	}

	// pass the request to the underlying segway rmp position devices for 2d
	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,-1,this->position2d_id))
	{
		Message *msg;
		//@TODO: compare responses of both segways.
		// hack: sending both segways the command, forwarding the response of the second segway rmp only;
		msg = this->segwayrmp2d[0]->Request(this->InQueue,hdr->type,hdr->subtype,(void*)data,hdr->size,&hdr->timestamp);
		msg = this->segwayrmp2d[1]->Request(this->InQueue,hdr->type,hdr->subtype,(void*)data,hdr->size,&hdr->timestamp);

		player_msghdr_t* rephdr = msg->GetHeader();
		void* repdata = msg->GetPayload();
		rephdr->addr = this->position2d_id;
		this->Publish(resp_queue,rephdr,repdata);
		delete msg;
	}

	// pass the request to the underlying segway rmp position devices for 3d
	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,-1,this->position3d_id))
	{
		Message *msg;
		//@TODO: Compare responses of both segways.
		// hack: sending both segways the command, forwarding the response of the second segway rmp only;
		msg = this->segwayrmp3d[0]->Request(this->InQueue,hdr->type,hdr->subtype,(void*)data,hdr->size,&hdr->timestamp);
		msg = this->segwayrmp3d[1]->Request(this->InQueue,hdr->type,hdr->subtype,(void*)data,hdr->size,&hdr->timestamp);

		player_msghdr_t* rephdr = msg->GetHeader();
		void* repdata = msg->GetPayload();
		rephdr->addr = this->position3d_id;
		this->Publish(resp_queue,rephdr,repdata);
		delete msg;
	}

	// is it something we can't handle?
	else
		return -1;

	// if all went well
	return 0;
}

////////////////////////////////////////////////////
// Simple function to forward position 3d commands
int SegwayRMP400::HandlePosition3DCmd(player_position3d_cmd_vel_t *cmd)
{

	for(int i=0;i<2;i++)
	{
		// Push the message forward to each of the subscribed devices
		this->segwayrmp3d[i]->PutMsg(this->InQueue,	PLAYER_MSGTYPE_CMD,
								PLAYER_POSITION3D_CMD_SET_VEL,
								(void*)cmd,sizeof(player_position3d_cmd_vel_t),NULL);
	}
	return 0;
}

////////////////////////////////////////////////////
// Simple function to forward position 2d commands
int SegwayRMP400::HandlePosition2DCmd(player_position2d_cmd_vel_t *cmd)
{

	for(int i=0;i<2;i++)
	{
		// Push the message forward to each of the subscribed devices
		this->segwayrmp2d[i]->PutMsg(this->InQueue,	PLAYER_MSGTYPE_CMD,
								PLAYER_POSITION2D_CMD_VEL,
								(void*)cmd,sizeof(player_position2d_cmd_vel_t),NULL);
	}
	return 0;
}

////////////////////////////////////////////////////
// Simple function to package position data and publish
// Makes an effort to not use or publish duplicate data.
void SegwayRMP400::ProcessData()
{
	/// @todo
	/// Add smarter approach to combine the data from both RMPs. For now, just average.

	bool pos3dready = false;
	bool pos2dready = false;
	
	// Repackage the position3d data if we have new data from both interfaces
	if (newfront3d && newback3d)
	{
		position3d_data.pos.px = (rmp3d_data[0].pos.px + rmp3d_data[1].pos.px)/2.0;
		position3d_data.pos.py = (rmp3d_data[0].pos.py + rmp3d_data[1].pos.py)/2.0;
		position3d_data.pos.pz = (rmp3d_data[0].pos.pz + rmp3d_data[1].pos.pz)/2.0;
		position3d_data.pos.proll = (rmp3d_data[0].pos.proll + rmp3d_data[1].pos.proll)/2.0;
		position3d_data.pos.ppitch = (rmp3d_data[0].pos.ppitch + rmp3d_data[1].pos.ppitch)/2.0;
		position3d_data.pos.pyaw = (rmp3d_data[0].pos.pyaw + rmp3d_data[1].pos.pyaw)/2.0;

		position3d_data.vel.px = (rmp3d_data[0].vel.px + rmp3d_data[1].vel.px)/2.0;
		position3d_data.vel.py = (rmp3d_data[0].vel.py + rmp3d_data[1].vel.py)/2.0;
		position3d_data.vel.pz = (rmp3d_data[0].vel.pz + rmp3d_data[1].vel.pz)/2.0;
		position3d_data.vel.proll = (rmp3d_data[0].vel.proll + rmp3d_data[1].vel.proll)/2.0;
		position3d_data.vel.ppitch = (rmp3d_data[0].vel.ppitch + rmp3d_data[1].vel.ppitch)/2.0;
		position3d_data.vel.pyaw = (rmp3d_data[0].vel.pyaw + rmp3d_data[1].vel.pyaw)/2.0;
		
		newfront3d = false;
		newback3d = false;
		pos3dready = true;
	}

	// Repackage the position 2d data if we have new data from both interfaces
	if (newfront2d && newback2d)
	{
		position2d_data.pos.px = (rmp2d_data[0].pos.px + rmp2d_data[1].pos.px)/2.0;
		position2d_data.pos.py = (rmp2d_data[0].pos.py + rmp2d_data[1].pos.py)/2.0;
		position2d_data.pos.pa = (rmp2d_data[0].pos.pa + rmp2d_data[1].pos.pa)/2.0;

		position2d_data.vel.px = (rmp2d_data[0].vel.px + rmp2d_data[1].vel.px)/2.0;
		position2d_data.vel.py = (rmp2d_data[0].vel.py + rmp2d_data[1].vel.py)/2.0;
		position2d_data.vel.pa = (rmp2d_data[0].vel.pa + rmp2d_data[1].vel.pa)/2.0;
		
		newfront2d = false;
		newback2d = false;
		pos2dready = true;
	}
	
	// Publish every tenth message from both interfaces, making sure it's new data.
	if (!fullspeed && counter >= 10)
	{
		if (pos3dready)
			this->Publish(position3d_id, PLAYER_MSGTYPE_DATA, PLAYER_POSITION3D_DATA_STATE,
					(void*)&position3d_data,sizeof(position3d_data),NULL);
		if (pos2dready)
			this->Publish(position2d_id, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
					(void*)&position2d_data,sizeof(position2d_data),NULL);
		counter = 0;
	}
	
	// Publish all valid messages we are able to combine, only uses new data.
	else if (fullspeed)
	{
		if (pos3dready)
			this->Publish(position3d_id, PLAYER_MSGTYPE_DATA, PLAYER_POSITION3D_DATA_STATE,
					(void*)&position3d_data,sizeof(position3d_data),NULL);
		if (pos2dready)
			this->Publish(position2d_id, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
					(void*)&position2d_data,sizeof(position2d_data),NULL);
		counter = 0;
	}
	
	counter++;
	
	

}
