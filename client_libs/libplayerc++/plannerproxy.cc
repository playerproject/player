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
 * client-side planner device 
 */

#include "playerc++.h"

using namespace PlayerCc;

PlannerProxy::PlannerProxy(PlayerClient *aPc, uint aIndex)
  : ClientProxy(aPc, aIndex),
  mDevice(NULL)
{
  Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
  mInfo = &(mDevice->info);
}

PlannerProxy::~PlannerProxy()
{
  Unsubscribe();
}

void
PlannerProxy::Subscribe(uint aIndex)
{
  mDevice = playerc_planner_create(mClient, aIndex);
  if (NULL==mDevice)
    throw PlayerError("PlannerProxy::PlannerProxy()", "could not create");

  if (0 != playerc_planner_subscribe(mDevice, PLAYER_OPEN_MODE))
    throw PlayerError("PlannerProxy::PlannerProxy()", "could not subscribe");
}

void
PlannerProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  playerc_planner_unsubscribe(mDevice);
  playerc_planner_destroy(mDevice);
  mDevice = NULL;
}

std::ostream& std::operator << (std::ostream &os, const PlayerCc::PlannerProxy &c)
{
  os << "#Planner (" << c.GetInterface() << ":" << c.GetIndex() << ")" << std::endl;
  
  return os;
}

/** Set the goal pose (gx, gy, ga) */
int PlannerProxy::SetGoalPose(double aGx, double aGy, double aGa)
{
  return playerc_planner_set_cmd_pose(mDevice, aGx, aGy, aGa);
}

/** Get the list of waypoints. Writes the result into the proxy
    rather than returning it to the caller. */
int PlannerProxy::RequestWaypoints()
{
  return playerc_planner_get_waypoints(mDevice);
}

/** Enable/disable the robot's motion.  Set state to true to enable, false to
    disable. */
int PlannerProxy::SetEnable(bool aEnable)
{
  return playerc_planner_enable(mDevice, aEnable);
}

