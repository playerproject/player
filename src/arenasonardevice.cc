
#include <arenasonardevice.h>
#include <sys/types.h> 
#include <string.h>

extern caddr_t arenaIO;

CArenaSonarDevice::CArenaSonarDevice(char *port)
                  :CSonarDevice( port )
{
  //puts("Arena sonar construct");
}

CArenaSonarDevice::~CArenaSonarDevice() 
{
  //puts("Arena sonar destruct");
}

int CArenaSonarDevice::Setup() 
{ 
  //puts( "Arena sonar setup" ); 
  
  if(!arena_initialized_data_buffer)
  {
    data     = (unsigned char*)arenaIO + P2OS_DATA_START; 
    arena_initialized_data_buffer = true;
  }
  //data       = (unsigned char*)arenaIO + SONAR_DATA_START;
  
  *(unsigned char*)(arenaIO + SUB_SONAR) = 1;

  return 0; 
};
  

int CArenaSonarDevice::Shutdown()
{ 
  //puts( "Arena sonar shutdown" );

  *(unsigned char*)(arenaIO + SUB_SONAR ) = 0;
 
  return 0; 
};

int CArenaSonarDevice::GetData( unsigned char *dest )
{
  memcpy(dest,(unsigned char*)arenaIO+SONAR_DATA_START,SONAR_DATA_BUFFER_SIZE);

  return SONAR_DATA_BUFFER_SIZE;
}
