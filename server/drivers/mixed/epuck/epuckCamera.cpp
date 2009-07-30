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

#include "epuckCamera.hpp"
#include <stdint.h>
#include <sstream>

std::string
EpuckCamera::camera_version_error::make_what(int wrongVersion)
{
  std::ostringstream out;

  out << "Unknown camera version: "
      << std::hex << std::showbase << wrongVersion;

  return out.str();
}

std::string
EpuckCamera::window_length_error::make_what(int maxWidth, int maxHeight)
{
  std::ostringstream out;

  out << "The image width and/or height has exceeded the maximum value.\n"
      << "The image width must has at most " << maxWidth << " pixels, "
      << "and the height " << maxHeight << " pixels.";

  return out.str();
}

EpuckCamera::EpuckCamera(const SerialPort* const serialPort,
                         unsigned sensor_x1, unsigned sensor_y1,
                         unsigned sensor_width, unsigned sensor_height,
                         unsigned zoom_fact_width, unsigned zoom_fact_height,
                         ColorModes color_mode) throw()
  :EpuckInterface(serialPort)
  ,sensor_x1(sensor_x1)
  ,sensor_y1(sensor_y1)
  ,sensor_width(sensor_width)
  ,sensor_height(sensor_height)
  ,zoom_fact_width(zoom_fact_width)
  ,zoom_fact_height(zoom_fact_height)
  ,color_mode(color_mode)
  ,tmpImageAllocated(false)
{
  unsigned bytePerPixel = 0;
  switch (color_mode)
  {
  case EpuckCamera::GREY_SCALE_MODE:
    this->bpp = bpp_8;
    bytePerPixel = 1;
    break;
  case EpuckCamera::RGB_565_MODE:
    this->bpp = bpp_16;
    bytePerPixel = 2;
    break;
  case EpuckCamera::YUV_MODE:
    this->bpp = bpp_16;
    bytePerPixel = 2;
    break;
  }

  this->imageByteSize = (sensor_width/zoom_fact_width) *
                        (sensor_height/zoom_fact_height) * bytePerPixel;

  this->rowPixelLength = sensor_width/zoom_fact_width;
  this->columnPixelLength = sensor_height/zoom_fact_height;
  this->imagePixelSize = this->rowPixelLength * this->columnPixelLength;
}

EpuckCamera::~EpuckCamera()
{
  if(this->tmpImageAllocated)
  {
    delete[] this->tmpImage;
  }
}

void
EpuckCamera::checkCameraParameters() const
{
  if(this->imageByteSize > 6500)
  {
    throw std::length_error("The e-puck camera image may not be larger than "
                            "6500 bytes");
  }

  unsigned maxWidth, maxHeight;
  if(this->epuckCameraVersion == EpuckCamera::PO3030K)
  {
    maxWidth = 480;
    maxHeight = 640;
  }
  else if(this->epuckCameraVersion == EpuckCamera::PO6030K)
  {
    maxWidth = 640;
    maxHeight = 480;
  }
  else
  {
    throw camera_version_error(this->epuckCameraVersion);
  }

  if( (this->sensor_width <= 0) || (this->sensor_height <= 0) )
  {
    throw window_length_error("The sensor width and height must be lager than zero.");
  }
  if((this->sensor_width > maxWidth) || (this->sensor_height > maxHeight))
  {
    throw window_length_error(maxWidth, maxHeight);
  }

  if( (this->sensor_x1 < 0) ||
      (this->sensor_y1 < 0) ||
      ((this->sensor_x1 + this->sensor_width) > maxWidth) ||
      ((this->sensor_y1 + this->sensor_height) > maxHeight) )
  {
    throw window_out_of_range("The windowing of e-puck camera image is out of range");
  }
}

