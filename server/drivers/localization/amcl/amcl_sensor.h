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
// Desc: Adaptive Monte-Carlo localization
// Author: Andrew Howard
// Date: 6 Feb 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifndef AMCL_SENSOR_H
#define AMCL_SENSOR_H

#include "device.h"
#include "configfile.h"
#include "pf/pf.h"


// Base class for all AMCL sensor measurements
class AMCLSensorData
{
};


// Base class for all AMCL sensors
class AMCLSensor
{
  // Default constructor
  public: AMCLSensor();

  // Load the model
  public: virtual int Load(ConfigFile* cf, int section);

  // Unload the model
  public: virtual int Unload(void);

  // Initialize the model
  public: virtual int Setup(void);

  // Finalize the model
  public: virtual int Shutdown(void);

  // Initialize the action model; returns true if the model has been initialized.
  public: virtual bool InitAction(pf_t *pf, uint32_t *tsec, uint32_t *tusec);

  // Update the filter based on the action model.  Returns true if the filter
  // has been updated.
  public: virtual bool UpdateAction(pf_t *pf, uint32_t *tsec, uint32_t *tusec);

  // Initialize the filter based on the sensor model.  Returns true if the
  // filter has been initialized.
  public: virtual bool InitSensor(pf_t *pf, pf_vector_t mean, pf_matrix_t cov);

  // Update the filter based on the sensor model.  Returns true if the
  // filter has been updated.
  public: virtual bool UpdateSensor(pf_t *pf);

#ifdef INCLUDE_RTKGUI
  // Setup the GUI
  public: virtual void SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);

  // Finalize the GUI
  public: virtual void ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);

  // Draw sensor data
  public: virtual void UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);
#endif
};




#endif
