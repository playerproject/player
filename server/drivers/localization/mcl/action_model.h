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
 *   Parameterized action models. This class will provide various models for
 *   various actuators.
 */
#ifndef _ACTIONMODEL_H
#define _ACTIONMODEL_H

#include "mcl_types.h"

#include <stdint.h>


/*
 * Odometry-based action model for Monte-Carlo Localization algorithms. The model
 * draws a sample from the probability P(s'|s,a) where "s" is a pose state, "a"
 * is an action performed, and "s'" is a new pose state.
*/
class MCLActionModel
{
    private:
	// action model parameters
	float a1, a2, a3, a4;

    protected:
	// sampling method
	float normal(float b);

	// abs() function that also does range conversion
	double abs(double angle);

    public:
	// constructor
	MCLActionModel(float a1, float a2, float a3, float a4);

	// draw a sample from P(s'|s,a)
	pose_t sample(pose_t state,			// state "s"
		      pose_t from, pose_t to);		// action "a"
};


#endif
