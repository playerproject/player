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
 *  The virtual class from which all driver classes inherit.  this
 *  defines the API that all drivers must implement.
 */

#ifndef _DRIVER_H
#define _DRIVER_H

#include <pthread.h>

#include <stddef.h> /* for size_t */
#include "playercommon.h"
#include "playerqueue.h"

extern bool debug;
extern bool experimental;

// Forward declarations
class CLock;
class ConfigFile;


/// @brief Base class for all drivers.
///
/// This class manages driver subscriptions, threads, and data
/// marshalling to/from device interfaces.  All drivers inherit from
/// this class, and most will overload the Setup(), Shutdown() and
/// Main() methods.
class Driver
{
  private:
    // this mutex is used to lock data, command, and req/rep buffers/queues
    // TODO: could implement different mutexes for each data structure, but
    // is it worth it?
    //
    // NOTE: StageDevice won't use this; it declares its own inter-process
    // locking mechanism (and overrides locking methods)
    pthread_mutex_t accessMutex;

    // the driver's thread
    pthread_t driverthread;

    // A condition variable (and accompanying mutex) that can be used to
    // signal other drivers that are waiting on this one.
    pthread_cond_t cond;
    pthread_mutex_t condMutex;
    

  public:
    /// Default device id (single-interface drivers)
    player_device_id_t device_id;
        
    /// Number of subscriptions to this driver.
    int subscriptions;

    /// Total number of entries in the device table using this driver.
    /// This is updated and read by the Device class.
    int entries;

    /// If true, driver should be "always on", i.e., player will
    /// "subscribe" at startup, before any clients subscribe. The
    /// "alwayson" parameter in the config file can be used to turn
    /// this feature on as well (in which case this flag will be set
    /// to reflect that setting).
    bool alwayson;

    /// Last error value; useful for returning error codes from
    /// constructors
    int error;

  public:

    /// @brief Constructor for single-interface drivers.
    //
    /// @param cf Current configuration file
    /// @param section Current section in configuration file
    /// @param interface Player interface code; e.g., PLAYER_POSITION_CODE
    /// @param access Allowed access mode; e.g., PLAYER_READ_MODE
    /// @param datasize Maximum size of the data packet.
    /// @param commandsize Maximum size of the command packet.
    /// @param reqqueuelen Length of the request queue.
    /// @param repqueuelen Length of the reply queue.
    Driver(ConfigFile *cf, int section, int interface, uint8_t access,
           size_t datasize, size_t commandsize, 
           size_t reqqueuelen, size_t repqueuelen);

    /// @brief Constructor for multiple-interface drivers.
    ///
    /// Use AddInterface() to specify individual interfaces.
    /// @param cf Current configuration file
    /// @param section Current section in configuration file
    Driver(ConfigFile *cf, int section);

    /// @brief Destructor
    virtual ~Driver();
    
    /// @brief Add a new-style interface.
    ///
    /// @param id Player device id.
    /// @param access Allowed access mode; e.g., PLAYER_READ_MODE
    /// @param datasize Maximum size of the data packet.
    /// @param commandsize Maximum size of the command packet.
    /// @param reqqueuelen Length of the request queue.
    /// @param repqueuelen Length of the reply queue.
    /// @returns Returns 0 on success
    int AddInterface(player_device_id_t id, unsigned char access,
                     size_t datasize, size_t commandsize,
                     size_t reqqueuelen, size_t repqueuelen);
    
    /// @brief Add a new-style interface, with pre-allocated memory.
    ///
    /// @param id Player device id.
    /// @param access Allowed access mode; e.g., PLAYER_READ_MODE
    /// @param data Pointer to pre-allocated data storage.
    /// @param datasize Maximum size of the data packet.
    /// @param command Pointer to pre-allocated command storage.
    /// @param commandsize Maximum size of the command packet.
    /// @param reqqueue Pointer to pre-allocated request queue storage.
    /// @param reqqueuelen Length of the request queue.
    /// @param repqueue Pointer to pre-allocated reply queue storage.
    /// @param repqueuelen Length of the reply queue.
    /// @returns Returns 0 on success
    int AddInterface(player_device_id_t id, unsigned char access,
                     void* data, size_t datasize, 
                     void* command, size_t commandsize, 
                     void* reqqueue, int reqqueuelen, 
                     void* repqueue, int repqueuelen);

