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
 * the Sony EVI-D30 PTZ camera device
 */

#ifndef _PTZDEVICE_H
#define _PTZDEVICE_H

#include <pthread.h>
#include <unistd.h>

#include <lock.h>
#include <device.h>

#include <playercommon.h>
#include <messages.h>

#define PTZ_SLEEP_TIME_USEC 100000

#define MAX_PTZ_PACKET_LENGTH 16
#define MAX_PTZ_MESSAGE_LENGTH 14
#define MAX_PTZ_REPLY_LENGTH 11

/* here we calculate our conversion factors.
 *  0x370 is max value for the PTZ pan command, and in the real
 *    world, it has +- 25.0 range.
 *  0x12C is max value for the PTZ tilt command, and in the real
 *    world, it has +- 100.0 range.
 */
#define PTZ_PAN_MAX 100.0
#define PTZ_TILT_MAX 25.0
#define PTZ_PAN_CONV_FACTOR (0x370 / PTZ_PAN_MAX)
#define PTZ_TILT_CONV_FACTOR (0x12C / PTZ_TILT_MAX)


class CPtzDevice:public CDevice 
{
 private:
  pthread_t thread; // the thread that continuously reads from the laser 
  bool command_pending1;  // keep track of how many commands are pending;
  bool command_pending2;  // that way, we can cancel them if necessary
  bool ptz_fd_blocking;
  
  CLock lock;

  // internal methods
  int Send(unsigned char* str, int len, unsigned char* reply);
  int Receive(unsigned char* reply);
  int SendCommand(unsigned char* str, int len);
  int CancelCommand(char socket);
  int SendRequest(unsigned char* str, int len, unsigned char* reply);

 public:
  player_ptz_cmd_t* command;
  player_ptz_data_t* data; 

  int ptz_fd; // ptz device file descriptor
  /* device used to communicate with the ptz */
  char ptz_serial_port[MAX_FILENAME_SIZE]; 

  CPtzDevice(int argc, char** argv);
  ~CPtzDevice();

  virtual int Setup();
  virtual int Shutdown();
  virtual int SendAbsPanTilt(short pan, short tilt);
  virtual int SendAbsZoom(short zoom);
  virtual int GetAbsZoom(short* zoom);
  virtual int GetAbsPanTilt(short* pan, short* tilt);
  virtual void PrintPacket(char* str, unsigned char* cmd, int len);

  virtual CLock* GetLock( void ){ return &lock; };

  //virtual size_t GetData( unsigned char *, size_t maxsize );
  //virtual void PutData( unsigned char *, size_t maxsize );

  //virtual void GetCommand( unsigned char *, size_t maxsize );
  //virtual void PutCommand( unsigned char *, size_t maxsize);

  virtual size_t GetConfig( unsigned char *, size_t maxsize);
  virtual void PutConfig( unsigned char *, size_t maxsize);
};

#endif
