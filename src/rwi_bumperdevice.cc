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

#include <rwi_bumperdevice.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>

CRWIBumperDevice::CRWIBumperDevice(int argc, char **argv)
    : CRWIDevice(argc, argv,
                 sizeof(player_bumper_data_t),
                 0 /* no commands for bumpers */,
		         1,1)
{
	// default to upper set if none is specified
	upper = true;
	bumper_count = PLAYER_NUM_RWI_BUMPERS_UPPER;
		
	// parse cmd line args
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "upper")) {
			upper = true;
			bumper_count = PLAYER_NUM_RWI_BUMPERS_UPPER;
		} else if (!strcmp(argv[i], "lower")) {
			upper = false;
			bumper_count = PLAYER_NUM_RWI_BUMPERS_LOWER;
		}
	}
}

int
CRWIBumperDevice::Setup()
{
	#ifdef USE_MOBILITY
	CORBA::Object_ptr temp;
	char *path = upper ? "/EnclosureContact/Point" : "/BaseContact/Point";
	
	if (RWIConnect(&temp, path) < 0) {
		fprintf(stderr, "rwi_bumperdevice unable to connect.\n");
		return -1;
	} else {
	    bumper_state = MobilityGeometry::PointState::_narrow(temp);
	}
	#else
	printf("Cannot create rwi_bumper device without mobility.\n");
	return -1;
	#endif			// USE_MOBILITY
	
	// Zero the common buffer
	player_bumper_data_t data;
	bzero(&data, sizeof(data));
	PutData((unsigned char *) &data, sizeof(data), 0, 0);	
	
	StartThread();
	return 0;
}

int
CRWIBumperDevice::Shutdown()
{
	StopThread();
	return 0;
}
	
void
CRWIBumperDevice::Main()
{
	// Working buffer space
	player_rwi_config_t  cfg;
	player_bumper_data_t data;
	
	void *client;
	
	#ifdef USE_MOBILITY
	MobilityGeometry::Point3Data_var bumper_data;
	#endif // USE_MOBILITY
		
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (true) {
	
		// First, check for a configuration request
		if (GetConfig(&client, (void *) &cfg, sizeof(cfg))) {
		    switch (cfg.request) {
			    case PLAYER_RWI_BUMPER_POWER_REQ:
		    		// RWI does not turn off bumper power: always on
		    		PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0);
					break;
				default:
					printf("rwi_bumper device received unknown %s",
					       "configuration request\n");
					PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0);
					break;
	    	}
		}

		// Bumpers take no commands to process
	
		// Finally, collect new data
#ifdef USE_MOBILITY
		bumper_data = bumper_state->get_sample(0);
		
		data.bumper_count = bumper_count;
				
		for (unsigned int i = 0; i < bumper_count; i++) {
		    if (bumper_data->point[i].flags == 1)
				data.bumpfield |= (1 << i);
		    else
				data.bumpfield &= ~(1 << i);
		}
#else
		data.bumper_count = 0;
		data.bumpfield = 0;
#endif			// USE_MOBILITY

	    PutData((unsigned char *) &data, sizeof(data), 0, 0);
	
	    pthread_testcancel();
	}
	
	// should not reach this point
	pthread_exit(NULL);
}