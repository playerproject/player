/*
  $Id$
*/

#ifndef PLAYERCLIENT_H
#define PLAYERCLIENT_H

#include "libplayerc++/playerc++config.h"

#include <string>
#include <list>

#ifdef HAVE_BOOST_SIGNALS
  #include <boost/signal.hpp>
  #include <boost/bind.hpp>
#endif

#ifdef HAVE_BOOST_THREAD
  #include <boost/thread/mutex.hpp>
  #include <boost/thread/thread.hpp>
  #include <boost/thread/xtime.hpp>
#else
  // we have to define this so we don't have to
  // comment out all the instances of scoped_lock
  // in all the proxies
  namespace boost
  {
    class thread
    {
      public:
        thread() {};
    };

    class mutex
    {
      public:
        mutex() {};
        class scoped_lock
        {
          public: scoped_lock(mutex m) {};
        };
    };
  }

#endif

namespace PlayerCc
{
/** @addtogroup player_clientlib_cplusplus libplayerc++

 @{

 */

/** @addtogroup player_clientlib_cplusplus_core Core functionality

 @{

 */

/** The default port number for PlayerClient */
const int PLAYER_PORTNUM(6665);
/** The default hostname for PlayerClient */
const std::string PLAYER_HOSTNAME("localhost");

class ClientProxy;

/** @brief The PlayerClient is used for communicating with the player server
 *
 * One PlayerClient object is used to control each connection to
 * a Player server.  Contained within this object are methods for changing the
 * connection parameters and obtaining access to devices.
 *
 * Since the threading functionality of the PlayerClient is built on Boost,
 * these options are conditionally available based on the Boost threading
 * library being present on the system.  The StartThread() and StopThread() are
 * the only functions conditionally available based on this.
*/
class PlayerClient
{
  friend class ClientProxy;

  // our thread type
  typedef boost::thread thread_t;

  // our mutex type
  typedef boost::mutex mutex_t;

  private:
    // list of proxies associated with us
    std::list<PlayerCc::ClientProxy*> mProxyList;

    std::list<playerc_device_info_t> mDeviceList;

    // Connect to the indicated host and port.
    // @exception throws PlayerError if unsuccessfull
    void Connect(const std::string aHostname, uint aPort);

    // Disconnect from server.
    void Disconnect();

    //  our c-client from playerc
    playerc_client_t* mClient;

    // The hostname of the server, stored for convenience
    std::string mHostname;

    // The port number of the server, stored for convenience
    uint mPort;

    // Is the thread currently stopped or stopping?
    bool mIsStop;

    // This is the thread where we run @ref Run()
    thread_t* mThread;

    // A helper function for starting the thread
    void RunThread();

  public:

    /// Make a client and connect it as indicated.
    PlayerClient(const std::string aHostname=PLAYER_HOSTNAME,
                 uint aPort=PLAYER_PORTNUM);

    /// destructor
    ~PlayerClient();

    /// A mutex for handling synchronization
    mutex_t mMutex;

    // ideally, we'd use the read_write mutex, but I was having some problems
    // (with their code) because it's under development
    //boost::read_write_mutex mMutex;

    /// Start the run thread
    void StartThread();

    /// Stop the run thread
    void StopThread();

    /// This starts a blocking loop on @ref Read()
    void Run(uint aTimeout=10); // aTimeout in ms

    /// Stops the @ref Run() loop
    void Stop();

    /// Check whether there is data waiting on the connection, blocking
    /// for up to @p timeout milliseconds (set to 0 to not block).
    ///
    /// @returns
    /// - false if there is no data waiting
    /// - true if there is data waiting
    bool Peek(uint timeout=0);

    /// A blocking Read
    ///
    /// Use this method to read data from the server, blocking until at
    /// least one message is received.  Use @ref PlayerClient::Peek() to check
    /// whether any data is currently waiting.
    /// In pull mode, this will block until all data waiting on the server has
    /// been received, ensuring as up to date data as possible.
    void Read();

    /// A nonblocking Read
    ///
    /// Use this method if you want to read in a nonblocking manner.  This
    /// is the equivalent of checking if Peek is true and then reading
    void ReadIfWaiting();

    /// You can change the rate at which your client receives data from the
    /// server with this method.  The value of @p freq is interpreted as Hz;
    /// this will be the new rate at which your client receives data (when in
    /// continuous mode).
    ///
    /// @exception throws PlayerError if unsuccessfull
//     void SetFrequency(uint aFreq);

    /// You can toggle the mode in which the server sends data to your
    /// client with this method.  The @p mode should be one of
    ///   - @ref PLAYER_DATAMODE_PUSH (all data)
    ///   - @ref PLAYER_DATAMODE_PULL (data on demand)
    /// When in pull mode, it is highly recommended that a replace rule is set
    /// for data packets to prevent the server message queue becoming flooded.
    ///
    /// @exception throws PlayerError if unsuccessfull
    void SetDataMode(uint aMode);

    /// Set a replace rule for the clients queue on the server.
	/// If a rule with the same pattern already exists, it will be replaced with
	/// the new rule (i.e., its setting to replace will be updated).
    ///
	/// @param aInterf Interface to set replace rule for (-1 for wildcard)
	///
	/// @param aIndex index to set replace rule for (-1 for wildcard)
	///
	/// @param aType type to set replace rule for (-1 for wildcard),
	/// i.e. PLAYER_MSGTYPE_DATA
	///
	/// @param aSubtype message subtype to set replace rule for (-1 for wildcard)
	///
	/// @param aReplace Should we replace these messages
	///
	/// @returns Returns 0 on success, non-zero otherwise.  Use
    ///
    /// @exception throws PlayerError if unsuccessfull
    void SetReplaceRule(int aInterf, int aIndex, int aType, int aSubtype, int aReplace);

    /// Get the list of available device ids. The data is written into the
    /// proxy structure rather than retured to the caller.
    void RequestDeviceList();

    std::list<playerc_device_info_t> GetDeviceList();

    /// Returns the hostname
    std::string GetHostname() const { return(mHostname); };

    /// Returns the port
    uint GetPort() const { return(mPort); };

    /// Get the interface code for a given name
    int LookupCode(std::string aName) const;

    /// Get the name for a given interface code
    std::string LookupName(int aCode) const;
};

/** }@ (core) */
/** }@ (c++) */

}

namespace std
{
  std::ostream& operator << (std::ostream& os, const PlayerCc::PlayerClient& c);
}

#endif

