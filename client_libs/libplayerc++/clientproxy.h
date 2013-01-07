/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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

/***************************************************************************
 * Desc: Player C++ client
 * Authors: Brad Kratochvil, Toby Collett
 *
 * Date: 23 Sep 2005
 # CVS: $Id$
 **************************************************************************/


#ifndef PLAYERCC_CLIENTPROXY_H
#define PLAYERCC_CLIENTPROXY_H

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERCC_EXPORT
  #elif defined (playerc___EXPORTS)
    #define PLAYERCC_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERCC_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERCC_EXPORT
#endif

namespace PlayerCc
{

/** @brief The client proxy base class
 *
 * Base class for all proxy devices. Access to a device is provided by a
 * device-specific proxy class.  These classes all inherit from the @p
 * ClientProxy class which defines an interface for device proxies.  As such,
 * a few methods are common to all devices and we explain them here.
 *
 * Since the ConnectReadSignal() and DisconnectReadSignal() member functions
 * are based on the Boost signals library, they are conditionally available
 * depending on Boost's presence in the system.  See the @em configure script
 * for more information.
*/
class PLAYERCC_EXPORT ClientProxy
{
  friend class PlayerClient;

  public:

#ifdef HAVE_BOOST_SIGNALS
    /// A connection type.  This is useful when attaching signals to the
    /// ClientProxy because it allows for detatching the signals.
    typedef boost::signals::connection connection_t;

    /// A scoped lock
    typedef boost::mutex::scoped_lock scoped_lock_t;

    /// the function pointer type for read signals signal
    typedef boost::signal<void (void)> read_signal_t;
#else
    // if we're not using boost, just define them.
    typedef int connection_t;
    // we redefine boost::mustex::scoped_lock in playerclient.h
    typedef boost::mutex::scoped_lock scoped_lock_t;
    // if we're not using boost, just define them.
    typedef int read_signal_t;
#endif

  protected:

    // The ClientProxy constructor
    // @attention Potected, so it can only be instantiated by other clients
    //
    // @throw PlayerError Throws a PlayerError if unable to connect to the client
    ClientProxy(PlayerClient* aPc, uint32_t aIndex);

    // destructor will try to close access to the device
    virtual ~ClientProxy();

    // Subscribe to the proxy
    // This needs to be defined for every proxy.
    // @arg aIndex the index of the devce we want to connect to

    // I wish these could be pure virtual,
    // but they're used in the constructor/destructor
    virtual void Subscribe(uint32_t /*aIndex*/) {};

    // Unsubscribe from the proxy
    // This needs to be defined for every proxy.
    virtual void Unsubscribe() {};

    // The controlling client object.
    PlayerClient* mPc;

    // A reference to the C client
    playerc_client_t* mClient;

    // contains convenience information about the device
    playerc_device_t *mInfo;

    // if set to true, the current data is "fresh"
    bool mFresh;

    // @brief Get a variable from the client
    // All Get functions need to use this when accessing data from the
    // c library to make sure the data access is thread safe.
    template<typename T>
    T GetVar(const T &aV) const
    { // these have to be defined here since they're templates
      scoped_lock_t lock(mPc->mMutex);
      T v = aV;
      return v;
    }

    // @brief Get a variable from the client by reference
    // All Get functions need to use this when accessing data from the
    // c library to make sure the data access is thread safe.  In this
    // case, a begin, end, and destination pointer must be given (similar
    // to C++ copy).  It is up to the user to ensure there is memory
    // allocated at aDest.
    template<typename T>
    void GetVarByRef(const T aBegin, const T aEnd, T aDest) const
    { // these have to be defined here since they're templates
      scoped_lock_t lock(mPc->mMutex);
      std::copy(aBegin, aEnd, aDest);
    }

  private:

    // The last time that data was read by this client in [s].
    double mLastTime;

