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

#include <laserbeaconproxy.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

// Set the bit properties
int LaserbeaconProxy::SetBits(unsigned char bit_count, unsigned short bit_size)
{
  if(!client)
    return(-1);

  player_laserbeacon_config_t config;

  // TODO: read existing config.
  
  config.subtype = PLAYER_LASERBEACON_SUBTYPE_SETCONFIG;
  config.bit_count = bit_count;
  config.bit_size = htons(bit_size);
  
  return(client->Request(PLAYER_LASERBEACON_CODE,index,(const char*)&config,
                         sizeof(config)));
}

// Set the bit thresholds
int LaserbeaconProxy::SetThresh(unsigned short zero_thresh, unsigned short one_thresh)
{
  if(!client)
    return(-1);

  player_laserbeacon_config_t config;

  // TODO: read existing config.
  
  config.subtype = PLAYER_LASERBEACON_SUBTYPE_SETCONFIG;
  config.zero_thresh = htons(zero_thresh);
  config.one_thresh = htons(one_thresh);

  return(client->Request(PLAYER_LASERBEACON_CODE,index,(const char*)&config,
                         sizeof(config)));
}

void LaserbeaconProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  player_laserbeacon_data_t *data = (player_laserbeacon_data_t*) buffer;
  
  if(hdr.size != sizeof(player_laserbeacon_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of laserbeacon data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_laserbeacon_data_t),hdr.size);
  }

  count = ntohs(data->count);
  bzero(beacons,sizeof(beacons));
  for(unsigned short i = 0; i < count && i < PLAYER_MAX_LASERBEACONS; i++)
  {
    
    beacons[i].id = data->beacon[i].id;
    beacons[i].range = ntohs(data->beacon[i].range);
    beacons[i].bearing = (short)ntohs(data->beacon[i].bearing);
    beacons[i].orient = (short) ntohs(data->beacon[i].orient);
  }
}

// interface that all proxies SHOULD provide
void LaserbeaconProxy::Print()
{
  printf("#Laserbeacon(%d:%d) - %c\n", device, index, access);
  puts("#count");
  printf("%d\n", count);
  puts("#id\trange\tbear\torient");
  for(unsigned short i=0;i<count && i<PLAYER_MAX_LASERBEACONS;i++)
    printf("%u\t%u\t%d\t%d\n", beacons[i].id, beacons[i].range, 
           beacons[i].bearing, beacons[i].orient);
}

