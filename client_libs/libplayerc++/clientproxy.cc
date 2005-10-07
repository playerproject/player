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

#define DEBUG_LEVEL LOW
#include "debug.h"

using namespace PlayerCc;

ClientProxy::ClientProxy(PlayerClient* aPc, uint aIndex) :
 mPc(aPc),
 mClient(aPc->mClient),
 mLastTime(0)
{
  assert(NULL != mPc);

  // add us to the PlayerClient list
  mPc->mProxyList.push_back(this);
  PRINT("Added " << this << " to ProxyList");
}

// destructor will try to close access to the device
ClientProxy::~ClientProxy()
{
  Unsubscribe();
  // each client needs to unsubscribe themselves,
  // but we will take care of removing them from the list
  mPc->mProxyList.remove(this);
  PRINT("Removed " << this << " from ProxyList");
}

void
ClientProxy::Lock() const
{
  mPc->Lock();
  //std::cout << "Lock()" << std::endl;
}

void
ClientProxy::Unlock() const
{
  mPc->Unlock();
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

