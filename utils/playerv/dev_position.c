/* 
 *  PlayerViewer
 *  Copyright (C) Andrew Howard 2002
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/***************************************************************************
 * Desc: Position device interface
 * Author: Andrew Howard
 * Date: 14 May 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "playerv.h"


// Draw the position scan
void position_draw(position_t *position);

// Dont draw the position data
void position_nodraw(position_t *position);

// Servo robot to goal pose
void position_update_servo(position_t *position);


// Create a position device
position_t *position_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client, int index)
{
  char label[64];
  char section[64];
  position_t *position;
  int subscribe, enable;
  double sizex, sizey, orgx, orgy;
  
  position = malloc(sizeof(position_t));

  position->proxy = playerc_position_create(client, index);
  position->datatime = 0;
  
  snprintf(section, sizeof(section), "position:%d", index);
  
  // Set initial device state
  subscribe = opt_get_int(opt, section, "", 0);
  subscribe = opt_get_int(opt, section, "subscribe", subscribe);
  if (subscribe)
  {
    if (playerc_position_subscribe(position->proxy, PLAYER_ALL_MODE) != 0)
      PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }

  enable = opt_get_int(opt, section, "enable", 0);
  if (position->proxy->info.subscribed)
    if (playerc_position_enable(position->proxy, enable) != 0)
      PRINT_ERR1("libplayerc error: %s", playerc_errorstr);

  // Get the robot geometry
  sizex = 0.45;
  sizey = 0.40;
  opt_get_double2(opt, "robot", "size", &sizex, &sizey);
  orgx = 0.08;
  orgy = 0.0;
  opt_get_double2(opt, "robot", "origin", &orgx, &orgy);

  // Create a figure representing the robot
  position->robot_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 1);
  rtk_fig_show(position->robot_fig, 0);
  rtk_fig_color_rgb32(position->robot_fig, COLOR_POSITION_ROBOT);
  rtk_fig_rectangle(position->robot_fig, -orgx, -orgy, 0, sizex, sizey, 0);

  // Create a figure representing the robot's control speed.
  position->control_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 0);
  rtk_fig_show(position->control_fig, 0);
  rtk_fig_color_rgb32(position->control_fig, COLOR_POSITION_CONTROL);
  rtk_fig_line(position->control_fig, -0.20, 0, +0.20, 0);
  rtk_fig_line(position->control_fig, 0, -0.20, 0, +0.20);
  rtk_fig_ellipse(position->control_fig, 0, 0, 0, 0.20, 0.20, 0);
  rtk_fig_movemask(position->control_fig, RTK_MOVE_TRANS);
  position->path_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 0);
  
  // Construct the menu
  snprintf(label, sizeof(label), "Position %d", index);
  position->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  position->subscribe_item = rtk_menuitem_create(position->menu, "Subscribe", 1);
  position->enable_item = rtk_menuitem_create(position->menu, "Enable", 0);
  position->disable_item = rtk_menuitem_create(position->menu, "Disable", 0);

  // Set the initial menu state
  rtk_menuitem_check(position->subscribe_item, position->proxy->info.subscribed);
  
  return position;
}


// Destroy a position device
void position_destroy(position_t *position)
{
  if (position->proxy->info.subscribed)
    playerc_position_unsubscribe(position->proxy);
  playerc_position_destroy(position->proxy);

  rtk_fig_destroy(position->path_fig);
  rtk_fig_destroy(position->control_fig);
  rtk_fig_destroy(position->robot_fig);

  rtk_menuitem_destroy(position->subscribe_item);
  rtk_menu_destroy(position->menu);
  
  free(position);
}


// Update a position device
void position_update(position_t *position)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(position->subscribe_item))
  {
    if (!position->proxy->info.subscribed)
      if (playerc_position_subscribe(position->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  else
  {
    if (position->proxy->info.subscribed)
      if (playerc_position_unsubscribe(position->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  rtk_menuitem_check(position->subscribe_item, position->proxy->info.subscribed);

  // Check enable flag
  if (rtk_menuitem_isactivated(position->enable_item))
  {
    if (position->proxy->info.subscribed)
      if (playerc_position_enable(position->proxy, 1) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  if (rtk_menuitem_isactivated(position->disable_item))
  {
    if (position->proxy->info.subscribed)
      if (playerc_position_enable(position->proxy, 0) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  
  // Servo to the goal position
  position_update_servo(position);
  
  if (position->proxy->info.subscribed)
  {
    // Draw in the position scan if it has been changed.
    if (position->proxy->info.datatime != position->datatime)
    {
      position_draw(position);
      position->datatime = position->proxy->info.datatime;
    }
  }
  else
  {
    // Dont draw the position.
    position_nodraw(position);
  }
}


// Draw the position data
void position_draw(position_t *position)
{
  rtk_fig_show(position->robot_fig, 1);

  // REMOVE
  //rtk_fig_show(position->odo_fig, 1);      
  //rtk_fig_clear(position->odo_fig);
  //rtk_fig_color_rgb32(position->odo_fig, COLOR_POSITION_ODO);
  // snprintf(text, sizeof(text), "[%+07.2f, %+07.2f, %+04.0f]",
  //         position->proxy->px, position->proxy->py, position->proxy->pa * 180/M_PI);
  //rtk_fig_text(position->odo_fig, 0, 0, 0, text);
}


// Dont draw the position data
void position_nodraw(position_t *position)
{
  rtk_fig_show(position->robot_fig, 0);
}


// Control the robot.
void position_update_servo(position_t *position)
{
  double d;
  double rx, ry, ra;
  double kr, ka;
  double vr, va;
  double min_vr, max_vr;
  double min_va, max_va;

  if (!position->proxy->info.subscribed)
  {
    rtk_fig_show(position->control_fig, 0);
    rtk_fig_show(position->path_fig, 0);
    return;
  }
  else
  {
    rtk_fig_show(position->control_fig, 1);
    rtk_fig_show(position->path_fig, 1);
  }
  
  min_vr = -0.10; max_vr = 0.20;
  min_va = -M_PI/8; max_va = +M_PI/8;

  kr = max_vr / 1.00;
  ka = max_va / 1.00;

  if (rtk_fig_mouse_selected(position->control_fig))
  {
    // Get goal pose in robot cs
    rtk_fig_get_origin(position->control_fig, &rx, &ry, &ra);
  }
  else
  { 
    // Reset the goal figure
    rx = ry = ra = 0;
    rtk_fig_origin(position->control_fig, rx, ry, ra);
  }

  vr = kr * rx;
  va = ka * ry;

  if (vr < 0)
    va *= -1;
  
  // Bound the speed
  if (vr > max_vr)
    vr = max_vr;
  if (vr < min_vr)
    vr = min_vr;
  if (va > max_va)
    va = max_va;
  if (va < min_va)
    va = min_va;

  // Set the new speed
  playerc_position_setspeed(position->proxy, vr, 0, va);

  // Draw in the path
  d = 0.30;
  rtk_fig_clear(position->path_fig);
  rtk_fig_color_rgb32(position->path_fig, COLOR_POSITION_CONTROL);
  if (rx >= 0)
  {
    rtk_fig_line(position->path_fig, 0, 0, d, 0);
    rtk_fig_line(position->path_fig, d, 0, rx, ry);
  }
  else
  {
    rtk_fig_line(position->path_fig, 0, 0, -d, 0);
    rtk_fig_line(position->path_fig, -d, 0, rx, ry);
  }
}

/*
// Servo robot to goal pose
void position_update_servo(position_t *position)
{
  double ox, oy, oa;
  double rx, ry, ra;
  double gx, gy, ga;

  double kr, ka;
  double vr, va;
  double min_vr, max_vr;
  double min_va, max_va;

  kr = 0.2;
  ka = 0.1;
  min_vr = -0.10; max_vr = 0.20;
  min_va = -M_PI/4; max_va = +M_PI/4;

  ox = position->proxy->px;
  oy = position->proxy->py;
  oa = position->proxy->pa;

  if (rtk_fig_mouse_selected(position->control_fig))
  {
    // Get goal pose in robot cs
    rtk_fig_get_origin(position->control_fig, &rx, &ry, &ra);

    // Compute goal pose in odometric cs
    gx = ox + rx * cos(oa) - ry * sin(oa);
    gy = oy + rx * sin(oa) + ry * cos(oa);
    ga = oa + ra;

    position->goal_x = gx;
    position->goal_y = gy;
    position->goal_a = ga;
  }
  else
  { 
    gx = position->goal_x;
    gy = position->goal_y;
    ga = position->goal_a;
  
    // Compute goal pose in robot cs
    rx = +(gx - ox) * cos(oa) + (gy - oy) * sin(oa);
    ry = -(gx - ox) * sin(oa) + (gy - oy) * cos(oa);
    ra = ga - oa;

    // Reset the goal figure
    rtk_fig_origin(position->control_fig, rx, ry, ra);
  }

  // Servo to the goal pose
  vr = kr * rx;
  va = ka * atan2(ry, rx);

  // Bound the speed
  if (vr > max_vr)
    vr = max_vr;
  if (vr < min_vr)
    vr = min_vr;
  if (va > max_va)
    va = max_va;
  if (va < min_va)
    va = min_va;

  printf("%f %f\n", vr, va);

  // Set the new speed
  playerc_position_setspeed(position->proxy, vr, 0, va);
}
*/





