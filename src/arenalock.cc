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
 *
 */

/*
 * $Id$
 *
 * The Stage locking mechanism, using semaphores in shared memory
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>  /* for exit() */

#include <device.h>
#include <arenalock.h>
#include <errors.h>
#include <netinet/in.h>
#include <sys/file.h> //for flock

#include "stagedevice.h"

//#define SEMKEY 2000

CArenaLock::CArenaLock():CLock()
{  
  // nothing to do here these days...

  //puts( "AL: construct" );
}

CArenaLock::~CArenaLock() 
{
  //puts( "AL: destruct" );
}

//  #ifdef POSIX_SEM
//  bool CArenaLock::InstallLock( sem_t* sem )
//  {
//    // get the pointer to the semaphore structure in shared memory
//    // this is called from main.cc CreateStageDevices()
 
//    m_lock = sem;

//    //printf( "Player: device lock at %p\n", m_lock );
//    return(true);
//  }
//  #else
//  bool CArenaLock::InstallLock( int fd )
//  {
//    // get the pointer to the semaphore structure in shared memory
//    // this is called from main.cc CreateStageDevices()
 
//    lock_fd = fd;

//    //printf( "Player: device lock at %d\n", lock_fd );
//    return(true);
//  }
//  #endif


int CArenaLock::Setup( CDevice *obj ) 
{
  //puts( "AL: setup" );
  return( obj->Setup() );
}

int CArenaLock::Shutdown( CDevice *obj ) 
{
  //puts( "AL: shutdown" );
  return( obj->Shutdown() );
}

bool CArenaLock::Lock( void )
{
  //printf( "P: LOCK %p\n", m_lock );

//  #ifdef POSIX_SEM

//      if( sem_wait( m_lock ) < 0 )
//        {
//          perror( "sem_wait failed" );
//          return false;
//        }

//  #else

//    // BSD file locking style
//    flock( lock_fd, LOCK_EX );

//  #endif

 // POSIX RECORD LOCKING METHOD
  struct flock cmd;

  cmd.l_type = F_WRLCK; // request write lock
  cmd.l_whence = SEEK_SET; // count bytes from start of file
  cmd.l_start = this->lock_byte; // lock my unique byte
  cmd.l_len = 1; // lock 1 byte


  //printf( "WAITING for byte %d\n", this->lock_byte );



  fcntl( this->lock_fd, F_SETLKW, &cmd );
  //printf( "DONE WAITING\n" );

   return true;
}

bool CArenaLock::Unlock( void )
{
  //printf( "P: UNLOCK %p\n", m_lock );

//  #ifdef POSIX_SEM

//    if( sem_post( m_lock ) < 0 )
//    {
//    perror( "sem_post failed" );
//    return false;
//    }

//  #else

//    // BSD file locking style
//    flock( lock_fd, LOCK_UN );

//  #endif

 // POSIX RECORD LOCKING METHOD
  struct flock cmd;

  cmd.l_type = F_UNLCK; // request  unlock
  cmd.l_whence = SEEK_SET; // count bytes from start of file
  cmd.l_start = this->lock_byte; // unlock my unique byte
  cmd.l_len = 1; // unlock 1 byte

  fcntl( this->lock_fd, F_SETLKW, &cmd );

  return true;
}


// same as GetData, except we read the data then mark the buffer as
// having no data available during the same lock
size_t CArenaLock::ConsumeData( CDevice *obj , 
				unsigned char *dest, size_t maxsize,
				uint32_t* timestamp_sec, 
				uint32_t* timestamp_usec)
{
  int size;
  Lock();
  //puts( "AL: get data" );
  // the GetData call also copies the data timestamp from shared memory
  // to the 'data_timestamp' field in the device object.
  size = obj->ConsumeData(dest, maxsize);
  if(timestamp_sec)
    *timestamp_sec = htonl(obj->data_timestamp_sec);
  if(timestamp_usec)
    *timestamp_usec = htonl(obj->data_timestamp_usec);
  Unlock();
  return(size);
}



size_t CArenaLock::GetData( CDevice *obj , unsigned char *dest, size_t maxsize,
                            uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  int size;
  Lock();
  //puts( "AL: get data" );
  // the GetData call also copies the data timestamp from shared memory
  // to the 'data_timestamp' field in the device object.
  size = obj->GetData(dest, maxsize);
  if(timestamp_sec)
    *timestamp_sec = htonl(obj->data_timestamp_sec);
  if(timestamp_usec)
    *timestamp_usec = htonl(obj->data_timestamp_usec);
  Unlock();
  return(size);
}

void CArenaLock::PutCommand(CDevice *obj ,unsigned char *dest, size_t size)
{  
  Lock();
  //puts( "AL: put command" );
  obj->PutCommand( dest,size );
  Unlock();
}

void CArenaLock::PutData( CDevice *obj,  unsigned char *dest, size_t maxsize) 
{
  puts( "Warning: attempt to put data in Stage mode" );
}

void CArenaLock::GetCommand( CDevice *obj , unsigned char *dest, size_t maxsize) 
{
  puts( "Warning: attempt to get commands in Stage mode" );
}
void CArenaLock::PutConfig( CDevice *obj,  unsigned char *dest, size_t size) 
{
  Lock();
  //puts( "AL: put config" );
  obj->PutConfig( dest,size );
  Unlock();
}

size_t CArenaLock::GetConfig( CDevice *obj , unsigned char *dest, size_t maxsize) 
{
  puts( "Warning: attempt to get commands in Stage mode" );
  return(0);
}


int CArenaLock::Subscribe( CDevice *obj ) 
{
  //puts( "AL: subscribe" );
  int res = 0 ;

  if( subscriptions == 0  )
  {
    res = obj->Setup();
    if (res == 0)
      subscriptions++;
  }
  else
    subscriptions++;

  return res;
}

int CArenaLock::Unsubscribe( CDevice *obj ) 
{
  //puts( "AL: unsubscribe" );
 
  int res = 0;

  if ( subscriptions == 0 )
    res = E_ALREADY_SHUTDOWN;
  else if( subscriptions == 1  )
    {
      res = obj->Shutdown();
      if (res == 0 )
          subscriptions--;    
      /* do we want to unsubscribe even though the shutdown went bad? */
    }
  else {
    subscriptions--;
    res = 0;
  }
  
  return res;
}