void
EpuckCamera::Initialize()
{
  this->SendRequest(EpuckInterface::CONFIG_CAMERA);

  this->epuckCameraVersion = this->serialPort->recvUnsigned();
  this->checkCameraParameters();

  if(this->epuckCameraVersion == EpuckCamera::PO3030K)
  {
    this->serialPort->sendInt(this->sensor_y1);
    this->serialPort->sendInt(this->sensor_x1);
    this->serialPort->sendInt(this->sensor_height);
    this->serialPort->sendInt(this->sensor_width);
    this->serialPort->sendInt(this->zoom_fact_height);
    this->serialPort->sendInt(this->zoom_fact_width);
    this->serialPort->sendInt(this->color_mode);

    this->tmpImage = new unsigned char[this->imageByteSize];
    this->tmpImageAllocated = true;
  }
  else if(this->epuckCameraVersion == EpuckCamera::PO6030K)
  {
    this->serialPort->sendInt(this->sensor_x1);
    this->serialPort->sendInt(this->sensor_y1);
    this->serialPort->sendInt(this->sensor_width);
    this->serialPort->sendInt(this->sensor_height);
    this->serialPort->sendInt(this->zoom_fact_width);
    this->serialPort->sendInt(this->zoom_fact_height);
    this->serialPort->sendInt(this->color_mode);
  }
  else
  {
    throw camera_version_error(this->epuckCameraVersion);
  }

  this->serialPort->recvChar(); // Wait for e-puck to send an end of task signal.
}

std::string
EpuckCamera::GetCameraVersion() const
{
  std::string version;
  if(this->epuckCameraVersion == EpuckCamera::PO3030K)
  {
    version = "PO3030K";
  }
  else if(this->epuckCameraVersion == EpuckCamera::PO6030K)
  {
    version = "PO6030K";
  }
  else
  {
    throw camera_version_error(this->epuckCameraVersion);
  }

  return version;
}

void
EpuckCamera::GetCameraData(unsigned &imageWidth, unsigned &imageHeight,
                            EpuckCamera::ColorModes &colorMode) const
{
  imageWidth  = this->sensor_width/this->zoom_fact_width;
  imageHeight = this->sensor_height/this->zoom_fact_height;
  colorMode = this->color_mode;
}

void
EpuckCamera::GetImage(unsigned char* const ptrImage)
{
  this->SendRequest(EpuckInterface::GET_CAMERA_IMG);

  if(this->epuckCameraVersion == EpuckCamera::PO3030K)
  {
    this->serialPort->recvUnsignedCharArray(this->tmpImage,
                                            this->imageByteSize);

    if(this->bpp == EpuckCamera::bpp_8)
    {
      this->processTmpImage<uint8_t>((uint8_t*)ptrImage);
    }
    else // bpp == EpuckCamera::bpp_16
    {
      this->processTmpImage<uint16_t>((uint16_t*)ptrImage);
    }
  }
  else
  {
    this->serialPort->recvUnsignedCharArray(ptrImage, this->imageByteSize);
  }
}

// This method will rotate the image received from e-puck in 90 degrees
// counterclockwise. For perform this, the this->tmpImage image will be copied
// row by row (left to right, top to down) in ptrImage column by column (down
// to top, left to right).
//
// original    final
// --------    -----
//  0 1 2       2 5
//  3 4 5   ->  1 4
//              0 3
template <typename T>
void
EpuckCamera::processTmpImage(T* ptrImage) const
{
  // Necessary increment to shift from topmost pixel in a column to the most
  // below pixel in next column.
  const unsigned toNextColumn = 1 + this->rowPixelLength *
                               (this->columnPixelLength - 1);

  // Necessary increment to shift to the upper pixel in same column.
  const unsigned toUpperLine = -this->rowPixelLength;

  T* castPtr = (T*)this->tmpImage;
  unsigned targetPixel = this->rowPixelLength * (this->columnPixelLength - 1);
  for(unsigned sourcePixel = 0;
      sourcePixel < this->imagePixelSize;
      ++sourcePixel)
  {
    ptrImage[targetPixel] = castPtr[sourcePixel];

    if(targetPixel < this->rowPixelLength) //It's in top line
    {
      targetPixel += toNextColumn;
    }
    else
    {
      targetPixel += toUpperLine;
    }
  }
}
