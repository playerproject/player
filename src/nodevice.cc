/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 *
 *   fake device to cause failure when running under a Stage that
 *   doesn't implement it.
 */

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

