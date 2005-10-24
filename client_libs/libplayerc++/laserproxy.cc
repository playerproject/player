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
#include <cstring>
#include <cstdio>
#include <cmath>
#include <climits>

using namespace PlayerCc;


LaserProxy::LaserProxy(PlayerClient *aPc, uint aIndex)
  : ClientProxy(aPc, aIndex),
  mDevice(NULL)
{
  Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
  mInfo = &(mDevice->info);
}

LaserProxy::~LaserProxy()
{
  Unsubscribe();
}

void
LaserProxy::Subscribe(uint aIndex)
{
  scoped_lock_t lock(mPc->mMutex);
  mDevice = playerc_laser_create(mClient, aIndex);
  if (NULL==mDevice)
    throw PlayerError("LaserProxy::LaserProxy()", "could not create");

  if (0 != playerc_laser_subscribe(mDevice, PLAYER_OPEN_MODE))
    throw PlayerError("LaserProxy::LaserProxy()", "could not subscribe");
}

void
LaserProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  scoped_lock_t lock(mPc->mMutex);
  playerc_laser_unsubscribe(mDevice);
  playerc_laser_destroy(mDevice);
  mDevice = NULL;
}

void
LaserProxy::Configure(double min_angle,
                      double max_angle,
                      uint scan_res,
                      uint range_res,
                      bool intensity)
{
  scoped_lock_t lock(mPc->mMutex);
  if (0 != playerc_laser_set_config(mDevice, min_angle, max_angle,
                                    scan_res, range_res, intensity?1:0))
    throw PlayerError("LaserProxy::RequestConfigure()", "error getting config");
}

void
LaserProxy::RequestConfigure()
{
  scoped_lock_t lock(mPc->mMutex);
  unsigned char temp_int;
  if (0 != playerc_laser_get_config(mDevice, &min_angle, &max_angle,
                                     &scan_res, &range_res, &temp_int))
    throw PlayerError("LaserProxy::RequestConfigure()", "error getting config");
  intensity = temp_int == 0 ? false : true;
  return;
}

std::ostream&
std::operator << (std::ostream &os, const PlayerCc::LaserProxy &c)
{
  /*
  os << "#min\tmax\tres\tcount\trange_res" << std::endl;
  os << RTOD(c.GetVar(c.mDevice->scan_start)) << "\t"
     << RTOD(c.GetVar(c.mDevice->scan_start +
        c.GetVar(c.mDevice->scan_count) * c.GetVar(c.mDevice->scan_res))) << "\t"
     << RTOD(c.GetVar(c.mDevice->scan_res)) << "\t"
     << c.GetVar(c.mDevice->scan_count) << "\t"
     << c.GetVar(c.mDevice->range_res) << std::endl;

  os << "#range\tbearing\tintensity" << std::endl;

  for(int i=0;i<c.GetVar(c.mDevice->scan_count) && i<PLAYER_LASER_MAX_SAMPLES;i++)
    os << c.GetVar(c.mDevice->scan[i][0]) << "\t"
       << RTOD(c.GetVar(c.mDevice->scan[i][1])) << "\t"
       << c.GetVar(c.mDevice->intensity[i]) << "\t";
  */
  return os;
}


