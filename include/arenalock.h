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
 *   the the Stage locking mechanism, using semaphores in shared memory,
 *   rather than thread mutexes
 */

#ifndef _ARENALOCK_H
#define _ARENALOCK_H

/*  #ifdef POSIX_SEM */
/*  #include <semaphore.h> */
/*  #else */
/*  #include <sys/file.h> // BSD file locking */
/*  #endif */

#include "lock.h"

class CArenaLock : public CLock
{
 private:

/*  #ifdef POSIX_SEM */
/*    sem_t* m_lock; // this points into a device's shared memory */
/*  #else */
/*    int lock_fd; // a file descriptor to lock using flock(2) */
/*  #endif */

 public:

  bool Lock( void );
  bool Unlock( void );

  CArenaLock( void );
  ~CArenaLock( void );

/*  #ifdef POSIX_SEM */
/*    bool InstallLock( sem_t* sem); */
/*  #else */
/*    bool InstallLock( int ); // BSD file locking */
/*  #endif */

  // file descriptor for record locking
  int lock_fd;
  int lock_byte;

  // store the file descriptor and byte number
  bool InstallLock( int fd, int index )
    { lock_fd = fd; lock_byte = index; return true; };

  virtual int Setup( CDevice *obj ); 
  virtual int Shutdown( CDevice *obj ); 

  virtual size_t GetData(CDevice *, unsigned char *, size_t,
                         uint32_t* timestamp_sec, uint32_t* timestamp_usec);
  virtual size_t ConsumeData(CDevice *, unsigned char *, size_t,
                         uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  virtual void PutData( CDevice *obj,  unsigned char *dest, size_t maxsize );
  virtual void GetCommand( CDevice *obj , unsigned char *dest, size_t maxsize); 
  virtual void PutCommand(CDevice *obj ,unsigned char *dest, size_t maxsize); 
  virtual size_t GetConfig( CDevice *obj , unsigned char *dest, size_t maxsize); 
  virtual void PutConfig(CDevice *obj ,unsigned char *dest, size_t maxsize); 
  virtual int Subscribe( CDevice *obj );
  virtual int Unsubscribe( CDevice *obj ); 
};

#endif

