/*
 *  Player - One Hell of a Robot Server
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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
///////////////////////////////////////////////////////////////////////////
//
// Desc: Gazebo (simulator) client functions
// Author: Andrew Howard
// Date: 7 May 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*#include "player.h"
#include "error.h"
*/

#include <libplayercore/playercore.h>

#include "gazebo.h"
#include "gz_client.h"


////////////////////////////////////////////////////////////////////////////////
// Data
const char *GzClient::prefix_id = "";
gz_client_t *GzClient::client = NULL;
gz_sim_t *GzClient::sim = NULL;
int GzClient::driverCount = 0;
Driver *GzClient::drivers[1024];


////////////////////////////////////////////////////////////////////////////////
// Initialize 
int GzClient::Init(int serverid, const char *prefixid)
{
  if (prefixid != NULL)
    GzClient::prefix_id = prefixid;

  GzClient::client = gz_client_alloc();
  assert(GzClient::client);

  printf("GzClient ServerId[%d]\n",serverid);

#ifdef GZ_CLIENT_ID_PLAYER
  // Use version 0.5.0
  if (gz_client_connect_wait(GzClient::client, serverid, GZ_CLIENT_ID_PLAYER) != 0)
    return -1;
#else
  // Use version 0.4.0
  if (gz_client_connect(GzClient::client, "default") != 0)
    return -1;
#endif

  GzClient::sim = gz_sim_alloc();
  assert(GzClient::sim);

  if (gz_sim_open(GzClient::sim, GzClient::client, "default") != 0)
    return -1;
  
#ifdef LIBGAZEBO_VERSION
  // Check that version numbers match
  if (GzClient::sim->data->head.version != LIBGAZEBO_VERSION)
  {
    PLAYER_ERROR2("libgazebo mismatch: Gazebo is using v%03X, Player is using v%03X\n"
                  "Try re-building Player",
                  GzClient::sim->data->head.version, LIBGAZEBO_VERSION);
    return -1;
  }
#else
  PLAYER_WARN("libgazebo has no version number\n"
              "Consider upgrading Gazebo and then re-building Player");
#endif

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Finalize
int GzClient::Fini()
{
  gz_sim_close(GzClient::sim);
  gz_sim_free(GzClient::sim);
  GzClient::sim = NULL;

  if (gz_client_disconnect(GzClient::client) != 0)
    return -1;

  gz_client_free(GzClient::client);
  GzClient::client = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Add a driver to the list of known Gazebo drivers.
void GzClient::AddDriver(Driver *driver)
{
  assert(GzClient::driverCount < (int) (sizeof(GzClient::drivers) / sizeof(GzClient::drivers[0])));
  GzClient::drivers[GzClient::driverCount++] = driver;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Remove a driver to the list of known Gazebo drivers.
void GzClient::DelDriver(Driver *driver)
{
  int i;

  for (i = 0; i < GzClient::driverCount; i++)
  {
    if (GzClient::drivers[i] == driver)
    {
      GzClient::driverCount--;
      memmove(GzClient::drivers + i,
              GzClient::drivers + i + 1,
              (GzClient::driverCount - i) * sizeof(GzClient::drivers[0]));
      break;
    }
  }
  
  return;
}


