#include "arenapositiondevice.h"
#include <sys/types.h> 
#include <stdio.h>
#include <string.h>

extern caddr_t arenaIO;

CArenaPositionDevice::CArenaPositionDevice(char* port)
                     :CPositionDevice( port )
{ 
  //puts( "Arena position construct" ); 
}

CArenaPositionDevice::~CArenaPositionDevice()
{ 
  //puts( "Arena position destruct" ); 
}


int CArenaPositionDevice::Setup() 
{ 
  //puts( "Arena position setup" ); 
 
  if(!arena_initialized_data_buffer)
  {
    data     = (unsigned char*)arenaIO + P2OS_DATA_START; 
    arena_initialized_data_buffer = true;
  }

  if(!arena_initialized_command_buffer)
  {
    command = (unsigned char*)arenaIO + P2OS_COMMAND_START;
    arena_initialized_command_buffer = true;
  }
  
  *(unsigned char*)(arenaIO + SUB_MOTORS) = 1;
  return 0;
}

int CArenaPositionDevice::Shutdown() 
{ 
  //puts( "Arena position setup" ); 
  *(unsigned char*)(arenaIO + SUB_MOTORS) = 0;

  return 0;
}

size_t CArenaPositionDevice::GetData( unsigned char *dest, size_t maxsize)
{
  // a little wierdness here, to do with the different packaging of P2OS data
  // between the arena and non-arena devices
  memcpy( dest, arenaIO + P2OS_DATA_START, POSITION_DATA_BUFFER_SIZE );
  //memcpy( dest, data, POSITION_DATA_BUFFER_SIZE );

  return POSITION_DATA_BUFFER_SIZE;
}
