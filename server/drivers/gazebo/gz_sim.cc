/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
// Desc: Gazebo (simulator) sim driver
// Author: Andrew Howard
// Date: 6 Apr 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_gz_sim gz_sim

The gz_sim driver controls the Gazebo simulator.  This driver must be
present when working with the simulator.

@par Compile-time dependencies

- Gazebo

@par Provides

- @ref player_interface_sim

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- gz_id (string)
  - Default: ""
  - ID of the Gazebo model.
      
@par Example 

@verbatim
driver
(
  name gz_sim
  provides ["simulation:0"]
)
@endverbatim

@par Authors

Andrew Howard
*/
/** @} */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>       // for atoi(3)

#include "player.h"
#include "error.h"
#include "driver.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"


/// @brief Driver for Gazebo simulator interface
class GzSim : public Driver
{
  /// Constructor
  public: GzSim(ConfigFile* cf, int section);

  /// Destructor
  public: virtual ~GzSim();

  /// Startup the driver
  public: virtual int Setup();

  /// Shutdown the driver
  public: virtual int Shutdown();

  /// Driver main loop
  public: virtual void Main();

  /// Gazebo client object
  private: gz_client_t *client;
  
  /// Gazebo simulator interface
  private: gz_sim_t *sim;  
};


// Initialization function
Driver* GzSim_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzSim(cf, section)));
}


// a driver registration function
void GzSim_Register(DriverTable* table)
{
  table->AddDriver("gz_sim", GzSim_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzSim::GzSim(ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_SIMULATION_CODE, PLAYER_ALL_MODE,
             sizeof(player_simulation_data_t), 0, 10, 10)
{
  // Get the globally defined Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the globally defined simulator interface
  this->sim = GzClient::sim;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzSim::~GzSim()
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzSim::Setup()
{  
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzSim::Shutdown()
{
  this->StopThread();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzSim::Main()
{
  int i;
  Driver *driver;
  
  while (true)
  {
    if (gz_client_wait(this->client))
    {
      PLAYER_ERROR("wait returned error; exiting simulation loop");
      break;
    }    
    pthread_testcancel();

    gz_sim_lock(this->sim, 1);
    //printf("%f  \n", this->sim->data->sim_time);
    gz_sim_unlock(this->sim);

    // Let each registered driver have a bite of the cherry.  Note
    // that this duplicates the behavior of the Player kernel, but
    // potentially does so a a higher rate.
    for (i = 0; i < GzClient::driverCount; i++)
    {
      driver = GzClient::drivers[i];
      driver->Update();
    }
  }

  return;
}
