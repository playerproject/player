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
// Desc: SONAR sensor model for AMCL
// Author: Oscar Gerelli, Dario Lodi Rizzini
// Date: 22 Jun 2005
//
///////////////////////////////////////////////////////////////////////////

#ifndef AMCL_SONAR_H
#define AMCL_SONAR_H

#include <iostream>
#include "amcl_sensor.h"
#include "map/map.h"
#include "models/sonar.h"


// Sonar sensor data
class AMCLSonarData : public AMCLSensorData
{
  public: int range_count;
  public: double ranges[PLAYER_SONAR_MAX_SAMPLES][2];
};


// Sonaretric sensor model
class AMCLSonar : public AMCLSensor
{
  // Default constructor
  public: AMCLSonar(player_device_id_t id);
  
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
  private: static double SensorModel(AMCLSonarData *data, pf_vector_t pose);

  // retrieve the map
  private: int SetupMap(void);

  // Device info
  private: player_device_id_t sonar_id;
  private: player_device_id_t map_id;
  private: Driver *driver;

  // Current data timestamp
  private: struct timeval time;

  // The sonar map
  private: map_t *map;

  // Sonar offset relative to robot
  private: pf_vector_t* sonar_pose;
  
  // Max beams to consider
  private: int max_beams;

  // Max valid laser range
  private: double range_max;
  
  // Laser range variance
  private: double range_var;

  // Probability of bad range readings
  private: double range_bad;

  // Number of sonar from configuration file
  private: int scount;

};

#endif
