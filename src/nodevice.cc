
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

size_t CNoDevice::GetData( unsigned char* p, size_t maxsize ) 
{ 
  return(0);
}
  
void CNoDevice::PutData( unsigned char* p, size_t maxsize ) 
{ 
}

void CNoDevice::GetCommand( unsigned char* p, size_t maxsize) 
{ 
}

void CNoDevice::PutCommand( unsigned char* p, size_t maxsize) 
{ 
}

size_t CNoDevice::GetConfig( unsigned char* p, size_t maxsize) 
{ 
  return(0);
}

void CNoDevice::PutConfig( unsigned char* p, size_t maxsize) 
{ 
}

