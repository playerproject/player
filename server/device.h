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
class ConfigFile;


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
    
    // the device's thread
    pthread_t devicethread;

    // A condition variable (and accompanying mutex) that can be used to
    // signal other devices that are waiting on this one.
    pthread_cond_t cond;
    pthread_mutex_t condMutex;
    
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

    // Flag set if this is a new-style driver
    bool new_style;

    // signal that new data is available (calls pthread_cond_broadcast()
    // on this device's condition variable, which will release other
    // devices that are waiting on this one).
    void DataAvailable(void);

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

    // Total number of entries in the device table using this driver.
    // This is updated and read in the CDeviceEntry class
    int entries;

    // If true, device should be "always on", i.e., player will "subscribe" 
    // at startup, before any clients subscribe. The "alwayson" parameter in
    // the config file can be used to turn this feature on as well (in which
    // case this flag will be set to reflect that setting)
    // WARNING: this feature is experimental and may be removed in the future
    bool alwayson;

    // Last error value; useful for returning error codes from
    // constructors
    int error;

  public:
    // this is the main constructor, used by most non-Stage devices.
    // storage will be allocated by this constructor
    CDevice(size_t datasize, size_t commandsize, 
            int reqqueuelen, int repqueuelen);
    
    // this is the other constructor, used mostly by Stage devices.
    // if any of the default Put/Get methods are to be used, then storage for 
    // the buffers must allocated, and SetupBuffers() called.
    CDevice();

    // Destructor
    virtual ~CDevice();

    // Set/reset error code
    void SetError(int code) {this->error = code;}

    // this method is used by devices that allocate their own storage, but wish to
    // use the default Put/Get methods
    void SetupBuffers(unsigned char* data, size_t datasize, 
                      unsigned char* command, size_t commandsize, 
                      unsigned char* reqqueue, int reqqueuelen, 
                      unsigned char* repqueue, int repqueuelen);

    // Read a device id from the config file (helper function for
    // new-style drivers).  Note that code is the *expected* interface
    // type; returns 0 if a suitable device id is found.
    int ReadDeviceId(ConfigFile *cf, int entity, int index,
                     int code, player_device_id_t *id);
    
    // Add a new-style interface; returns 0 on success
    int AddInterfaceEx(player_device_id_t id, const char *drivername,
                       unsigned char access, size_t datasize, size_t commandsize,
                       size_t reqqueuelen, size_t repqueuelen);
    
    // these are used to control subscriptions to the device; a device MAY
    // override them, but usually won't (P2OS being the main exception).
    // The client pointer acts an identifier for devices that need to maintain
    // their own client lists (such as the broadcast device).  It's a void*
    // since it may refer to another device (such as when the laserbeacon device
    // subscribes to the laser device).
    virtual int Subscribe(void *client);
    virtual int Unsubscribe(void *client);

    // these are called when the first client subscribes, and when the last
    // client unsubscribes, respectively.
    // these two MUST be implemented by the device itself
    virtual int Setup() = 0;
    virtual int Shutdown() = 0;


    // This method is called by Player on each device after all devices have
    // been loaded, and immediately before entering the main loop, so override
    // it in your device subclass if you need to do some last minute setup with
    // Player all set up and ready to go
    // WANRING: this feature is experimental and may be removed in the future
    virtual void Prepare() {}

    // This method is called once per loop (in Linux, probably either 50Hz or 
    // 100Hz) by the server.  Threaded drivers can use the default
    // implementation, which does nothing.  Drivers which don't have their
    // own threads and do all their work in an overridden GetData() should 
    // also override Update(), in order to call DataAvailable(), allowing
    // other drivers to Wait() on this driver.
    virtual void Update() {}

    // Note: the Get* and Put* functions MAY be overridden by the
    // device itself, but then the device is reponsible for Lock()ing
    // and Unlock()ing appropriately

    // Write new data to the device
    virtual void PutData(void* src, size_t len,
                         uint32_t timestamp_sec, uint32_t timestamp_usec);

    // New-style PutData; [id] specifies the interface to be written
    virtual void PutDataEx(player_device_id_t id,
                           void* src, size_t len,
                           uint32_t timestamp_sec, uint32_t timestamp_usec);

    // Read new data from the device
    virtual size_t GetNumData(void* client);
    virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                           uint32_t* timestamp_sec, uint32_t* timestamp_usec);

    // New-style GetData; [id] specifies the interface to be read
    virtual size_t GetNumDataEx(player_device_id_t id, void* client);
    virtual size_t GetDataEx(player_device_id_t id, void* client,  
                             unsigned char* dest, size_t len,
                             uint32_t* timestamp_sec, uint32_t* timestamp_usec);

    // Write a new command to the device
    virtual void PutCommand(void* client, unsigned char* src, size_t len);

    // New-style: Write a new command to the device
    virtual void PutCommandEx(player_device_id_t id, void* client,
                              unsigned char* src, size_t len);

    // Read the current command for the device
    virtual size_t GetCommand(void* dest, size_t len);

    // New-style: Read the current command for the device
    virtual size_t GetCommandEx(player_device_id_t id, void* dest, size_t len);

    // Write configuration request to device
    virtual int PutConfig(player_device_id_t* device, void* client, 
                          void* data, size_t len);

    // New-style: Write configuration request to device
    virtual int PutConfigEx(player_device_id_t id, void *client, void *data, size_t len);

    // Read configuration request from device
    virtual size_t GetConfig(player_device_id_t* device, void** client, 
                             void *data, size_t len);

    // New-style: Get next configuration request for device
    virtual size_t GetConfigEx(player_device_id_t id, void **client, void *data, size_t len);

    // Write configuration reply to device
    virtual int PutReply(player_device_id_t* device, void* client, 
                         unsigned short type, struct timeval* ts, 
                         void* data, size_t len);

    // New-style: Write configuration reply to device
    virtual int PutReplyEx(player_device_id_t id, void* client, 
                           unsigned short type, struct timeval* ts, 
                           void* data, size_t len);

    // Read configuration reply from device
    virtual int GetReply(player_device_id_t* device, void* client, 
                         unsigned short* type, struct timeval* ts, 
                         void* data, size_t len);

    // New-style: Read configuration reply from device;
    // The src_id field is a legacy hack for p2os-syle request/reply tom-foolery.
    virtual int GetReplyEx(player_device_id_t id, player_device_id_t *src_id, void* client, 
                           unsigned short* type, struct timeval* ts, 
                           void* data, size_t len);

    // Convenient short form
    virtual size_t GetConfig(void** client, void* data, size_t len);

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

    // Cleanup function for device thread (called when main exits)
    //
    virtual void MainQuit(void);

    // A helper method for internal use; e.g., when one device wants to make a
    // request of another device.
    virtual int Request(player_device_id_t* device, void* requester, 
                        void* request, size_t reqlen,
                        unsigned short* reptype, struct timeval* ts,
                        void* reply, size_t replen);

    // Waits on the condition variable associated with this device.
    // This method is called by other devices to wait for new data.
    void Wait(void);

  protected:
    // Dummy main (just calls real main).  This is used to simplify thread
    // creation.
    //
    static void* DummyMain(void *device);

    // Dummy main cleanup (just calls real main cleanup).  This is
    // used to simplify thread termination.
    //
    static void DummyMainQuit(void *device);

    // these methods are used to lock and unlock the various buffers and
    // queues; they are implemented with mutexes in CDevice and overridden
    // in CStageDevice
    virtual void Lock();
    virtual void Unlock();

};

#endif