    /// @brief Set/reset error code
    void SetError(int code) {this->error = code;}

    /// @brief Subscribe to this driver.
    ///
    /// The Subscribe() and Unsubscribe() methods are used to control
    /// subscriptions to the driver; a driver MAY override them, but
    /// usually won't.  
    ///
    /// @param id Id of the device to subscribe to (the driver may
    /// have more than one interface).
    /// @returns Returns 0 on success.
    virtual int Subscribe(player_device_id_t id);

    /// @brief Unsubscribe from this driver.
    ///
    /// The Subscribe() and Unsubscribe() methods are used to control
    /// subscriptions to the driver; a driver MAY override them, but
    /// usually won't.  
    ///
    /// @param id Id of the device to unsubscribe from (the driver may
    /// have more than one interface).
    /// @returns Returns 0 on success.
    virtual int Unsubscribe(player_device_id_t id);

    /// @brief Initialize the driver.
    ///
    /// This function is called with the first client subscribes; it MUST
    /// be implemented by the driver.
    ///
    /// @returns Returns 0 on success.
    virtual int Setup() = 0;

    /// @brief Finalize the driver.
    ///
    /// This function is called with the last client unsubscribes; it MUST
    /// be implemented by the driver.
    ///
    /// @returns Returns 0 on success.
    virtual int Shutdown() = 0;

    /// @brief Do some extra initialization.
    ///
    /// This method is called by Player on each driver after all drivers have
    /// been loaded, and immediately before entering the main loop, so override
    /// it in your driver subclass if you need to do some last minute setup with
    /// Player all set up and ready to go.
    /// @todo I think this can be deprecated.
    virtual void Prepare() {}

  public:

    /// @brief Start the driver thread
    ///
    /// This method is usually called from the overloaded Setup() method to
    /// create the driver thread.  This will call Main().
    void StartThread(void);

    /// @brief Cancel (and wait for termination) of the driver thread
    ///
    /// This method is usually called from the overloaded Shutdown() method
    /// to terminate the driver thread.
    void StopThread(void);

    /// @brief Main method for driver thread.
    ///
    /// Most drivers have their own thread of execution, created using
    /// StartThread(); this is the entry point for the driver thread,
    /// and must be overloaded by all threaded drivers.
    virtual void Main(void);

    /// @brief Cleanup method for driver thread (called when main exits)
    ///
    /// Overload this method and to do additional cleanup when the
    /// driver thread exits.
    virtual void MainQuit(void);

  private:

    // Dummy main (just calls real main).  This is used to simplify
    // thread creation.
    static void* DummyMain(void *driver);

    // Dummy main cleanup (just calls real main cleanup).  This is
    // used to simplify thread termination.
    static void DummyMainQuit(void *driver);

  public:
    
    /// @brief Wait on the condition variable associated with this driver.
    ///
    /// This method blocks until new data is available (as indicated
    /// by a call to PutData() or DataAvailable()).  Usually called in
    /// the context of another driver thread.
    void Wait(void);

    /// @brief Signal that new data is available.
    ///
    /// Calling this method will release any threads currently waiting
    /// on this driver.  Called automatically by the default PutData()
    /// implementation.
    ///
    /// @todo 
    /// Fix the semantics of
    /// DataAvailable() and Wait().  As currenly implemented, we Wait() on
    /// devices, but call DataAvailable() on drivers.  For drivers with multiple
    /// interfaces, this means than threads blocked on a wait will resume when
    /// *any* of the driver's interfaces is updated (not the expected behavior).
    void DataAvailable(void);

    // a static version of DataAvailable that can be used as a
    // callback from libraries. It calls driver->DataAvailable(). 
    static void DataAvailableStatic( Driver* driver );

