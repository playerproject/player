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
// Desc: Vector stuff for "Incremental" navigation driver.
// Author: Andrew Howard
// Date: 29 Jan 2003
// CVS: $Id$
///////////////////////////////////////////////////////////////////////////

#include <math.h>
#include "inav_vector.h"

// Normalize an angle
double inav_vector_normalize(double a)
{
  return atan2(sin(a), cos(a));
}


// Transform from local to global coords (a + b)
inav_vector_t inav_vector_cs_add(inav_vector_t a, inav_vector_t b)
{
  inav_vector_t c;

  c.v[0] = b.v[0] + a.v[0] * cos(b.v[2]) - a.v[1] * sin(b.v[2]);
  c.v[1] = b.v[1] + a.v[0] * sin(b.v[2]) + a.v[1] * cos(b.v[2]);
  c.v[2] = inav_vector_normalize(b.v[2] + a.v[2]);
  
  return c;
}


// Transform from global to local coords (a - b)
inav_vector_t inav_vector_cs_sub(inav_vector_t a, inav_vector_t b)
{
  inav_vector_t c;

  c.v[0] = +(a.v[0] - b.v[0]) * cos(b.v[2]) + (a.v[1] - b.v[1]) * sin(b.v[2]);
  c.v[1] = -(a.v[0] - b.v[0]) * sin(b.v[2]) + (a.v[1] - b.v[1]) * cos(b.v[2]);
  c.v[2] = inav_vector_normalize(a.v[2] - b.v[2]);
  
  return c;
}
