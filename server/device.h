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
    
    pthread_t devicethread;
    
    // did we allocate memory, or did someone else?
    bool allocp;

  protected:
    
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
    
  public:
    // who we are (currently set by CDeviceTable::AddDevice(); not a great
    // place)
    player_device_id_t device_id;
    
    // to record the time at which the device gathered the data
    // these are public because one device (e.g., P2OS) might need to set the
    // timestamp of another (e.g., sonar)
    uint32_t data_timestamp_sec;
    uint32_t data_timestamp_usec;
    
    // likewise, we sometimes need to access subscriptions from outside
    // number of current subscriptions
    int subscriptions;

    virtual ~CDevice();

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
    // The client pointer acts an identifier for devices that need to maintain
    // their own client lists (such as the broadcast device).  It's a void*
    // since it may refer to another device (such as when the laserbeacon device
    // subscribes to the laser device).
    virtual int Subscribe(void *client);
    virtual int Unsubscribe(void *client);

    // these two MUST be implemented by the device itself
    virtual int Setup() = 0;
    virtual int Shutdown() = 0;

    // these MAY be overridden by the device itself, but then the device
    // is reponsible for Lock()ing and Unlock()ing appropriately
    virtual size_t GetNumData(void* client);
    virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                           uint32_t* timestamp_sec, uint32_t* timestamp_usec);
    virtual void PutData(void* src, size_t len,
                         uint32_t timestamp_sec, uint32_t timestamp_usec);
    
    virtual size_t GetCommand(void* dest, size_t len);
    virtual void PutCommand(void* client, unsigned char* src, size_t len);
    
    virtual size_t GetConfig(void** client, void *data, size_t len);
    /* a "long form" GetConfig, this one returns the target device ID */
    virtual size_t GetConfig(player_device_id_t* device, void** client, 
                             void *data, size_t len);
    virtual int PutConfig(player_device_id_t* device, void* client, 
                          void* data, size_t len);

    virtual int GetReply(player_device_id_t* device, void* client, 
                         unsigned short* type, struct timeval* ts, 
                         void* data, size_t len);
    virtual int PutReply(player_device_id_t* device, void* client, 
                         unsigned short type, struct timeval* ts, 
                         void* data, size_t len);
    /* a short form, for common use; assumes zero-length reply and that the
     * originating device can be inferred from the client's subscription 
     * list 
     */
    virtual int PutReply(void* client, unsigned short type);
    /* another short form; this one allows actual replies */
    virtual int PutReply(void* client, unsigned short type, struct timeval* ts, 
                         void* data, size_t len);


    /* start a thread that will invoke Main() */
    virtual void StartThread();

    /* cancel (and wait for termination) of the thread */
    virtual void StopThread();
    
    // Main function for device thread
    //
    virtual void Main();

    // A helper method for internal use; e.g., when one device wants to make a
    // request of another device
    virtual int Request(player_device_id_t* device, void* requester, 
                        void* request, size_t reqlen,
                        unsigned short* reptype, struct timeval* ts,
                        void* reply, size_t replen);

  protected:
    // Dummy main (just calls real main).  This is used to simplify thread
    // creation.
    //
    static void* DummyMain(void *laserdevice);

    // these methods are used to lock and unlock the various buffers and
    // queues; they are implemented with mutexes in CDevice and overridden
    // in CStageDevice
    virtual void Lock();
    virtual void Unlock();

};

#endif
