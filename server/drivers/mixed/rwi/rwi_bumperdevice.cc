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
#include <drivertable.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>

// a factory creation function
CDevice* RWIBumper_Init(char* interface, ConfigFile* cf, int section)
{
  return((CDevice *)(new CRWIBumperDevice(interface, cf, section)));
}

void 
RWIBumper_Register(DriverTable* table)
{
  table->AddDriver("rwi_bumper", PLAYER_READ_MODE, RWIBumper_Init);
}
	

CRWIBumperDevice::CRWIBumperDevice(char* interface, ConfigFile* cf, int section)
    : CRWIDevice(interface, cf, section,
                 sizeof(player_bumper_data_t),
                 0 /* no commands for bumpers */,
		         1,1)
{
        char* tmp;

        tmp = (char*)cf->ReadString(section, "array", "upper");

        if (!strcmp(tmp, "upper")) {
          upper = true;
        } else if(!strcmp(tmp, "lower")) {
          upper = false;
        }
}

int
CRWIBumperDevice::Setup()
{
#ifdef USE_MOBILITY
	CORBA::Object_ptr temp;
	const char *path = upper ? "/EnclosureContact/Point" : "/BaseContact/Point";
	
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
	memset(&data, 0, sizeof(data));
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
	// start enabled
	bool enabled = true;
	
	// Working buffer space
	player_rwi_config_t  cfg;
	player_bumper_data_t data;
	
	void *client;
	
#ifdef USE_MOBILITY
	MobilityGeometry::Point3Data_var bumper_data;
#endif // USE_MOBILITY
		
    if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) != 0) {
		perror("rwi_bumper call to pthread_setcanceltype failed");
    }

	while (true) 
        {

          // First, check for a configuration request
          if (GetConfig(&client, (void *) &cfg, sizeof(cfg))) {
            switch (cfg.request) {
              case PLAYER_BUMPER_GET_GEOM_REQ:
                // FIXME: not yet implemented
                if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,
                             NULL, NULL, 0)) {
                  PLAYER_ERROR("Failed to PutReply in "
                               "rwi_bumperdevice.\n");
                }
                break;
              default:
                printf("rwi_bumper device received unknown %s",
                       "configuration request\n");
                if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,
                             NULL, NULL, 0)) {
                  PLAYER_ERROR("Failed to PutReply in "
                               "rwi_bumperdevice.\n");
                }
                break;
            }
          }

          // Bumpers take no commands to process

          // Finally, collect new data
          if (enabled) {
#ifdef USE_MOBILITY
            //bumper_data = bumper_state->get_sample(0);

            data.bumper_count = bumper_data->point.length();
            bzero(data.bumpers, sizeof(data.bumpers));

            for (unsigned int i = 0; (i < bumper_data->point.length())
                 && (i < PLAYER_MAX_BUMPER_SAMPLES); i++) {
              if (bumper_data->point[i].flags == 1)
                data.bumpers[i] = 1;
              else
                data.bumpers[i] = 0;
            }
#else
            data.bumper_count = 0;
            bzero(data.bumpers, sizeof(data.bumpers));
#endif			// USE_MOBILITY

            PutData((unsigned char *) &data, sizeof(data), 0, 0);
          }

          pthread_testcancel();
        }

	// should not reach this point
	pthread_exit(NULL);
}

