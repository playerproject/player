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
#include <message.h>
#include <player.h>
#include <clientdata.h> // needed so clients using ClientDataInternal can cast it to ClientData implicitly
#include <clientmanager.h> // needed so clients can access the global clientmangager

extern bool debug;
extern bool experimental;

// Forward declarations
class CLock;
class ConfigFile;
class Driver;
class ClientData;




/// @brief Base class for all drivers.
///
/// This class manages driver subscriptions, threads, and data
/// marshalling to/from device interfaces.  All drivers inherit from
/// this class, and most will overload the Setup(), Shutdown() and
/// Main() methods.
class Driver
{
  protected:
    /// The driver's thread, when managed by StartThread() and
    /// StopThread().
    pthread_t driverthread;

  private:
    /// This mutex is used to lock data, command, and req/rep buffers/queues,
    /// via Lock() and Unlock().
    pthread_mutex_t accessMutex;

    /// A condition variable that can be used to signal, via
    /// DataAvailable(), other drivers that are Wait()ing on this
    /// driver.
    pthread_cond_t cond;

    /// Mutex to go with condition variable cond.
    pthread_mutex_t condMutex;

  protected:
    // Dummy main (just calls real main).  This is used to simplify
    // thread creation.
    static void* DummyMain(void *driver);

    // Dummy main cleanup (just calls real main cleanup).  This is
    // used to simplify thread termination.
    static void DummyMainQuit(void *driver);
    
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
    /// constructors.
    int error;

    /// Queue for all incoming messages for this driver
    MessageQueue* InQueue;

    /// @brief Constructor for single-interface drivers.
    //
    /// @param cf Current configuration file
    /// @param section Current section in configuration file
    /// @param overwrite_cmds Do new commands overwrite old ones?
    /// @param queue_maxlen How long can the incoming queue grow?
    /// @param interface Player interface code; e.g., PLAYER_POSITION_CODE
    /// @param access Allowed access mode; e.g., PLAYER_READ_MODE
    Driver(ConfigFile *cf, 
           int section, 
           bool overwrite_cmds, 
           size_t queue_maxlen, 
           int interface, 
           uint8_t access);

    /// @brief Constructor for multiple-interface drivers.
    ///
    /// Use AddInterface() to specify individual interfaces.
    /// @param cf Current configuration file
    /// @param section Current section in configuration file
    /// @param overwrite_cmds Do new commands overwrite old ones?
    /// @param queue_maxlen How long can the incoming queue grow?
    Driver(ConfigFile *cf, 
           int section,
           bool overwrite_cmds = true, 
           size_t queue_maxlen = PLAYER_MSGQUEUE_DEFAULT_MAXLEN);

    /// @brief Destructor
    virtual ~Driver();
    
    /// @brief Add a new-style interface.
    ///
    /// @param id Player device id.
    /// @param access Allowed access mode; e.g., PLAYER_READ_MODE
    /// @returns Returns 0 on success
    int AddInterface(player_device_id_t id, unsigned char access);
    
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

    /// @brief Start the driver thread
    ///
    /// This method is usually called from the overloaded Setup() method to
    /// create the driver thread.  This will call Main().
    virtual void StartThread(void);

    /// @brief Cancel (and wait for termination) of the driver thread
    ///
    /// This method is usually called from the overloaded Shutdown() method
    /// to terminate the driver thread.
    virtual void StopThread(void);

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

    /// @brief Helper for message processing.
    ///
    /// Returns true if @p hdr matches the supplied @p type, @p subtype, 
    /// and @p id.
    bool MatchMessage(player_msghdr_t* hdr, 
                      uint8_t type, uint8_t subtype, player_device_id_t id)
    {
      return((hdr->type == type) && 
             (hdr->subtype == subtype) && 
             (hdr->device == id.code) && 
             (hdr->device_index == id.index));
    }

    /// Call this to automatically process messages using registered handler
    /// Processes messages until no messages remaining in the queue or
    /// a message with no handler is reached
    void ProcessMessages();
	
    /// This function is called once for each message in the incoming queue.
    /// Reimplement it to provide message handling.
    /// Return 0 for no response, otherwise with player_msgtype if response 
    /// needed.
    /// If calling this method from outside the driver lock the driver mutex 
    /// first.
    /// When writing the driver make sure to protect accesses that could 
    /// clash in the main thread with mutex locks.
    /// @p resp_data will be filled out with the response data and 
    /// @p resp_len set to the actual response size; @p resp_data should be 
    /// able to take max player msg size.
    virtual int ProcessMessage(ClientData * client, player_msghdr * hdr, 
                               uint8_t * data, uint8_t * resp_data,
                               size_t * resp_len) 
    {return -1;};

    /// Helper function that creates the header and then calls driver ProcessMessage
    /// for use by drivers for internal requests
    int ProcessMessage(uint8_t Type, uint8_t SubType,
                       player_device_id_t device,
                       size_t size, uint8_t * data, 
                       uint8_t * resp_data, size_t * resp_len);

    /// Helper function that creates the header and then calls driver ProcessMessage
    /// for use by drivers for internal requests that expect no reply
    int ProcessMessage(uint8_t Type, uint8_t SubType,
                       player_device_id_t device,
                       size_t size, uint8_t * data);

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
    virtual void Update() 
    {
    	if (!driverthread)
    		ProcessMessages();
    }

    /// Put Msg to Client
    virtual void PutMsg(player_device_id_t id, 
                        ClientData* client, 
                        uint8_t type, 
                        uint8_t subtype,
                        void* src, 
                        size_t len = 0,
                        struct timeval* timestamp = NULL);

  protected:
    // these methods are used to lock and unlock the various buffers and
    // queues; they are implemented with mutexes in Driver and overridden
    // in CStageDriver
    virtual void Lock(void);
    virtual void Unlock(void);
    
    // used for subscriptions to other drivers internally
    ClientDataInternal * BaseClient;
    
    /// @brief Subscribe to another driver using the internal BaseClient
    ///
    /// This method subcribes internally to another driver, it will return a 
    /// pointer to the driver if successful, this pointer will be valid
    /// until unsubscribe is called
    Driver * SubscribeInternal(player_device_id_t id);

    /// @brief Unsubscribe to another driver using the internal BaseClient
    ///
    /// This method unsibscribes internally from another driver
    void UnsubscribeInternal(player_device_id_t id);

};



#endif
