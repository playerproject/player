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
// Desc: Gazebo (simulator) time
// Author: Andrew Howard
// Date: 7 May 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifndef GZ_TIME_H
#define GZ_TIME_H

#include "playertime.h"

// Forward declarations
typedef struct gz_sim gz_sim_t;


// Incremental navigation driver
class GzTime : public PlayerTime
{
  // Constructor
  public: GzTime();

  // Destructor
  public: virtual ~GzTime();

  // Get the simulator time
  public: int GetTime(struct timeval* time);

  // Pointer to the simulator interface
  private: gz_sim_t *sim;
};

#endif
