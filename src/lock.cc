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
/*
 * $Id$
 *   
 *   a general purpose lock class.  each device object has one of
 *   these and uses it to control access to common data buffers.
 */
#include<pthread.h>
#include<stdio.h>

#include <device.h>
#include <lock.h>
#include <errors.h>

CLock::CLock() 
{
  /* the mutex's are defined to be default type (NULL) which is fast */
  pthread_mutex_init( &dataAccessMutex, NULL );
  pthread_mutex_init( &commandAccessMutex, NULL );
  pthread_mutex_init( &configAccessMutex, NULL );
  pthread_mutex_init( &subscribeMutex, NULL );
  pthread_mutex_init( &setupDataMutex, NULL );

  /* lock setup mutexes if not already locked */
  pthread_mutex_lock( &setupDataMutex );
  firstdata = true;
  subscriptions = 0;
}

CLock::~CLock() 
{
  pthread_mutex_destroy( &dataAccessMutex );
  pthread_mutex_destroy( &commandAccessMutex );
  pthread_mutex_destroy( &configAccessMutex );
  pthread_mutex_destroy( &subscribeMutex );
  pthread_mutex_destroy( &setupDataMutex );
}

int CLock::Setup( CDevice *obj ) 
{
  return(obj->Setup());
}

int CLock::Shutdown( CDevice *obj ) 
{

  if ( firstdata == false ) 
  {
    // this means that the setup went okay and the call to lock will 
    // not be blocked
    pthread_mutex_lock( &setupDataMutex );
  }
  firstdata = true;
  return( obj->Shutdown() );
}

int CLock::GetData( CDevice *obj , unsigned char *dest ) 
{
  int size;

  pthread_mutex_lock( &setupDataMutex );
  pthread_mutex_lock( &dataAccessMutex );
  size = obj->GetData(dest);
  pthread_mutex_unlock( &dataAccessMutex );
  pthread_mutex_unlock( &setupDataMutex );
  return(size);
}

void CLock::PutData( CDevice *obj,  unsigned char *dest ) 
{
  pthread_mutex_lock( &dataAccessMutex );
  obj->PutData(dest);
  pthread_mutex_unlock( &dataAccessMutex );
  if (firstdata) 
  {
    pthread_mutex_unlock( &setupDataMutex );
    firstdata=false;
  }
}

void CLock::GetCommand( CDevice *obj , unsigned char *dest ) 
{
  pthread_mutex_lock( &commandAccessMutex );
  obj->GetCommand(dest);
  pthread_mutex_unlock( &commandAccessMutex );
}

void CLock::PutCommand(CDevice *obj ,unsigned char *dest, int size) 
{
  pthread_mutex_lock( &commandAccessMutex );
  obj->PutCommand(dest,size);
  pthread_mutex_unlock( &commandAccessMutex );
}
int CLock::GetConfig( CDevice *obj , unsigned char *dest ) 
{
  int size;
  pthread_mutex_lock( &configAccessMutex );
  size = obj->GetConfig(dest);
  pthread_mutex_unlock( &configAccessMutex );
  return(size);
}

void CLock::PutConfig(CDevice *obj ,unsigned char *dest, int size) 
{
  pthread_mutex_lock( &configAccessMutex );
  obj->PutConfig(dest,size);
  pthread_mutex_unlock( &configAccessMutex );
}

int CLock::Subscribe( CDevice *obj ) 
{
  int setupResult;

  pthread_mutex_lock( &subscribeMutex );
  
  if ( subscriptions == 0 ) 
  {
    setupResult = obj->Setup();
    if (setupResult == 0 ) subscriptions++;
  }
  else 
  {
    subscriptions++;
    setupResult = 0;
  }
  
  pthread_mutex_unlock( &subscribeMutex );
  return( setupResult );
}

int CLock::Unsubscribe( CDevice *obj ) 
{
  int shutdownResult;

  pthread_mutex_lock( &subscribeMutex );
  
  if ( subscriptions == 0 ) 
  {
    shutdownResult = E_ALREADY_SHUTDOWN;
  }
  else if ( subscriptions == 1) 
  {
    shutdownResult = Shutdown( obj );
    if (shutdownResult == 0 ) subscriptions--;
    /* do we want to unsubscribe even though the shutdown went bad? */
  }
  else {
    subscriptions--;
    shutdownResult = 0;
  }
  
  pthread_mutex_unlock( &subscribeMutex );
  return( shutdownResult );
}

