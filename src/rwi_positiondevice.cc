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
	bzero(&cmd, sizeof(cmd));
	PutCommand((unsigned char *) &cmd, sizeof(cmd));
	
	player_position_data_t data;
	bzero(&data, sizeof(data));
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
	// Working buffer space
	player_rwi_config_t    cfg;
	player_position_cmd_t  cmd;
	player_position_data_t data;
	
	void *client;
	
	#ifdef USE_MOBILITY
	MobilityActuator::ActuatorData_var odo_data;
	
	int16_t degrees;
	#endif // USE_MOBILITY
	
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (true) {
	
		// First, check for a configuration request
		if (GetConfig(&client, (void *) &cfg, sizeof(cfg))) {
		    switch (cfg.request) {
		    	case PLAYER_RWI_POSITION_MOTOR_POWER_REQ:
		    		// RWI does not turn off motor power:
		    		// the motors are always on when connected
		    		PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0);
					break;
			    case PLAYER_RWI_POSITION_RESET_ODO_REQ:
					ResetOdometry();
					PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0);
					break;
				default:
					printf("rwi_position device received unknown %s",
					       "configuration request\n");
					PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0);
					break;
	    	}
		}

		// Next, process a command
		GetCommand((unsigned char *) &cmd, sizeof(cmd));
		// always apply the latest speed command: RWI stops us otherwise
    	PositionCommand(ntohs(cmd.speed), ntohs(cmd.turnrate));
	
		// Finally, collect new data
#ifdef USE_MOBILITY
		odo_data = odo_state->get_sample(0);
		
		// yes, this looks backwards, but RWI puts Y before X
		data.ypos = htonl((int32_t) ((odo_data->position[0] + odo_correct_y)
		                             * 1000.0));
		data.xpos = htonl((int32_t) ((odo_data->position[1] + odo_correct_x)
		                             * 1000.0));
		degrees = (int16_t)
		           RTOD(NORMALIZE(odo_data->position[2] + odo_correct_theta));
		if (degrees < 0) degrees += 180;
		data.theta = htons(degrees);
				
		// velocity array seems to be flaky... not always there
		if (odo_data->velocity.length()) {
			data.speed = htons((uint16_t) (1000.0*
			sqrt(odo_data->velocity[0]*odo_data->velocity[0] +
			     odo_data->velocity[1]*odo_data->velocity[1])));
			degrees = (int16_t) RTOD(NORMALIZE(odo_data->velocity[2]));
			data.turnrate = (int16_t) htons(degrees);
		} else {
			PLAYER_TRACE0("MOBILITY SUCKS: Unable to read velocity data!\n");
			data.speed = 0;
			data.turnrate = 0;
		}
#else
		data.xpos = data.ypos = 0;
		data.theta = data.speed = data.turnrate = 0;
#endif			// USE_MOBILITY

		// FIXME: I do not have a compass, so I don't know how to find it		
		data.compass = 0;
		
		// FIXME: I can implement this
		data.stalls = 0;
	
	    PutData((unsigned char *) &data, sizeof(data), 0, 0);
	
	    pthread_testcancel();
	}
	
	// should not reach this point
	pthread_exit(NULL);
}

void
CRWIPositionDevice::PositionCommand(int16_t speed, int16_t rot_speed)
{
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
    PLAYER_TRACE0("Resetting RWI Odometry");
#ifdef USE_MOBILITY
    MobilityActuator::ActuatorData_var odo_data;
    odo_data = odo_state->get_sample(0);

    // yes, this looks backwards, but RWI puts Y before X
    odo_correct_y = -odo_data->position[0];
    odo_correct_x = -odo_data->position[1];
    odo_correct_theta = -odo_data->position[2];
#endif				// USE_MOBILITY
}
