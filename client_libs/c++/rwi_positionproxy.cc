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

#include <rwi_positionproxy.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif
    
// send a motor command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int
RWIPositionProxy::SetSpeed(const int16_t speed, const int16_t turn_rate)
{
	if(!client)
		return(-1);

	player_position_cmd_t cmd;

	cmd.speed = htons(speed);
	cmd.turnrate = htons(turn_rate);

	return(client->Write(PLAYER_RWI_POSITION_CODE, index,
           (const char *) &cmd, sizeof(cmd)));
}

// enable/disable the motors
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int
RWIPositionProxy::SetMotorState(const unsigned char state)
{
	if(!client)
		return(-1);

	player_rwi_config_t cfg;

	cfg.request = PLAYER_POSITION_MOTOR_POWER_REQ;
	cfg.value = state;


	return(client->Request(PLAYER_RWI_POSITION_CODE, index,
           (const char *) &cfg, sizeof(cfg)));
}

// not supported by RWI robots
int
RWIPositionProxy::SelectVelocityControl(unsigned char mode)
{
	fprintf(stderr, "SelectVelocityControl not supported by RWI robots.\n");
	return -1;
}

// reset odometry to (0,0,0)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int
RWIPositionProxy::ResetOdometry()
{
	if(!client)
		return(-1);

	player_rwi_config_t cfg;

	cfg.request = PLAYER_POSITION_RESET_ODOM_REQ;

	return(client->Request(PLAYER_RWI_POSITION_CODE, index,
           (const char *) &cfg, sizeof(cfg)));
}

void
RWIPositionProxy::FillData(player_msghdr_t hdr, const char *buffer)
{
	if(hdr.size != sizeof(player_position_data_t)) {
		if(player_debug_level(-1) >= 1)
      		fprintf(stderr,"WARNING: rwi_positionproxy expected %d bytes of "
                    "position data, but received %d. Unexpected results may "
                    "ensue.\n",
                    sizeof(player_position_data_t),hdr.size);
	}

	xpos = (int32_t)ntohl(((player_position_data_t *)buffer)->xpos);
	ypos = (int32_t)ntohl(((player_position_data_t *)buffer)->ypos);
	theta = (uint16_t)ntohs(((player_position_data_t *)buffer)->theta);
	speed = (uint16_t)ntohs(((player_position_data_t *)buffer)->speed);
	turn_rate = (uint16_t)ntohs(((player_position_data_t *)buffer)->turnrate);
	compass = ntohs(((player_position_data_t *)buffer)->compass);
	stalls = ((player_position_data_t *)buffer)->stalls;
}

// interface that all proxies SHOULD provide
void
RWIPositionProxy::Print()
{
	printf("#RWIPosition(%d:%d) - %c\n", device, index, access);
	puts("#xpos\typos\ttheta\tspeed\tturn\tcompass\tstalls");
	printf("%d\t%d\t%d\t%d\t%d\t%u\t%d",
	       xpos,ypos,theta,speed,turn_rate,compass,stalls);
	puts(" ");
}
