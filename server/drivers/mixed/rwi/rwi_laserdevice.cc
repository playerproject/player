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
#if HAVE_STRINGS_H
  #include <strings.h>
#endif

CDevice* RWILaser_Init(char* interface, ConfigFile* cf, int section)
{
  return((CDevice *)(new CRWILaserDevice(interface, cf, section)));
}

void 
RWILaser_Register(DriverTable* table)
{
  table->AddDriver("rwi_laser", PLAYER_READ_MODE, RWILaser_Init);
}

int
CRWILaserDevice::Setup()
{
	#ifdef USE_MOBILITY

        char fullpath[RWI_MOBILITY_PATH_MAX];
        sprintf(fullpath, "%s/Laser/Segment", name);

	// Cannot use RWIConnect since laser exists independently of the robot
	try {
		laser_state = MobilityGeometry::SegmentState::_narrow(
 		    helper->find_object(fullpath));
//		    helper->find_object("laser/Laser/Segment"));
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
  player_laser_geom_t geom;
  
  void *client;
  
#ifdef USE_MOBILITY
  MobilityGeometry::SegmentData_var laser_data;
#endif // USE_MOBILITY
  
  if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) != 0)
  {
    perror("rwi_laser call to pthread_setcanceltype failed");
  }
  
  while (true) 
  {
      
    // First, check for a configuration request
    if (GetConfig(&client, (void *) &cfg, sizeof(cfg))) 
    {
      switch (cfg.request) 
	    {
        case PLAYER_LASER_GET_GEOM:
          geom.subtype=PLAYER_LASER_GET_GEOM;
          geom.pose[0]=0;
          geom.pose[1]=0;
          geom.pose[2]=0;
          geom.size[0]=ntohs(150);
          geom.size[1]=ntohs(180);

          if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
                       NULL, &geom, sizeof(geom))) 
          {
            PLAYER_ERROR("Failed to PutReply in "
                         "rwi_laserdevice.\n");
          }
          break;
        default:
          printf("rwi_laser device received unknown %s",
                 "configuration request\n");
          if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,
                       NULL, NULL, 0)) 
          {
            PLAYER_ERROR("Failed to PutReply in "
                         "rwi_laserdevice.\n");
          }
          break;
	    }
    }
      
    // Laser takes no commands to process
      
    // Finally, collect new data
    if (enabled) 
    {
#ifdef USE_MOBILITY
      laser_data = laser_state->get_sample(0);
      //printf("Data recieved is %d\n", laser_data->end.length());  

      data.min_angle=htons(-8950);
      data.max_angle=htons(8950);
      data.range_res = htons(1);
      data.resolution=htons(100);

      data.range_count = htons((uint16_t) laser_data->end.length());
	  
      for (unsigned int i = 0; (i < laser_data->end.length())
             && (i < PLAYER_LASER_MAX_SAMPLES); i++) 
	    {
	      //printf("%.3f %.3f : ", laser_data->end[i].x, laser_data->end[i].y);
	      data.ranges[i] = htons((uint16_t) (1000.0 *
                                           sqrt(laser_data->end[i].x * laser_data->end[i].x +
                                                laser_data->end[i].y * laser_data->end[i].y)));
        data.intensity[i] = 0;
	    }
      //printf("\n");
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

