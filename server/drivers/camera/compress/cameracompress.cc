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
// Desc: jpeg compression and decompression routines
// Author: Nate Koenig, Andrew Howard
// Date: 31 Aug 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_cameracompress Camera image compression driver

The CameraCompress driver accepts data from another camera device,
compresses it, and makes the compressed data available on a new interface.

@par Interfaces
- This driver supports the @ref player_interface_camera interface.

@par Supported configuration requests

- None.

@par Configuration file options

- camera 0
  - Source camera (data from this camera will be compressed).
      
@par Example 

@verbatim
driver
(
  name "cameracompress"
  devices ["camera:1"]
  camera 0  # Compress data from device camera:0
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

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"
#include "playertime.h"

#include "playerpacket.h"



class CameraCompress : public Driver
{
  public: CameraCompress( ConfigFile *cf, int section);

  public: virtual int Setup();
  public: virtual int Shutdown();

  private: virtual void Main();

  private: int UpdateCamera();
  private: void WriteData();
  private: void SaveFrame(const char *filename);

  // Input camera device
  private: int camera_index;
  private: player_device_id_t camera_id;
  private: Driver *camera;

  // Acquired camera data
  private: struct timeval camera_time;
  private: player_camera_data_t camera_data;

  // Output (compressed) camera data
  private: player_camera_data_t data;

  // Image quality for JPEG compression
  private: double quality;

  // Save image frames?
  private: int save;
  private: int frameno;
};


Driver *CameraCompress_Init(ConfigFile *cf, int section)
{
  return ((Driver*) (new CameraCompress(cf, section)));
}

void CameraCompress_Register(DriverTable *table)
{
  table->AddDriver("cameracompress", CameraCompress_Init);
}

CameraCompress::CameraCompress( ConfigFile *cf, int section)
  : Driver(cf,section, PLAYER_CAMERA_CODE, PLAYER_READ_MODE,
      sizeof(player_camera_data_t), 0, 10, 10)
{

  this->frameno = 0;

  this->camera_index = cf->ReadInt(section,"camera", 0);
  
  this->save = cf->ReadInt(section,"save",0);
  this->quality = cf->ReadFloat(section, "image_quality", 0.8);

  this->camera = NULL;

  return;
}

int CameraCompress::Setup()
{

  // Subscribe to the camera
  this->camera_id.code = PLAYER_CAMERA_CODE;
  this->camera_id.index = this->camera_index;
  this->camera_id.port = this->device_id.port;
  this->camera = deviceTable->GetDriver(this->camera_id);
  
  if (!this->camera)
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return (-1);
  }

  if (this->camera->Subscribe(this->camera_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    return (-1);
  }

  // Start the driver thread.
  this->StartThread();

  return 0;
}

int CameraCompress::Shutdown()
{
  // Stop the driver thread
  StopThread();

  // Unsubscribe from devices
  this->camera->Unsubscribe(this->camera_id);

  return 0;
}

void CameraCompress::Main()
{
  size_t size;
  char filename[256];

  while (true)
  {
    // Let the camera driver update this thread
    this->camera->Wait();

    // Test if we are suppose to cancel this thread.
    pthread_testcancel();

    // Get the latest camera data
    if (this->UpdateCamera())
    {

      this->data.image_size = jpeg_compress( (char*)this->data.image, 
                                             (char*)this->camera_data.image,
                                             this->camera_data.width, 
                                             this->camera_data.height,
                                             PLAYER_CAMERA_IMAGE_SIZE, 
                                             (int)(this->quality*100));

      if (this->save)
      {
        snprintf(filename, sizeof(filename), "click-%04d.ppm",this->frameno++);
        FILE *fp = fopen(filename, "w+");
        fwrite (this->data.image, 1, this->data.image_size, fp);
        fclose(fp);
      }

      size = sizeof(this->data) - sizeof(this->data.image) + this->data.image_size;

      this->data.width = htons(this->camera_data.width);
      this->data.height = htons(this->camera_data.height);
      this->data.depth = this->camera_data.depth;
      this->data.compression = PLAYER_CAMERA_COMPRESS_JPEG;
      this->data.image_size = htonl(this->data.image_size);
      
      PutData((void*) &this->data, size, &this->camera_time);
    }
  }
  return;
}

int CameraCompress::UpdateCamera()
{
  size_t size;

  // Get the camera data.
  size = this->camera->GetData(this->camera_id, 
                               &this->camera_data,
                               sizeof(this->camera_data), &this->camera_time);
  
  // Do some byte swapping
  this->camera_data.width = ntohs(this->camera_data.width);
  this->camera_data.height = ntohs(this->camera_data.height); 
  this->camera_data.depth = this->camera_data.depth;
  this->camera_data.image_size = ntohl(this->camera_data.image_size);

  if (this->camera_data.compression != PLAYER_CAMERA_COMPRESS_RAW)
    PLAYER_WARN("compressing already compressed camera images (not good)");

  return 1;
}
