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
 * Desc: localization device interface
 * Author: Boyoon Jung
 * Date: 20 Nov 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "playerv.h"

// Compute eigen values and eigen vectors of a 2x2 covariance matrix
static void eigen(double cm[][2], double values[], double vectors[][2]);

// Update the localization configuration
//void localization_update_config(localization_t *localization);

// Draw the localization hypothesis
void localization_draw(localization_t *localization);


// Create a localization device
localization_t *localization_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client,
		  int index, const char *drivername, int subscribe)
{
    char label[64];
    char section[64];
    localization_t *localization;
  
    localization = malloc(sizeof(localization_t));
    localization->proxy = playerc_localization_create(client, index);
    localization->drivername = strdup(drivername);
    localization->datatime = 0;

    snprintf(section, sizeof(section), "localization:%d", index);

    // Construct the menu
    snprintf(label, sizeof(label), "localization:%d (%s)", index, localization->drivername);
    localization->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
    localization->subscribe_item = rtk_menuitem_create(localization->menu, "Subscribe", 1);
    localization->reset_item = rtk_menuitem_create(localization->menu, "Reset", 0);
    localization->showmap_item = rtk_menuitem_create(localization->menu, "Show Map", 1);

    // Set the initial menu state
    rtk_menuitem_check(localization->subscribe_item, subscribe);
    rtk_menuitem_check(localization->showmap_item, 0);

    // Default scale for drawing the map (m/pixel)
    localization->scale = 0.05;

    // Construct figures
    localization->image_init = 0;
    localization->map_fig = rtk_fig_create(mainwnd->canvas, NULL, 99);
    rtk_fig_movemask(localization->map_fig, RTK_MOVE_TRANS);

    // Initialize variables
    localization->map_data = NULL;

    return localization;
}


// Destroy a localization device
void localization_destroy(localization_t *localization)
{
    // Unsubscribe/destroy the proxy
    if (localization->proxy->info.subscribed)
        playerc_localization_unsubscribe(localization->proxy);
    playerc_localization_destroy(localization->proxy);

    // Destroy figures
    rtk_fig_destroy(localization->map_fig);

    // Destroy menu items
    rtk_menuitem_destroy(localization->subscribe_item);
    rtk_menuitem_destroy(localization->reset_item);
    rtk_menuitem_destroy(localization->showmap_item);
    rtk_menu_destroy(localization->menu);
  
    // Free memory
    free(localization->drivername);
    if (localization->map_data) free(localization->map_data);
    free(localization);
}


