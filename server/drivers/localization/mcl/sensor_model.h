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
 *   Parameterized sensor models. This class will provide various models for
 *   various sensors.
 */
#ifndef _SENSORMODEL_H
#define _SENSORMODEL_H

#include "mcl_types.h"
#include "world_model.h"

#include <stdint.h>


/*
 * Beam-based sensor model for Monte-Carlo Localization algorithms. The model
 * compute the probability P(o|s,m) where `o' is an observation, `s' is a pose
 * state, and `m' is an internal map.
*/
class MCLSensorModel
{
    private:
	// type of a distance sensor
	mcl_sensor_t type;
	// the number of ranges
	int num_ranges;
	// the pose of ranges
	pose_t* poses;
	// the maximum ranges
	int max_range;
	// build a pre-computed table for fast computing ?
	bool precompute;
	// pointer to the pre-computed table
	double **table;

	// sensor model parameters for linear combination of error models
	double z_hit, z_unexp, z_max, z_rand;

	// error model parameters
	double s_hit;			// for measurement noise
	double lambda;			// for unexpectedd object error
	double o_small;			// for sensor failures

    protected:
	// construct a pre-computed table
	void buildTable(void);

	// dump the pre-computed table into a file (debugging purpose)
	void dumpTable(const char* filename);

	// model measurement noise
	inline double measurementNoise(uint16_t observation, uint16_t estimation);

	// model unexpected objects
	inline double unexpectedObjectError(uint16_t observation);

	// model sensor failure (e.g. max range readings)
	inline double sensorFailure(uint16_t observation);

	// model random measurements (e.g. crosstalk)
	inline double randomMeasurement(void);

	// compute the probability : P( observation | estimation )
	double iProbability(uint16_t observation, uint16_t estimation);

    public:
	// constructor
	MCLSensorModel(mcl_sensor_t type,	// type of a distance sensor
		       int num_ranges,		// the number of ranges
		       pose_t* poses,		// the poses of ranges
		       uint16_t max_range,	// maximum range of the distance sensor
		       float s_hit,		// error model parameters
		       float lambda,
		       float o_small,
		       float z_hit,		// parameters for linear combination
		       float z_unexp,
		       float z_max,
		       float z_rand,
		       bool precompute=true);	// build a table for fast computing ?
	// destructor
	~MCLSensorModel(void);

	// compute the probability : P(o|s,m)
	double probability(uint16_t observation[],	// observations `o'
			   pose_t state,		// state `s'
			   WorldModel* map);		// map `m'
};


#endif
