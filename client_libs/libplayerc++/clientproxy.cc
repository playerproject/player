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
 */

#include <iostream>

#include "playerc++.h"
#include "debug.h"

using namespace PlayerCc;

ClientProxy::ClientProxy(PlayerClient* aPc, uint aIndex) :
 mPc(aPc),
 mClient(aPc->mClient),
 mFresh(false),
 mLastTime(0)
{
  assert(NULL != mPc);
  // add us to the PlayerClient list
  mPc->mProxyList.push_back(this);
  PRINT("added " << this << " to ProxyList");
}

// destructor will try to close access to the device
ClientProxy::~ClientProxy()
{
  Unsubscribe();
  // each client needs to unsubscribe themselves,
  // but we will take care of removing them from the list
  mPc->mProxyList.remove(this);
  PRINT("removed " << this << " from ProxyList");
}

void ClientProxy::ReadSignal()
{
  PRINT("read " << *this);
  // only emit a signal when the interface has received data
  if (GetVar(mInfo->datatime) > GetVar(mLastTime))
  {
    {
      scoped_lock_t lock(mPc->mMutex);
      mFresh = true;
      mLastTime = mInfo->datatime;
    }
#ifdef HAVE_BOOST_SIGNALS
    mReadSignal();
#endif
  }
}

// add replace rule
void ClientProxy::SetReplaceRule(bool aReplace,
                                 int aType,
                                 int aSubtype)
{
  scoped_lock_t lock(mPc->mMutex);
  if (0!=playerc_client_set_replace_rule(mClient,
                                         mInfo->addr.interf,
                                         mInfo->addr.index,
                                         aType,
                                         aSubtype,
                                         aReplace))
  {
    throw PlayerError("ClientProxy::SetReplaceRule()", playerc_error_str());
  }
}

int ClientProxy::HasCapability(uint aType, uint aSubtype)
{
  scoped_lock_t lock(mPc->mMutex);
  return playerc_device_hascapability (mInfo, aType, aSubtype);
}

void ClientProxy::NotFresh()
{
  scoped_lock_t lock(mPc->mMutex);
  mFresh = false;
}

std::ostream& std::operator << (std::ostream& os, const PlayerCc::ClientProxy& c)
{
  return os << c.GetDriverName()
            << ": "
            << c.GetInterfaceStr()
            << "("
            << c.GetIndex()
            << ")";
}

