/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *     Nik Melchior
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

#include <playerclient.h>
#include <string.h>

bool
BumperProxy::BumpedAny()
{
  for (int i=0; (i < bumper_count)&&(i < PLAYER_BUMPER_MAX_SAMPLES); i++) {
    if (bumpers[i])
      return true;
  }
  // if we got this far...
  return false;
}

bool
BumperProxy::Bumped(const unsigned int i)
{
  if (i < bumper_count)
    return (bumpers[i]) ? true : false;
  else
    return false;
}

int
BumperProxy::GetBumperGeom( player_bumper_geom_t* bumper_geom )
{
  player_msghdr_t hdr;
  
  if(!client)
    return(-1);

  assert( bumper_geom );



  bumper_geom->subtype = PLAYER_BUMPER_GET_GEOM_REQ;

  if((client->Request(PLAYER_BUMPER_CODE,index,(const char*)bumper_geom,
                      sizeof(bumper_geom->subtype), &hdr, (char*)bumper_geom, 
                      sizeof(*bumper_geom)) < 0) ||
     (hdr.type != PLAYER_MSGTYPE_RESP_ACK))
    return(-1);

  bumper_geom->bumper_count = ntohs(bumper_geom->bumper_count);

  // fix the byte order for all the geom definitions
  for(int i=0;i<bumper_geom->bumper_count;i++)
  {
    bumper_geom->bumper_def[i].x_offset =
      ntohs(bumper_geom->bumper_def[i].x_offset);

    bumper_geom->bumper_def[i].y_offset = 
      ntohs(bumper_geom->bumper_def[i].y_offset);
    
    bumper_geom->bumper_def[i].th_offset = 
      ntohs(bumper_geom->bumper_def[i].th_offset);
    
    bumper_geom->bumper_def[i].length = 
      ntohs(bumper_geom->bumper_def[i].length);
    
    bumper_geom->bumper_def[i].radius = 
      ntohs(bumper_geom->bumper_def[i].radius);
  }

  return(0);
}



void
BumperProxy::FillData(player_msghdr_t hdr, const char *buffer)
{
  if(hdr.size != sizeof(player_bumper_data_t)) {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: rwi_bumperproxy expected %d bytes of"
              " bumper data, but received %d. Unexpected results may"
              " ensue.\n",
              sizeof(player_bumper_data_t), hdr.size);
  }

  bumper_count = ((player_bumper_data_t *)buffer)->bumper_count;
  for(int i=0;i<bumper_count && i<PLAYER_BUMPER_MAX_SAMPLES;i++)
    bumpers[i] = ((player_bumper_data_t *)buffer)->bumpers[i];
}

// interface that all proxies SHOULD provide
void 
BumperProxy::Print()
{
  printf("#Bumper(%d:%d) - %c\n", device, index, access);
  printf("%d\n", bumper_count);
  for (int i = min(bumper_count, PLAYER_BUMPER_MAX_SAMPLES) - 1; i >= 0; i--)
    putchar((bumpers[i]) ? '1' : '0');
  puts(" ");
}

