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

#include "epuckPosition2d.hpp"
#include <math.h>
#include <string.h>

#define PI       3.141592654
#define DOIS_PI  6.283185308

// Diameter of e-puck wheels [m]
const float EpuckPosition2d::WHEEL_DIAMETER = 0.0412;
// Distance between e-puck wheels [m]
const float EpuckPosition2d::TRACK = 0.05255;
// Wheel radius divided by TRACK [m]
const float EpuckPosition2d::r_DIV_L = 0.392007612;
// Half of wheel radius [m]
const float EpuckPosition2d::r_DIV_2 = 0.0103;
// Angular displacement of one motor step [rad]
const float EpuckPosition2d::STEP_ANG_DISP = 6.283185308e-3;

EpuckPosition2d::EpuckPosition2d(const SerialPort* const serialPort)
  :EpuckInterface(serialPort)
{
   this->geometry.width  = EpuckInterface::EPUCK_DIAMETER;
   this->geometry.height = EpuckInterface::EPUCK_DIAMETER;
}

void
EpuckPosition2d::SetVel(float px, float pa) const
{
  // Angular speed for each wheel [rad/s]
  float angSpeedRw = ( 2*px + TRACK*pa )/( WHEEL_DIAMETER );
  float angSpeedLw = ( 2*px - TRACK*pa )/( WHEEL_DIAMETER );

  // Speed for each motor [steps/s]
  int stepsR = (int)( ( 1000*angSpeedRw )/ DOIS_PI );
  int stepsL = (int)( ( 1000*angSpeedLw )/ DOIS_PI );

  if (stepsR > 1000)
  {
    stepsR = 1000;
//     PLAYER_WARN("Rotational speed from epuck motor saturated in maximum value");

  }else if (stepsR < -1000)
  {
    stepsR = -1000;
//     PLAYER_WARN("Rotational speed from epuck motor saturated in maximum value");
  }

  if (stepsL > 1000)
  {
    stepsL = 1000;
//     PLAYER_WARN("Rotational speed from epuck motor saturated in maximum value");

  }else if (stepsL < -1000)
  {
    stepsL = -1000;
//     PLAYER_WARN("Rotational speed from epuck motor saturated in maximum value");
  }

  this->SendRequest(EpuckInterface::SET_VEL);

  this->serialPort->sendInt(stepsR);
  this->serialPort->sendInt(stepsL);

  this->serialPort->recvChar(); // Wait for e-puck to send an end of task signal.
}

void
EpuckPosition2d::SetOdometry(Triple odometry)
{
  // Send this message for side effect of reset the step counters on e-puck.
  this->SendRequest(EpuckInterface::GET_STEPS);
  this->serialPort->recvInt();
  this->serialPort->recvInt();

  // Overwrite the current odometric pose.
  this->odometryState.pose.x     = odometry.x;
  this->odometryState.pose.y     = odometry.y;
  this->odometryState.pose.theta = odometry.theta;
}


void
EpuckPosition2d::ResetOdometry()
{
  memset(&this->odometryState, 0, sizeof(DynamicConfiguration));
}

void
EpuckPosition2d::StopMotors() const
{
  this->SendRequest(EpuckInterface::STOP_MOTORS);

  this->serialPort->recvChar(); // Wait for e-puck to send an end of task signal.
}

EpuckPosition2d::DynamicConfiguration
EpuckPosition2d::UpdateOdometry()
{
  this->SendRequest(EpuckInterface::GET_STEPS);
  int steps_right = this->serialPort->recvInt();
  int steps_left = this->serialPort->recvInt();

  double delta_theta = r_DIV_L * STEP_ANG_DISP * (steps_right - steps_left);

  // Linear displacement, in direction of last theta
  double delta_l = r_DIV_2 * STEP_ANG_DISP * (steps_right + steps_left);

  // delta_l components on global system coordinates
  double delta_x = delta_l * cos(this->odometryState.pose.theta);
  double delta_y = delta_l * sin(this->odometryState.pose.theta);

  double deltaTime = this->timer.intervalDelay();
  this->timer.resetInterval();
  this->odometryState.velocity.x     = delta_x/deltaTime;
  this->odometryState.velocity.y     = delta_y/deltaTime;
  this->odometryState.velocity.theta = delta_theta/deltaTime;

  this->odometryState.pose.x     += delta_x;
  this->odometryState.pose.y     += delta_y;
  this->odometryState.pose.theta += delta_theta;
  if(this->odometryState.pose.theta > PI)
  {
    this->odometryState.pose.theta -= DOIS_PI;
  }
  if(this->odometryState.pose.theta < -PI)
  {
    this->odometryState.pose.theta += DOIS_PI;
  }

  return this->odometryState;
}
