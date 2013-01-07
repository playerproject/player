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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
/********************************************************************
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 ********************************************************************/

/*
 * $Id$
 *
 *  The virtual class from which all driver classes inherit.  this
 *  defines the API that all drivers must implement.
 */

#ifndef _DRIVER_H
#define _DRIVER_H

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERCORE_EXPORT
  #elif defined (playercore_EXPORTS)
    #define PLAYERCORE_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERCORE_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERCORE_EXPORT
#endif

#include <pthread.h>

#include <libplayercommon/playercommon.h>
#include <libplayercore/message.h>
#include <libplayerinterface/player.h>
#include <libplayercore/property.h>

//using namespace std;

/**
@brief capabilities request handler macro

This macro tests the current message to see if its a capabilities
request and creates an ACK message if it is and matches the defined
capability.

    @param device_addr The device address for the capability
    @param queue The queue of the recieved message
    @param hdr The recieved header
    @param data The recieved data
    @param cap_type The supported capability type
    @param cap_subtype The supported capability subtype

*/
#define HANDLE_CAPABILITY_REQUEST(device_addr, queue, hdr, data, cap_type, cap_subtype) \
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ, device_addr)) \
  { \
    player_capabilities_req_t & cap_req = * reinterpret_cast<player_capabilities_req_t *> (data);\
    if (cap_req.type == cap_type && cap_req.subtype == cap_subtype) \
    { \
      Publish(device_addr, queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_CAPABILITIES_REQ); \
      return 0; \
    } \
  }


// Forward declarations
class ConfigFile;

/**
@brief Base class for all drivers.

This class manages driver subscriptions, and data
marshalling to/from device interfaces.  Non threaded drivers inherit directly from
this class, and most will overload the Setup(), Shutdown() methods.
*/
class PLAYERCORE_EXPORT Driver
{
  private:
    /* @brief Last error value; useful for returning error codes from
    constructors. */
    int error;

    PropertyBag propertyBag;

    /** @brief Number of subscriptions to this driver. */
	int subscriptions;
  public:
    bool HasSubscriptions();

  protected:

    /** @brief Add an interface.

    @param addr Player device address.

    @returns 0 on success, non-zero otherwise. */
    int AddInterface(player_devaddr_t addr);

    /** @brief Add an interface.

    This form loads the address details from the config file and then adds the interface.

    @param addr Pointer to Player device address, this is filled in with the address details.
    @param cf Current configuration file
    @param section Current section in configuration file
    @param code Interface code
    @param key Configuration file key value
    @returns 0 on success, non-zero otherwise. */
    int AddInterface(player_devaddr_t *addr, ConfigFile * cf, int section, int code, const char * key = NULL);


    /** @brief Set/reset error code */
    void SetError(int code) {this->error = code;}

    /** @brief Wait for new data to arrive on the driver's queue.

    Call this method to block until a new message arrives on
    Driver::InQueue.  This method will return true immediately if at least
    one message is already waiting.

    If TimeOut is set to a positive value this method will return false if the
    timeout occurs before and update is recieved.
    */
    virtual bool Wait(double TimeOut=0.0);

    /** @brief Wake up the driver if the specified event occurs on the file descriptor */
    int AddFileWatch(int fd, bool ReadWatch = true, bool WriteWatch = false, bool ExceptWatch = true);

    /** @brief Remove a previously added watch, call with the same arguments as when adding the watch */
    int RemoveFileWatch(int fd, bool ReadWatch = true, bool WriteWatch = false, bool ExceptWatch = true);


  private:
    /** @brief Mutex used to lock access, via Lock() and Unlock(), to
    driver internals, like the list of subscribed queues. */
    pthread_mutex_t accessMutex;
    /** @brief Mutex used to protect the subscription count for the driver. */
    pthread_mutex_t subscriptionMutex;
  protected:
    /** @brief Lock access between the server and driver threads. In particular used
     * to procect the drivers thread pointer */
     virtual void Lock(void);
    /** @brief Unlock access to driver internals. */
    virtual void Unlock(void);

