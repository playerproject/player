/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: Read image sequence
// Author: Andrew Howard
// Date: 24 Sep 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_imageseq imageseq

The imageseq driver simulates a camera by reading an image sequence
from the filesystem.  The filenames for the image sequence must be
numbered; e.g.: "image_0000.pnm", "image_0001.pnm", "image_0002.pnm",
etc.

Note that only grayscale images are currently supported.

@todo Change this driver to use OpenCV instead of GDAL, and add
support for color images.

@par Interfaces
- This driver supports the @ref player_interface_camera interface.

@par Supported configuration requests

- None.

@par Configuration file options

- rate
  - Data rate (Hz); e.g., rate 20 will generate data at 20Hz.

- pattern "image_%04d.pnm"
  - A printf-style format string for describing the image filenames; the
  format string must contain at most one integer argument.

@par Example 

@verbatim
driver
(
  name "imageseq"
  devices ["camera:1"]
  rate 10
  pattern "image_%04d.pnm"
)
@endverbatim
*/
/** @} */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <math.h>

#include <gdal_priv.h>

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"
#include "playertime.h"

#include "playerpacket.h"



class ImageSeq : public Driver
{
  // Constructor
  public: ImageSeq( ConfigFile *cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Read an image
  private: int LoadImage(const char *filename);
  
  // Write camera data
  private: void WriteData();
    
  // Data rate
  private: double rate;

  // Sequence format string
  private: const char *pattern;

  // Frame number
  private: int frame;

  // Output camera data
  private: player_camera_data_t data;
};


// Initialization function
Driver *ImageSeq_Init(ConfigFile *cf, int section)
{
  return ((Driver*) (new ImageSeq(cf, section)));
}

// Driver registration function
void ImageSeq_Register(DriverTable *table)
{
  table->AddDriver("imageseq", ImageSeq_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
ImageSeq::ImageSeq(ConfigFile *cf, int section)
  : Driver(cf, section, PLAYER_CAMERA_CODE, PLAYER_READ_MODE,
           sizeof(player_camera_data_t), 0, 10, 10)
{
  // Data rate
  this->rate = cf->ReadFloat(section, "rate", 10);

  // Format string
  this->pattern = cf->ReadString(section, "pattern", "image_%06d.pnm");

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int ImageSeq::Setup()
{
  // Register all known drivers for GDAL
  GDALAllRegister();
  
  // Start at frame 0
  this->frame = 0;
    
  // Start the driver thread.
  this->StartThread();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int ImageSeq::Shutdown()
{
  // Stop the driver thread
  StopThread();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void ImageSeq::Main()
{
  char filename[1024];
  struct timespec req;

  req.tv_sec = (time_t) (1.0 / this->rate);
  req.tv_nsec = (long) (fmod(1e9 / this->rate, 1e9));

  while (1)
  {
    pthread_testcancel();
    if (nanosleep(&req, NULL) == -1)
      continue;    
    
    // Test if we are suppose to cancel this thread.
    pthread_testcancel();

    // Compose filename
    snprintf(filename, sizeof(filename), this->pattern, this->frame);

    // Load the image
    if (this->LoadImage(filename) != 0)
      break;

    // Write new camera data
    this->WriteData();
    this->frame++;          
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
/// Load an image
/// @todo Change this to use OpenCV for image loading
int ImageSeq::LoadImage(const char *filename)
{
  int i, bandCount;
  GDALDataset *dataSet;
  GDALRasterBand *band;
  
  dataSet = (GDALDataset*) GDALOpen(filename, GA_ReadOnly);
  if (!dataSet)
    return -1;

  bandCount = dataSet->GetRasterCount();

  // For one band, assume this is an greyscale image
  if (bandCount == 1)
  {
    this->data.width = dataSet->GetRasterXSize();
    this->data.height = dataSet->GetRasterYSize();
    this->data.depth = 8;
    this->data.format = PLAYER_CAMERA_FORMAT_GREY8;
    this->data.image_size = this->data.width * this->data.height * this->data.depth / 8;
  }

  // For three bands, assume this is an RGB image
  else if (bandCount == 3)
  {
    // TODO
    PLAYER_ERROR("unsupported image format");
    return -1;
  }
  else
  {
    PLAYER_ERROR("unsupported image format");
    return -1;
  }

  // Check image size
  if (this->data.image_size > PLAYER_CAMERA_IMAGE_SIZE)
  {
    PLAYER_ERROR1("image size is too large [%d]", this->data.image_size);
    return -1;
  }

  for (i = 0; i < bandCount; i++)
  {
    // Get a pointer to each raster band of the data
    band = dataSet->GetRasterBand(i + 1);

    // Fill image with all the raster information
    band->RasterIO(GF_Read, 0, 0,
                   dataSet->GetRasterXSize(),
                   dataSet->GetRasterYSize(),
                   this->data.image + i,
                   this->data.width, this->data.height, GDT_Byte, 1, 0);
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Write camera data
void ImageSeq::WriteData()
{
  size_t size;
  
  size = sizeof(this->data) - sizeof(this->data.image) + this->data.image_size;

  this->data.width = htons(this->data.width);
  this->data.height = htons(this->data.height);
  this->data.depth = this->data.depth;
  this->data.compression = PLAYER_CAMERA_COMPRESS_RAW;
  this->data.image_size = htonl(this->data.image_size);
      
  PutData(&this->data, size, NULL);
      
  return;
}
