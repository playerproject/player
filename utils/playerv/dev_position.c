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
#include <string.h>
#include "playerv.h"


// Draw the position scan
void position_draw(position_t *self);

// Dont draw the position data
void position_nodraw(position_t *self);

// Servo the robot (position control)
void position_servo_pos(position_t *self);

// Servo the robot (velocity control)
void position_servo_vel(position_t *self);


// Create a position device
position_t *position_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client,
                            int robot, int index, const char *drivername, int subscribe)
{
  char label[64];
  char section[64];
  position_t *self;
  
  self = malloc(sizeof(position_t));

  self->proxy = playerc_position_create(client, robot, index);
  self->drivername = strdup(drivername);
  self->datatime = 0;
  
  snprintf(section, sizeof(section), "position:%d", index);
  
  // Construct the menu
  snprintf(label, sizeof(label), "position:%d (%s)", index, self->drivername);
  self->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  self->subscribe_item = rtk_menuitem_create(self->menu, "Subscribe", 1);
  self->command_item = rtk_menuitem_create(self->menu, "Command", 1);
  self->pose_mode_item = rtk_menuitem_create(self->menu, "Position mode", 1);
  self->enable_item = rtk_menuitem_create(self->menu, "Enable", 0);
  self->disable_item = rtk_menuitem_create(self->menu, "Disable", 0);

  // Set the initial menu state
  rtk_menuitem_check(self->subscribe_item, subscribe);
  
  // Create a figure representing the robot
  self->robot_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 10);

  // Create a figure representing the robot's control speed.
  self->control_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 11);
  rtk_fig_show(self->control_fig, 0);
  rtk_fig_color_rgb32(self->control_fig, COLOR_POSITION_CONTROL);
  rtk_fig_line(self->control_fig, -0.20, 0, +0.20, 0);
  rtk_fig_line(self->control_fig, 0, -0.20, 0, +0.20);
  rtk_fig_ellipse(self->control_fig, 0, 0, 0, 0.20, 0.20, 0);
  rtk_fig_movemask(self->control_fig, RTK_MOVE_TRANS);
  self->path_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 2);

  return self;
}


// Destroy a position device
void position_destroy(position_t *self)
{
  if (self->proxy->info.subscribed)
    playerc_position_unsubscribe(self->proxy);
  playerc_position_destroy(self->proxy);

  rtk_fig_destroy(self->path_fig);
  rtk_fig_destroy(self->control_fig);
  rtk_fig_destroy(self->robot_fig);

  rtk_menuitem_destroy(self->subscribe_item);
  rtk_menu_destroy(self->menu);

  free(self->drivername);
  free(self);

  return;
}


