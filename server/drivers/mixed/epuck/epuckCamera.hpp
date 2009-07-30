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

#ifndef EPUCK_CAMERA_HPP
#define EPUCK_CAMERA_HPP

#include "epuckInterface.hpp"
#include <stdexcept>
#include <string>

/** \file
 * Header file where are the EpuckCamera class, and the exception classes
 * camera_version_error, window_out_of_range and window_length_error.
 */

/** Class for to get images from e-puck camera.
 *
 * \exception camera_version_error
 * Raised when the version of camera in e-puck is unknown.
 *
 * \exception window_length_error
 * Raised when the desired image window width or height is larger that e-puck
 * camera can support.
 *
 * \exception window_out_of_range
 * Raised when the positioning of image window is out of image sensor border.
 *
 *  \author Renato Florentino Garcia.
 *  \date October 2008
 */
class EpuckCamera : public EpuckInterface
{
public:

  class camera_version_error : public std::logic_error
  {
  public:
    camera_version_error(int wrongVersion)
      :logic_error(make_what(wrongVersion)){}
  private:
    std::string make_what(int wrongVersion);
  };

  class window_out_of_range : public std::out_of_range
  {
  public:
    window_out_of_range(const std::string& whatArg)
      :out_of_range(whatArg){}
  };

  class window_length_error : public std::length_error
  {
  public:
    window_length_error(const std::string& whatArg)
      :length_error(whatArg){}
    window_length_error(int maxWidth, int maxHeight)
      :length_error(make_what(maxWidth, maxHeight)){}
  private:
    std::string make_what(int maxWidth, int maxHeight);
  };

  /** Possible color modes for e-puck camera.
   *
   * This are the possible color modes for e-puck camera. The YUV_MODE is
   * unknown by Player, because of that, in this mode the variable "format"
   * of playerc_camera_t struct, will be set to PLAYER_CAMERA_FORMAT_MONO16.
   */
  enum ColorModes
  {
    GREY_SCALE_MODE = 0, ///< Grey color mode, with 8 bits per pixel.
    RGB_565_MODE    = 1, ///< RGB color mode, with 16 bits per pixel.
    YUV_MODE        = 2  ///< YUV color mode, with 16 bits per pixel.
  };

  /** The EpuckCamera class constructor.
   *
   * Except by serialPort, the other variables are mapped directly to variables
   * with same name in e_po3030k_config_cam function, from official e-puck
   * library.
   * @param serialPort Pointer for a SerialPort class already created
   *                   and connected with an e-puck.
   * @param sensor_x1 The X coordinate of the window's corner.
   * @param sensor_y1 The Y coordinate of the window's corner.
   * @param sensor_width The Width of the interest area, in FULL sampling
   *                     scale.
   * @param sensor_height The Height of the insterest area, in FULL
   *                      sampling scale .
   * @param zoom_fact_width The subsampling to apply for the window's width.
   * @param zoom_fact_height The subsampling to apply for the window's height.
   * @param color_mode The color mode in which the camera should be configured.
   */
  EpuckCamera(const SerialPort* const serialPort, unsigned sensor_x1,
              unsigned sensor_y1, unsigned sensor_width,
              unsigned sensor_height, unsigned zoom_fact_width,
              unsigned zoom_fact_height, ColorModes color_mode) throw();

  ~EpuckCamera();

  /** Send the configurations givens in EpuckCamera constructor to e-puck.
   *
   * This function must be called before an image be captured.<br>
   * It can throw the following exceptions: std::length_error,
   * camera_version_error, window_length_error and window_out_of_range.
   */
  void Initialize();

  /** Get the version of camera in e-puck.
   *
   * This method must be called after the Initialize method.<br>
   * It can throw the camera_version_error exception.
   * @return A string representing the camera version.
   */
  std::string GetCameraVersion() const;

  /** Get the relevant configurations camera data.
   *
   * @param imageWidth Image width in pixels.
   * @param imageHeight Image height in pixels.
   * @param colorMode Image color mode.
   */
  void GetCameraData(unsigned &imageWidth, unsigned &imageHeight,
                     EpuckCamera::ColorModes &colorMode) const;

  /** Get a new image from e-puck.
   *
   * @param ptrImage A pointer to a const char vector, where the
   *                 incoming image will be put it.
   */
  void GetImage(unsigned char* const ptrImage);

private:

  static const unsigned PO3030K = 0x3030;
  static const unsigned PO6030K = 0x6030;

  unsigned sensor_x1;
  unsigned sensor_y1;
  unsigned sensor_width;
  unsigned sensor_height;
  unsigned zoom_fact_width;
  unsigned zoom_fact_height;
  ColorModes color_mode;

  enum{
    bpp_8,
    bpp_16
  }bpp;
  unsigned imageByteSize;

  unsigned imagePixelSize;
  unsigned rowPixelLength;
  unsigned columnPixelLength;

  unsigned epuckCameraVersion;

  unsigned char* tmpImage;
  bool tmpImageAllocated;

  // Check if the given camera parameters will be accepted by e-puck, and
  // throw exceptions otherwise.
  void checkCameraParameters() const;

  // Copy the tmpImage in ptrImage, rotating 90 degrees counterclockwise.
  template <typename T>
  void processTmpImage(T* ptrImage) const;

};

#endif /* EPUCK_CAMERA_HPP */
