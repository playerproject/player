/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
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

/*
 *   the mcl (Monte-Carlo Localization) device. This device implements
 *   the regular MCL only. Other extensions (i.g. mixture MCL, adaptive MCL)
 *   will be implemented in separate MCL devices.
 */
#ifndef _REGULARMCL_H
#define _REGULARMCL_H

#include <pthread.h>
#include <sys/time.h>
#include <vector>
using std::vector;

#include <device.h>
#include <drivertable.h>
#include <player.h>

#include "mcl_types.h"
#include "world_model.h"
#include "sensor_model.h"
#include "action_model.h"
#include "clustering.h"



class RegularMCL : public CDevice 
{
    private:
	// configuration : update speed
	float frequency;			// "frequency <float>"

	// configuration : the number of particles
	unsigned int num_particles;		// "num_particles <int>"

	// configuration : distance sensor
	mcl_sensor_t sensor_type;		// "sensor_type <string>"
	int sensor_index;			// "sensor_index <int>"
	uint16_t sensor_max;			// "sensor_max <int>"
	uint16_t sensor_num_samples;		// "sensor_num_samples <int>"

	// configuration : motion sensor
	int motion_index;			// "motion_index <int>"

	// configuration : world model (map)
	const char *map_file;			// "map <string>"
	uint16_t map_ppm;			// "map_ppm <int>"
	uint8_t map_threshold;			// "map_threshold <int>"

	// configuration : sensor model
	float sm_s_hit;				// "sm_s_hit" <float>
	float sm_lambda;			// "sm_lambda" <float>
	float sm_o_small;			// "sm_o_small" <float>
	float sm_z_hit;				// "sm_z_hit" <float>
	float sm_z_unexp;			// "sm_z_unexp" <float>
	float sm_z_max;				// "sm_z_max" <float>
	float sm_z_rand;			// "sm_z_rand" <float>
	int sm_precompute;			// "sm_precompute" <int>

	// configuration : action model
	float am_a1;				// "am_a1" <float>
	float am_a2;				// "am_a2" <float>
	float am_a3;				// "am_a3" <float>
	float am_a4;				// "am_a4" <float>

    protected:
	// for synchronization
	double period;

	// Pointer to sensors to get sensor data from
	CDevice *distance_device;		// distance sensor
	CDevice *motion_device;			// motion sensor

	// Pointer to models
	WorldModel *map;			// world model (map)
	MCLSensorModel *sensor_model;		// distance sensor model
	MCLActionModel *action_model;		// motion sensor model

	// Pointer to clustering algorithm class
	MCLClustering *clustering;

	// Particle set
	double unit_importance;			// uniform importance
	vector<particle_t> particles;		// a particle set
	vector<particle_t> p_buffer;		// a temporary buffer

	// Distance sensor data
	int num_ranges;				// the number of ranges
	int inc;				// for subsampling
	pose_t *poses;				// poses of distance sensors
	uint16_t *ranges;			// range data buffer

	// Motion sensor data
	pose_t p_odometry;			// previous odometry reading

    protected:
	// Main function for device thread
	virtual void Main(void);

	// Process configuration requests. Returns 1 if the configuration has changed.
	void UpdateConfig(void);

	// Reset MCL device
	void Reset(void);


	// Read the configuration of a distance sensor
	void ReadConfiguration(void);

	// Read range data from a distance senosr
	bool ReadRanges(uint16_t* buffer);

	// Read odometry data from a motion sensor
	bool ReadOdometry(pose_t& pose);

	//////////////////////////////////////////////////////////////////////
	// Monte-Carlo Localization Algorithm
	//
	// [step 1] : draw new samples from the previous PDF
	void SamplingImportanceResampling(pose_t from, pose_t to);

	// [step 2] : update importance factors based on the sensor model
	void ImportanceFactorUpdate(uint16_t *ranges);

	// [step 3] : construct hypothesis by grouping particles
	void HypothesisConstruction(player_localization_data_t& data);
	//
	//////////////////////////////////////////////////////////////////////


    public:
	// Constructor
	RegularMCL(char* interface, ConfigFile* cf, int section);

	// When the first client subscribes to a device
	virtual int Setup(void);
	// When the last client unsubscribes from a device
	virtual int Shutdown(void);

};


// a factory creation function
extern CDevice* MCL_Init(char* interface, ConfigFile* cf, int section);

// a driver registration function
extern void MCL_Register(DriverTable* table);


#endif
