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
 * client-side GPS device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>

void GpsProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  player_gps_data_t* buf = (player_gps_data_t*)buffer;
  if(hdr.size != sizeof(player_gps_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of GPS data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_gps_data_t),hdr.size);
  }

  latitude = ((int)ntohl(buf->latitude)) / 60.0 / 60.0 / 60.0;
  longitude = (int)ntohl(buf->longitude) / 60.0 / 60.0 / 60.0;

  altitude = (int)ntohl(buf->altitude) / 1000.0;

  satellites = buf->num_sats;

  quality = buf->quality;

  hdop = (int) (unsigned int) ntohs(buf->hdop) * 10.0;

  time.tv_sec = ntohl(buf->time_sec);
  time.tv_usec = ntohl(buf->time_usec);
}

// interface that all proxies SHOULD provide
void GpsProxy::Print()
{
  printf("#GPS(%d:%d) - %c\n",
         m_device_id.code, m_device_id.index, access);
  puts("#(fix,lat,long,alt,sats)");
  printf("%d\t%f\t%f\t%f\t%d\n",quality,latitude,longitude,altitude,satellites);
}

