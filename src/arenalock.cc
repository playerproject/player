
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>  /* for exit() */

#include <device.h>
#include <arenalock.h>
#include <errors.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define SEMKEY 2000

CArenaLock::CArenaLock():CLock()
{  
  //puts( "AL: construct" );

  // create a single semaphore to sync access to the shared memory segments
  //puts( "Obtaining semaphore" );

  semKey = SEMKEY;

  union semun
    {
      int val;
      struct semid_ds *buf;
      ushort *array;
  } argument;

  argument.val = 0; // initial semaphore value

  semid = semget( semKey, 1, 0666 );

  if( semid < 0 ) // semget failed
    {
      perror( "Can't find Arena's semaphore" );
      exit( -1 );
    }
  //else
  //puts( "found semaphore" );

  // setup locking operation buffer
  lock_ops[0].sem_num = 0;
  lock_ops[0].sem_op = 1;
  lock_ops[0].sem_flg = 0;
  
  // setup unlocking operation buffer
  unlock_ops[0].sem_num = 0;
  unlock_ops[0].sem_op = -1;
  unlock_ops[0].sem_flg = 0;
}

CArenaLock::~CArenaLock() 
{
  //puts( "AL: destruct" );
}

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

inline int CArenaLock::Lock( void )
{
  return( semop( semid, lock_ops, 1 ) );
}

inline int CArenaLock::Unlock( void )
{
  return( semop( semid, unlock_ops, 1 ) );
}


size_t CArenaLock::GetData( CDevice *obj , unsigned char *dest, size_t maxsize)
{
  int size;
  Lock();
  //puts( "AL: get data" );
  size = obj->GetData(dest, maxsize);
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
  puts( "Warning: attempt to put data in Arena mode" );
}

void CArenaLock::GetCommand( CDevice *obj , unsigned char *dest, size_t maxsize) 
{
  puts( "Warning: attempt to get commands in Arena mode" );
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
  puts( "Warning: attempt to get commands in Arena mode" );
  return(0);
}


int CArenaLock::Subscribe( CDevice *obj ) 
{
  //puts( "AL: subscribe" );

  int res = 0 ;

  if( subscriptions == 0  )
    {
      res = obj->Setup();
      subscriptions = 1;
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
      subscriptions--;
    }
  else {
    subscriptions--;
    res = 0;
  }
  
  return res;
}

