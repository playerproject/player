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

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>

#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

int LaserProxy::Configure(short tmp_min_angle, short tmp_max_angle, 
                      unsigned short tmp_resolution, bool tmp_intensity)
{
  if(!client)
    return(-1);

  player_laser_config_t config;

  config.subtype = PLAYER_LASER_SET_CONFIG;
  config.min_angle = htons(tmp_min_angle);
  config.max_angle = htons(tmp_max_angle);
  config.resolution = htons(tmp_resolution);
  config.intensity = tmp_intensity ? 1 : 0;

  // copy them locally
  min_angle = tmp_min_angle;
  max_angle = tmp_max_angle;
  resolution = tmp_resolution;
  intensity = tmp_intensity;

  return(client->Request(PLAYER_LASER_CODE,index,(const char*)&config,
                         sizeof(config)));
}

/** Get the current laser configuration; it is read into the
  relevant class attributes.\\
  Returns the 0 on success, or -1 of there is a problem.
 */
int LaserProxy::GetConfigure()
{
  if(!client)
    return(-1);

  player_laser_config_t config;
  player_msghdr_t hdr;

  config.subtype = PLAYER_LASER_GET_CONFIG;

  if(client->Request(PLAYER_LASER_CODE,index,
                     (const char*)&config, sizeof(config.subtype),
                     &hdr, (char*)&config, sizeof(config)) < 0)
    return(-1);

  min_angle = ntohs(config.min_angle);
  max_angle = ntohs(config.max_angle);
  resolution = ntohs(config.resolution);
  intensity = config.intensity;
  return(0);
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
  for(unsigned short i=0;i<range_count && i<PLAYER_LASER_MAX_SAMPLES;i++)
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

// returns coords in mm of laser hit relative to sensor position
// x axis is forwards
int LaserProxy::CartesianCoordinate( int i, int *x, int *y )
{
  // bounds check
  if( i < 0 || i > range_count ) return false;

  double min = DTOR(min_angle / 100.0);
  double max = DTOR(max_angle / 100.0);
  double fov = (double)( max - min );
  double angle_per_ray = fov /(double)range_count; 
  double angle = min + i * angle_per_ray; 
  int range = ranges[i];
 
  *x = (int)(range * cos( angle ));
  *y = (int)(range * sin( angle ));

  //printf( "x: %.2f   y: %.2f\n", x, y );

  return true;
}

// interface that all proxies SHOULD provide
void LaserProxy::Print()
{
  printf("#LASER(%d:%d) - %c\n", device, index, access);
  puts("#min\tmax\tres\tcount");
  printf("%d\t%d\t%u\t%u\n", min_angle,max_angle,resolution,range_count);
  puts("#range\tintensity");
  for(unsigned short i=0;i<range_count && i<PLAYER_LASER_MAX_SAMPLES;i++)
    printf("%u %u ", ranges[i],intensities[i]);
  puts("");
}

