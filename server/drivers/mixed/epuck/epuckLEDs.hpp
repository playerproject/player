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

#ifndef EPUCK_LEDS_HPP
#define EPUCK_LEDS_HPP

#include "epuckInterface.hpp"

/** \file
 * Header file where are the EpuckLEDs class and the EpuckLEDs::LEDstate
 * struct.
 */

/** This class controls the turn on and turn off of e-puck LEDs.
 *
 * \author Renato Florentino Garcia
 * \date December 2008
 */
class EpuckLEDs : public EpuckInterface
{
public:

  /// The quantity of LEDs in e-puck ring LEDs.
  static const unsigned RING_LEDS_NUM = 8;

  /// Struct with the state of e-puck LEDs.
  struct LEDstate
  {
    bool ring[RING_LEDS_NUM]; ///< The state of each of eighth ring LEDs.
    bool front;               ///< The state of front LED.
    bool body;                ///< The state of body LED.
  };

  /** EpuckLEDs class constructor
   *
   * @param serialPort Pointer for a SerialPort class already created and
   *                   connected with an e-puck.
   */
  EpuckLEDs(const SerialPort* const serialPort);

  /** Set the e-puck ring LEDs.
   *
   * The state of all ring LEDs is given in ringLED parameter.
   * @param ringLED The state of each ring LED, where true is LED on.
   */
  void SetRingLED(bool ringLED[RING_LEDS_NUM]);

  /** \overload
   *
   *
   * The state of only one LED will be set, the others won't change.
   * @param id The index of the ring LED.
   * @param state The desired state. True for on, false for off.
   */
  void SetRingLED(unsigned id, bool state);

  /** Set the e-puck front LED.
   *
   * @param state True for on, false for off.
   */
  void SetFrontLED(bool state);

  /** Set the e-puck body LED.
   *
   * @param state True for on, false for off.
   */
  void SetBodyLED(bool state);

  /** Turn off the internal representation of all e-puck LEDs.
   *
   * This function turn off only the internal representation of the LEDs, in
   * e-puck their state won't change. This is useful in unsubscribe operation,
   * at end of program the LEDs won't chage. But in a new client program
   * execution, only the desired LEDs will tuned on.
   */
  void ClearInternal();

private:

  LEDstate ledState;

  void SendLEDState() const;
};


#endif /* EPUCK_LEDS_HPP */
