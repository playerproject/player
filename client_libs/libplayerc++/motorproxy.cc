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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ********************************************************************/

/*
 * $Id$
 */

#include "playerc++.h"

using namespace PlayerCc;

MotorProxy::MotorProxy(PlayerClient *aPc, uint aIndex)
  : ClientProxy(aPc, aIndex),
  mDevice(NULL)
{
  Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
  mInfo = &(mDevice->info);
}

MotorProxy::~MotorProxy()
{
  Unsubscribe();
}

void
MotorProxy::Subscribe(uint aIndex)
{
  scoped_lock_t lock(mPc->mMutex);
  mDevice = playerc_motor_create(mClient, aIndex);
  if (NULL==mDevice)
    throw PlayerError("MotorProxy::MotorProxy()", "could not create");

  if (0 != playerc_motor_subscribe(mDevice, PLAYER_OPEN_MODE))
    throw PlayerError("MotorProxy::MotorProxy()", "could not subscribe");
}

void
MotorProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  scoped_lock_t lock(mPc->mMutex);
  playerc_motor_unsubscribe(mDevice);
  playerc_motor_destroy(mDevice);
  mDevice = NULL;
}

void
MotorProxy::SetSpeed(double aSpeed)
{
  scoped_lock_t lock(mPc->mMutex);
  playerc_motor_set_cmd_vel(mDevice, aSpeed, 0);
}

void
MotorProxy::GoTo(double aAngle)
{
  scoped_lock_t lock(mPc->mMutex);
  playerc_motor_set_cmd_pose(mDevice, aAngle, 0);
}

void
MotorProxy::SetMotorEnable(bool aEnable)
{
  scoped_lock_t lock(mPc->mMutex);
  playerc_motor_enable(mDevice,aEnable);
}

void
MotorProxy::ResetOdometry()
{
  SetOdometry(0);
}

void
MotorProxy::SetOdometry(double aAngle)
{
  scoped_lock_t lock(mPc->mMutex);
  playerc_motor_set_odom(mDevice, aAngle);
}

void
MotorProxy::SetSpeedPID(double aKp, double aKi, double aKd)
{
  std::cerr << "MotorProxy::SetSpeedPID() not implemented in libplayerc"
            << std::endl;
  /*
  if (0!=playerc_motor_set_speed_pid(mClient, aKp, aKi, aKd))
  {
    throw PlayerError("MotorProxy::SetSpeedPID()", playerc_error_str());
  }
  */
}

void
MotorProxy::SetPositionPID(double aKp, double aKi, double aKd)
{
  std::cerr << "MotorProxy::SetPositionPID() not implemented in libplayerc"
            << std::endl;
  /*
  if (0!=playerc_motor_set_position_pid(mClient, aKp, aKi, aKd))
  {
    throw PlayerError("MotorProxy::SetPositionPID()", playerc_error_str());
  }
  */
}

std::ostream&
std::operator << (std::ostream &os, const PlayerCc::MotorProxy &c)
{
  os << "#Motor (" << c.GetInterface() << ":" << c.GetIndex() << ")" << std::endl;
  os << "#pos\tvel\tmin\tcenter\tmax\tstall" << std::endl;
  os << c.GetPos() << "\t" << c.GetSpeed() << "\t";
  os << c.IsLimitMin() << "\t" << c.IsLimitCenter() << "\t" << c.IsLimitMax() << "\t";
  os << c.GetStall() << std::endl;
  return os;
}

