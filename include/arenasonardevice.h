

#ifndef ARENASONARDEVICE
#define ARENASONARDEVICE

#include "arenalock.h"
#include "sonardevice.h"

class CArenaSonarDevice:public CSonarDevice {
 private:
  CArenaLock alock;

 public:
  
  CArenaSonarDevice( char* port );
  ~CArenaSonarDevice( void );
  
  virtual CLock* GetLock( void ){ return &alock; };
  
  // i've stored the sonar data separately from the other P2OS data
  // so we override the get data
  virtual int GetData( unsigned char * );
  
  virtual int Setup();
  virtual int Shutdown();
};

#endif

