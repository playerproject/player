/* 
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/***************************************************************************
 * Desc: Example program for libplayerc.
 * Author: Andrew Howard
 * Date: 28 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <stdio.h>
#include "playerc.h"


int main(int argc, const char **argv)
{
  int i;
  const char *host;
  int port;
  playerc_client_t *client;
  playerc_position_t *position;

  host = "localhost";
  port = 6665;
    
  // Create a client and connect it to the server.
  client = playerc_client_create(NULL, host, port);
  if (playerc_client_connect(client) != 0)
    return -1;

  // Create and subscribe to a position device.
  position = playerc_position_create(client, 0);
  if (playerc_position_subscribe(position, PLAYER_ALL_MODE))
    return -1;

  // Enable the position device
  if (playerc_position_enable(position, 1) != 0)
    return -1;

  // Make the robot spin!
  if (playerc_position_set_speed(position, 0, 0, 0.1, 1) != 0)
    return -1;
  
  for (i = 0; i < 200; i++)
  {
    // Wait for new data from server
    playerc_client_read(client);
    
    // Print current robot pose
    printf("position : %f %f %f\n",
           position->px, position->py, position->pa);
  } 

  // Shutdown
  playerc_position_unsubscribe(position);
  playerc_position_destroy(position);
  playerc_client_disconnect(client);
  playerc_client_destroy(client);

  return 0;
}
