/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
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
 *
 * this is the StageTime class, which gets current simulated time from
 * shared memory
 *
 */

#include "stagetime.h"
#include <stdio.h>

// constuctor
StageTime::StageTime( stage_clock_t* clock ) 
{ 
  simtimep = &clock->time;  
  stagelock.InstallSemaphore( &clock->lock );
}


int StageTime::GetTime(struct timeval* time)
{ 
  if( !stagelock.Lock() )
  {
    perror("StageTime::GetTime(): semop() failed on Lock().");
    return(-1);
  }

  *time = *simtimep;

  if( !stagelock.Unlock() )
  {
    perror("StageTime::GetTime(): semop() failed on Unlock().");
    return(-1);
  }
  return(0);
}
