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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 */
/***************************************************************************
 * Desc: localization device interface
 * Author: Boyoon Jung
 * Date: 20 Nov 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <libplayerutil/localization.h>
#include "playerv.h"

#if defined (WIN32)
  #define M_PI_2 (M_PI/2.0)
#endif

// Compute eigen values and eigen vectors of a 2x2 covariance matrix
static void eigen(double cm[][2], double values[], double vectors[][2]);

// Reset the pose
void localize_reset_pose(localize_t *localize);

// Draw the hypotheses
void localize_draw_hypoth(localize_t *localize);

// Draw particles
void localize_draw_particles(localize_t *localize);

// Create a localize device
localize_t *localize_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client,
                            int index, const char *drivername, int subscribe)
{
  char label[64];
  char section[64];
  localize_t *self;
  
  self = malloc(sizeof(localize_t));

  self->mainwnd = mainwnd;
  self->proxy = playerc_localize_create(client, index);
  self->drivername = strdup(drivername);
  self->datatime = 0;

  snprintf(section, sizeof(section), "localize:%d", index);

  // Construct the menu
  snprintf(label, sizeof(label), "localize:%d (%s)", index, self->drivername);
  self->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  self->subscribe_item = rtk_menuitem_create(self->menu, "Subscribe", 1);
  self->reset_item = rtk_menuitem_create(self->menu, "Reset", 0);
  self->showparticles_item = rtk_menuitem_create(self->menu, "Show Particles", 1);

  // We can use this device to give us a coordinate system
  snprintf(label, sizeof(label), "Frame localize:%d (%s)", index, self->drivername);
  self->frame_item = rtk_menuitem_create(mainwnd->view_menu, label, 1);

  // Set the initial menu state
  rtk_menuitem_check(self->subscribe_item, subscribe);
  rtk_menuitem_check(self->showparticles_item, 1);
  rtk_menuitem_check(self->frame_item, 0);  

  // Construct figures
  self->hypoth_fig = rtk_fig_create(mainwnd->canvas, NULL, 85);
  self->particles_fig = rtk_fig_create(mainwnd->canvas, self->hypoth_fig, 85);

  return self;
}


// Destroy a localize device
void localize_destroy(localize_t *self)
{
  // Unsubscribe/destroy the proxy
  if (self->proxy->info.subscribed)
    playerc_localize_unsubscribe(self->proxy);
  playerc_localize_destroy(self->proxy);

  // Destroy figures
  rtk_fig_destroy(self->hypoth_fig);
  rtk_fig_destroy(self->particles_fig);

  // Destroy menu items
  rtk_menuitem_destroy(self->subscribe_item);
  rtk_menuitem_destroy(self->reset_item);
  rtk_menuitem_destroy(self->showparticles_item);
  rtk_menu_destroy(self->menu);
  
  // Free memory
  free(self->drivername);
  free(self);

  return;
}


// Update a localize device
void localize_update(localize_t *self)
{
  double ox, oy, oa;
  
  // Update the device subscription
  if (rtk_menuitem_ischecked(self->subscribe_item))
  {
    if (!self->proxy->info.subscribed)
    {
	    if (playerc_localize_subscribe(self->proxy, PLAYER_OPEN_MODE) != 0)
        PRINT_ERR1("subscribe failed : %s", playerc_error_str());
    }
  }
  else
  {
    if (self->proxy->info.subscribed)
    {
	    if (playerc_localize_unsubscribe(self->proxy) != 0)
        PRINT_ERR1("unsubscribe failed : %s", playerc_error_str());
    }
  }
  rtk_menuitem_check(self->subscribe_item, self->proxy->info.subscribed);

  // See if the reset button has been pressed
  if (rtk_menuitem_isactivated(self->reset_item))
    localize_reset_pose(self);

  // update the screen
  if (self->proxy->info.subscribed)
  {    
    
    // Show the figures
    rtk_fig_show(self->particles_fig, rtk_menuitem_ischecked(self->showparticles_item));
    rtk_fig_show(self->hypoth_fig, 1);

    
    // Draw in the localize hypothesis and particles if they have been changed.
    if (self->proxy->info.datatime != self->datatime)
    {
	    localize_draw_hypoth(self);
    }
    if(rtk_menuitem_ischecked(self->showparticles_item))
        if (playerc_localize_get_particles(self->proxy)==0)
	    localize_draw_particles(self);

        
    self->datatime = self->proxy->info.datatime;

    // Set the global robot pose
    if (rtk_menuitem_ischecked(self->frame_item))
    {
      if (self->proxy->hypoth_count > 0)
      {
        ox = self->proxy->hypoths[0].mean.px;
        oy = self->proxy->hypoths[0].mean.py;
        oa = self->proxy->hypoths[0].mean.pa;
        rtk_fig_origin(self->mainwnd->robot_fig, ox, oy, oa);
      }
    }
  }
  else
  {
    // Hide the figures
    rtk_fig_show(self->hypoth_fig, 0);    
    rtk_fig_show(self->particles_fig, 0);    
    self->datatime = 0;
  }
}


// Reset the pose
void localize_reset_pose(localize_t *self)
{
  double pose[3];
  double cov[3];

  pose[0] = 0.0;
  pose[1] = 0.0;
  pose[2] = 0.0;

  cov[0] = 1e3 * 1e3;
  cov[1] = 1e3 * 1e3;
  cov[2] = 1e3 * 1e3;  

  if (playerc_localize_set_pose(self->proxy, pose, cov) != 0)
    PRINT_ERR1("set pose failed : %s", playerc_error_str());
  return;
} 


// Draw the hypotheses
void localize_draw_hypoth(localize_t *self)
{
  int i;
  player_pose2d_t epose;
  double sx, sy;
  player_localize_hypoth_t *hypoth;

  rtk_fig_clear(self->hypoth_fig);
  rtk_fig_color_rgb32(self->hypoth_fig, COLOR_LOCALIZE);

  for (i = 0; i < self->proxy->hypoth_count; i++)
  {
    hypoth = self->proxy->hypoths + i;

    derive_uncertainty_ellipsis2d(&epose, &sx, &sy,
				  hypoth, 0.68);

    if (sx < 0.10)
      sx = 0.10;
    if (sy < 0.10)
      sy = 0.10;
    
    if (sx > 1e-3 && sy > 1e-3)
    {
      rtk_fig_line_ex(self->hypoth_fig, epose.px, epose.py, epose.pa, sx);
      rtk_fig_line_ex(self->hypoth_fig, epose.px, epose.py, 
		      epose.pa + M_PI / 2, sy);
      rtk_fig_ellipse(self->hypoth_fig, epose.px, epose.py, epose.pa, sx,sy, 0);
    }
  }
  
  return;
}

// Draw the particles
void localize_draw_particles(localize_t *self)
{
  int i;
  double ox, oy, oa;
  i=0;  
  rtk_fig_clear(self->particles_fig);
  rtk_fig_color_rgb32(self->particles_fig, COLOR_LOCALIZE_PARTICLES);
  for (i = 0; i < self->proxy->num_particles; i++)
  {
    playerc_localize_particle_t particle=self->proxy->particles[i];
    ox=particle.pose[0];
    oy=particle.pose[1];
    oa=particle.pose[2];
    rtk_fig_line_ex(self->particles_fig, ox, oy, oa, 0.03);
    rtk_fig_line_ex(self->particles_fig, ox, oy, oa + M_PI_2, 0.03);
  }
  
  return;
}

// Compute eigen values and eigenvectors of a 2x2 covariance matrix
static void eigen(double cm[][2], double values[], double vectors[][2])
{
  double s = (double) sqrt(cm[0][0]*cm[0][0] - 2*cm[0][0]*cm[1][1] +
                           cm[1][1]*cm[1][1] + 4*cm[0][1]*cm[0][1]);
  values[0] = 0.5 * (cm[0][0] + cm[1][1] + s);
  values[1] = 0.5 * (cm[0][0] + cm[1][1] - s);
  vectors[0][0] = -0.5 * (-cm[0][0] + cm[1][1] - s);
  vectors[0][1] = -0.5 * (-cm[0][0] + cm[1][1] + s);
  vectors[1][0] = vectors[1][1] = cm[0][1];
}
