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
 *   a general purpose lock class.  each device object has one of
 *   these and uses it to control access to common data buffers.
 */

#ifndef _LOCK_H
#define _LOCK_H

#include <pthread.h>
#include <device.h>

// For size_t
//
#include <stddef.h>
#include <playercommon.h>

// getting around circular inclusion
class CDevice;

class CLock {
private:
  pthread_mutex_t dataAccessMutex;
  pthread_mutex_t commandAccessMutex;
  pthread_mutex_t configAccessMutex;
  pthread_mutex_t subscribeMutex;
  pthread_mutex_t setupDataMutex;

  bool firstdata;

 protected:
  int subscriptions;

public:
  CLock();
  virtual ~CLock();

  virtual int Setup(CDevice *);
  virtual int Shutdown(CDevice *);

  virtual int Subscribe(CDevice *);
  virtual int Unsubscribe(CDevice *);

  virtual size_t GetData(CDevice *, unsigned char *, size_t,
                         uint32_t* timestamp_sec, uint32_t *timestamp_usec);
  virtual void PutData(CDevice *, unsigned char *, size_t,
                       uint32_t timestamp_sec = 0, uint32_t timestamp_usec = 0);

  virtual void GetCommand(CDevice *, unsigned char *, size_t );
  virtual void PutCommand(CDevice *, unsigned char *, size_t);

  virtual size_t GetConfig(CDevice *, unsigned char *, size_t );
  virtual void PutConfig(CDevice *, unsigned char *, size_t);
};

#endif
