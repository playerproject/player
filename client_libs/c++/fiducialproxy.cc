/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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
 * client-side beacon device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>


void FiducialProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  player_fiducial_data_t *data = (player_fiducial_data_t*) buffer;
  
  if(hdr.size != sizeof(player_fiducial_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of fiducial data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_fiducial_data_t),hdr.size);
  }

  count = ntohs(data->count);
  bzero(beacons,sizeof(beacons));
  for(unsigned short i = 0; i < count && i < PLAYER_FIDUCIAL_MAX_SAMPLES; i++)
  {
    beacons[i].id = ntohs(data->fiducials[i].id);
    beacons[i].pose[0] = (short)ntohs(data->fiducials[i].pose[0]);
    beacons[i].pose[1] = (short)ntohs(data->fiducials[i].pose[1]);
    beacons[i].pose[2] = (short) ntohs(data->fiducials[i].pose[2]);
    beacons[i].upose[0] = (short)ntohs(data->fiducials[i].upose[0]);
    beacons[i].upose[1] = (short)ntohs(data->fiducials[i].upose[1]);
    beacons[i].upose[2] = (short) ntohs(data->fiducials[i].upose[2]);
  }
}

// interface that all proxies SHOULD provide
void FiducialProxy::Print()
{
  printf("#Fiducial(%d:%d) - %c\n", device, index, access);
  puts("#count");
  printf("%d\n", count);
  puts("#id\trange\tbear\torient");
  for(unsigned short i=0;i<count && i<PLAYER_FIDUCIAL_MAX_SAMPLES;i++)
    printf("%u\t%u\t%d\t%d\n", beacons[i].id, beacons[i].pose[0], 
           beacons[i].pose[1], beacons[i].pose[2]);
}

