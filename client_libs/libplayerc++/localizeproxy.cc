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

#include "playerc++.h"

using namespace PlayerCc;

LocalizeProxy::LocalizeProxy(PlayerClient *aPc, uint aIndex)
  : ClientProxy(aPc, aIndex),
  mDevice(NULL)
{
  Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
  mInfo = &(mDevice->info);
}

LocalizeProxy::~LocalizeProxy()
{
  Unsubscribe();
}

void
LocalizeProxy::Subscribe(uint aIndex)
{
  scoped_lock_t lock(mPc->mMutex);
  mDevice = playerc_localize_create(mClient, aIndex);
  if (NULL==mDevice)
    throw PlayerError("LocalizeProxy::LocalizeProxy()", "could not create");

  if (0 != playerc_localize_subscribe(mDevice, PLAYER_OPEN_MODE))
    throw PlayerError("LocalizeProxy::LocalizeProxy()", "could not subscribe");
}

void
LocalizeProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  scoped_lock_t lock(mPc->mMutex);
  playerc_localize_unsubscribe(mDevice);
  playerc_localize_destroy(mDevice);
  mDevice = NULL;
}

std::ostream&
std::operator << (std::ostream &os, const PlayerCc::LocalizeProxy &c)
{
  os << "#Localize (" << c.GetInterface() << ":" << c.GetIndex() << ")" << std::endl;
  os << "Hypothesis Count: " << c.GetNumParticles() << std::endl;
  for (unsigned int i = 0; i < c.GetNumParticles(); ++i)
  {
    os << i << " (weight " << c.GetHypoth(i).alpha << "): ";
    os << "[" << c.GetHypoth(i).mean << "]" << std::endl;
  }
  return os;
}

void
LocalizeProxy::SetPose(double pose[3], double cov[3])
{
  scoped_lock_t lock(mPc->mMutex);
  if (0 != playerc_localize_set_pose(mDevice, pose, cov))
    throw PlayerError("LocalizeProxy::SetPose()", "error setting pose");
  return;
}