    /** @brief Lock to protect the subscription count for the driver */
     virtual void SubscriptionLock(void);
    /** @brief Unlock to protect the subscription count for the driver. */
    virtual void SubscriptionUnlock(void);

    /** enable thread cancellation and test for cancellation
     *
     * This should only ever be called from the driver thread with *no* locks held*/
    virtual void TestCancel() {};


  public:
    /** @brief Last requester's queue.

    Pointer to a queue to which this driver owes a reply.  Used mainly
    by non-threaded drivers to cache the return address for requests
    that get forwarded to other devices. */
    QueuePointer ret_queue;

    /** @brief Publish a message via one of this driver's interfaces.

    This form of Publish will assemble the message header for you.

    @param addr The origin address
    @param queue If non-NULL, the target queue; if NULL,
    then the message is sent to all interested parties.
    @param type The message type
    @param subtype The message subtype
    @param src The message body
    @param deprecated Used to be the length of the message this is now calculated
    @param timestamp Timestamp for the message body (if NULL, then the
    current time will be filled in)
    @param copy if set to false the data will be claimed and the caller should no longer use or free it */
    virtual void Publish(player_devaddr_t addr,
                 QueuePointer &queue,
                 uint8_t type,
                 uint8_t subtype,
                 void* src=NULL,
                 size_t deprecated=0,
                 double* timestamp=NULL,
                 bool copy=true);

     /** @brief Publish a message via one of this driver's interfaces.

     This form of Publish will assemble the message header for you.
     The message is broadcast to all interested parties

     @param addr The origin address
     @param type The message type
     @param subtype The message subtype
     @param src The message body
     @param deprecated Used to be the length of the message this is now calculated
     @param timestamp Timestamp for the message body (if NULL, then the
     current time will be filled in)
     @param copy if set to false the data will be claimed and the caller should no longer use or free it */
     virtual void Publish(player_devaddr_t addr,
                  uint8_t type,
                  uint8_t subtype,
                  void* src=NULL,
                  size_t deprecated=0,
                  double* timestamp=NULL,
                  bool copy=true);



    /** @brief Publish a message via one of this driver's interfaces.

    Use this form of Publish if you already have the message header
    assembled and have a target queue to send to.
    @param queue the target queue.
    @param hdr The message header
    @param src The message body
    @param copy if set to false the data will be claimed and the caller should no longer use or free it */
    virtual void Publish(QueuePointer &queue,
                 player_msghdr_t* hdr,
                 void* src,
                 bool copy = true);

    /** @brief Publish a message via one of this driver's interfaces.

    Use this form of Publish if you already have the message header
    assembled and wish to broadcast the message to all subscribed parties.
    @param hdr The message header
    @param src The message body
    @param copy if set to false the data will be claimed and the caller should no longer use or free it */
    virtual void Publish(player_msghdr_t* hdr,
                 void* src,
                 bool copy = true);


    /** @brief Default device address (single-interface drivers) */
    player_devaddr_t device_addr;

    /** @brief Total number of entries in the device table using this driver.
    This is updated and read by the Device class. */
    int entries;

    /** @brief Always on flag.

    If true, driver should be "always on", i.e., player will
    "subscribe" at startup, before any clients subscribe. The
    "alwayson" parameter in the config file can be used to turn
    this feature on as well (in which case this flag will be set
    to reflect that setting). */
    bool alwayson;

    /** @brief Queue for all incoming messages for this driver */
    QueuePointer InQueue;

    /** @brief Constructor for single-interface drivers.

    @param cf Current configuration file
    @param section Current section in configuration file
    @param overwrite_cmds Do new commands overwrite old ones?
    @param queue_maxlen How long can the incoming queue grow?
    @param interf Player interface code; e.g., PLAYER_POSITION2D_CODE */
    Driver(ConfigFile *cf,
           int section,
           bool overwrite_cmds,
           size_t queue_maxlen,
           int interf);

