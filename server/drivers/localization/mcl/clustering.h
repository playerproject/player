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
 *   Clustering algorithm. This class utilize EM algorithm to estimate
 *   particle sets using a Gaussian mixture. Since it should run in
 *   real-time, the clustring algorithm stops when it reaches the maximum
 *   iteration, even though it hasn't converge yet.
 */
#ifndef _CLUSTERING_H
#define _CLUSTERING_H

#include "mcl_types.h"

#include <player.h>

#include <vector>
#include <stdint.h>
using std::vector;


/*
 *   Clustering algorithm that utilize EM algorithm to estimate
 *   particle sets using a Gaussian mixture.
 */
class MCLClustering
{
    public:
	// the parameters of mixure gaussian
	double mean[PLAYER_LOCALIZATION_MAX_HYPOTHESIS][3];
	double cov[PLAYER_LOCALIZATION_MAX_HYPOTHESIS][3][3];
	double pi[PLAYER_LOCALIZATION_MAX_HYPOTHESIS];

    private:
	// input size (the number of particles)
	uint32_t N;
	// the number of iteration for each step
	uint32_t iteration;

	// input matrix
	double (*X)[3];
	// hidden variables
	double (*Z)[PLAYER_LOCALIZATION_MAX_HYPOTHESIS];
	// likelihood
	double (*likelihood)[PLAYER_LOCALIZATION_MAX_HYPOTHESIS];

	// constant variable
	const static double C1;
	double width_ratio;
	double height_ratio;
	double yaw_ratio;
	double cov_ratio;
	double coefficient;

    protected:
	// compute the determinant of a 3x3 covariance matrix
	inline double determinant(const double cov[][3]);

	// compute the inverse matrix of a 3x3 covariance matrix
	inline void inverse(const double cov[][3], double inv[][3]);

	// compute the (3D) multivariate gaussian 
	inline double gaussian(const double x[], const double mean[], const double cov[][3]);

	// estimate model parameters based on the current grouping (E-step)
	void estimate(void);

	// maximize the expected complete log-likelihood (M-step)
	void maximize(void);

	// EM procedure to model the distribution of a given particle set
	// using a mixture of Gaussians
	void EM(void);

    public:
	// constructor
	MCLClustering(uint32_t N=0, uint32_t iteration=10);
	// destructor
	~MCLClustering(void);

	// initialize the parameters
	void Reset(uint32_t width, uint32_t height);

	// group the particles using a fixed number of Gaussians
	void cluster(vector<particle_t>& particles);
};


#endif
