/* Copyright 2008 Renato Florentino Garcia <fgar.renato@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EPUCK_POSITION2D_HPP
#define EPUCK_POSITION2D_HPP

#include "epuckInterface.hpp"
#include "timer.hpp"

/** \file
 * Header file of the EpuckPosition2d class, and the
 * EpuckPosition2d::BodyGeometry and EpuckPosition2d::DynamicConfiguration
 * structs.
 */

/** It controls the e-puck velocity and odometry.
 *
 *  \author Renato Florentino Garcia
 *  \date August 2008
 */
class EpuckPosition2d : public EpuckInterface
{
public:

  /// Struct which represents the geometry of e-puck body.
  struct BodyGeometry
  {
    float width; ///< Width of e-puck body.
    float height; ///< Height of e-puck body.
  };

  /// Struct which represents the pose and velocity of e-puck.
  struct DynamicConfiguration
  {
    Triple pose; ///< E-puck pose in a 2D surface.
    Triple velocity; ///< E-puck velocity in a 2D surface.
  };

  /** The EpuckPosition2d class constructor.
   *
   * @param serialPort Pointer for a SerialPort class already created and
   *                   connected with an e-puck.
   */
  EpuckPosition2d(const SerialPort* const serialPort);

  /** Set linear and angular velocities to e-puck.
   *
   * Receive the linear and agular velocities, convert to correspondent
   * steps per seconds motors commands and send to e-puck.
   * @param px Linear velocity. [m/s]
   * @param pa Angular velocity. [rad/s]
   */
  void SetVel(float px, float pa) const;

  /** Estimate the current pose and velocities.
   *
   * Receive from e-puck the motors steps made since the last call at this
   * function, and estimate the new current pose and velocities.
   *
   * @return A DynamicConfiguration struct.
   */
  DynamicConfiguration UpdateOdometry();

  /** Set the current pose estimated by odometry.
   *
   * Set the internal representation for e-puck current pose estimated
   * from odometry to the given value.
   * @param odometry The value which will be set.
   */
  void SetOdometry(Triple odometry);


  /** Set the current pose estimated by odometry - (x,y,theta) - to (0,0,0).
   */
  void ResetOdometry();

  /** Stop the e-puck motors.
   *
   * Stop the two motors and update the odometry by the steps yet non
   * computed.
   */
  void StopMotors() const;

  /** Give the e-puck geometry.
   *
   * @return A BodyGeometry struct.
   */
  inline BodyGeometry GetGeometry() const
  {
    return this->geometry;
  }

private:

  EpuckTimer timer;

  // Holder for current position data estimated by e-puck robot odometry.
  DynamicConfiguration odometryState;

  BodyGeometry geometry;

  // Diameter of e-puck wheels [m]
  static const float WHEEL_DIAMETER;

  // Distance between e-puck wheels [m]
  static const float TRACK;

  // Wheel radius divided by TRACK [m]
  static const float r_DIV_L;

  // Half of wheel radius [m]
  static const float r_DIV_2;

  // Angular displacement of one motor step [rad]
  static const float STEP_ANG_DISP;
};

#endif