    /** @brief Constructor for multiple-interface drivers.

    Use AddInterface() to specify individual interfaces.
    @param cf Current configuration file
    @param section Current section in configuration file
    @param overwrite_cmds Do new commands overwrite old ones?
    @param queue_maxlen How long can the incoming queue grow? */
    Driver(ConfigFile *cf,
           int section,
           bool overwrite_cmds = true,
           size_t queue_maxlen = PLAYER_MSGQUEUE_DEFAULT_MAXLEN);

    /** @brief Destructor */
    virtual ~Driver();

    /** @brief Get last error value.  Call this after the constructor to
    check whether anything went wrong. */
    int GetError() { return(this->error); }

    /** @brief Subscribe to this driver.

    The Subscribe() and Unsubscribe() methods are used to control
    subscriptions to the driver; a driver MAY override them, but
    usually won't.

    @param addr Address of the device to subscribe to (the driver may
    have more than one interface).
    @returns Returns 0 on success. */
    virtual int Subscribe(player_devaddr_t addr);

    /** @brief Subscribe to this driver.

    The Subscribe() and Unsubscribe() methods are used to control
    subscriptions to the driver; a driver MAY override them, but
    usually won't. This alternative form includes the clients queue
    so you can map future requests and unsubscriptions to a specific queue.

    If this methods returns a value other than 1 then the other form of subscribe wont be called

    @param queue The queue of the subscribing client
    @param addr Address of the device to subscribe to (the driver may
    have more than one interface).
    @returns Returns 0 on success, -ve on error and 1 for unimplemented. */
    virtual int Subscribe(QueuePointer &/*queue*/, player_devaddr_t /*addr*/) {return 1;};

    /** @brief Unsubscribe from this driver.

    The Subscribe() and Unsubscribe() methods are used to control
    subscriptions to the driver; a driver MAY override them, but
    usually won't.

    @param addr Address of the device to unsubscribe from (the driver may
    have more than one interface).
    @returns Returns 0 on success. */
    virtual int Unsubscribe(player_devaddr_t addr);

    /** @brief Unsubscribe from this driver.

    The Subscribe() and Unsubscribe() methods are used to control
    subscriptions to the driver; a driver MAY override them, but
    usually won't.This alternative form includes the clients queue
    so you can map future requests and unsubscriptions to a specific queue.

    If this methods returns a value other than 1 then the other form of subscribe wont be called

    @param queue The queue of the subscribing client
    @param addr Address of the device to unsubscribe from (the driver may
    have more than one interface).
    @returns Returns 0 on success. */
    virtual int Unsubscribe(QueuePointer &/*queue*/, player_devaddr_t /*addr*/) {return 1;};

    /** @brief Terminate the driver.

    This method doesnt go through the niceities of unsubscribing etc, only use
    when the server is actually shutting down as it takes a few shortcuts

    @returns Returns 0 on success. */
    virtual int Terminate();


    /** @brief Initialize the driver.

    This function is called with the first client subscribes; it MUST
    be implemented by the driver.

    @returns Returns 0 on success. */
    virtual int Setup() {return 0;};

    /** @brief Finalize the driver.

    This function is called with the last client unsubscribes.

    @returns Returns 0 on success. */
    virtual int Shutdown() {return 0;};

    /** @brief Process pending messages.

    Call this to automatically process messages using registered handler,
    Driver::ProcessMessage.

    @param maxmsgs The maximum number of messages to process.  If -1, then process until the queue is empty (this may result in an infinite loop if messages are being added to the queue faster than they are processed).  If 0, then check the current length and process up to that many messages.  If > 0, process up to the indicated number of messages.
    */
    void ProcessMessages(int maxmsgs);

    /** @brief Process pending messages.

      Equivalent to ProcessMessages(0).
      */
    void ProcessMessages(void);

