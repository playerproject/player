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
 * client-side planner device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>
#include <limits.h>

// Constructor
PlannerProxy::PlannerProxy( PlayerClient *pc, unsigned short index,
    unsigned char access )
  : ClientProxy (pc, PLAYER_PLANNER_CODE, index, access)
{
  this->pathDone = 1;
  return;
}

PlannerProxy::~PlannerProxy()
{
  return;
}


// Set a new goal point
int PlannerProxy::SetCmdPose( double gx, double gy, double ga, int /*state*/)
{
  if (!client)
    return -1;


  player_planner_cmd_t cmd;
  memset (&cmd, 0, sizeof(cmd));

  cmd.gx = (int32_t)htonl((int) (gx * 1000.0));
  cmd.gy = (int32_t)htonl((int) (gy * 1000.0));
  cmd.ga = (int32_t)htonl((int) (ga * 180.0 / M_PI));

  return (client->Write( m_device_id, (const char*)&cmd, sizeof(cmd)));
}


// Get the list of waypoints. This write the result into the proxy rather
// returning it to the caller.
int PlannerProxy::GetWaypoints()
{
  if (!client)
    return (-1);

  player_planner_waypoints_req_t config;
  player_msghdr_t hdr;

  memset(&config, 0, sizeof(config));
  config.subtype = PLAYER_PLANNER_GET_WAYPOINTS_REQ;

  if (client->Request(m_device_id, (const char*)&config, 
                        sizeof(config.subtype), &hdr, (char *)&config, 
                        sizeof(config)) < 0)
  {
    PLAYER_ERROR("failed to get waypoints");
    return -1;
  }

  if (hdr.size == 0)
  {
    PLAYER_ERROR("got unexpected zero-length reply");
    return -1;
  }

  this->waypointCount = (int)ntohs(config.count);
  
  for (int i=0; i<this->waypointCount; i++)
  {
    this->waypoints[i][0] = ((int)ntohl(config.waypoints[i].x)) / 1e3;
    this->waypoints[i][1] = ((int)ntohl(config.waypoints[i].y)) / 1e3;
    this->waypoints[i][2] = DTOR((int)ntohl(config.waypoints[i].a));
  }

  return 0;
}


// Process incoming data
void PlannerProxy::FillData( player_msghdr_t hdr, const char *buffer)
{

  if (hdr.size != sizeof(player_planner_data_t))
  {
    if (player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of position data, but "
          "received %d. Unexpected results may ensue.\n",
          sizeof(player_planner_data_t),hdr.size);

  }

  player_planner_data_t *data = (player_planner_data_t*)buffer;

  this->pathValid = data->valid;
  this->pathDone = data->done;

  this->px = (long) ntohl(data->px) / 1000.0;
  this->py = (long) ntohl(data->py) / 1000.0;
  this->pa = (long) ntohl(data->pa) * M_PI / 180.0;
  this->pa = atan2(sin(this->pa), cos(this->pa));

  this->gx = (long) ntohl(data->gx) / 1000.0;
  this->gy = (long) ntohl(data->gy) / 1000.0;
  this->ga = (long) ntohl(data->ga) * M_PI / 180.0;
  this->ga = atan2(sin(this->ga), cos(this->ga));

  this->wx = (long) ntohl(data->wx) / 1000.0;
  this->wy = (long) ntohl(data->wy) / 1000.0;
  this->wa = (long) ntohl(data->wa) * M_PI / 180.0;
  this->wa = atan2(sin(this->wa), cos(this->wa));

  this->currWaypoint = (short) ntohs(data->curr_waypoint);
  this->waypointCount = (unsigned short)ntohs(data->waypoint_count);

  return;
}
