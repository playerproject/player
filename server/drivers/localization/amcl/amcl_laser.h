/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al.
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
///////////////////////////////////////////////////////////////////////////
//
// Desc: LASER sensor model for AMCL
// Author: Andrew Howard
// Date: 17 Aug 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifndef AMCL_LASER_H
#define AMCL_LASER_H

#include "amcl_sensor.h"
#include "map/map.h"
#include "models/laser.h"

// Laseretric sensor model
class AMCLLaser : public AMCLSensor
{
  // Default constructor
  public: AMCLLaser();
  
  // Load the model
  public: virtual int Load(ConfigFile* cf, int section);

  // Unload the model
  public: virtual int Unload(void);

  // Initialize the model
  public: virtual int Setup(void);

  // Finalize the model
  public: virtual int Shutdown(void);

  // Check for new sensor measurements
  private: virtual bool GetData(void);

  // Update the filter based on the sensor model.  Returns true if the
  // filter has been updated.
  public: virtual bool UpdateSensor(pf_t *pf);
  
  // Device info
  private: int laser_index;
  private: CDevice *device;

  // The laser map
  private: map_t *map;
  
  // Max ranges to consider
  private: int max_ranges;
  
  // Laser sensor/action model
  private: laser_t *model;

  // Current data timestamp
  private: uint32_t tsec, tusec;

  // Current laser data
  private: int range_count;
  private: double ranges[PLAYER_LASER_MAX_SAMPLES][2];

#ifdef INCLUDE_RTKGUI
  // Setup the GUI
  private: virtual void SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);

  // Finalize the GUI
  private: virtual void ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);

  // Draw sensor data
  private: virtual void UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);

  // Figuers
  private: rtk_fig_t *fig, *map_fig;
#endif
};




#endif
