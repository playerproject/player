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
 * $Id$
 *
 *   the RWI position device.  accepts commands for changing
 *   speed and rotation, and returns data on x,y,theta.
 *   (Compass data will come).
 */

#include <rwi_positiondevice.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif
#include <math.h>


int
CRWIPositionDevice::Setup()
{
#ifdef USE_MOBILITY
	CORBA::Object_ptr temp;
	
	if (RWIConnect(&temp, "/Drive/Command") < 0) {
		fprintf(stderr, "rwi_positiondevice unable to connect.\n");
		return -1;
	} else {
	    base_state = MobilityActuator::ActuatorState::_duplicate(
			MobilityActuator::ActuatorState::_narrow(temp));
	}
	
	if (RWIConnect(&temp, "/Drive/State") < 0) {
		fprintf(stderr, "rwi_positiondevice unable to connect.\n");
		return -1;
	} else {
	    odo_state = MobilityActuator::ActuatorState::_duplicate(
	    	MobilityActuator::ActuatorState::_narrow(temp));
	}
	
	odo_correct_x = odo_correct_y = odo_correct_theta = 0.0;

#else
	printf("Cannot create rwi_position device without mobility.\n");
	return -1;
#endif			// USE_MOBILITY
	
	// Zero the common buffers
	player_position_cmd_t cmd;
	memset(&cmd, 0, sizeof(cmd));
	PutCommand((unsigned char *) &cmd, sizeof(cmd));
	
	player_position_data_t data;
	memset(&data, 0, sizeof(data));
	PutData((unsigned char *) &data, sizeof(data), 0, 0);	
	
	ResetOdometry();
	StartThread();
	return 0;
}

int
CRWIPositionDevice::Shutdown()
{
	StopThread();
	// Since there are no more position device clients, stop the robot.
	PositionCommand(0,0);
	return 0;
}
	
/*
 * RWI returns distance data in meters as doubles.
 * Player prefers millimeters.
 * Thus, many values are multiplied by 1000.0 here
 * before they are stored.
 */

void
CRWIPositionDevice::Main()
{
	// start enabled
	bool enabled = true;
	
	// Working buffer space
	player_position_config_t cfg;
	player_position_cmd_t    cmd;
	player_position_data_t   data;
	
	void *client;
	
#ifdef USE_MOBILITY
	MobilityActuator::ActuatorData_var odo_data;
	int16_t degrees;
	double cos_theta, sin_theta, tmp_y, tmp_x;
#endif // USE_MOBILITY
	
    if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) != 0) {
		perror("rwi_position call to pthread_setcanceltype failed");
    }

	while (true) {
	
		// First, check for a configuration request
		if (GetConfig(&client, (void *) &cfg, sizeof(cfg))) {
		    switch (cfg.request) {
		    	case PLAYER_POSITION_MOTOR_POWER_REQ:
		    		// RWI does not turn off motor power:
		    		// the motors are always on when connected.
		    		// we simply stop processing movement commands
		    		if (cfg.value == 0)
		    			enabled = false;
		    		else
		    			enabled = true;
		    		
		    		if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		    		             NULL, NULL, 0)) {
		    			PLAYER_ERROR("Failed to PutReply in "
		    			             "rwi_positiondevice.\n");
		    		}
					break;
				case PLAYER_POSITION_VELOCITY_CONTROL_REQ:
					// unsupported for RWI
					if (PutReply(client, PLAYER_MSGTYPE_RESP_NSUP,
		    		             NULL, NULL, 0)) {
		    			PLAYER_ERROR("Failed to PutReply in "
		    			             "rwi_positiondevice.\n");
		    		}
					break;
			    case PLAYER_POSITION_RESET_ODOM_REQ:
					ResetOdometry();
					if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		    		             NULL, NULL, 0)) {
		    			PLAYER_ERROR("Failed to PutReply in "
		    			             "rwi_positiondevice.\n");
		    		}
					break;
				case PLAYER_POSITION_GET_GEOM_REQ:
					// FIXME: evil hack, no actual data
					player_position_geom_t geom;
					geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
					geom.pose[0] = geom.pose[1] = geom.pose[2] = 0;
					geom.size[0] = geom.size[1] = htons((uint16_t)525);
					if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		    		             NULL, &geom, sizeof(geom))) {
		    			PLAYER_ERROR("Failed to PutReply in "
		    			             "rwi_positiondevice.\n");
		    		}
					break;
				default:
					printf("rwi_position device received unknown "
					       "configuration request\n");
					if (PutReply(client, PLAYER_MSGTYPE_RESP_NSUP,
		    		             NULL, NULL, 0)) {
		    			PLAYER_ERROR("Failed to PutReply in "
		    			             "rwi_positiondevice.\n");
		    		}
					break;
	    	}
		}

		// Next, process a command
		GetCommand((unsigned char *) &cmd, sizeof(cmd));
		
		if (enabled) {
			// always apply the latest speed command: RWI stops us otherwise
    		PositionCommand(ntohs(cmd.speed), ntohs(cmd.turnrate));
    	}
	
		// Finally, collect new data
