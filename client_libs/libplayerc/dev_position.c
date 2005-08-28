/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) <insert dates here>
 *     <insert author's name(s) here>
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
 *  Player - One Hell of a Robot Server
 *  Copyright (C) <insert dates here>
 *     <insert author's name(s) here>
 *                      
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
 */

#include "playerc.h"

/* Position device, for backward compatibility */

/** Create a position device proxy. */
playerc_position_t *playerc_position_create(playerc_client_t *client, 
                                            int index)
{
  return playerc_position2d_create(client, index);
}

/** Destroy a position2d device proxy. */
void playerc_position_destroy(playerc_position_t *device)
{
  playerc_position2d_destroy(device);
}

/** Subscribe to the position2d device */
int playerc_position_subscribe(playerc_position_t *device, int access)
{
  return playerc_position2d_subscribe(device, access);
}
        

/** Un-subscribe from the position2d device */
int playerc_position_unsubscribe(playerc_position_t *device)
{
  return playerc_position2d_unsubscribe(device);
}

/** Enable/disable the motors */
int playerc_position_enable(playerc_position_t *device, int enable)
{
  return playerc_position2d_enable(device, enable);
}

/** Get the position2d geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
int playerc_position_get_geom(playerc_position_t *device)
{
  return playerc_position2d_get_geom(device);
}

/** Set the target speed.  vx : forward speed (m/s).  vy : sideways
    speed (m/s); this field is used by omni-drive robots only.  va :
    rotational speed (rad/s).  All speeds are defined in the robot
    coordinate system. */
int playerc_position_set_cmd_vel(playerc_position_t *device,
                                   double vx, double vy, double va, int state)
{
  return playerc_position2d_set_cmd_vel(device,vx,vy,va,state);
}

/** Set the target pose (gx, gy, ga) is the target pose in the
    odometric coordinate system. */
int playerc_position_set_cmd_pose(playerc_position_t *device,
                                  double gx, double gy, double ga, int state)
{
  return playerc_position2d_set_cmd_pose(device,gx,gy,ga,state);
}