// Update a position device
void position_update(position_t *self)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(self->subscribe_item))
  {
    if (!self->proxy->info.subscribed)
    {
      if (playerc_position_subscribe(self->proxy, PLAYER_ALL_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());

      // Get the robot geometry
      if (playerc_position_get_geom(self->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());
      
      rtk_fig_color_rgb32(self->robot_fig, COLOR_POSITION_ROBOT);
      rtk_fig_rectangle(self->robot_fig, self->proxy->pose[0],
                        self->proxy->pose[1], self->proxy->pose[2],
                        self->proxy->size[0], self->proxy->size[1], 0);
    }
  }
  else
  {
    if (self->proxy->info.subscribed)
      if (playerc_position_unsubscribe(self->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());
  }
  rtk_menuitem_check(self->subscribe_item, self->proxy->info.subscribed);

  // Check enable flag
  if (rtk_menuitem_isactivated(self->enable_item))
  {
    if (self->proxy->info.subscribed)
      if (playerc_position_enable(self->proxy, 1) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());
  }
  if (rtk_menuitem_isactivated(self->disable_item))
  {
    if (self->proxy->info.subscribed)
      if (playerc_position_enable(self->proxy, 0) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());
  }
  
  // Servo to the goal position
  if (rtk_menuitem_ischecked(self->command_item))
  {
    if (rtk_menuitem_ischecked(self->pose_mode_item))
      position_servo_pos(self);
    else
      position_servo_vel(self);
  }
  
  if (self->proxy->info.subscribed)
  {
    // Draw in the position scan if it has been changed.
    if (self->proxy->info.datatime != self->datatime)
    {
      position_draw(self);
      self->datatime = self->proxy->info.datatime;
    }
  }
  else
  {
    // Dont draw the position.
    position_nodraw(self);
  }
}


// Draw the position data
void position_draw(position_t *self)
{
  rtk_fig_show(self->robot_fig, 1);

  // REMOVE
  //rtk_fig_show(self->odo_fig, 1);      
  //rtk_fig_clear(self->odo_fig);
  //rtk_fig_color_rgb32(self->odo_fig, COLOR_POSITION_ODO);
  // snprintf(text, sizeof(text), "[%+07.2f, %+07.2f, %+04.0f]",
  //         self->proxy->px, self->proxy->py, self->proxy->pa * 180/M_PI);
  //rtk_fig_text(self->odo_fig, 0, 0, 0, text);
}


// Dont draw the position data
void position_nodraw(position_t *self)
{
  rtk_fig_show(self->robot_fig, 0);
  return;
}


// Servo the robot (position control)
void position_servo_pos(position_t *self)
{
  double rx, ry, ra;
  double gx, gy, ga;
  
  // Only servo if we are subscribed and have enabled commands.
  if (self->proxy->info.subscribed &&
      rtk_menuitem_ischecked(self->command_item))    
  {
    rtk_fig_show(self->control_fig, 1);
    rtk_fig_show(self->path_fig, 1);
  }
  else
  {
    rtk_fig_show(self->control_fig, 0);
    rtk_fig_show(self->path_fig, 0);
    return;
  }
  
  if (rtk_fig_mouse_selected(self->control_fig))
  {
    // Get goal pose in robot cs
    rtk_fig_get_origin(self->control_fig, &rx, &ry, &ra);
  }
  else
  { 
    // Reset the goal figure
    rx = ry = ra = 0;
    rtk_fig_origin(self->control_fig, rx, ry, ra);
  }

  // Compute goal point in position cs
  gx = self->proxy->px + rx * cos(self->proxy->pa) - ry * sin(self->proxy->pa);
  gy = self->proxy->py + rx * sin(self->proxy->pa) + ry * cos(self->proxy->pa);
  ga = self->proxy->pa + ra;

  // Set the new speed
  playerc_position_set_pose(self->proxy, gx, gy, ga);

  // Dont draw the path
  rtk_fig_clear(self->path_fig);
    
  return;
}


// Servo the robot (velocity control)
void position_servo_vel(position_t *self)
{
  double d;
  double rx, ry, ra;
  double kr, ka;
  double vr, va;
  double min_vr, max_vr;
  double min_va, max_va;

  // Only servo if we are subscribed and have enabled commands.
  if (self->proxy->info.subscribed &&
      rtk_menuitem_ischecked(self->command_item))    
  {
    rtk_fig_show(self->control_fig, 1);
    rtk_fig_show(self->path_fig, 1);
  }
  else
  {
    rtk_fig_show(self->control_fig, 0);
    rtk_fig_show(self->path_fig, 0);
    return;
  }
  
  min_vr = -0.10; max_vr = 0.30;
  min_va = -M_PI/8; max_va = +M_PI/8;

  kr = max_vr / 1.00;
  ka = max_va / 1.00;

  if (rtk_fig_mouse_selected(self->control_fig))
  {
    // Get goal pose in robot cs
    rtk_fig_get_origin(self->control_fig, &rx, &ry, &ra);
  }
  else
  { 
    // Reset the goal figure
    rx = ry = ra = 0;
    rtk_fig_origin(self->control_fig, rx, ry, ra);
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
  playerc_position_set_speed(self->proxy, vr, 0, va);

  // Draw in the path
  d = 0.30;
  rtk_fig_clear(self->path_fig);
  rtk_fig_color_rgb32(self->path_fig, COLOR_POSITION_CONTROL);
  if (rx >= 0)
  {
    rtk_fig_line(self->path_fig, 0, 0, d, 0);
    rtk_fig_line(self->path_fig, d, 0, rx, ry);
  }
  else
  {
    rtk_fig_line(self->path_fig, 0, 0, -d, 0);
    rtk_fig_line(self->path_fig, -d, 0, rx, ry);
  }
}
