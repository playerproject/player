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

#include <rwi_laserdevice.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif


int
CRWILaserDevice::Setup()
{
	#ifdef USE_MOBILITY
	// Cannot use RWIConnect since laser exists independently of the robot
	try {
		laser_state = MobilityGeometry::SegmentState::_narrow(
		    helper->find_object("laser/Laser/Segment"));
	} catch(...) {
	    fprintf(stderr, "Cannot get laser interface\n");
	    return -1;
	}
	#else
	printf("Cannot create rwi_laser device without mobility.\n");
	return -1;
	#endif			// USE_MOBILITY
		
	// Zero the common buffer
	player_laser_data_t data;
	memset(&data, 0, sizeof(data));
	PutData((unsigned char *) &data, sizeof(data), 0, 0);	
	
	StartThread();
	return 0;
}

int
CRWILaserDevice::Shutdown()
{
	StopThread();
	return 0;
}
	
/*
 * RWI returns distance data in meters as doubles.
 * Player prefers millimeters.
 * Thus, values are multiplied by 1000.0 here
 * before they are stored.
 */
void
CRWILaserDevice::Main()
{
	// start enabled
	bool enabled = true;
	
	// Working buffer space
	player_rwi_config_t cfg;
	player_laser_data_t data;
	
	void *client;
	
#ifdef USE_MOBILITY
	MobilityGeometry::SegmentData_var laser_data;
#endif // USE_MOBILITY
		
    if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) != 0) {
		perror("rwi_laser call to pthread_setcanceltype failed");
    }

	while (true) {
	
		// First, check for a configuration request
		if (GetConfig(&client, (void *) &cfg, sizeof(cfg))) {
		    switch (cfg.request) {
			    case PLAYER_LASER_POWER_REQ:
		    		// RWI does not turn off laser power: all we can do is
		    		// stop updating the data
		    		if (cfg.value == 0)
		    			enabled = false;
		    		else
		    			enabled = true;
		    			
		    		if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
		    		             NULL, NULL, 0)) {
		    			PLAYER_ERROR("Failed to PutReply in "
		    			             "rwi_laserdevice.\n");
		    		}
					break;
				case PLAYER_LASER_GET_GEOM:
					// FIXME: not yet implemented
					if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,
		    		             NULL, NULL, 0)) {
		    			PLAYER_ERROR("Failed to PutReply in "
		    			             "rwi_laserdevice.\n");
		    		}
					break;
				default:
					printf("rwi_laser device received unknown %s",
					       "configuration request\n");
					if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,
		    		             NULL, NULL, 0)) {
		    			PLAYER_ERROR("Failed to PutReply in "
		    			             "rwi_laserdevice.\n");
		    		}
					break;
	    	}
		}

		// Laser takes no commands to process
	
		// Finally, collect new data
		if (enabled) {
#ifdef USE_MOBILITY
			laser_data = laser_state->get_sample(0);
		
			data.range_count = htons((uint16_t) laser_data->end.length());
		
			for (unsigned int i = 0; (i < laser_data->end.length())
			                         && (i < PLAYER_NUM_LASER_SAMPLES); i++) {
			    data.ranges[i] = htons((uint16_t) (1000.0 *
			    	sqrt(laser_data->end[i].x * laser_data->end[i].x +
			    	     laser_data->end[i].y * laser_data->end[i].y)));
			}
#else
			data.range_count = 0;
#endif			// USE_MOBILITY

	    	PutData((unsigned char *) &data, sizeof(data), 0, 0);
	    }
	
	    pthread_testcancel();
	}
	
	// should not reach this point
	pthread_exit(NULL);
}

