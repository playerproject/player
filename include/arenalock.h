

#ifndef ARENALOCK
#define ARENALOCK

#include "lock.h"
#include "device.h"
#include <sys/ipc.h>
#include <sys/sem.h>

class CArenaLock : public CLock{
 private:
  int semid, semKey;

  struct sembuf lock_ops[1];
  struct sembuf unlock_ops[1];

  int Lock( void );
  int Unlock( void );

 public:

  CArenaLock( void );
  ~CArenaLock( void );

  virtual int Setup( CDevice *obj ); 
  virtual int Shutdown( CDevice *obj ); 

  virtual size_t GetData( CDevice *obj , unsigned char *dest, size_t maxsize );
  virtual void PutData( CDevice *obj,  unsigned char *dest, size_t maxsize );
  virtual void GetCommand( CDevice *obj , unsigned char *dest, size_t maxsize); 
  virtual void PutCommand(CDevice *obj ,unsigned char *dest, size_t maxsize); 
  virtual size_t GetConfig( CDevice *obj , unsigned char *dest, size_t maxsize); 
  virtual void PutConfig(CDevice *obj ,unsigned char *dest, size_t maxsize); 
  virtual int Subscribe( CDevice *obj );
  virtual int Unsubscribe( CDevice *obj ); 
};

#endif