    /// @brief Update non-threaded drivers.
    ///
    /// This method is called once per loop (in Linux, probably either
    /// 50Hz or 100Hz) by the server.  Threaded drivers can use the
    /// default implementation, which does nothing.  Non-threaded
    /// drivers should do whatever they have to do, and call PutData()
    /// when they have new data.
    virtual void Update() {}

    // Note: the Get* and Put* functions MAY be overridden by the
    // driver itself, but then the driver is reponsible for Lock()ing
    // and Unlock()ing appropriately

    /// @brief Write data to the driver.
    ///
    /// This method will usually be called by the driver.
    /// @param id Id of the device to write to (drivers may have
    /// multiple interfaces).
    /// @param src Pointer to data source buffer.
    /// @param len Length of the data (bytes).
    /// @param timestamp Data timestamp; if NULL, the current server time will be used.
    virtual void PutData(player_device_id_t id, void* src, size_t len,
                         struct timeval* timestamp);

    /// @brief Write data to the driver (short form).
    ///
    /// Convenient short form of PutData() for single-interface drivers.
    void PutData(void* src, size_t len, struct timeval* timestamp)
      { PutData(this->device_id,src,len,timestamp); }

    /// @brief Read data from the driver.
    ///
    /// This function will usually be called by the server.
    /// @param id Specifies the device to be read.
    /// @param dest Pointer to data destination buffer.
    /// @param len Size of the data destination buffer (bytes).
    /// @param timestamp If non-null, will be filled with the data timestamp.
    /// @returns Returns the number of bytes written to the destination buffer.
    virtual size_t GetData(player_device_id_t id,
                           void* dest, size_t len,
                           struct timeval* timestamp);

    /// @brief Write a new command to the driver.
    ///
    /// This function will usually be called by the server.
    /// @param id Specifies the device to be written.
    /// @param src Pointer to command source buffer.
    /// @param len Length of the command (bytes).
    /// @param timestamp Command timestamp; if NULL, the current server time will be used.
    virtual void PutCommand(player_device_id_t id,
                            void* src, size_t len,
                            struct timeval* timestamp);

    /// @brief Read the current command for the driver
    ///
    /// This function will usually be called by the driver.
    /// @param id Specifies the device to be read.
    /// @param dest Pointer to command destination buffer.
    /// @param len Length of the destination buffer (bytes).
    /// @param timestamp If non-null, will be filled with the command timestamp.
    /// @returns Returns the number of bytes written to the destination buffer.
    virtual size_t GetCommand(player_device_id_t id,
                              void* dest, size_t len,
                              struct timeval* timestamp);

    /// @brief Read the current command (short form).
    ///
    /// Convenient short form for single-interface drivers.
    size_t GetCommand(void* dest, size_t len,
                      struct timeval* timestamp)
      { return GetCommand(this->device_id,dest,len,timestamp); }

    /// @brief Clear the current command buffer
    ///
    /// This method is called by drivers to "consume" commands.
    /// @param id Specifies the device to be cleared .    
    virtual void ClearCommand(player_device_id_t id);

    /// @brief Clear the current command buffer
    ///
    /// Convenient short form of ClearCommand(), for single-interface
    /// drivers.
    void ClearCommand(void)
      { ClearCommand(this->device_id); }

    /// @brief Write configuration request to the request queue.
    ///
    /// Unlike data and commands, requests are added to a queue.
    /// This function will usually be called by the server.
    /// @param id Specifies the device to be written.
    /// @param client A generic pointer for routing replies (advanced usage only).
    /// @param src Pointer to source buffer.
    /// @param len Length of the source buffer (bytes).
    /// @param timestamp Command timestamp; if NULL, the current server time will be used.
    /// @returns Returns 0 on success (queues may overflow, generating an error).
    virtual int PutConfig(player_device_id_t id, void *client, 
                          void* src, size_t len,
                          struct timeval* timestamp);

