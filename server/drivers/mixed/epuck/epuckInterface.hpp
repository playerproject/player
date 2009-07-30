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

#ifndef EPUCK_INTERFACE_HPP
#define EPUCK_INTERFACE_HPP

#include "serialPort.hpp"

/** \file
 * Header file of the EpuckInterface class and the struct
 * EpuckInterface::Triple.
 */

/** Base class for all concrete interfaces of e-puck.
 *
 * This class must be inherited by every class which implement a concrete
 * e-puck interface. It's a abstract class only, and can't be instanced.
 * \author Renato Florentino Garcia
 * \date August 2008
 */
class EpuckInterface
{
public:

  /// Struct which represents a triple (x,y,theta).
  struct Triple
  {
    float x;     ///< x component.
    float y;     ///< y component.
    float theta; ///< theta component.
  };

protected:

  /** The EpuckInterface class constructor
   *
   * @param serialPort Pointer for a SerialPort class already created and
   *                   connected with an e-puck.
   */
  EpuckInterface(const SerialPort* const serialPort);

  /** A SerialPort class instance shared among the device interfaces.
   *
   * This pointer points to the only instance of SerialPort Class. Each
   * interface will be this pointer pointed to the same instance.
   */
  const SerialPort* const serialPort;

  /// Diameter of e-puck body [m].
  static const float EPUCK_DIAMETER;

  /// Request codes acceptable by e-puck.
  enum Request
  {
    CONFIG_CAMERA  = 0x02, ///< Send configurations for camera initialization.
    SET_VEL        = 0x13, ///< Send motors steps per second velocity.
    GET_STEPS      = 0x14, ///< Receive the steps made and reset the stepp counter.
    STOP_MOTORS    = 0x15, ///< Stop the motors.
    GET_IR_PROX    = 0x16, ///< Receive the IR sensors readings.
    GET_CAMERA_IMG = 0x17, ///< Receive an image from camera.
    SET_LED_POWER  = 0x18  ///< Send the state of all LEDs.
  };

  void SendRequest(Request request) const;
};

#endif
