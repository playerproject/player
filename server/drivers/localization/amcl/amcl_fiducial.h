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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: FIDUCIAL sensor model for AMCL
// Author: David Feil-Seifer
// Date: 17 Aug 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifndef AMCL_FIDUCIAL_H
#define AMCL_FIDUCIAL_H

#include "amcl_sensor.h"
#include "map/map.h"
#include "models/laser.h"

// Laser sensor data
class AMCLFiducialData : public AMCLSensorData
{
  // Laser range data (range, bearing tuples)
  public: int fiducial_count;
  public: double fiducials[PLAYER_FIDUCIAL_MAX_SAMPLES][3];
};


class AMCLFiducialMap
{
  //x,y,id
  public: int fiducial_count;
  public: double origin_x;
  public: double origin_y;
  public: double scale;
  public: double fiducials[100][3];
};

double fiducial_map_calc_range( AMCLFiducialMap* fmap, double ox, double oy, double oa, double max_range, int id, int k);
double fiducial_map_calc_bearing( AMCLFiducialMap* fmap, double ox, double oy, double oa, double max_range, int id, int k);

AMCLFiducialMap* fiducial_map_alloc();

// Laseretric sensor model
class AMCLFiducial : public AMCLSensor
{
  // Default constructor
  public: AMCLFiducial(player_devaddr_t addr);
  
  // Load the model
  public: virtual int Load(ConfigFile* cf, int section);

  // Unload the model
  public: virtual int Unload(void);

  // Initialize the model
  public: virtual int Setup(void);

  // Finalize the model
  public: virtual int Shutdown(void);

  // Check for new sensor measurements
  private: virtual AMCLSensorData *GetData(void);
  
  // Update the filter based on the sensor model.  Returns true if the
  // filter has been updated.
  public: virtual bool UpdateSensor(pf_t *pf, AMCLSensorData *data);

  // Determine the probability for the given pose
  private: static double SensorModel(AMCLFiducialData *data, pf_vector_t pose);

  // retrieve the map
  private: int SetupMap(void);

  private: int read_map_file(const char *);

  // Device info
  private: player_device_id_t fiducial_id;
  private: player_device_id_t map_id;
  private: Driver *driver;

  // Current data timestamp
  private: double time;

  // The laser map
  private: map_t *map;

  // The fiducial map
  private: AMCLFiducialMap *fmap;

  // Laser offset relative to robot
  private: pf_vector_t laser_pose;
  
  // Max valid laser range
  private: double range_max;
  
  // Laser range variance
  private: double range_var;

  // Laser angle variance
  private: double angle_var;

  // Probability of bad range readings
  private: double range_bad;

  // Probability of bad angle readings
  private: double angle_bad;

#ifdef INCLUDE_RTKGUI
  // Setup the GUI
  private: virtual void SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);

  // Finalize the GUI
  private: virtual void ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig);

  // Draw sensor data
  public: virtual void UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig, AMCLSensorData *data);

  // Figures
  private: rtk_fig_t *fig, *map_fig;
#endif

};




#endif
