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

// pose
struct pose_t
{
    float x;			// x position
    float y;			// y position
    float a;			// heading
};


// particle
struct particle_t
{
    pose_t pose;		// robot pose on a given occupancy map
    double importance;		// importance factor
    double cumulative;		// cumulative importance factor - for importance sampling
};


// valid distance sensors
enum mcl_sensor_t { PLAYER_MCL_SONAR,		// sonar
    		    PLAYER_MCL_LASER,		// laser rangefinder
		    PLAYER_MCL_NOSENSOR };	// no sensor (error)


// macros
#define SQR(x) ((x)*(x))
#define SQR2(x,y) (((x)-(y)) * ((x)-(y)))
#define D2R(a) ((float)((a)*M_PI/180))
#define R2D(a) ((int)((a)*180/M_PI))


// inline functions
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
