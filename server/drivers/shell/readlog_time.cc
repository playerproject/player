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
// Desc: Log file time
// Author: Andrew Howard
// Date: 28 May 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "readlog_manager.h"
#include "readlog_time.h"



////////////////////////////////////////////////////////////////////////////////
// Constructor
ReadLogTime::ReadLogTime()
{
  this->manager = ReadLogManager_Get();
  assert(this->manager != NULL);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
ReadLogTime::~ReadLogTime()
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Get the simulator time
int ReadLogTime::GetTime(struct timeval* time)
{  
  time->tv_sec = this->manager->server_time / 1000000;
  time->tv_usec = this->manager->server_time % 1000000;

  //printf("%d.%06d\n", (uint32_t) time->tv_sec, (uint32_t) time->tv_usec);
    
  return 0;
}