    // A boost::signal which is used for our callbacks.
    // The signal will normally be of a type such as:
    // - boost::signal<void ()>
    // - boost::signal<void (T)>
    // where T can be any type.
    //
    // @attention we currently only use signals that return void because we
    // don't have checks to make sure a signal is registered.  If an empty
    // signal is called:
    //
    // @attention "Calling the function call operator may invoke undefined
    // behavior if no slots are connected to the signal, depending on the
    // combiner used. The default combiner is well-defined for zero slots when
    // the return type is void but is undefined when the return type is any
    // other type (because there is no way to synthesize a return value)."
    //
    read_signal_t mReadSignal;

    // Outputs the signal if there is new data
    void ReadSignal();

  public:

    /// @brief Proxy has any information
    ///
    /// This function can be used to see if any data has been received
    /// from the driver since the ClientProxy was created.
    ///
    /// @return true if we have received any data from the device.
    bool IsValid() const { return 0!=GetVar(mInfo->datatime); };

    /// @brief Check for fresh data.
    ///
    /// Fresh is set to true on each new read.  It is up to the user to
    /// set it to false if the data has already been read, by using @ref NotFresh()
    /// This is most useful when used in conjunction with the PlayerMultiClient
    ///
    /// @return True if new data was read since the Fresh flag was last set false
    bool IsFresh() const { return GetVar(mFresh); };

    /// @brief Reset Fresh flag
    ///
    /// This sets the client's "Fresh" flag to false.  After this is called, 
    /// @ref IsFresh() will return false until new information is available
    /// after a call to a @ref Read() method.
    void NotFresh();

    /// @brief Get the underlying driver's name
    ///
    /// Get the name of the driver that the ClientProxy is connected to.
    ///
    /// @return Driver name
    /// @todo GetDriverName isn't guarded by locks yet
    std::string GetDriverName() const { return mInfo->drivername; };

    /// Returns the received timestamp of the last data sample [s]
    double GetDataTime() const { return GetVar(mInfo->datatime); };

    /// Returns the time between the current data time and the time of
    /// the last data sample [s]
    double GetElapsedTime() const
      { return GetVar(mInfo->datatime) - GetVar(mInfo->lasttime); };

    /// @brief Get a pointer to the Player Client
    ///
    /// Returns a pointer to the PlayerClient object that this client proxy
    /// is connected through.
    PlayerClient * GetPlayerClient() const { return mPc;}
    
    /// @brief Get device index
    ///
    /// Returns the device index of the interface the ClientProxy object
    /// is connected to.
    ///
    /// @return interface's device index
    uint32_t GetIndex() const { return GetVar(mInfo->addr.index); };

    /// @brief Get Interface Code
    ///
    /// Get the interface code of the underlying proxy.  See @ref message_codes for
    /// a list of supported interface codes.
    /// 
    /// @return Interface code
    uint32_t GetInterface() const { return GetVar(mInfo->addr.interf); };

    /// @brief Get Interface Name
    ///
    /// Get the interface type of the proxy as a string.
    ///
    /// @return Interface name
    std::string GetInterfaceStr() const
      { return interf_to_str(GetVar(mInfo->addr.interf)); };

    /// @brief Set a replace rule for this proxy on the server.
    ///
    /// If a rule with the same pattern already exists, it will be replaced
    /// with the new rule (i.e., its setting to replace will be updated).
    ///
    /// @param aReplace Should we replace these messages
    /// @param aType The type to set replace rule for (-1 for wildcard),
    ///        see @ref message_types.
    /// @param aSubtype Message subtype to set replace rule for (-1 for
    ///        wildcard).  This is dependent on the @ref interfaces.
    ///
    /// @exception throws PlayerError if unsuccessfull
    ///
    /// @see PlayerClient::SetReplaceRule, PlayerClient::SetDataMode
    void SetReplaceRule(bool aReplace,
                        int aType = -1,
                        int aSubtype = -1);

    /// @brief Request capabilities of device.
    ///
    /// Send a message asking if the device supports the given message
    /// type and subtype. If it does, the return value will be 1, and 0 otherwise.
    ///
    /// @param aType The capability type
    /// @param aSubtype The capability subtype
    /// @return 1 if capability is supported, 0 otherwise.
    int HasCapability(uint32_t aType, uint32_t aSubtype);