    /** @brief Message handler.

    This function is called once for each message in the incoming queue.
    Reimplement it to provide message handling.
    Return 0 if you handled the message and -1 otherwise

    @param resp_queue The queue to which any response should go.
    @param hdr The message header
    @param data The message body */
    virtual int ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr,
                               void * data);

    /** @brief Update non-threaded drivers. */
    virtual void Update()
    {
        this->ProcessMessages();
    }

    /** @brief Internal message handler.

    This function handles messages internal to the driver's operation.

    @param resp_queue The queue to which any response should go.
    @param hdr The message header
    @param data The message body */
    virtual int ProcessInternalMessages(QueuePointer& resp_queue,
                                        player_msghdr * hdr,
                                        void * data);

    /** @brief Property registration.

    @param key Property key
    @param property Pointer to a Property object - must exist for as long as the property bag does
    @param cf Configuration file
    @param section Configuration file section that may define the property value
    @return True if the property was registered, false otherwise */
    virtual bool RegisterProperty(const char *key, 
                                  Property *property, 
                                  ConfigFile* cf, 
                                  int section);

    /** @brief Property registration.

    Simplified form that uses the key already in the property
    @param property Pointer to a Property object - must exist for as long as the property bag does
    @param cf Configuration file
    @param section Configuration file section that may define the property value
    @return True if the property was registered, false otherwise */
    virtual bool RegisterProperty(Property *property, 
                                  ConfigFile* cf, 
                                  int section);
};

typedef enum player_thread_state
{
	PLAYER_THREAD_STATE_STOPPED,
	PLAYER_THREAD_STATE_RUNNING,
	PLAYER_THREAD_STATE_STOPPING,
	PLAYER_THREAD_STATE_RESTARTING
} player_thread_state_t;

class PLAYERCORE_EXPORT PlayerBarrier
{
public:
	PlayerBarrier()
	{
		pthread_mutex_init(&barrierMutex,NULL);
		pthread_cond_init(&barrierCond,NULL);
		barrierValue = 0;
	}
	~PlayerBarrier()
	{
		pthread_mutex_destroy(&barrierMutex);
		pthread_cond_destroy(&barrierCond);
	};

	int SetValue(int Value)
	{
		return barrierValue = Value;
	};

	int Wait()
	{
		pthread_mutex_lock(&barrierMutex);
		assert(barrierValue);
		if (--barrierValue)
			pthread_cond_wait(&barrierCond,&barrierMutex);
		else
			pthread_cond_broadcast(&barrierCond);
		pthread_mutex_unlock(&barrierMutex);
		return 0;
	};
private:
    /** @brief barrier to make sure StartThread doesnt return until
    * cleanup handlers etc have been installed.*/
    pthread_mutex_t barrierMutex;

    int barrierValue;

    pthread_cond_t barrierCond;

};



/**
@brief Base class for drivers which oeprate with a thread.

This class manages driver subscriptions, threads, and data
marshalling to/from device interfaces.  All drivers inherit from
this class, and most will overload the MainSetup(), MainQuit() and
Main() methods.

The ThreadedDriver uses a fairly simple thread model with the only complications
coming from the deferred cancellation.

The default setup method simply calls StartThread and likewise the default Shutdown method calls
StopThread. Resources for the driver should be allocated in MainSetup and cleaned up in MainQuit, these
two methods will be called in the driver thread before and after the main method respectively.

When StopThread is called it will request the driver thread to be cancelled, this cancellation will be
deffered until one of three cancellation points, ProcessMessages, Wait or TestCancel. In this way the driver
has control over when the thread is cancelled.

If a new subscription arrives before the thread has terminated then upon completion of the MainQuit method the thread
will be relaunched and MainSetup run. In this way it is gaurunteed that there is only ever one thread running for
the driver and that Main is always proceeded by MainSetup and followed by MainQuit before any new threads are launched.

To manage the thread lifetime a small state machine is implemented, containing four states: STOPPED, RUNNING, STOPPING
and RESTARTING. Initially the thread is in STOPPED state.

From the STOPPED state the only valid transition is to RUNNING, this is triggered by a call to StartThread.

From the RUNNING state the driver can transition to STOPPING, triggered by a call to StopThread.

In the STOPPING state the driver can transition to STOPPED, triggered by MainQuit running, or to RESTARTING on a call to
StartThread.

From RESTARTING the driver can transition to RUNNING once MainQuit has run and the new thread is started, or to STOPPING if
StopThread is called before the original thread terminates.

*/
class PLAYERCORE_EXPORT ThreadedDriver : public Driver
{
  protected:

