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

#ifndef GZ_CLIENT_H
#define GZ_CLIENT_H


// Forward declarations
typedef struct _gz_client_t gz_client_t;
typedef struct _gz_sim_t gz_sim_t;


// Gazebo client handling; the class is just for namespacing purposes.
class GzClient
{
  // Initialize 
  public: static int Init(const char *serverid);

  // Finalize
  public: static int Fini();

  // The one and only gazebo client
  public: static gz_client_t *client;

  // The simulator control interface
  public: static gz_sim_t *sim;
};

#endif
