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
// Desc: Odometry sensor model for AMCL
// Author: Andrew Howard
// Date: 17 Aug 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifndef AMCL_ODOM_H
#define AMCL_ODOM_H

#include "amcl_sensor.h"
#include "models/odometry.h"


// Odometric sensor data
class AMCLOdomData : public AMCLSensorData
{
  // Data time-stamp
  public: uint32_t time_sec, time_usec;

  // Odometric pose
  public: pf_vector_t pose;
};


// Odometric sensor model
class AMCLOdom : public AMCLSensor
{
  // Default constructor
  public: AMCLOdom();
  
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
  
  // Device info
  private: int odom_index;
  private: CDevice *device;

  // Odometry sensor/action model
  private: odometry_t *model;

  // Current data timestamp
  private: uint32_t tsec, tusec;
  
  // Last odometric value used to update filter
  private: pf_vector_t pose;
  private: pf_vector_t last_pose;

#ifdef INCLUDE_RTKGUI
  // Setup the GUI
  private: virtual void SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);

  // Finalize the GUI
  private: virtual void ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);

  // Draw sensor data
  private: virtual void UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);
#endif
};




#endif
