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

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/file.h>

#include "playercommon.h"
#include "stagetime.h"


// constuctor
StageTime::StageTime(const char *directory) 
{
  // open and map the stage clock
  char clockname[MAX_FILENAME_SIZE];
  snprintf( clockname, MAX_FILENAME_SIZE-1, "%s/%s", 
            directory, STAGE_CLOCK_NAME );
  
#ifdef DEBUG
  printf("Opening %s\n", clockname );
#endif

  int tfd;
  
  if( (tfd = open( clockname, O_RDWR )) < 0 )
  {
    perror( "Failed to open clock file" );
    printf("Tried to open file \"%s\"\n", clockname );
    exit( -1 );
  }

  stage_clock_t *clock = 0;
  clock = (stage_clock_t*) mmap( NULL, sizeof(struct timeval),
                                 PROT_READ | PROT_WRITE, 
                                 MAP_SHARED, tfd, (off_t) 0);
  if ((char*)clock == MAP_FAILED)
  {
    perror( "Failed to map clock memory" );
    exit( -1 );
  }
  assert(clock);

  simtimep = &clock->time; 
  
  // use the first byte of the clock file to synchronize access
  InstallLock( tfd, 0 );
}


void StageTime::Lock( void )
{
  // POSIX RECORD LOCKING METHOD
  struct flock cmd;

  cmd.l_type = F_WRLCK; // request write lock
  cmd.l_whence = SEEK_SET; // count bytes from start of file
  cmd.l_start = this->lock_byte; // lock my unique byte
  cmd.l_len = 1; // lock 1 byte

  fcntl( this->lock_fd, F_SETLKW, &cmd );
}

void StageTime::Unlock( void )
{
  // POSIX RECORD LOCKING METHOD
  struct flock cmd;

  cmd.l_type = F_UNLCK; // request  unlock
  cmd.l_whence = SEEK_SET; // count bytes from start of file
  cmd.l_start = this->lock_byte; // unlock my unique byte
  cmd.l_len = 1; // unlock 1 byte

  fcntl( this->lock_fd, F_SETLKW, &cmd );
}

int StageTime::GetTime(struct timeval* time)
{ 
  Lock();
  *time = *simtimep;
  Unlock();
  return(0);
}