// Update a localization device
void localization_update(localization_t *localization)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(localization->subscribe_item))
  {
    if (!localization->proxy->info.subscribed)
    {
	    if (playerc_localization_subscribe(localization->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("subscribe failed : %s", playerc_error_str());

	    // Get the localization map header
	    if (playerc_localization_get_map_header(localization->proxy, 1,
                                              &localization->map_header) != 0)
        PRINT_ERR1("get_map_header failed : %s", playerc_error_str());    

	    // Determine a proper scale factor to build (50x??) size map
	    localization->map_scale = localization->map_header.width / 50;

	    // retrieve the map header
	    if (playerc_localization_get_map_header(localization->proxy, localization->map_scale,
                                              &(localization->map_header)) != 0)
        PRINT_ERR1("get_map_header failed : %s", playerc_error_str());    
    }

    // retrieve the map data if neccessary
    if (rtk_menuitem_ischecked(localization->showmap_item) && localization->map_data==NULL)
    {
	    // allocate a memory block for the map
	    localization->map_data = (unsigned char*)malloc(localization->map_header.width *
                                                      localization->map_header.height);

	    // Get the localization map data
	    if (playerc_localization_get_map(localization->proxy, localization->map_scale,
                                       &(localization->map_header), localization->map_data) != 0)
        PRINT_ERR1("get_map failed : %s", playerc_error_str());    
    }
  }
  else
  {
    if (localization->proxy->info.subscribed)
    {
	    if (playerc_localization_unsubscribe(localization->proxy) != 0)
        PRINT_ERR1("unsubscribe failed : %s", playerc_error_str());
    }
    // turn off the 'showmap' checkbutton
    if (rtk_menuitem_ischecked(localization->showmap_item))
	    rtk_menuitem_check(localization->showmap_item, 0);
    // free memory
    if (localization->map_data) {
	    free(localization->map_data);
	    localization->map_data = NULL;
    }
  }
  rtk_menuitem_check(localization->subscribe_item, localization->proxy->info.subscribed);

  // See if the reset button has been pressed
  if (rtk_menuitem_isactivated(localization->reset_item))
  {
    if (playerc_localization_reset(localization->proxy) != 0)
	    PRINT_ERR1("get_map failed : %s", playerc_error_str());    
  }

  // update the screen
  if (localization->proxy->info.subscribed)
  {
    // Draw in the localization hypothesis if it has been changed.
    if (localization->proxy->info.datatime != localization->datatime)
	    localization_draw(localization);
    localization->datatime = localization->proxy->info.datatime;
  }
  else
  {
    // Dont draw the localization.
    rtk_fig_show(localization->map_fig, 0);
    localization->datatime = 0;
  }
}


// macros to drraw objects in image coordinates in which origin is on the center
#define CX(x) ((double)(x) * localization->scale)
#define CY(y) ((double)(y) * localization->scale)

// macros to drraw objects in image coordinates in which origin is on the top-left corner
#define IX(x) (((double)(x)-localization->map_header.width/2) * localization->scale)
#define IY(y) ((localization->map_header.height/2-(double)(y)) * localization->scale)

// macros to draw objects in map coordinates in which origin is on the bottom-left corner
#define MX(x) ((((x)/1000000.0*localization->map_header.ppkm)-localization->map_header.width/2) * localization->scale)
#define MY(y) ((((y)/1000000.0*localization->map_header.ppkm)-localization->map_header.height/2) * localization->scale)

// macros to convert a distance in map coordinates into one in image coordinates
#define MS(d) ((d)/1000000.0 * localization->map_header.ppkm * localization->scale)

// macros to convert radians into degrees and vice versa
#define R2D(a) (((a)*M_PI/180))
#define D2R(a) (((int)((a)*180/M_PI)))


// Draw the localization hypothesis
void localization_draw(localization_t *localization)
{
    static double cov[2][2], eval[2], evec[2][2];
    int i, w, h;
    int sizex, sizey;
    double scalex, scaley;
    double ox, oy, oa, sx, sy;

    rtk_fig_show(localization->map_fig, 1);
    rtk_fig_clear(localization->map_fig);

    // Set the initial pose of the image if it hasnt already been set.
    if (localization->image_init == 0)
    {
	rtk_canvas_get_size(localization->map_fig->canvas, &sizex, &sizey);
	rtk_canvas_get_scale(localization->map_fig->canvas, &scalex, &scaley);
	rtk_fig_origin(localization->map_fig, -sizex * scalex / 4, -sizey * scaley / 4, 0);
	localization->image_init = 1;
    }

    // Draw an opaque rectangle on which to render the image.
    rtk_fig_color_rgb32(localization->map_fig, 0xFFFFFF);
    rtk_fig_rectangle(localization->map_fig, CX(0), CY(0), 0,
	     CX(localization->map_header.width), CY(localization->map_header.height), 1);
    rtk_fig_color_rgb32(localization->map_fig, 0x000000);
    rtk_fig_rectangle(localization->map_fig, CX(0), CY(0), 0,
	     CX(localization->map_header.width), CY(localization->map_header.height), 0);

    // Draw the map
    if (rtk_menuitem_ischecked(localization->showmap_item) && localization->map_data!=NULL)
    {
	for (h=0; h<localization->map_header.height; h++)
	    for (w=0; w<localization->map_header.width; w++)
	    {
		uint8_t gray=localization->map_data[h*localization->map_header.width + w];
		uint32_t color = (gray << 16) | (gray << 8) | gray;
		rtk_fig_color_rgb32(localization->map_fig, color);
		rtk_fig_rectangle(localization->map_fig, IX(w), IY(h), 0,
			localization->scale, localization->scale, TRUE);
	    }
    }

    // Draw the hypothesis.
    for (i=0; i<localization->proxy->num_hypothesis; i++)
    {
	ox = MX(localization->proxy->hypothesis[i].mean[0]);
	oy = MY(localization->proxy->hypothesis[i].mean[1]);

	cov[0][0] = localization->proxy->hypothesis[i].cov[0][0];
	cov[0][1] = localization->proxy->hypothesis[i].cov[0][1];
	cov[1][1] = localization->proxy->hypothesis[i].cov[1][1];
	eigen(cov, eval, evec);

	oa = atan(evec[1][0] / evec[0][0]);
	sx = MS(3*sqrt(fabs(eval[0])));
	sy = MS(3*sqrt(fabs(eval[1])));

	rtk_fig_color_rgb32(localization->map_fig, COLOR_LOCALIZATION);
	rtk_fig_line_ex(localization->map_fig, ox, oy, oa, MS(1000));
	rtk_fig_line_ex(localization->map_fig, ox, oy, oa+M_PI_2, MS(1000));
	rtk_fig_ellipse(localization->map_fig, ox, oy, oa, sx, sy, FALSE);
    }
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
