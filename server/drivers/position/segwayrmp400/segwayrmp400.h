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

#ifndef __SEGWAYRMP400_H_
#define __SEGWAYRMP400_H_

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#if !defined (WIN32)
  #include <unistd.h>
  #include <netinet/in.h>
#endif

#include <libplayercore/playercore.h>


/// SegwayRMP400 Position Driver
class SegwayRMP400 : public ThreadedDriver
{

public:

	/// Standard Constructor
	SegwayRMP400(ConfigFile* cf, int section);

	/// Initialize (Player Standard)
	virtual int MainSetup();

	/// Shutdown (Player Standard)
	virtual void MainQuit();

private:

	// Devices
	Device* segwayrmp2d[2];				///< child segwayrmp200 devices for 2d subsrciption
	Device* segwayrmp3d[2];				///< child segwayrmp200 devices for 3d subsrciption

	// Device Addresses
	player_devaddr_t segwayrmp2d_id[2];		///< 2d Position Interface Address (Output)
	player_devaddr_t segwayrmp3d_id[2];		///< 3d Position Interface Address (Output)

	player_devaddr_t position3d_id;			///< 3d Position Interface Address (Input)
	player_devaddr_t position2d_id;			///< 2d Position Interface Address (Input)

	// Device Data Structures
	player_position2d_data_t rmp2d_data[2];		///< Incoming data from child segwayrmp200 devices 2d
	player_position3d_data_t rmp3d_data[2];		///< Incoming data from child segwayrmp200 devices 3d

	player_position2d_data_t position2d_data;	///< Output data for parent segwayrmp400 device 2d
	player_position2d_cmd_vel_t position2d_cmd;	///< Output cmd for parent segwayrmp400 device 2d

	player_position3d_data_t position3d_data;	///< Output data for parent segwayrmp400 device 3d
	player_position3d_cmd_vel_t position3d_cmd;	///< Output cmd for parent segwayrmp400 device 3d

	// Flags
	bool provide_2d;				///< Provide 2d interface Flag
	bool provide_3d;				///< Provide 3d interface Flag

	int counter;
	/// Main
	void Main();

	/// Process message function (Player Standard)
	int ProcessMessage(QueuePointer &resp_queue, player_msghdr_t* hdr, void* data);

	/// Packages position data and publishes
	void ProcessData();

	/// Internal method to handle position 3D commands
	int HandlePosition3DCmd(player_position3d_cmd_vel_t* cmd);

	/// Internal method to handle position 2D commands
	int HandlePosition2DCmd(player_position2d_cmd_vel_t* cmd);
	
	/// Flags for new data
	bool newfront3d, newback3d, newfront2d, newback2d;

	/// Flag for full speed data reporting.
	bool fullspeed;
};

#endif
