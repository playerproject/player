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
 * client-side beacon device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>

void SimulationProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  // empty
}

// interface that all proxies SHOULD provide
void SimulationProxy::Print()
{
  printf("#Simulation(%d:%d) - %c\n",
         m_device_id.code, m_device_id.index, access);
}


int SimulationProxy::SetPose2D( char* identifier, double x, double y, double a )
{
  
  player_simulation_pose2d_req_t req;

  req.subtype = PLAYER_SIMULATION_SET_POSE2D;
  strncpy( req.name, identifier, PLAYER_SIMULATION_IDENTIFIER_MAXLEN );
  req.x = htonl( (int32_t)(1000.0 * x) );
  req.y = htonl( (int32_t)(1000.0 * y) );
  req.a = htonl( (int32_t)RTOD(a) );

  return client->Request( m_device_id,
			  (const char *)&req, sizeof(req));
}

int SimulationProxy::GetPose2D( char* identifier, double& x, double& y, double& a )
{
  player_simulation_pose2d_req_t request;
  request.subtype = PLAYER_SIMULATION_GET_POSE2D;
  strncpy( request.name, identifier, PLAYER_SIMULATION_IDENTIFIER_MAXLEN );
  
  player_simulation_pose2d_req_t reply;
  player_msghdr_t hdr;    
  
  if(client->Request(m_device_id,
                     (const char*)&request, sizeof(request),
                     &hdr, (char*)&reply, sizeof(reply)) < 0)
    return(-1);
  
  x =  ((int32_t)ntohl(reply.x)) / 1e3;
  y =  ((int32_t)ntohl(reply.y)) / 1e3;
  a =  DTOR((int32_t)ntohl(reply.a));
  
  return 0;
}
