
#include <nodevice.h>
#include <sys/types.h> 

extern caddr_t arenaIO;

CNoDevice::CNoDevice( void )
          :CDevice()
{
}

CNoDevice::~CNoDevice() 
{
}

int CNoDevice::Setup() 
{ 
  // not supported - setup returns -1
  return -1; 
}
  

int CNoDevice::Shutdown()
{ 
  // not supported - shutdown should never be called, but
  // just in case, report failure
  return -1; 
}

int CNoDevice::GetData( unsigned char* p ) 
{ 
  return(0);
}
  
void CNoDevice::PutData( unsigned char* p ) 
{ 
}

void CNoDevice::GetCommand( unsigned char* p ) 
{ 
}

void CNoDevice::PutCommand( unsigned char* p, int size ) 
{ 
}

int CNoDevice::GetConfig( unsigned char* p ) 
{ 
  return(0);
}

void CNoDevice::PutConfig( unsigned char* p, int size ) 
{ 
}

