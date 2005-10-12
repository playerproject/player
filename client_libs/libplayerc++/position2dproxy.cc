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

Position2dProxy::Position2dProxy(PlayerClient *aPc, uint aIndex)
  : ClientProxy(aPc, aIndex),
  mDevice(NULL)
{
  Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
  mInfo = &(mDevice->info);
}

Position2dProxy::~Position2dProxy()
{
  Unsubscribe();
}

void
Position2dProxy::Subscribe(uint aIndex)
{
  boost::mutex::scoped_lock lock(mPc->mMutex);
  mDevice = playerc_position2d_create(mClient, aIndex);
  if (NULL==mDevice)
    throw PlayerError("Position2dProxy::Position2dProxy()", "could not create");

  if (0 != playerc_position2d_subscribe(mDevice, PLAYER_OPEN_MODE))
    throw PlayerError("Position2dProxy::Position2dProxy()", "could not subscribe");
}

void
Position2dProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  boost::mutex::scoped_lock lock(mPc->mMutex);
  playerc_position2d_unsubscribe(mDevice);
  playerc_position2d_destroy(mDevice);
  mDevice = NULL;
}

std::ostream&
std::operator << (std::ostream &os, const PlayerCc::Position2dProxy &c)
{
  os << "#Position2D (" << c.GetInterface() << ":" << c.GetIndex() << ")" << std::endl;
  os << "#xpos\typos\ttheta\tspeed\tsidespeed\tturn\tstall" << std::endl;
  os << c.GetXPos() << " " << c.GetYPos() << " " << c.GetYaw() << " " ;
  os << c.GetXSpeed() << " " << c.GetYSpeed() << " " << c.GetYawSpeed() << " ";
  os << c.GetStall() << std::endl;
  return os;
}

void
Position2dProxy::SetSpeed(double aXSpeed, double aYSpeed, double aYawSpeed)
{
  boost::mutex::scoped_lock lock(mPc->mMutex);
  playerc_position2d_set_cmd_vel(mDevice,aXSpeed,aYSpeed,aYawSpeed,0);
}

void
Position2dProxy::GoTo(double aX, double aY, double aYaw)
{
  boost::mutex::scoped_lock lock(mPc->mMutex);
  playerc_position2d_set_cmd_pose(mDevice,aX,aY,aYaw,0);
}

void
Position2dProxy::SetMotorEnable(bool aEnable)
{
  boost::mutex::scoped_lock lock(mPc->mMutex);
  playerc_position2d_enable(mDevice,aEnable);
}

void
Position2dProxy::ResetOdometry()
{
  SetOdometry(0,0,0);
}

void
Position2dProxy::SetOdometry(double aX, double aY, double aYaw)
{
  boost::mutex::scoped_lock lock(mPc->mMutex);
  playerc_position2d_set_odom(mDevice,aX,aY,aYaw);
}


