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
 * the AUDIO device
 */

#ifndef AUDIODEVICE
#define AUDIODEVICE

#include <pthread.h>
#include <unistd.h>

#include "lock.h"
#include "device.h"

#define AUDIO_SLEEP_TIME_USEC 100000

#define MAX_AUDIO_PACKET_LENGTH 16
#define MAX_AUDIO_MESSAGE_LENGTH 14
#define MAX_AUDIO_REPLY_LENGTH 11

#define LENGTH 1    /* how many tenth of seconds of samplig max */
#define RATE 8000   /* the sampling rate */
#define SIZE 8      /* sample size: 8 or 16 bits */
#define CHANNELS 1  /* 1 = mono 2 = stereo */
#define N 1024 /* The resolution in frequency space desired */
#define nIterations 10000
#define heardHistoryN 5
#define nHighestPeaks 5
#define MIN_FREQUENCY 800

// possibly to "offsets.h"
#define AUDIO_DATA_BUFFER_SIZE 20
#define AUDIO_COMMAND_BUFFER_SIZE 3*sizeof(short)

class CAudioDevice:public CDevice {
 private:
  pthread_t thread; // the thread that continuously reads/writes from the dsp 
  bool command_pending1;  // keep track of how many commands are pending;
  bool command_pending2;  // that way, we can cancel them if necessary
  bool audio_fd_blocking;
  
  CLock lock;

 public:
  unsigned char* command;   // array holding the client's commands
  unsigned char* data;      // array holding the most recent feedback

  // Esbens own additions
  int audio_fd; // audio device file descriptor
  int configureDSP();
  void openDSPforRead();
  void openDSPforWrite();
  void insertPeak(int f,int a);
  int sampleSound();
  void listenForTones();
  int playSound(int duration);

  CAudioDevice();
  ~CAudioDevice();

  virtual int Setup();
  virtual int Shutdown();
  virtual void PrintPacket(char* str, unsigned char* cmd, int len);
 
  virtual CLock* GetLock( void ){ return &lock; };

  virtual size_t GetData( unsigned char *, size_t maxsize );
  virtual void PutData( unsigned char *, size_t maxsize );

  virtual void GetCommand( unsigned char *, size_t maxsize );
  virtual void PutCommand( unsigned char *, size_t maxsize);

  virtual size_t GetConfig( unsigned char *, size_t maxsize);
  virtual void PutConfig( unsigned char *, size_t maxsize);
};

#endif
