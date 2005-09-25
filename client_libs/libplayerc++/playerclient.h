/*
  $Id$
*/

#ifndef PLAYERCLIENT_H
#define PLAYERCLIENT_H

#include <string>
#include <list>

#include <stdint.h>

#include <boost/signal.hpp>
#include <boost/bind.hpp>

/** @defgroup player_clientlib_cplusplus libplayerc++ */
/** @{ */

/** @addtogroup player_clientlib_cplusplus_core Core functionality */
/** @{ */

/**/
namespace PlayerCc
{
/**/
const int PLAYER_PORTNUM(6665);
/**/
const std::string PLAYER_HOSTNAME("localhost");

class ClientProxy;

/**
One @p PlayerClient object is used to control each connection to
a Player server.  Contained within this object are methods for changing the
connection parameters and obtaining access to devices, which we explain
next.
*/
class PlayerClient
{
  friend class ClientProxy;

  protected:
    /** list of proxies associated with us */
    std::list<PlayerCc::ClientProxy*> mProxyList;

    /** Connect to the indicated host and port.
     * @exception throws PlayerError if unsuccessfull
     */
    void Connect(const std::string aHostname, uint aPort);

    /** Disconnect from server.
      */
    void Disconnect();

    /**  our c-client from playerc */
    playerc_client_t* mClient;

    std::string mHostname;

    uint mPort;

    void Lock();

    void Unlock();

    bool mIsStop;

  public:

    void Run(uint aTimeout=10);
    void Stop();

    /** Make a client and connect it as indicated. */
    PlayerClient(const std::string aHostname=PLAYER_HOSTNAME,
                 uint aPort=PLAYER_PORTNUM);

    /** destructor */
    ~PlayerClient();

    /**
     Check whether there is data waiting on the connection, blocking
     for up to @p timeout milliseconds (set to 0 to not block).

     Returns:
       - 0 if there is no data waiting
       - 1 if there is data waiting
    */
    int Peek(uint timeout=0);

    /**
     * Use this method to read data from the server, blocking until at
     * least one message is received.  Use @ref PlayerClient::Peek() to check
     * whether any data is currently waiting.
     */
    void Read();

    /**
      You can change the rate at which your client receives data from the
      server with this method.  The value of @p freq is interpreted as Hz;
      this will be the new rate at which your client receives data (when in
      continuous mode).
      @exception throws PlayerError if unsuccessfull
     */
    void SetFrequency(uint aFreq);

    /** You can toggle the mode in which the server sends data to your
        client with this method.  The @p mode should be one of
          - PLAYER_DATAMODE_PUSH_ALL (all data at fixed frequency)
          - PLAYER_DATAMODE_PULL_ALL (all data on demand)
          - PLAYER_DATAMODE_PUSH_NEW (only new new data at fixed freq)
          - PLAYER_DATAMODE_PULL_NEW (only new data on demand)
          - PLAYER_DATAMODE_PUSH_ASYNC (new data, as fast as it is produced)
        @exception throws PlayerError if unsuccessfull
      */
    void SetDataMode(uint aMode);

    /** Get the list of available device ids. The data is written into the
     * proxy structure rather than retured to the caller.
     */
    void RequestDeviceList();

    /** Returns the hostname */
    std::string GetHostname() const { return(mHostname); };

    /** Returns the port */
    uint GetPort() const { return(mPort); };
};

/** }@ */
/** }@ */

}

/* These need to be outside of the namespace */

std::ostream& operator << (std::ostream& os, const PlayerCc::PlayerClient& c);

#endif

