/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *     Nik Melchior
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

/* Misc componets of libplayerc++ */

#include "playerc++.h"

std::ostream& std::operator << (std::ostream& os, const player_pose_t& c)
{
  os << "pos: " << c.px << "," << c.py << "," << c.pa;
  return os;
}

std::ostream& std::operator << (std::ostream& os, const player_pose3d_t& c)
{
  os << "pos: " << c.px << "," << c.py << "," << c.pz << " " << c.proll << "," << c.ppitch << "," << c.pyaw;
  return os;
}
