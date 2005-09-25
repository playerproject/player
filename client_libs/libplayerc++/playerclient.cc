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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 *
 * The C++ client
 */

#include <cassert>
#include <cstdio>
#include <iostream>

#include <time.h>

#include "playerc++.h"

#define DEBUG_LEVEL LOW
#include "debug.h"

using namespace PlayerCc;

PlayerClient::PlayerClient(const std::string aHostname, uint aPort) :
  mClient(NULL),
  mHostname(aHostname),
  mPort(aPort)
{
  Connect(mHostname, mPort);
}

PlayerClient::~PlayerClient()
{
  Disconnect();
}

void PlayerClient::Connect(const std::string aHostname, uint aPort)
{
  assert("" != aHostname);
  assert(0  != aPort);

  mClient = playerc_client_create(NULL, aHostname.c_str(), aPort);
  if (0 != playerc_client_connect(mClient))
  {
    throw PlayerError("PlayerClient::Connect()", playerc_error_str());
  }
  EVAL(mClient);
}

void PlayerClient::Disconnect()
{
  // Should go through here and disconnect all the proxies associated
  // with us?
  // how can we do the loop with a for_each?
  //std::for_each(mProxyList.begin(), mProxyList.end(), boost::mem_fn(&ClientProxy::mSignal));
  /*
  std::list<ClientProxy*>::iterator it = mProxyList.begin();
  for(; it != mProxyList.end(); ++it)
  {
    ClientProxy* x = *it;
    // only emit a signal when the interface has received data
    x->Unsubscribe();
  }
  */

  if (NULL != mClient)
  {
    playerc_client_disconnect(mClient);
    playerc_client_destroy(mClient);
    mClient = NULL;
  }
}

// non-blocking
void PlayerClient::Run(uint aTimeout)
{
  timespec sleep = {0,aTimeout*1000000};
  mIsStop = false;
  PRINT("Starting Run");
  while (!mIsStop)
  {
    if (Peek())
    {
      Read();
    }
    nanosleep(&sleep, NULL);
  }
  PRINT("Finished");
}

void PlayerClient::Stop()
{
  mIsStop = true;
}

int PlayerClient::Peek(uint aTimeout)
{
  return(playerc_client_peek(mClient, aTimeout));
}

void PlayerClient::Read()
{
  assert(NULL!=mClient);
  PRINT("Read()");
  EVAL(mClient);
  // first read the data
  Lock();
  if (NULL==playerc_client_read(mClient))
  {
    Unlock();
    throw PlayerError("PlayerClient::Read()", playerc_error_str());
  }
  Unlock();

  // how can we do the loop with a for_each?
  //std::for_each(mProxyList.begin(), mProxyList.end(), boost::mem_fn(&ClientProxy::mReadSignal));
  std::list<PlayerCc::ClientProxy*>::iterator it = mProxyList.begin();
  for(; it != mProxyList.end(); ++it)
  {
    EVAL(reinterpret_cast<int>(*it));
    ClientProxy* x = *it;
    // only emit a signal when the interface has received data
    if (x->GetDataTime()>x->mLastTime)
    {
      x->mLastTime = x->GetDataTime();
      x->mReadSignal();
    }
  }
}

void PlayerClient::Lock()
{
  //std::cerr << "Lock()...";
  //mMyMutex.lock();
  //std::cerr << "OK" << std::endl;
}

void PlayerClient::Unlock()
{
  //std::cerr << "Unlock()...";
  //mMyMutex.unlock();
  //std::cerr << "OK" << std::endl;
}

// change continuous data rate (freq is in Hz)
void PlayerClient::SetFrequency(uint aFreq)
{
  std::cerr << "PlayerClient::SetFrequency() not implemented ";
  /*
  if (0!=playerc_client_datafreq(mClient, aFreq))
  {
    throw PlayerError("PlayerClient::SetFrequency()", playerc_error_str());
  }
  */
}

// change data delivery mode
// valid modes are given in include/messages.h
void PlayerClient::SetDataMode(uint aMode)
{
  std::cerr << "PlayerClient::SetDataMode() not implemented ";
  /*
  if (0!=playerc_client_datamode(mClient, aMode))
  {
    throw PlayerError("PlayerClient::SetDataMode()", playerc_error_str());
  }
  */
}

// authenticate
#if 0
void PlayerClient::Authenticate(const std::string* aKey)
{

  std::cerr << "PlayerClient::Authenticate() not implemented ";
/*
  if (0!=playerc_client_authenticate(mClient, aKey->c_str()))
  {
    throw PlayerError("PlayerClient::Authenticate()", playerc_error_str());
  }
*/

}

// get the pointer to the proxy for the given device and index
ClientProxy* PlayerClient::GetProxy(player_devaddr_t aAddr)
{
  std::cerr << "PlayerClient::GetProxy() not implemented ";
//  return *find(mProxyList.begin(), mProxyList.end(), aAddr);
  return NULL;
}



// Get the list of available device ids. The data is written into the
// proxy structure rather than retured to the caller.
void PlayerClient::GetDeviceList()
{
  if (0!=playerc_client_get_devlist(mClient))
  {
    throw PlayerError("PlayerClient::GetDeviceList()", playerc_error_str());
  }
}

#endif

std::ostream& operator << (std::ostream& os, const PlayerCc::PlayerClient& c)
{
  return os << c.GetHostname()
            << ": "
            << c.GetPort();
}
