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

#include <rwi_sonardevice.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif


CRWISonarDevice::CRWISonarDevice(int argc, char **argv)
    : CRWIDevice(argc, argv,
                 sizeof(player_sonar_data_t),
                 0 /* no commands for sonar */,
		         1,1)
{
	// default to upper ring if none is specified
	upper = true;
	
	// parse cmd line args
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "upper"))
			upper = true;
		else if (!strcmp(argv[i], "lower"))
			upper = false;
	}
}

int
CRWISonarDevice::Setup()
{
	#ifdef USE_MOBILITY
	CORBA::Object_ptr temp;
	char *path = upper ? "/Sonar/Segment" : "/BaseSonar/Segment";
	
	if (RWIConnect(&temp, path) < 0) {
		fprintf(stderr, "rwi_sonardevice unable to connect.\n");
		return -1;
	} else {
	    sonar_state = MobilityGeometry::SegmentState::_narrow(temp);
	}
	#else
	printf("Cannot create rwi_sonar device without mobility.\n");
	return -1;
	#endif			// USE_MOBILITY
	
	// Zero the common buffer
	player_sonar_data_t data;
	bzero(&data, sizeof(data));
	PutData((unsigned char *) &data, sizeof(data), 0, 0);	
	
	StartThread();
	return 0;
}

int
CRWISonarDevice::Shutdown()
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
CRWISonarDevice::Main()
{
	// Working buffer space
	player_rwi_config_t cfg;
	player_sonar_data_t data;
	
	void *client;
	
	#ifdef USE_MOBILITY
	MobilityGeometry::SegmentData_var sonar_data;
	#endif // USE_MOBILITY
		
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (true) {
	
		// First, check for a configuration request
		if (GetConfig(&client, (void *) &cfg, sizeof(cfg))) {
		    switch (cfg.request) {
			    case PLAYER_RWI_SONAR_POWER_REQ:
		    		// RWI does not turn off sonar power: always on
		    		PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0);
					break;
				default:
					printf("rwi_sonar device received unknown %s",
					       "configuration request\n");
					PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0);
					break;
	    	}
		}

		// Sonar takes no commands to process
	
		// Finally, collect new data
#ifdef USE_MOBILITY
		sonar_data = sonar_state->get_sample(0);
		
		data.range_count = htons((uint16_t) sonar_data->org.length());
		
		for (unsigned int i = 0;
		     (i < sonar_data->org.length()) && (i < PLAYER_NUM_SONAR_SAMPLES);
		     i++) {
		    data.ranges[i] = htons((uint16_t) (1000.0 *
				sqrt(((sonar_data->org[i].x - sonar_data->end[i].x) *
				      (sonar_data->org[i].x - sonar_data->end[i].x)) +
				     ((sonar_data->org[i].y - sonar_data->end[i].y) *
				      (sonar_data->org[i].y - sonar_data->end[i].y)))));
		}
#else
		data.range_count = 0;
#endif			// USE_MOBILITY

	    PutData((unsigned char *) &data, sizeof(data), 0, 0);
	
	    pthread_testcancel();
	}
	
	// should not reach this point
	pthread_exit(NULL);
}