    /* @brief Start the driver thread

    This method is usually called from the overloaded Setup() method to
    create the driver thread.  This will call Main(). */
    virtual void StartThread(void);

    /** @brief Cancel (and wait for termination) of the driver thread

    This method is usually called from the overloaded Shutdown() method
    to terminate the driver thread. */
    virtual void StopThread(void);

    /** @brief Dummy main (just calls real main).  This is used to simplify
    thread creation. */
    static void* DummyMain(void *driver);

    /** @brief Dummy main cleanup (just calls real main cleanup).  This is
    used to simplify thread termination. */
    static void DummyMainQuit(void *driver);

  private:
    /** @brief The driver's thread.
    The driver's thread, when managed by StartThread() and
    StopThread(). */
    pthread_t driverthread;

    /** @brief TODO: insert state machine here
     */
    player_thread_state_t ThreadState;
    bool SetupSuccessful;

    /// Barrier to synchronise threads on setup
    PlayerBarrier SetupBarrier;

  protected:
    /** enable thread cancellation and test for cancellation
     *
     * This should only ever be called from the driver thread with *no* locks held*/
    void TestCancel();


  public:

	/** @brief Constructor with implicit interface
	 @param cf Current configuration file
	 @param section Current section in configuration file
	 @param overwrite_cmds Do new commands overwrite old ones?
	 @param queue_maxlen How long can the incoming queue grow?
	 @param interface_ The interface that you want this driver to provide*/
	 ThreadedDriver(ConfigFile *cf,
			int section,
			bool overwrite_cmds,
			size_t queue_maxlen,
			int interface_);

    /** @brief Constructor for multiple-interface drivers.

    Use AddInterface() to specify individual interfaces.
    @param cf Current configuration file
    @param section Current section in configuration file
    @param overwrite_cmds Do new commands overwrite old ones?
    @param queue_maxlen How long can the incoming queue grow? */
    ThreadedDriver(ConfigFile *cf,
           int section,
           bool overwrite_cmds = true,
           size_t queue_maxlen = PLAYER_MSGQUEUE_DEFAULT_MAXLEN);

    /** @brief Destructor */
    virtual ~ThreadedDriver();

    /** @brief Initialize the driver.

    This function is called with the first client subscribes; it MUST
    be implemented by the driver.

    @returns Returns 0 on success. */
    virtual int Setup();

    /** @brief Finalize the driver.

    This function is called with the last client unsubscribes; the default version simple stops the
    driver thread if one is present. In this case resources should be cleaned up in MainQuit.

    @returns Returns 0 on success. */
    virtual int Shutdown();

    /** @brief Terminate the driver.

    This method doesnt go through the niceities of unsubscribing etc, only use
    when the server is actually shutting down as it takes a few shortcuts

    @returns Returns 0 on success. */
    virtual int Terminate();

    /** @brief Main method for driver thread.

    drivers have their own thread of execution, created using
    StartThread(); this is the entry point for the driver thread,
    and must be overloaded by all threaded drivers. */
    virtual void Main(void) = 0;

    /** @brief Sets up the resources needed by the driver thread */
    virtual int MainSetup(void) {return 0;};

    /** @brief Cleanup method for driver thread (called when main exits)

    Overload this method and to do additional cleanup when the
    driver thread exits. */
    virtual void MainQuit(void) {};

    /** @brief Wait for new data to arrive on the driver's queue.

    Call this method to block until a new message arrives on
    Driver::InQueue.  This method will return true immediately if at least
    one message is already waiting.

    If TimeOut is set to a positive value this method will return false if the
    timeout occurs before and update is recieved.
    */
    bool Wait(double TimeOut=0.0);

    virtual void Update()
    {};

};


#endif
