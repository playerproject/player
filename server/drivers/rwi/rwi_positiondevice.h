/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2002
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *     Nik Melchior
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
 *   the RWI position device.  accepts commands for changing
 *   speed and rotation, and returns data on x, y, theta.
 *   (Compass data will come later).
 */

#ifndef _RWI_POSITIONDEVICE_H
#define _RWI_POSITIONDEVICE_H

#include <rwidevice.h>

class CRWIPositionDevice: public CRWIDevice {
public:
	CRWIPositionDevice (int argc, char *argv[])
		: CRWIDevice(argc, argv,
		             sizeof(player_position_data_t),
		             sizeof(player_position_cmd_t),
		             1,1),
		  old_xpos(0),
		  old_ypos(0),
		  old_theta(0),
		  moving(false),
		  last_known_speed(0),
		  last_known_turnrate(0)
		{}
	
	virtual int Setup ();
	virtual int Shutdown ();
	
	virtual void Main ();
	
private:
	
	#ifdef USE_MOBILITY
	MobilityActuator::ActuatorState_var base_state;
	MobilityActuator::ActuatorState_var odo_state;
	
	// for "resetting" the odometry
	double odo_correct_x, odo_correct_y, odo_correct_theta;
	#endif // USE_MOBILITY
	
	// for determining whether we are stalled
	int32_t old_xpos, old_ypos;
	uint16_t old_theta;
	bool moving;
	
	// sometimes velocity cannot be read
	uint16_t last_known_speed;
	int16_t last_known_turnrate;
	
	void PositionCommand (const int16_t speed, const int16_t rot_speed);
	void ResetOdometry ();
};

#endif // _RWI_POSITIONDEVICE_H

