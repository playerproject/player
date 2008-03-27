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
// Desc: Driver for returning 'relevant' laser rays from a larger laser
//       packet
// Author: Radu Bogdan Rusu
// Date: 14 Nov 2006
//
// Theory of operation - given a min and max angle, returns only the rays
// between them, everything else gets cut away.
//
// Requires - Laser device.
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_lasercutter lasercutter
 * @brief Laser cutter

The lasercutter driver processes a laser scan, and removes all rays except
those between min_angle and max_angle. Useful if you already know/computed
ahead the 'area of interest'.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_laser : output of the cut

@par Requires

- @ref interface_laser : raw laser data

@par Configuration requests

- PLAYER_LASER_REQ_GET_GEOM

@par Configuration file options
- min_angle (float)
  - Default: -pi/2
  - Minimum angle of the new scan data
- max_angle (float)
  - Default: pi/2
  - Maximum angle of the new scan data

@par Example

@verbatim
driver
(
  name "sicklms200"
  provides ["laser:0"]
  port "/dev/ttyS0"
)
driver
(
  name "laserescan"
  requires ["laser:0"] # read from laser:0
  provides ["laser:1"] # output results on laser:1
  # Return only the rays between -30:30 degrees
  min_angle -30
  max_angle 30
)
@endverbatim

@author Radu Bogdan Rusu

*/
/** @} */

#include <libplayercore/playercore.h>

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <unistd.h>

#include "lasertransform.h"

// Driver class starts here
class LaserCutter : public LaserTransform
{
  // Constructor
  public: LaserCutter( ConfigFile* cf, int section);
  ~LaserCutter();

  // Process laser data.  Returns non-zero if the laser data has been
  // updated.
  protected: int UpdateLaser(player_laser_data_t * data);

  double min_angle, max_angle;
  unsigned int allocated_ranges;
};


// Initialization function
Driver* LaserCutter_Init( ConfigFile* cf, int section)
{
  return ((Driver*) (new LaserCutter( cf, section)));
}


// a driver registration function
void lasercutter_Register(DriverTable* table)
{
  table->AddDriver("lasercutter", LaserCutter_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserCutter::LaserCutter( ConfigFile* cf, int section)
    : LaserTransform(cf, section)
{
  // Settings.
  allocated_ranges = 0;
  data.ranges = NULL;
  this->max_angle = cf->ReadAngle(section, "max_angle", M_PI/2.0);
  this->min_angle = cf->ReadAngle(section, "min_angle", -M_PI/2.0);

  return;
}

LaserCutter::~LaserCutter()
{
  free(data.ranges);
}


////////////////////////////////////////////////////////////////////////////////
// Process laser data.
int LaserCutter::UpdateLaser(player_laser_data_t * data)
{
  unsigned int i;
  double current_angle;

  // Construct the outgoing laser packet
  this->data.resolution   = data->resolution;
  this->data.min_angle    = (min_angle);
  this->data.max_angle    = (max_angle);
  this->data.max_range    = data->max_range;
  this->data.id           = data->id;

  this->data.ranges_count    = 0;
  this->data.intensity_count = 0;

  current_angle = data->min_angle;
  if (data->ranges_count+1 > allocated_ranges)
    data->ranges = (float*)realloc(data->ranges,sizeof(data->ranges[0])*(data->ranges_count+1));
  for (i = 0; i < data->ranges_count; i++)
  {
    current_angle += data->resolution;

    if ((current_angle >= min_angle) && (current_angle <= max_angle))
    {
      this->data.ranges[this->data.ranges_count] = data->ranges[i];
      this->data.ranges_count++;
    }
  }

  current_angle = data->min_angle;
  for (i = 0; i < data->intensity_count; i++)
  {
    current_angle += data->resolution;

    if ((current_angle >= min_angle) && (current_angle <= max_angle))
    {
      this->data.intensity[this->data.intensity_count] = data->intensity[i];
      this->data.intensity_count++;
    }
  }

  this->Publish(this->device_addr,
                PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN,
                &this->data, sizeof(this->data), NULL);

  return 1;
}
