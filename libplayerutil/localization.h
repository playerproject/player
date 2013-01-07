/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 ********************************************************************/

/***************************************************************************
 * Desc: Localization helpers
 * Author: Thimo Langbehn <thimo@g4t3.de>
 * Date: 05 July 2010
 **************************************************************************/

#ifndef LOCALIZATION_HH
#define LOCALIZATION_HH

#include <libplayerinterface/player.h>
#include <math.h>

/** Calculates the 2d uncertainty ellipsis parameters for a given hypothesis
 *
 *  This function can be used to calculate the parameters for an uncertainty 
 *  ellipsis to a given, two dimensional gaussian distribution. The function
 *  only requires the values of a two dimensional covariance matrix, however
 *  to it makes sense (semantically) to use the player_localize_hypoth_t
 *  structure as parameter instead of a plain array.
 * 
 *  The unccertainty ellipsis derived from the hypothesis is a projection
 *  of the 3-dimensional (x,y,angle) ellipsoid into a 2-dimensional ellipsis
 *  (x,y), ignoring variance and covariances related to the angle.
 * 
 *  The probability that is to be covered by the ellipsis (i.e. the scale) 
 *  can be controlled by the parameter probability_coverage. Pay attention to
 *  the semantics, this is not the probability of the 2d-Gaussian on the 
 *  border of the ellipsis, in fact it is the integral of the 2d-gaussian 
 *  enclosed in the ellipsis.
 *
 *  \param ellipse_pose (out) will store the ellipsis position and angle
 *  \param radius_x (out) will store the first ellipsis radius
 *  \param radius_y (out) will store the second ellipsis radius
 *  \param hypothesis (in) contains the covariance matrix used
 *  \param probability_coverage (in) contains a value in the range [0,1]
 *    that represents the probability covered by the ellipsis. 
 */
void
derive_uncertainty_ellipsis2d(player_pose2d_t* ellipse_pose,
			      double* radius_x, double* radius_y,
			      player_localize_hypoth_t* hypothesis,
			      double probability_coverage);

#endif //LOCALIZATION_HH
