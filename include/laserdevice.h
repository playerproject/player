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

#ifndef LASERDEVICE
#define LASERDEVICE
#include <pthread.h>
#include <unistd.h>

#include "lock.h"
#include "device.h"

class CLaserDevice:public CDevice {
 private:
  bool data_swapped;     // whether or not the data buffer has been byteswapped
  pthread_t thread;           // the thread that continuously reads from the laser 
  int debuglevel;             // debuglevel 0=none, 1=basic, 2=everything
  
  CLock lock;

  ssize_t WriteToLaser( unsigned char *data, ssize_t len ); 

  /* methods used to decode the response from the laser */
  ssize_t RecieveAck();
  void DecodeStatusByte( unsigned char byte );
  bool CheckDatagram( unsigned char *datagram );

  /* methods used to configure the laser */
  int Request38k();
  int RequestData();
  int ChangeMode( );

 public:
  unsigned char* data;   // array holding the most recent laser scan
  unsigned char* config;   // latest config request from client

  // PutConfig sets this to the right size, and GetConfig zeros it
  int config_size;

  int laser_fd;               // laser device file descriptor
  // device used to communicate with the laserk
  char LASER_SERIAL_PORT[LASER_SERIAL_PORT_NAME_SIZE]; 


  /* Calculates CRC for a telegram */
  unsigned short CreateCRC( unsigned char* commData, unsigned int uLen );

  CLaserDevice(char *port);

  void Run();

  int Setup();
  int Shutdown();

  virtual CLock* GetLock( void ){ return &lock; };

  int GetData( unsigned char * );
  void PutData( unsigned char * );

  void GetCommand( unsigned char * );
  void PutCommand( unsigned char *,int );

  int GetConfig( unsigned char * );
  void PutConfig( unsigned char *,int );
};

#endif
