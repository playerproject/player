/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2010
 *     Mayte Lázaro, Alejandro R. Mosteo
 *
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
 */


#ifndef ROBOTLOCATION_H_
#define ROBOTLOCATION_H_

#include "uloc.hh"
#include "segment_map.hh"
#include "scan.hh"
#include "feature.hh"

class RobotLocation {

public:
    RobotLocation(double odom_noise_x,
                  double odom_noise_y,
                  double odom_noise_th);

	// Initializers

    /// Inform the current odometric and global (map) pose
    void SetPoses(double ox, double oy, double oth,
                  double gx, double gy, double gth);
    /// Inform the error for the initial pose
    /// Robot is expected to be at x +/- (ex/2)
    void SetCurrentError(double ex, double ey, double eth);

	SegmentMap& map(void) { return map_; };
    const SegmentMap& map(void) const { return map_; };

	// Usage

	bool Locate(const Transf odom, const Scan s); // true if performed
	void PrintState() const;

	Pose   EstimatedPose(void) const;
	Matrix Covariance(void) const;

    // Constants
    const double odom_noise_x_;
    const double odom_noise_y_;
    const double odom_noise_th_;
private:
	void Prediction();
	void Update(ObservedFeatures obs);

	Uloc XwRk_1;
    Uloc XwRk;
    Transf odomk_1;
    Transf odomk;
    SegmentMap map_;

    bool first_update_;
};

#endif /* ROBOTLOCATION_H_ */
