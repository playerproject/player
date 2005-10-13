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
 * $Id$
 */

/** @addtogroup clientlibs Client Libraries */
/** @{ */

/** @addtogroup player_clientlib_cplusplus libplayerc++

The C++ library is built on a "service proxy" model in which the client
maintains local objects that are proxies for remote services.  There are
two kinds of proxies: the special server proxy @p PlayerClient and the
various device-specific proxies.  Each kind of proxy is implemented as a
separate class.  The user first creates a @p PlayerClient proxy and uses
it to establish a connection to a Player server.  Next, the proxies of the
appropriate device-specific types are created and initialized using the
existing @p PlayerClient proxy.  To make this process concrete, consider
the following simple example (for clarity, we omit some error-checking):

@include example0.cc

This program performs simple (and bad) sonar-based obstacle avoidance with
a mobile robot .  First, a @p PlayerClient
proxy is created, using the default constructor to connect to the
server listening at @p localhost:6665.  Next, a @p SonarProxy is
created to control the sonars and a @p PositionProxy to control the
robot's motors.  The constructors for these objects use the existing @p
PlayerClient proxy to establish access to the 0th @p sonar and @p position
devices, respectively. Finally, we enter a simple loop that reads the current
sonar state and writes appropriate commands to the motors.

Callbacks can also be registered to the different proxies to be called on a
Read().  This is illustrated by the following example:

@include example2.cc

*/
/** @} */

#include "playerc++.h"

std::ostream&
std::operator << (std::ostream& os, const player_pose_t& c)
{
  os << "pos: " << c.px << "," << c.py << "," << c.pa;
  return os;
}

std::ostream&
std::operator << (std::ostream& os, const player_pose3d_t& c)
{
  os << "pos: " << c.px << "," << c.py << "," << c.pz << " "
     << c.proll << "," << c.ppitch << "," << c.pyaw;
  return os;
}
