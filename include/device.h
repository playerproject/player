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
 *  the virtual class from which all device classes inherit.  this
 *  defines the interface that all devices must implement.
 */

#ifndef _DEVICE_H
#define _DEVICE_H

#include <pthread.h>

#include <stddef.h> /* for size_t */
#include <clientdata.h>
#include <playercommon.h>
#include <playerqueue.h>

extern bool debug;
extern bool experimental;

// getting around circular inclusion
class CLock;

class CDevice 
{
  private:
    // this mutex is used to lock data, command, and req/rep buffers/queues
    // TODO: could implement different mutexes for each data structure, but
    // is it worth it?
    //
    // NOTE: StageDevice won't use this; it declares its own inter-process
    // locking mechanism (and overrides locking methods)
    pthread_mutex_t accessMutex;

    // this mutex is used to mutually exclude calls to Setup and Shutdown
    pthread_mutex_t setupMutex;


  public:
    // number of current subscriptions
    int subscriptions;
    
    // buffers for data and command
    unsigned char* device_data;
    unsigned char* device_command;

    // maximum sizes of data and command buffers
    size_t device_datasize;
    size_t device_commandsize;

    // amount at last write into each respective buffer
    size_t device_used_datasize;
    size_t device_used_commandsize;

    // queues for incoming requests and outgoing replies
    PlayerQueue* device_reqqueue;
    PlayerQueue* device_repqueue;

    virtual ~CDevice() {};


    // this is the main constructor, used by most non-Stage devices.
    // storage will be allocated by this constructor
    CDevice(size_t datasize, size_t commandsize, 
            int reqqueuelen, int repqueuelen);
    
    // this is the other constructor, used mostly by Stage devices.
    // if any of the default Put/Get methods are to be used, then storage for 
    // the buffers must allocated, and SetupBuffers() called.
    CDevice();

    void SetupBuffers(unsigned char* data, size_t datasize, 
                      unsigned char* command, size_t commandsize, 
                      unsigned char* reqqueue, int reqqueuelen, 
                      unsigned char* repqueue, int repqueuelen);

    // these are used to control subscriptions to the device; a device MAY
    // override them, but usually won't (P2OS being the main exception).
    virtual int Subscribe();
    virtual int Unsubscribe();

    // these two MUST be implemented by the device itself
    virtual int Setup() = 0;
    virtual int Shutdown() = 0;

    // these MAY be overridden by the device itself, but then the device
    // is reponsible for Lock()ing and Unlock()ing appropriately
    virtual size_t GetData(unsigned char* dest, size_t len,
                        uint32_t* timestamp_sec, uint32_t* timestamp_usec);
    virtual void PutData(unsigned char* src, size_t len,
                         uint32_t timestamp_sec, uint32_t timestamp_usec);
    
    virtual size_t GetCommand( unsigned char *, size_t );
    virtual void PutCommand( unsigned char * , size_t );
    
    virtual size_t GetConfig(CClientData** client, unsigned char *, size_t);
    virtual int PutConfig(CClientData* client, unsigned char * , size_t);

    virtual int GetReply(CClientData* client, unsigned short* type,
                         struct timeval* ts, unsigned char *, size_t);
    virtual int PutReply(CClientData* client, unsigned short type,
                         struct timeval* ts, unsigned char * , size_t);


    // to record the time at which the device gathered the data
    uint32_t data_timestamp_sec;
    uint32_t data_timestamp_usec;

  protected:
    // these methods are used to lock and unlock the various buffers and
    // queues; they are implemented with mutexes in CDevice and overridden
    // in CStageDevice
    virtual void Lock();
    virtual void Unlock();

    // these methods are used to control calls to Setup and Shutdown
    virtual void SetupLock();
    virtual void SetupUnlock();
};

#endif
