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

PtzProxy::PtzProxy(PlayerClient *aPc, uint aIndex)
  : ClientProxy(aPc, aIndex),
  mDevice(NULL)
{
  Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
  mInfo = &(mDevice->info);
}

PtzProxy::~PtzProxy()
{
  Unsubscribe();
}

void
PtzProxy::Subscribe(uint aIndex)
{
  mDevice = playerc_ptz_create(mClient, aIndex);
  if (NULL==mDevice)
    throw PlayerError("PtzProxy::PtzProxy()", "could not create");

  if (0 != playerc_ptz_subscribe(mDevice, PLAYER_OPEN_MODE))
    throw PlayerError("PtzProxy::PtzProxy()", "could not subscribe");
}

void
PtzProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  playerc_ptz_unsubscribe(mDevice);
  playerc_ptz_destroy(mDevice);
  mDevice = NULL;
}

std::ostream& std::operator << (std::ostream &os, const PlayerCc::PtzProxy &c)
{
  os << "#PTZ (" << c.GetInterface() << ":" << c.GetIndex() << ")" << std::endl;
  os << c.GetPan() << " " << c.GetTilt() << " " << c.GetZoom() << std::endl;
  return os;
}

/** Change the camera state.
    Specify the new @p pan, @p tilt, and @p zoom values
    (all degrees).
*/
void PtzProxy::SetCam(double aPan, double aTilt, double aZoom)
{
  playerc_ptz_set(mDevice, aPan, aTilt, aZoom);
}

/** Specify new target velocities */
void PtzProxy::SetSpeed(double aPanSpeed, double aTiltSpeed, double aZoomSpeed)
{
  playerc_ptz_set_ws(mDevice, 0, 0, 0, aPanSpeed, aTiltSpeed);
}

