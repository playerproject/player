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


#ifndef SCAN_H_
#define SCAN_H_

#include <vector>
#include "types.hh"
#include "uloc.hh"

class Scan
{
public:

    /// Provide laser parameters: maximum range and its pose on top of robot,
    /// and noise model.
    Scan(double max_range,
         double laser_x,
         double laser_y,
         double laser_angle,
         double laser_noise_range,
         double laser_noise_bearing);

    const Uloc & uloc(const int i) const; // read only access

    /// Update laser pose
    void SetLaserPose(double x, double y, double a);

    /// Set last laser reading
    /// Removes out of range values and attaches the uncertainty model
    void SetLastScan(const DoublesVector& ranges,
                     const DoublesVector& bearings);

    int ScanCount(void) const;

    double phi(const int i) const;
    double rho(const int i) const;

    const double kOutOfRange_;
    const double kLaserNoiseRange_;
    const double kLaserNoiseBearing_;
private:
    Uloc AttachReferenceToScanPoint(double rho, double phi);

    void addUloc(Uloc u);
    vector<Uloc> ulocs_;

    DoublesVector rho_; // Distance
    DoublesVector phi_; // Bearing

    Transf xform_laser_to_robot_;
};

#endif /* SCAN_H_ */
