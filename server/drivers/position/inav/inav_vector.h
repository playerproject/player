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

#ifndef INAV_VECTOR_H
#define INAV_VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif


// Vector data
typedef struct 
{
  double v[3];
  
} inav_vector_t;


// Transform from local to global coords (a + b)
inav_vector_t inav_vector_cs_add(inav_vector_t a, inav_vector_t b);

// Transform from global to local coords (a - b)
inav_vector_t inav_vector_cs_sub(inav_vector_t a, inav_vector_t b);


#ifdef __cplusplus
}
#endif

#endif
