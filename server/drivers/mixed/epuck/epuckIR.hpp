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

#ifndef EPUCK_IR_HPP
#define EPUCK_IR_HPP

#include "epuckInterface.hpp"
#include <vector>

/** \file
 *  Header file of EpuckIR class and struct EpuckIR::IRData.
 */

/** Class for to get data from e-puck IR sensors.
 *
 *  \author Renato Florentino Garcia.
 *  \date August 2008
 */
class EpuckIR : public EpuckInterface
{
public:

  /// The number of IR sensors on e-puck.
  static const unsigned SENSOR_QUANTITY = 8;

  /// Represents the data got from e-puck IR sensors.
  struct IRData
  {
    std::vector<float> voltages; ///< The raw IR readings.
    std::vector<float> ranges;   ///< The equivalent obstacle distance.
  };


  /** The EpuckIR class constructor.
   *
   * @param serialPort Pointer for a SerialPort class already created
   *                   and connected with an e-puck.
   *
   */
  EpuckIR(const SerialPort* const serialPort);

  /** Read the IR sensors.
   *
   * Read the values of IR sensor from e-puck, and translate it for distance
   * in meters.
   * @return A IRData struct with the read values.
   */
  IRData GetIRData() const;

  /** Give the geometry of each IR sensor in e-puck.
   *
   * @return A std::vector with the sensors geometry.
   */
  inline std::vector<EpuckInterface::Triple> GetGeometry() const
  {
    return this->geometry;
  }

private:

  std::vector<EpuckInterface::Triple> geometry;
};

#endif
