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
 * client-side sonar device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h> // for memset
    
// enable/disable the sonars
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int 
SonarProxy::SetSonarState(unsigned char state)
{
  if(!client)
    return(-1);

  player_sonar_power_config_t config;

  config.subtype = PLAYER_SONAR_POWER_REQ;
  config.value = state;

  return(client->Request(m_device_id,(const char*)&config,
                         sizeof(config)));
}

int 
SonarProxy::GetSonarGeom()
{
  player_msghdr_t hdr;
  player_sonar_geom_t sonar_pose;

  if(!client)
    return(-1);

  sonar_pose.subtype = PLAYER_SONAR_GET_GEOM_REQ;

  if((client->Request(m_device_id,(const char*)&sonar_pose,
                      sizeof(sonar_pose.subtype), &hdr, (char*)&sonar_pose, 
                      sizeof(sonar_pose)) < 0) ||
     (hdr.type != PLAYER_MSGTYPE_RESP_ACK))
    return(-1);

  pose_count = ntohs(sonar_pose.pose_count);
  for(int i=0;i<pose_count;i++)
  {
    poses[i][0] = ((short)ntohs(sonar_pose.poses[i][0])) / 1e3;
    poses[i][1] = ((short)ntohs(sonar_pose.poses[i][1])) / 1e3;
    poses[i][2] = DTOR((short)ntohs(sonar_pose.poses[i][2]));
  }

  return(0);
}

void SonarProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_sonar_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of sonar data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_sonar_data_t),hdr.size);
  }

  range_count = ntohs(((player_sonar_data_t*)buffer)->range_count);
  memset(ranges,0,sizeof(ranges));
  for(size_t i=0;i<range_count;i++)
  {
    ranges[i] = ntohs(((player_sonar_data_t*)buffer)->ranges[i]) / 1e3;
  }
}

// interface that all proxies SHOULD provide
void SonarProxy::Print()
{
  printf("#Sonar(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  for(size_t i=0;i<range_count;i++)
    printf("%.3f ", ranges[i]);
  puts("");
}