#ifdef USE_MOBILITY
		odo_data = odo_state->get_sample(0);
		
		// get ready to rotate our coordinate system
		// (remembering that RWI puts y before x)
		tmp_y = odo_data->position[0] + odo_correct_y;
		tmp_x = odo_data->position[1] + odo_correct_x;
		cos_theta = cos(-odo_correct_theta);
		sin_theta = sin(-odo_correct_theta);
		
		data.xpos = htonl((int32_t) ((cos_theta*tmp_x - sin_theta*tmp_y)
		                             * 1000.0));
		data.ypos = htonl((int32_t) ((sin_theta*tmp_x + cos_theta*tmp_y)
		                             * 1000.0));
		degrees = (int16_t)
		           RTOD(NORMALIZE(odo_data->position[2] + odo_correct_theta));
		if (degrees < 0)
			degrees += 360;
		data.theta = htons((uint16_t) degrees);
				
		// velocity array seems to be flaky... not always there
		if (odo_data->velocity.length()) {
			data.speed = htons((uint16_t) (1000.0*
				sqrt(odo_data->velocity[0]*odo_data->velocity[0] +
				     odo_data->velocity[1]*odo_data->velocity[1])));
			degrees = (int16_t) RTOD(odo_data->velocity[2]);
			data.turnrate = (int16_t) htons(degrees);
			
			// just in case we cannot get them next time
			last_known_speed = data.speed;
			last_known_turnrate = data.turnrate;
			
		} else {
			PLAYER_TRACE0("MOBILITY SUCKS: Unable to read velocity data!\n");
			// replay the last valid values
			data.speed = last_known_speed;
			data.turnrate = last_known_turnrate;
		}
#else
		data.xpos = data.ypos = 0;
		data.theta = data.speed = data.turnrate = 0;
#endif			// USE_MOBILITY

		// FIXME: I do not have a compass, so I don't know how to find it		
		data.compass = 0;
		
		// determine stall value
		if (moving && old_xpos == data.xpos && old_ypos == data.ypos
		    && old_theta == data.theta) {
		    data.stalls = 1;
		} else {
			data.stalls = 0;
		}
		
		// Keep a copy of our new data for stall computation
		old_xpos = data.xpos;
		old_ypos = data.ypos;
		old_theta = data.theta;
	
	    PutData((unsigned char *) &data, sizeof(data), 0, 0);
	
	    pthread_testcancel();
	}
	
	// should not reach this point
	pthread_exit(NULL);
}

void
CRWIPositionDevice::PositionCommand(const int16_t speed,
                                    const int16_t rot_speed)
{
	if (speed == 0 && rot_speed == 0)
		moving = false;
	else
		moving = true;
		
#ifdef USE_MOBILITY
    MobilityActuator::ActuatorData position;
    position.velocity.length(2);

    position.velocity[0] = speed / 1000.0;
    position.velocity[1] = DTOR(rot_speed);

    base_state->new_sample(position, 0);
#endif				// USE_MOBILITY
}


void
CRWIPositionDevice::ResetOdometry()
{
	// Update stall detection variables as well
	old_xpos = old_ypos = 0;
	old_theta = 0;
#ifdef USE_MOBILITY
    MobilityActuator::ActuatorData_var odo_data;
    odo_data = odo_state->get_sample(0);

    // yes, this looks backwards, but RWI puts Y before X
    odo_correct_y = -odo_data->position[0];
    odo_correct_x = -odo_data->position[1];
    odo_correct_theta = -odo_data->position[2];
#endif				// USE_MOBILITY
}