    /// @brief Request a boolean property
    ///
    /// Request a boolean property from the driver.  If the has the property requested,
    /// the current value of the property will be returned.
    ///
    /// @param[in] aProperty String containing the desired property name
    /// @param[out] aValue Value of the requested property, if available
    /// @return 0 If property request was successful, nonzero otherwise
    int GetBoolProp(char *aProperty, bool *aValue);

    /// @brief Set a boolean property
    ///
    /// Set a boolean property to a given value in the driver.  If the property exists and
    /// can be set, it will be set to the new value.
    ///
    /// @param[in] aProperty String containing the property name
    /// @param aValue The value to set the property to
    /// @return 0 if property change was successful, nonzero otherwise
    int SetBoolProp(char *aProperty, bool aValue);

    /// @brief Request an integer property
    ///
    /// Request an integer property from the driver.  If the has the property requested,
    /// the current value of the property will be returned.
    ///
    /// @param[in] aProperty String containing the desired property name
    /// @param[out] aValue Value of the requested property, if available
    /// @return 0 If property request was successful, nonzero otherwise.                   
    int GetIntProp(char *aProperty, int32_t *aValue);

    /// @brief Set an integer property
    ///
    /// Set an integer property to a given value in the driver.  If the property exists and
    /// can be set, it will be set to the new value.
    ///
    /// @param[in] aProperty String containing the property name
    /// @param aValue The value to set the property to
    /// @return 0 if property change was successful, nonzero otherwise
    int SetIntProp(char *aProperty, int32_t aValue);

    /// @brief Request a double property
    ///
    /// Request a double property from the driver.  If the has the property requested,
    /// the current value of the property will be returned.
    ///
    /// @param[in] aProperty String containing the desired property name
    /// @param[out] aValue Value of the requested property, if available
    /// @return 0 If property request was successful, nonzero otherwise.
    int GetDblProp(char *aProperty, double *aValue);

    /// @brief Set a double property
    ///
    /// Set an integer property to a given value in the driver.  If the property exists and
    /// can be set, it will be set to the new value.
    ///
    /// @param[in] aProperty String containing the property name
    /// @param aValue The value to set the property to
    /// @return 0 if property change was successful, nonzero otherwise
    int SetDblProp(char *aProperty, double aValue);

    /// @brief Request a string property
    ///
    /// Request a double property from the driver.  If the has the property requested,
    /// the current value of the property will be returned.
    ///
    /// @param[in] aProperty String containing the desired property name
    /// @param[out] aValue Value of the requested property, if available
    /// @return 0 If property request was successful, nonzero otherwise.
    int GetStrProp(char *aProperty, char **aValue);

    /// @brief Set a string property
    ///
    /// Set a string property to a given value in the driver.  If the property exists and
    /// can be set, it will be set to the new value.
    ///
    /// @param[in] aProperty String containing the property name
    /// @param aValue The value to set the property to
    /// @return 0 if property change was successful, nonzero otherwise
    int SetStrProp(char *aProperty, char *aValue);

    /// @brief Connect a read signal to this proxy
    ///
    /// Connects a signal to the proxy to trigger.  This functionality depends on
    /// boost::signals, and will fail if Player was not compiled against the boost
    /// signal library.
    /// For more information check out @ref player_clientlib_multi
    template<typename T>
    connection_t ConnectReadSignal(T aSubscriber)
      {
#ifdef HAVE_BOOST_SIGNALS
        scoped_lock_t lock(mPc->mMutex);
        return mReadSignal.connect(aSubscriber);
#else
        return -1;
#endif
      }

    /// @brief Disconnect a signal from this proxy
    ///
    /// Disconnects a connected read signal from the proxy
    /// For more information check out @ref player_clientlib_multi
    void DisconnectReadSignal(connection_t aSubscriber)
      {
#ifdef HAVE_BOOST_SIGNALS
        scoped_lock_t lock(mPc->mMutex);
        aSubscriber.disconnect();
#else
       // This line is here to prevent compiler warnings of "unused varaibles"
       aSubscriber = aSubscriber;
#endif
      }

};

}// namespace

#endif
