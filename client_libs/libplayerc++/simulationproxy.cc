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

SimulationProxy::SimulationProxy(PlayerClient *aPc, uint aIndex)
  : ClientProxy(aPc, aIndex),
  mDevice(NULL)
{
  Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
  mInfo = &(mDevice->info);
}

SimulationProxy::~SimulationProxy()
{
  Unsubscribe();
}

void
SimulationProxy::Subscribe(uint aIndex)
{
  scoped_lock_t lock(mPc->mMutex);
  mDevice = playerc_simulation_create(mClient, aIndex);
  if (NULL==mDevice)
    throw PlayerError("SimulationProxy::SimulationProxy()", "could not create");

  if (0 != playerc_simulation_subscribe(mDevice, PLAYER_OPEN_MODE))
    throw PlayerError("SimulationProxy::SimulationProxy()", "could not subscribe");
}

void
SimulationProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  scoped_lock_t lock(mPc->mMutex);
  playerc_simulation_unsubscribe(mDevice);
  playerc_simulation_destroy(mDevice);
  mDevice = NULL;
}

std::ostream&
std::operator << (std::ostream &os, const PlayerCc::SimulationProxy &c)
{
  os << "#Simulation (" << c.GetInterface() << ":" << c.GetIndex() << ")" << std::endl;
  return os;
}

void SimulationProxy::SetPose2d(char* identifier, double x, double y, double a)
{
  scoped_lock_t lock(mPc->mMutex);
  playerc_simulation_set_pose2d(mDevice,identifier, x,y,a);
}

void SimulationProxy::GetPose2d(char* identifier, double& x, double& y, double& a)
{
  scoped_lock_t lock(mPc->mMutex);
  playerc_simulation_get_pose2d(mDevice,identifier, &x,&y,&a);
}

