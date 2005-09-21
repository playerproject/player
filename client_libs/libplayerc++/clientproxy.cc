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
 * parent class for client-side device proxies
 */

#include <playercc.h>
#include <cstring>
#include <cstdio>
#include <iostream>

using namespace PlayerCc;

ClientProxy::ClientProxy(PlayerClient* aPc) :
 mPc(aPc),
 mClient(aPc->mClient)
{
  assert(NULL != mPc);

  // add us to the PlayerClient list
  mPc->mProxyList.push_back(this);

  // not sure if this works, or is usefull
  //mInterfaceName = playerc_lookup_name(mDeviceAddr->interf);
}

// destructor will try to close access to the device
ClientProxy::~ClientProxy()
{
  // each client needs to unsubscribe themselves,
  // but we will take care of removing them from the list

  mPc->mProxyList.remove(this);
}



void
ClientProxy::Lock() const
{
  //aPc->mClient.mMutex.lock();
  //std::cout << "Lock()" << std::endl;
}

void
ClientProxy::Unlock() const
{
  //aPc->mClient.mMutex.unlock();
  //std::cout << "Unlock()" << std::endl;
}

std::ostream& operator << (std::ostream& os, const PlayerCc::ClientProxy& c)
{
  return os << c.GetDriverName()
            << ": "
            << c.GetInterfaceStr()
            << "("
            << c.GetIndex()
            << ")";
}

