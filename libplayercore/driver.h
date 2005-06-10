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

#include <libplayercore/playercommon.h>
#include <libplayercore/message.h>
#include <libplayercore/player.h>

// Forward declarations
class ConfigFile;
class Driver;

/// @brief Base class for all drivers.
///
/// This class manages driver subscriptions, threads, and data
/// marshalling to/from device interfaces.  All drivers inherit from
/// this class, and most will overload the Setup(), Shutdown() and
/// Main() methods.
class Driver
{
  private:
    /// @brief Mutex used to lock access, via Lock() and Unlock(), to
    /// driver internals, like the list of subscribed queues.
    pthread_mutex_t accessMutex;

    /// @brief Last error value; useful for returning error codes from
    /// constructors.
    int error;

  protected:
    /// @brief The driver's thread. 
    ///
    /// The driver's thread, when managed by StartThread() and
    /// StopThread().
    pthread_t driverthread;

    /// @brief Last requester's queue.
    ///
    /// Pointer to a queue to which this driver owes a reply.  Used mainly
    /// by non-threaded drivers to cache the return address for requests
    /// that get forwarded to other devices.
    MessageQueue* ret_queue;

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

    /// @brief Dummy main (just calls real main).  This is used to simplify
    /// thread creation.
    static void* DummyMain(void *driver);

    /// @brief Dummy main cleanup (just calls real main cleanup).  This is
    /// used to simplify thread termination.
    static void DummyMainQuit(void *driver);

    /// @brief Publish a message via one of this driver's interfaces.
    ///
    /// This form of Publish will assemble the message header for you.
    /// @param addr The origin address
    /// @param queue If non-NULL, the target queue; if NULL,
    ///        then the message is sent to all interested parties.
    /// @param type The message type
    /// @param subtype The message subtype
    /// @param src The message body
    /// @param len Length of the message body
    /// @param timestamp Timestamp for the message body
    virtual void Publish(player_devaddr_t addr, 
                         MessageQueue* queue, 
                         uint8_t type, 
                         uint8_t subtype,
                         void* src, 
                         size_t len,
                         struct timeval* timestamp);

    /// @brief Publish a message via one of this driver's interfaces.
    ///
    /// Use this form of Publish if you already have the message header
    /// assembled.
    /// @param queue If non-NULL, the target queue; if NULL,
    ///        then the message is sent to all interested parties.
    /// @param hdr The message header
    /// @param src The message body
    void Publish(MessageQueue* queue, 
                 player_msghdr_t* hdr,
                 void* src);

    /// @brief Add an interface.
    ///
    /// @param addr Player device address.
    /// @param access Allowed access mode; e.g., PLAYER_READ_MODE
    /// @returns Returns 0 on success
    int AddInterface(player_devaddr_t addr, unsigned char access);
    
    /// @brief Set/reset error code
    void SetError(int code) {this->error = code;}
    
  public:
    /// @brief Lock access to driver internals.
    virtual void Lock(void);
    /// @brief Unlock access to driver internals.
    virtual void Unlock(void);

    /// @brief Default device address (single-interface drivers)
    player_devaddr_t device_addr;
        
    /// @brief Number of subscriptions to this driver.
    int subscriptions;

    /// @brief Total number of entries in the device table using this driver.
    /// This is updated and read by the Device class.
    int entries;

    /// @brief Always on flag.
    ///
    /// If true, driver should be "always on", i.e., player will
    /// "subscribe" at startup, before any clients subscribe. The
    /// "alwayson" parameter in the config file can be used to turn
    /// this feature on as well (in which case this flag will be set
    /// to reflect that setting).
    bool alwayson;

    /// @brief Queue for all incoming messages for this driver
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

    /// @brief Get last error value.  Call this after the constructor to
    ///        check whether anything went wrong.
    int GetError() { return(this->error); }
    
    /// @brief Subscribe to this driver.
    ///
    /// The Subscribe() and Unsubscribe() methods are used to control
    /// subscriptions to the driver; a driver MAY override them, but
    /// usually won't.  
    ///
    /// @param addr Address of the device to subscribe to (the driver may
    /// have more than one interface).
    /// @returns Returns 0 on success.
    virtual int Subscribe(player_devaddr_t addr);

    /// @brief Unsubscribe from this driver.
    ///
    /// The Subscribe() and Unsubscribe() methods are used to control
    /// subscriptions to the driver; a driver MAY override them, but
    /// usually won't.  
    ///
    /// @param addr Address of the device to unsubscribe from (the driver may
    /// have more than one interface).
    /// @returns Returns 0 on success.
    virtual int Unsubscribe(player_devaddr_t addr);

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

    /// @brief Process pending messages.
    ///
    /// Call this to automatically process messages using registered handler,
    /// Driver::ProcessMessage.
    /// Processes messages until no messages remaining in the queue or
    /// a message with no handler is reached
    void ProcessMessages();
	
    /// @brief Message handler.
    ///
    /// This function is called once for each message in the incoming queue.
    /// Reimplement it to provide message handling.
    /// Return 0 for no response, otherwise with player_msgtype if response 
    /// needed.  If a response is needed, storage to hold it will be
    /// allocated in this method and @p resp_data will point to it.  The
    /// caller is responsible for freeing this storage.
    /// If calling this method from outside the driver lock the driver mutex 
    /// first.
    /// When writing the driver make sure to protect accesses that could 
    /// clash in the main thread with mutex locks.
    /// @param resp_queue The queue to which any response should go.
    /// @param hdr The message header
    /// @param data The message body
    /// @param resp_data Place to put a response
    /// @param resp_len Place to put length of response
    virtual int ProcessMessage(MessageQueue* resp_queue, player_msghdr * hdr, 
                               uint8_t * data, uint8_t** resp_data,
                               size_t * resp_len) 
    {*resp_len = 0; *resp_data = NULL; return -1;};

    /// @brief Update non-threaded drivers.
    virtual void Update() 
    {
      if(!this->driverthread)
        this->ProcessMessages();
    }
};


#endif
