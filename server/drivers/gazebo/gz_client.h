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
class Driver;


/// @brief Gazebo client handler
///
/// This class handles the Gazebo client object, and acts as a shared
/// data-structure for all Gazebo related drivers.  Note that there
/// can only be one instance of this class (it is entirely static).
class GzClient
{
  /// Initialize 
  public: static int Init(int serverid, const char *prefixid);

  /// Finalize
  public: static int Fini();

  /// @brief Add a driver to the list of known Gazebo drivers.
  public: static void AddDriver(Driver *driver);

  /// @brief Remove a driver to the list of known Gazebo drivers.
  public: static void DelDriver(Driver *driver);

  /// The prefix used for all gazebo ID's
  public: static const char *prefix_id;

  /// The one and only gazebo client
  public: static gz_client_t *client;

  /// The simulator control interface
  public: static gz_sim_t *sim;

  /// List of all known Gazebo drivers.
  /// The GzSim driver (if present) will use this list to update
  /// Gazebo drivers when new data becomes available.  If the GzSim
  /// driver is not present, drivers will be updated at the server's
  /// native rate (default 10Hz).
  public: static int driverCount;
  public: static Driver *drivers[1024];
};

#endif
