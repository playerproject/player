/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * $Id$
 *
 * client-side laser device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

int LaserProxy::Configure(double tmp_min_angle, 
                          double tmp_max_angle, 
			  unsigned int tmp_scan_res,
			  unsigned int tmp_range_res, 
                          bool tmp_intensity)
{
  if(!client)
    return(-1);

  player_laser_config_t config;

  //config.subtype = PLAYER_LASER_SET_CONFIG;
  config.min_angle = htons((int16_t)rint(RTOD(tmp_min_angle)*1e2));
  config.max_angle = htons((int16_t)rint(RTOD(tmp_max_angle)*1e2));
  config.resolution = htons(tmp_scan_res);
  config.range_res = htons(tmp_range_res);
  config.intensity = tmp_intensity ? 1 : 0;

  // copy them locally
  min_angle = tmp_min_angle;
  max_angle = tmp_max_angle;
  scan_res = DTOR(tmp_scan_res / 1e2);
  intensity = tmp_intensity;
  range_res = (double)tmp_range_res;

  return(client->Request(m_device_id,PLAYER_LASER_SET_CONFIG,(const char*)&config,
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

 // config.subtype = PLAYER_LASER_GET_CONFIG;

  if(client->Request(m_device_id,PLAYER_LASER_GET_CONFIG,
                     (const char*)&config, 0,
                     &hdr, (char*)&config, sizeof(config)) < 0)
    return(-1);

  min_angle = DTOR(((int16_t)ntohs(config.min_angle)) / 1e2);
  max_angle = DTOR(((int16_t)ntohs(config.max_angle)) / 1e2);
  scan_res = DTOR(((int16_t)ntohs(config.resolution)) / 1e2);
  range_res = (double)ntohs(config.range_res);
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

  min_angle = DTOR(((int16_t)ntohs(((player_laser_data_t*)buffer)->min_angle))/1e2);
  max_angle = DTOR(((int16_t)ntohs(((player_laser_data_t*)buffer)->max_angle))/1e2);
  scan_res = DTOR(ntohs(((player_laser_data_t*)buffer)->resolution)/1e2);
  scan_count = ntohs(((player_laser_data_t*)buffer)->range_count);
  range_res = (double)ntohs(((player_laser_data_t*)buffer)->range_res);

  memset(scan,0,sizeof(scan));
  memset(point,0,sizeof(point));
  memset(intensities,0,sizeof(intensities));
  min_left = 1e9;
  min_right = 1e9;
  for(int i=0;i<scan_count && i<PLAYER_LASER_MAX_SAMPLES;i++)
  {
    // convert to mm, then m, according to given range resolution
    scan[i][0] = ntohs(((player_laser_data_t*)buffer)->ranges[i]) * 
            range_res / 1e3;
    // compute bearing
    scan[i][1] = min_angle + (i * scan_res);

    // compute Cartesian coords
    point[i][0] = scan[i][0] * cos(scan[i][1]);
    point[i][1] = scan[i][0] * sin(scan[i][1]);

    intensities[i] = 
            (unsigned char)(((player_laser_data_t*)buffer)->intensity[i]);
    if(i>(scan_count/2) && scan[i][0] < min_left)
      min_left = scan[i][0];
    else if(i<(scan_count/2) && scan[i][0] < min_right)
      min_right = scan[i][0];
  }
}

// enable/disable the laser
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int
LaserProxy::SetLaserState(const unsigned char state)
{
    if (!client)
		return (-1);

    player_laser_power_config_t cfg;

 //   cfg.subtype = PLAYER_LASER_POWER_CONFIG;
    cfg.value = state;

    return (client->Request(m_device_id,PLAYER_LASER_POWER_CONFIG,
			(const char *) &cfg, sizeof(cfg)));
}

// print the configuration 
void LaserProxy::PrintConfig()
{
  printf("#LASER(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  puts("#min\tmax\tres\tcount\trange_res");
  printf("%.3f\t%.3f\t%.2f\t%u\t%.3f\n", 
         RTOD(min_angle),RTOD(max_angle),RTOD(scan_res),scan_count, range_res);
}

// interface that all proxies SHOULD provide
void LaserProxy::Print()
{
  this->PrintConfig();

  puts("#range\tbearing\tintensity");
  for(int i=0;i<scan_count && i<PLAYER_LASER_MAX_SAMPLES;i++)
    printf("%.3f\t%.3f\t%u ", scan[i][0],RTOD(scan[i][1]),intensities[i]);
  puts("");
}


