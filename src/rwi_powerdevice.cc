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

#include <rwi_powerdevice.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

int
CRWIPowerDevice::Setup()
{
#ifdef USE_MOBILITY
	CORBA::Object_ptr temp;
	
	if (RWIConnect(&temp, "/Power") < 0) {
		fprintf(stderr, "rwi_powerdevice unable to connect.\n");
		return -1;
	} else {
	    power_state = MobilityData::PowerManagementState::_narrow(temp);
	}
#else
	printf("Cannot create rwi_power device without mobility.\n");
	return -1;
#endif			// USE_MOBILITY
	
	// Zero the common buffer
	player_power_data_t data;
	memset(&data, 0, sizeof(data));
	PutData((unsigned char *) &data, sizeof(data), 0, 0);	
	
	StartThread();
	return 0;
}

int
CRWIPowerDevice::Shutdown()
{
	StopThread();
	return 0;
}
	
void
CRWIPowerDevice::Main()
{
	// Working buffer space
	player_rwi_config_t cfg;
	player_power_data_t data;
	
	void *client;
	
#ifdef USE_MOBILITY
	MobilityData::PowerManagementStatus *power_data;
#endif // USE_MOBILITY
		
    if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) != 0) {
		perror("rwi_power call to pthread_setcanceltype failed");
    }

	while (true) {
	
		// First, check for a configuration request
		if (GetConfig(&client, (void *) &cfg, sizeof(cfg))) {
		    switch (cfg.request) {
			    case PLAYER_MAIN_POWER_REQ:
		    		// RWI does not turn off main power:
		    		// ignore this request
		    		
		    		if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,
		    		             NULL, NULL, 0)) {
		    			PLAYER_ERROR("Failed to PutReply in "
		    			             "rwi_powerdevice.\n");
		    		}
					break;
				default:
					printf("rwi_power device received unknown %s",
					       "configuration request\n");
					if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,
		    		             NULL, NULL, 0)) {
		    			PLAYER_ERROR("Failed to PutReply in "
		    			             "rwi_powerdevice.\n");
		    		}
					break;
	    	}
		}

		// Power takes no commands to process
	
		// Finally, collect new data
#ifdef USE_MOBILITY
		power_data = power_state->get_sample(0);
	
		data.charge = htons((uint16_t)
		                    (100.0 * power_data->RegulatorVoltage[0]));
#else
		data.charge = 0;
#endif			// USE_MOBILITY

		PutData((unsigned char *) &data, sizeof(data), 0, 0);
	
	    pthread_testcancel();
	}
	
	// should not reach this point
	pthread_exit(NULL);
}

