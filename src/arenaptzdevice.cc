#include "arenaptzdevice.h"
#include <stdio.h> // for puts()
#include <sys/types.h> 
#include <string.h>

extern caddr_t arenaIO;


CArenaPtzDevice::CArenaPtzDevice( char* port )
  :CPtzDevice( port )
{
  // do nowt (!)
  //puts( "ArenaPTZ constructor" );
}

CArenaPtzDevice::~CArenaPtzDevice()
{
  // do nowt again 
  //puts( "ArenaPTZ destructor" );
}

int CArenaPtzDevice::Setup()
{
  // move the data pointers to shared mem
  command      = (unsigned char*)arenaIO + PTZ_COMMAND_START;
  data         = (unsigned char*)arenaIO + PTZ_DATA_START;
 
  *(unsigned char*)(arenaIO + SUB_PTZ) = 1;
  return 0; 
};

int CArenaPtzDevice::Shutdown()
{ 
  *(unsigned char*)(arenaIO + SUB_PTZ) = 0;
  return 0; 
};

