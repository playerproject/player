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

#ifndef SERIAL_PORT_HPP
#define SERIAL_PORT_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <sys/time.h>
#include <termios.h>

/** \file
 * Header file of SerialPort class.
 */

/** Send and receive messages from e-puck.
 *
 * This class connects and communicates with the e-puck robot through the
 * serial interface. Only one instance must be created and shared among
 * the device interfaces.
 *
 * \warning Not thread safe
 * \author Renato Florentino Garcia
 * \date August 2008

 */
class SerialPort
{
public:

  /** Constructor of SerialPort class.
   *
   * This constructor only creates the object, it don't open or initialize
   * the serial port device.
   * @param serialPort A absolute path for the serial port device
   *                   (e.g. "/dev/rfcomm0").
   */
  SerialPort(std::string &serialPort);

  ~SerialPort();

  /** Return the last error.
   *
   * Return the description of last error.
   * @return A std::string describing the error.
   */
  inline std::string getError() const
  {
    return this->errorDescription;
  }

  /// Open the serial port device and set the configurations for it.
  int initialize();

  /** Receive a signed interger from e-puck.
   *
   * Wait indefinitely two bytes to arrive from e-puck and converts them to
   * signed integer.
   * @return The received signed integer.
   */
  int recvInt() const;

  /** Receive an unsigned interger from e-puck.
   *
   * Wait indefinitely two bytes to arrive from e-puck and converts them to
   * unsigned integer.
   * @return The received unsigned integer.
   */
  unsigned recvUnsigned() const;

  /** Receive a char from e-puck.
   *
   * Wait indefinitely one byte to arrive from e-puck and converts them to
   * char.
   * @return The received char.
   */
  char recvChar() const;

  /** Receive an array of unsigned char from e-puck.
   *
   * @param array Pointer to array where the received char will be put.
   * @param length Length of array that will be received.
   */
  int recvUnsignedCharArray(unsigned char* const array, unsigned length) const;

  /** Send an integer to e-puck.
   *
   * Send two bytes to e-puck. It's an integer number up to 65535.
   * @param message The integer to be sent.
   */
  void sendInt(int message) const;

  /** Send a character to e-puck.
   *
   * Send one character to e-puck.
   * @param message The charecter to be sent.
   */
  void sendChar(char message) const;

private:

  int fileDescriptor;

  std::string serialPort;

  // Describe the last error.
  std::string errorDescription;

  // Backup the tremios struct for reload it on exit.
  struct termios termios_backup;
};

#endif
