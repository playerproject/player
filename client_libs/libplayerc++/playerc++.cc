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

#include "playerc++.h"

/** @addtogroup clientlibs Client Libraries */
/** @{ */

/** @addtogroup player_clientlib_cplusplus libplayerc++

The C++ library is built on a "service proxy" model in which the client
maintains local objects that are proxies for remote services.  There are
two kinds of proxies: the special server proxy  PlayerClient and the
various device-specific proxies.  Each kind of proxy is implemented as a
separate class.  The user first creates a PlayerClient proxy and uses
it to establish a connection to a Player server.  Next, the proxies of the
appropriate device-specific types are created and initialized using the
existing PlayerClient proxy.  To make this process concrete, consider
the following simple example (for clarity, we omit some error-checking):

@include example0.cc

This program performs simple (and bad) sonar-based obstacle avoidance with
a mobile robot .  First, a PlayerClient
proxy is created, using the default constructor to connect to the
server listening at @p localhost:6665.  Next, a SonarProxy is
created to control the sonars and a PositionProxy to control the
robot's motors.  The constructors for these objects use the existing
 PlayerClient proxy to establish access to the 0th @ref player_interface_sonar
and @ref player_interface_position2d devices, respectively. Finally, we enter
a simple loop that reads the current sonar state and writes appropriate
commands to the motors.

In order to use libplayerc++ in your application, you will need to load
the appropriate library and set the appropriate compiler flags.  An Automake
package config file is included(playerc++.pc).  To use this in your automake
project, simply add the following to your configure.in or configure.ac:

@verbatim
# Player C++ Library
PKG_CHECK_MODULES(PLAYERCC, playerc++)
AC_SUBST(PLAYERCC_CFLAGS)
AC_SUBST(PLAYERCC_LIBS)
@endverbatim

Then, in your Makefile.am you can add:
@verbatim
AM_CPPFLAGS += PLAYERCC_CFLAGS
programname_LDFLAGS = $(PLAYERCC_LIBS)
@endverbatim

If you are not using Automake, you need to add the following statements to the
compiling step:
@verbatim
$ g++ -Wall -g3 -pthread -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT -c foo.cc
@endverbatim
and the following to the linking stage:
@verbatim
$ g++ -o foo foo.o -lplayerc++ -lm -lboost_signals -lboost_thread
@endverbatim
*/

/** @} */

/** @addtogroup player_clientlib_cplusplus libplayerc++ */
/** @{ */

/** @addtogroup player_clientlib_multi Signals & multithreading

Along with providing access to the basic C functions of @ref player_clientlib_libplayerc
in a C++ fashion, libplayerc++ also provides additional functionality along
the lines of signaling and multithreading.  The multithreaded ability of
libplayerc++ allieves the developer from having to worry about allotting time
to handle messaging.  It also allows for the PlayerClient to act as a
messaging loop for event driven programs.  The signaling and multithreading
ability of libplayerc++ is built from the <a href="http://www.boost.org">
Boost c++ libraries</a>.  This is relevant because we will be using boost
semantincs for connecting the signals to the client.  Much of this functionality
can best be illustrated through the use of an example:

@include example1.cc

*/
/** @} */


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