    /// @brief Read the next configuration request from the request queue.
    /// 
    /// Unlike data and commands, requests are added to and removed from a queue.
    /// This method will usually be called by the driver.
    /// @param id Specifies the device to be read.
    /// @param client Pointer to a generic pointer for routing replies (advanced usage only).
    /// @param dest Pointer to destination buffer.
    /// @param len Length of the destination buffer (bytes).
    /// @param timestamp If non-null, will be filled with the request timestamp.
    /// @returns Returns 0 if there are no pending requests, otherwise returns the
    /// number of bytes written to the destination buffer.
    virtual int GetConfig(player_device_id_t id, void **client, 
                          void* dest, size_t len,
                          struct timeval* timestamp);

    /// @brief Read the next configuration request from the request queue.
    ///
    /// Convenient short form of GetConfig(), for single-interface drivers.
    int GetConfig(void** client, void* dest, size_t len, 
                  struct timeval* timestamp)
      { return GetConfig(this->device_id,client,dest,len,timestamp); }

    /// @brief Write configuration reply to the reply queue.
    ///
    /// This method will usually be called by the driver.
    /// @param id Specifies the device to be written.
    /// @param client A generic pointer for routing replies (advanced usage only).
    /// @param type Reply type; e.g., PLAYER_MSGTYPE_RESP_ACK or PLAYER_MSGTYPE_RESP_NACK.
    /// @param src Pointer to source buffer.
    /// @param len Length of the source buffer (bytes).
    /// @param timestamp Command timestamp; if NULL, the current server time will be used.
    /// @returns Returns 0 on success (queues may overflow, generating an error).
    virtual int PutReply(player_device_id_t id, void* client, 
                         unsigned short type, 
                         void* src, size_t len,
                         struct timeval* timestamp);

    /// @brief Write configuration reply to the reply queue.
    ///
    /// Convenient short form of PutReply() for single-interface
    /// drivers.
    int PutReply(void* client, 
                 unsigned short type, 
                 void* src, size_t len,
                 struct timeval* timestamp)
      { return PutReply(this->device_id,client,type,src,len,timestamp); }

    /// @brief Write configuration reply to the reply queue.
    ///
    /// Convenient short form of PutReply(): empty reply
    int PutReply(player_device_id_t id, void* client, unsigned short type,
                 struct timeval* timestamp)
      { return PutReply(id,client,type,NULL,0,timestamp); }

    /// @brief Write configuration reply to the reply queue.
    ///
    /// Convenient short form of PutReply(): empty reply for single-interface drivers.
    int PutReply(void* client, unsigned short type,
                 struct timeval* timestamp)
      { return PutReply(this->device_id,client,type,NULL,0,timestamp); }

    /// @brief Read configuration reply from reply queue
    ///
    /// This function will usually be called by the server.
    /// @param id Specifies the device to be read.
    /// @param client A generic pointer for routing replies (advanced usage only).
    /// @param type Reply type; e.g., PLAYER_MSGTYPE_RESP_ACK or PLAYER_MSGTYPE_RESP_NACK.
    /// @param dest Pointer to destination buffer.
    /// @param len Size of the destination buffer (bytes).
    /// @param timestamp If non-null, will be filled with the data timestamp.
    /// @returns Returns the number of bytes written to the destination buffer.
    virtual int GetReply(player_device_id_t id, void* client, 
                         unsigned short* type, 
                         void* dest, size_t len,
                         struct timeval* timestamp);

    /// A helper method for internal use; e.g., when one driver wants to make a
    /// request of another driver.
    virtual int Request(player_device_id_t id, void* requester, 
                        void* request, size_t reqlen,
                        struct timeval* req_timestamp,
                        unsigned short* reptype, 
                        void* reply, size_t replen,
                        struct timeval* rep_timestamp);

  protected:
    // these methods are used to lock and unlock the various buffers and
    // queues; they are implemented with mutexes in Driver and overridden
    // in CStageDriver
    virtual void Lock(void);
    virtual void Unlock(void);

};


#endif
