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

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "gazebo.h"
#include "gz_client.h"
#include "gz_time.h"


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzTime::GzTime()
{
  this->sim = GzClient::sim;
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzTime::~GzTime()
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Get the simulator time
int GzTime::GetTime(struct timeval* time)
{
  //printf("%.3f\n", this->sim->data->sim_time);

  // TODO: put gz_sim_lock in here ?
  
  time->tv_sec = (int) floor(this->sim->data->sim_time);
  time->tv_usec = (int) floor(fmod(this->sim->data->sim_time, 1.0) * 1e6);
  
  return 0;
}

int GzTime::GetTimeDouble(double* time)
{
  struct timeval ts;

  ts.tv_sec = (int) floor(this->sim->data->sim_time);
  ts.tv_usec = (int) floor(fmod(this->sim->data->sim_time, 1.0) * 1e6);
 
  *time = ts.tv_sec + ts.tv_usec/1e6;

  return 0;
}
