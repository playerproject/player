#include <stdio.h>
#include "arenavisiondevice.h"
#include <string.h>

extern caddr_t arenaIO;

CArenaVisionDevice::CArenaVisionDevice(int port, char* path, bool old )
                  :CVisionDevice( port, path, old )
{
  //puts("Arena vision construct");
}

CArenaVisionDevice::~CArenaVisionDevice() 
{
  //puts("Arena vision destruct");
}

int CArenaVisionDevice::Setup() 
{ 
  //puts( "Arena vision setup" ); 
  data  = (unsigned char*)arenaIO + ACTS_DATA_START;

  *(unsigned char*)(arenaIO + SUB_VISION) = 1;

  return 0; 
};
  

int CArenaVisionDevice::Shutdown()
{ 
  //puts( "Arena vision shutdown" ); 

  *(unsigned char*)(arenaIO + SUB_VISION) = 0;

  return 0; 
};

