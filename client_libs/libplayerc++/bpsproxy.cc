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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
/********************************************************************
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 ********************************************************************/

/*
 * $Id$
 */

#include "playerc++.h"

int BpsProxy::AddBeacon(char id, int px, int py, int pa)
{
  if(!client)
    return(-1);

  player_bps_beacon_t req;

  req.subtype = PLAYER_BPS_SET_BEACON;
  req.id = id;
  req.px = htonl(px);
  req.py = htonl(py);
  req.pa = htonl(pa);
  req.ux = 0;
  req.uy = 0;
  req.ua = 0;

  return(client->Request(PLAYER_BPS_CODE,index,(const char*)&req,
                         sizeof(player_bps_beacon_t)));
}

void BpsProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_bps_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of bps data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_bps_data_t),hdr.size);
  }

  px = ntohl(((player_bps_data_t*)buffer)->px);
  py = ntohl(((player_bps_data_t*)buffer)->py);
  pa = ntohl(((player_bps_data_t*)buffer)->pa);
  ux = ntohl(((player_bps_data_t*)buffer)->ux);
  uy = ntohl(((player_bps_data_t*)buffer)->uy);
  ua = ntohl(((player_bps_data_t*)buffer)->ua);
  err = ntohl(((player_bps_data_t*)buffer)->err);
}

// interface that all proxies SHOULD provide
void BpsProxy::Print()
{
  printf("#BPS(%d:%d) - %c\n", device, index, access);
  puts("#px py pa ux uy ua err");
  printf("%d %d %d %d %d %d %d\n",
         px,py,pa,ux,uy,ua,err);
}

