/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
/**************************************************************************
 * Desc: Sensor models for wifi.
 * Author: Andrew Howard
 * Date: 30 May 2003
 * CVS: $Id$
 *************************************************************************/

#ifndef WIFI_H
#define WIFI_H

#include "../pf/pf.h"
#include "../pf/pf_pdf.h"
#include "../map/map.h"

#ifdef __cplusplus
extern "C" {
#endif

  
// Model information
typedef struct
{
  // Pointer to the map map
  map_t *map;
  
} wifi_t;


// Create an sensor model
wifi_t *wifi_alloc(map_t *map);

// Free an sensor model
void wifi_free(wifi_t *sensor);

// Set the wifi level readings that will be used.
void wifi_set_level(wifi_t *sensor, int index, int level);

// Prepare to update the distribution using the sensor model.
void wifi_sensor_init(wifi_t *self);

// Finish updating the distribution using the sensor model.
void wifi_sensor_term(wifi_t *self);

// The sensor model function
double wifi_sensor_model(wifi_t *self, pf_vector_t pose);

#ifdef __cplusplus
}
#endif

#endif

