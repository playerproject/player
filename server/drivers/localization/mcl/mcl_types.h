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
 *   common data structures
 */
#ifndef _MCLTYPES_H
#define _MCLTYPES_H

#include <player.h>


/*****************************************************************************
 * constants
 *****************************************************************************/

#ifdef INCLUDE_STAGE

#define MCL_MAX_PARTICLES 100000		// packet size must be fixed

#define MCL_CMD_CONFIG	0			// command 'send configuration'
#define MCL_CMD_UPDATE	1			// command 'update'
#define MCL_CMD_STOP	2			// command 'stop update'

#endif


/*****************************************************************************
 * common data structures
 *****************************************************************************/

// pose
typedef struct pose
{
    float x;			// x position
    float y;			// y position
    float a;			// heading
} __attribute__ ((packed)) pose_t;


// particle
typedef struct particle
{
    pose_t pose;		// robot pose on a given occupancy map
    double importance;		// importance factor
    double cumulative;		// cumulative importance factor - for importance sampling
} __attribute__ ((packed)) particle_t;


// valid distance sensors
enum mcl_sensor_t { PLAYER_MCL_SONAR,		// sonar
    		    PLAYER_MCL_LASER,		// laser rangefinder
		    PLAYER_MCL_NOSENSOR };	// no sensor (error)


// configuration
typedef struct mcl_config
{
    float frequency;			// update speed
    unsigned int num_particles;		// the number of particles

    mcl_sensor_t sensor_type;		// distance sensor
    int sensor_index;			// distance sensor index
    uint16_t sensor_max;		// the maximum range of the sensor
    uint16_t sensor_num_ranges;		// the number of ranges of the snesor

    int motion_index;			// motion sensor index

    char map_file[PATH_MAX];		// the path of the map file
    uint16_t map_ppm;			// pixels per meter
    uint8_t map_threshold;		// empty threshold

    float sm_s_hit;			// sensor model parameter: sm_s_hit
    float sm_lambda;			// sensor model parameter: sm_lambda
    float sm_o_small;			// sensor model parameter: sm_o_small
    float sm_z_hit;			// sensor model parameter: sm_z_hit
    float sm_z_unexp;			// sensor model parameter: sm_z_unexp
    float sm_z_max;			// sensor model parameter: sm_z_max
    float sm_z_rand;			// sensor model parameter: sm_z_rand
    int sm_precompute;			// pre-compute sensor models ?

    float am_a1;			// action model parameter: am_a1
    float am_a2;			// action model parameter: am_a2
    float am_a3;			// action model parameter: am_a3
    float am_a4;			// action model parameter: am_a4
} __attribute__ ((packed)) mcl_config_t;


#ifdef INCLUDE_STAGE

// hypothesis type for internal use
typedef struct mcl_hypothesis
{
    double mean[3];			// mean of a hypothesis
    double cov[3][3];			// covariance matrix
    double alpha;			// coefficient for linear combination
} __attribute__ ((packed)) mcl_hypothesis_t;


// data coming from Stage
typedef mcl_config_t stage_data_t;


// command going to Stage
typedef struct stage_command
{
    bool command;			// is a request message ?
    uint32_t num_hypothesis;
    mcl_hypothesis_t hypothesis[PLAYER_LOCALIZATION_MAX_HYPOTHESIS];
    uint32_t num_particles;
    particle_t particles[MCL_MAX_PARTICLES];
} __attribute__ ((packed)) stage_command_t;

#endif


/*****************************************************************************
 * macros
 *****************************************************************************/

#define SQR(x) ((x)*(x))
#define SQR2(x,y) (((x)-(y)) * ((x)-(y)))
#define D2R(a) ((float)((a)*M_PI/180))
#define R2D(a) ((int)((a)*180/M_PI))


/*****************************************************************************
 * inline functions
 *****************************************************************************/

inline bool operator < (const particle_t& x, const particle_t& y) {
    return x.importance < y.importance;
}

inline bool operator == (const particle_t& x, const particle_t& y) {
    return x.importance == y.importance;
}

inline bool operator == (const pose_t& p, const pose_t& q) {
    return (p.x == q.x && p.y == q.y && p.a == q.a);
}

inline bool operator != (const pose_t& p, const pose_t& q) {
    return (p.x != q.x || p.y != q.y || p.a != q.a);
}


#endif
