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
#include "playerclient.h"

#include "debug.h"

using namespace PlayerCc;

PlayerClient::PlayerClient(const std::string aHostname, uint aPort) :
  mClient(NULL),
  mHostname(aHostname),
  mPort(aPort)
{
#ifdef HAVE_BOOST_THREAD
  mThread = NULL;
#endif
  Connect(mHostname, mPort);
}

PlayerClient::~PlayerClient()
{
#ifdef HAVE_BOOST_THREAD
  if (!mIsStop)
  {
    StopThread();
  }
#endif

  Disconnect();
}

void PlayerClient::Connect(const std::string aHostname, uint aPort)
{
  assert("" != aHostname);
  assert(0  != aPort);

  LOG("Connecting " << *this);

  mClient = playerc_client_create(NULL, aHostname.c_str(), aPort);
  if (0 != playerc_client_connect(mClient))
  {
    throw PlayerError("PlayerClient::Connect()", playerc_error_str());
  }
  EVAL(mClient);
}

void PlayerClient::Disconnect()
{
  LOG("Disconnecting " << *this);

  std::for_each(mProxyList.begin(),
                mProxyList.end(),
                std::mem_fun(&ClientProxy::Unsubscribe));

  if (NULL != mClient)
  {
    playerc_client_disconnect(mClient);
    playerc_client_destroy(mClient);
    mClient = NULL;
  }
}

void PlayerClient::StartThread()
{
#ifdef HAVE_BOOST_THREAD
  assert(NULL == mThread);
  mThread = new boost::thread(boost::bind(&PlayerClient::RunThread, this));
#else
  throw PlayerError("PlayerClient::StartThread","Thread support not included");
#endif
}

void PlayerClient::StopThread()
{
#ifdef HAVE_BOOST_THREAD
  Stop();
  assert(mThread);
  mThread->join();
  delete mThread;
  mThread = NULL;
  PRINT("joined");
#else
  throw PlayerError("PlayerClient::StopThread","Thread support not included");
#endif
}

// non-blocking
void PlayerClient::RunThread()
{
#ifdef HAVE_BOOST_THREAD
  mIsStop = false;
  PRINT("starting run");
  while (!mIsStop)
  {
    if (Peek())
    {
      Read();
    }
    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC);
    // we sleep for 0 seconds
    boost::thread::sleep(xt);
  }
#else
  throw PlayerError("PlayerClient::RunThread","Thread support not included");
#endif

}

// blocking
void PlayerClient::Run(uint aTimeout)
{
  timespec sleep = {0,aTimeout*1000000};
  mIsStop = false;
  PRINT("starting run");
  while (!mIsStop)
  {
    if (Peek())
    {
      Read();
    }
    nanosleep(&sleep, NULL);
  }
}

void PlayerClient::Stop()
{
  mIsStop = true;
}

bool PlayerClient::Peek(uint aTimeout)
{
  ClientProxy::scoped_lock_t lock(mMutex);
  //EVAL(playerc_client_peek(mClient, aTimeout));
  return playerc_client_peek(mClient, aTimeout);
}


void PlayerClient::ReadIfWaiting()
{
  if (Peek())
    Read();
}

void PlayerClient::Read()
{
  assert(NULL!=mClient);
  PRINT("read()");
  // first read the data
  {
    ClientProxy::scoped_lock_t lock(mMutex);
    if (NULL==playerc_client_read(mClient))
    {
      throw PlayerError("PlayerClient::Read()", playerc_error_str());
    }
  }

  std::for_each(mProxyList.begin(),
                mProxyList.end(),
                std::mem_fun(&ClientProxy::ReadSignal));
}

void PlayerClient::RequestDeviceList()
{
  ClientProxy::scoped_lock_t lock(mMutex);
  if (0!=playerc_client_get_devlist(mClient))
  {
    throw PlayerError("PlayerClient::RequestDeviceList()", playerc_error_str());
  }
}

std::list<playerc_device_info_t> PlayerClient::GetDeviceList()
{
  std::list<playerc_device_info_t> dev_list;
  for (int i=0; i < mClient->devinfo_count; ++i)
  {
    PRINT(mClient->devinfos[i]);
    dev_list.push_back(mClient->devinfos[i]);
  }

  return dev_list;
}

// change continuous data rate (freq is in Hz)
void PlayerClient::SetFrequency(uint aFreq)
{
  std::cerr << "PlayerClient::SetFrequency() not implemented in libplayerc"
            << std::endl;
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
  std::cerr << "PlayerClient::SetDataMode() not implemented in libplayerc"
            << std::endl;
  /*
  if (0!=playerc_client_datamode(mClient, aMode))
  {
    throw PlayerError("PlayerClient::SetDataMode()", playerc_error_str());
  }
  */
}

int PlayerClient::LookupCode(std::string aName) const
{
  return playerc_lookup_code(aName.c_str());
}

std::string PlayerClient::LookupName(int aCode) const
{
  return std::string(playerc_lookup_name(aCode));
}

std::ostream&
std::operator << (std::ostream& os, const PlayerCc::PlayerClient& c)
{
  return os << c.GetHostname()
            << ": "
            << c.GetPort();
}
