/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
 * the SICK laser device
 */

#ifndef NODEVICE
#define NODEVICE

#include "device.h"
#include "lock.h"


class CNoDevice:public CDevice {
 public:  
  CNoDevice( void );
  ~CNoDevice( void );
  
  CLock* GetLock( void ){ return NULL; };
  
  int Setup();
  int Shutdown();
  size_t GetData( unsigned char *, size_t maxsize );
  void PutData( unsigned char *, size_t maxsize );
  void GetCommand( unsigned char *, size_t maxsize );
  void PutCommand( unsigned char *, size_t maxsize);
  size_t GetConfig( unsigned char *, size_t maxsize);
  void PutConfig( unsigned char *, size_t maxsize);
};

#endif

