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

ImuProxy::ImuProxy(PlayerClient *aPc, uint aIndex)
  : ClientProxy(aPc, aIndex),
  mDevice(NULL)
{
  Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
  mInfo = &(mDevice->info);
}

ImuProxy::~ImuProxy()
{
  Unsubscribe();
}

void
ImuProxy::Subscribe(uint aIndex)
{
  scoped_lock_t lock(mPc->mMutex);
  mDevice = playerc_imu_create(mClient, aIndex);
  if (NULL==mDevice)
	  throw PlayerError("ImuProxy::ImuProxy()", "could not create");

  if (0 != playerc_imu_subscribe(mDevice, PLAYER_OPEN_MODE))
	  throw PlayerError("ImuProxy::ImuProxy()", "could not subscribe");
}

void
ImuProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  scoped_lock_t lock(mPc->mMutex);
  playerc_imu_unsubscribe(mDevice);
  playerc_imu_destroy(mDevice);
  mDevice = NULL;
}

std::ostream&
std::operator << (std::ostream &os, const PlayerCc::ImuProxy &c)
{
  player_imu_data_calib_t data = c.GetRawValues();
   os << "Accel: " << data.accel_x << " " << data.accel_y << " " << data.accel_z << " Gyro: " << data.gyro_x << " " << data.gyro_y << " " << data.gyro_z << std::endl;
  return os;
}

