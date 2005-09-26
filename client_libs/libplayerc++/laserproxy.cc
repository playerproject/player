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
 * client-side laser device 
 */

#include "playerc++.h"
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

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
  playerc_laser_unsubscribe(mDevice);
  playerc_laser_destroy(mDevice);
  mDevice = NULL;
}

void LaserProxy::Configure(double min_angle, 
                          double max_angle, 
			              uint scan_res,
			              uint range_res, 
                          bool intensity)
{
  Lock();
  playerc_laser_set_config(mDevice,min_angle,max_angle,scan_res,range_res,intensity?1:0);
  Unlock();	
}
	

/** Get the current laser configuration; it is read into the
  relevant class attributes.\\
  Returns the 0 on success, or -1 of there is a problem.
 */
int LaserProxy::RequestConfigure()
{
  Lock();
  unsigned char temp_int;
  int ret = playerc_laser_get_config(mDevice,&min_angle, &max_angle, &scan_res, &range_res, &temp_int);
  Unlock();
  intensity = temp_int == 0 ? false : true;
  return ret;
}

std::ostream &PlayerCc::operator << (std::ostream &os, const PlayerCc::LaserProxy &c)
{
  os << "#min\tmax\tres\tcount\trange_res" << std::endl;
  os << RTOD(c.GetVar(c.mDevice->scan_start)) << 
    RTOD(c.GetVar(c.mDevice->scan_start + c.GetVar(c.mDevice->scan_count) * c.GetVar(c.mDevice->scan_res))) <<
    RTOD(c.GetVar(c.mDevice->scan_res)) << c.GetVar(c.mDevice->scan_count) << c.GetVar(c.mDevice->range_res) << std::endl;
	
  os << "#range\tbearing\tintensity" << std::endl;
  for(int i=0;i<c.GetVar(c.mDevice->scan_count) && i<PLAYER_LASER_MAX_SAMPLES;i++)
    os << c.GetVar(c.mDevice->scan[i][0]) << " " << RTOD(c.GetVar(c.mDevice->scan[i][1])) << " " << c.GetVar(c.mDevice->intensity[i]) << " ";
  return os;
}


