
#include <arenalaserdevice.h>
#include <sys/types.h> 
#include <stdio.h>
#include <string.h>

extern caddr_t arenaIO;

CArenaLaserDevice::CArenaLaserDevice(char *port)
                  :CLaserDevice( port )
{
  //puts("Arena laser construct");
}

CArenaLaserDevice::~CArenaLaserDevice() 
{
  //puts("Arena Laser destruct");
}

int CArenaLaserDevice::Setup() 
{ 
  //puts( "Arena laser setup" ); 
  
  // move the data pointer to shared mem
  data       = (unsigned char*)arenaIO + LASER_DATA_START;
  
  *(unsigned char*)(arenaIO + SUB_LASER) = 1;

  return 0; 
};
  

int CArenaLaserDevice::Shutdown()
{ 
  //puts( "Arena laser shutdown" ); 

  *(unsigned char*)(arenaIO + SUB_LASER) = 0;

  return 0; 
};

size_t CArenaLaserDevice::GetData( unsigned char *dest, size_t maxsize )
{
  memcpy( dest, arenaIO + LASER_DATA_START, LASER_DATA_BUFFER_SIZE );

  return LASER_DATA_BUFFER_SIZE;
}
