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
 * client-side laser device 
 */

#include <laserproxy.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

int LaserProxy::Configure(short min_angle, short max_angle, 
                      unsigned short resolution, bool intensity)
{
  if(!client)
    return(-1);

  player_laser_config_t config;

  config.min_angle = htons(min_angle);
  config.max_angle = htons(max_angle);
  config.resolution = htons(resolution);
  config.intensity = intensity ? 1 : 0;

  return(client->Request(PLAYER_LASER_CODE,index,(const char*)&config,
                         sizeof(config)));
}

void LaserProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_laser_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of laser data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_laser_data_t),hdr.size);
  }

  min_angle = (short)ntohs(((player_laser_data_t*)buffer)->min_angle);
  max_angle = (short)ntohs(((player_laser_data_t*)buffer)->max_angle);
  resolution = ntohs(((player_laser_data_t*)buffer)->resolution);
  range_count = ntohs(((player_laser_data_t*)buffer)->range_count);

  bzero(ranges,sizeof(ranges));
  bzero(intensities,sizeof(intensities));
  min_left = 10000;
  min_right = 10000;
  for(unsigned short i=0;i<range_count && i<PLAYER_NUM_LASER_SAMPLES;i++)
  {
    // only want the lower 13 bits for range info
    ranges[i] = ntohs(((player_laser_data_t*)buffer)->ranges[i]) & 0x1FFF;
    // only want the higher 3 bits for intensity info
    intensities[i] = 
      (unsigned char)((ntohs(((player_laser_data_t*)buffer)->ranges[i])) >> 13);

    if(i>(range_count/2) && ranges[i] < min_left)
      min_left = ranges[i];
    else if(i<(range_count/2) && ranges[i] < min_right)
      min_right = ranges[i];
  }
}

// interface that all proxies SHOULD provide
void LaserProxy::Print()
{
  printf("#Laser(%d:%d) - %c\n", device, index, access);
  puts("#min\tmax\tres\tcount");
  printf("%d\t%d\t%u\t%u\n", min_angle,max_angle,resolution,range_count);
  puts("#range\tintensity");
  for(unsigned short i=0;i<range_count && i<PLAYER_NUM_LASER_SAMPLES;i++)
    printf("%u %u\n", ranges[i],intensities[i]);
}

