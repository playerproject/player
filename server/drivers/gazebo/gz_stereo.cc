/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
// Desc: Gazebo (simulator) stereo camera driver
// Author: Andrew Howard
// Date: 10 Oct 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_gz_stereo gz_stereo


The gz_stereo driver is used to access Gazebo models that support the
libgazebo stereo interface (such as the StereoHead model).

@par Compile-time dependencies

- Gazebo

@par Provides

This driver provides four named camera interfaces:

- "left" @ref player_interface_camera
  - Left camera image (RGB)

- "right" @ref player_interface_camera
  - Right camera image (RGB)

- "left_disparity" @ref player_interface_camera
  - Left disparity image; this is a 16-bit monochrome image (MONO16), with
    each pixel recording the horizontal disparity in the rectified image.

- "right_disparity" @ref player_interface_camera
  - Right disparity image; this is a 16-bit monochrome image (MONO16), with
    each pixel recording the horizontal disparity in the rectified image.


@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- gz_id (string)
  - Default: ""
  - ID of the Gazebo model.

- save (integer)
  - Default: 0
  - Save images to disk (for debugging).
      
@par Example 

@verbatim
driver
(
  name gz_stereo
  provides ["left::camera:0" "right::camera:1" "left_disparity::camera:2"]
  gz_id "stereo1"
)
@endverbatim

@par Authors

Andrew Howard
*/
/** @} */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef INCLUDE_GAZEBO_STEREO
#warning "gz_stereo not supported by libgazebo; skipping"
#else

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>       // for atoi(3)

#include "player.h"
#include "error.h"
#include "driver.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"

// Incremental navigation driver
class GzStereo : public Driver
{
  // Constructor
  public: GzStereo(ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzStereo();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  // Save an image frame
  private: void SaveFrame(const char *filename, player_camera_data_t *data);

  // Gazebo device id
  private: char *gz_id;

  // Save image frames?
  private: int save;
  private: int frameno;
  
  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_stereo_t *iface;

  // Left/right camera interfaces
  private: player_device_id_t leftId, rightId;

  // Left disparity camera interface
  private: player_device_id_t leftDisparityId, rightDisparityId;

  // Most recent data
  private: player_camera_data_t leftImage, rightImage;
  private: player_camera_data_t leftDisparity, rightDisparity;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
Driver* GzStereo_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzStereo(cf, section)));
}


// a driver registration function
void GzStereo_Register(DriverTable* table)
{
  table->AddDriver("gz_stereo", GzStereo_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzStereo::GzStereo(ConfigFile* cf, int section)
    : Driver(cf, section)
{
  // Get left camera interface
  memset(&this->leftId, 0, sizeof(this->leftId));
  if (cf->ReadDeviceId(&this->leftId, section, "provides",
                       PLAYER_CAMERA_CODE, -1, "left") == 0)
  {
    if (this->AddInterface(this->leftId, PLAYER_READ_MODE,
                           sizeof(player_camera_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Get right camera interface
  memset(&this->rightId, 0, sizeof(this->rightId));
  if (cf->ReadDeviceId(&this->rightId, section, "provides",
                       PLAYER_CAMERA_CODE, -1, "right") == 0)
  {
    if (this->AddInterface(this->rightId, PLAYER_READ_MODE,
                           sizeof(player_camera_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Get disparity camera interface
  memset(&this->leftDisparityId, 0, sizeof(this->leftDisparityId));
  if (cf->ReadDeviceId(&this->leftDisparityId, section, "provides",
                       PLAYER_CAMERA_CODE, -1, "left_disparity") == 0)
  {
    if (this->AddInterface(this->leftDisparityId, PLAYER_READ_MODE,
                           sizeof(player_camera_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Get disparity camera interface
  memset(&this->rightDisparityId, 0, sizeof(this->rightDisparityId));
  if (cf->ReadDeviceId(&this->rightDisparityId, section, "provides",
                       PLAYER_CAMERA_CODE, -1, "right_disparity") == 0)
  {
    if (this->AddInterface(this->rightDisparityId, PLAYER_READ_MODE,
                           sizeof(player_camera_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Must have at least one interface
  if (this->leftId.code == 0 && this->rightId.code == 0 &&
      this->leftDisparityId.code == 0 && this->rightDisparityId.code == 0)
  {
    PLAYER_ERROR("no usable interfaces");
    this->SetError(-1);
    return;
  }
  
  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));
  
  // Save frames?
  this->save = cf->ReadInt(section, "save", 0);
  this->frameno = 0;

  // Get the globally defined  Gazebo client (one per instance of Player)
  this->client = GzClient::client;
  
  // Create an interface
  this->iface = gz_stereo_alloc();

  this->datatime = -1;
    
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzStereo::~GzStereo()
{
  gz_stereo_free(this->iface);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzStereo::Setup()
{ 
  // Open the interface
  if (gz_stereo_open(this->iface, this->client, this->gz_id) != 0)
    return -1;

  // Add ourselves to the update list
  GzClient::AddDriver(this);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzStereo::Shutdown()
{
  // Remove ourselves to the update list
  GzClient::DelDriver(this);

  gz_stereo_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzStereo::Update()
{
  int i, j;
  size_t size;
  struct timeval ts;
  uint16_t *dst_pix;
  float *src_pix;
  char filename[256];
  gz_stereo_data_t *src;
  player_camera_data_t *dst;
  
  gz_stereo_lock(this->iface, 1);

  src = this->iface->data;
  
  if (src->time > this->datatime)
  {
    this->datatime = src->time;
    
    ts.tv_sec = (int) (src->time);
    ts.tv_usec = (int) (fmod(src->time, 1) * 1e6);

    if (this->leftId.code)
    {
      // Left image
      dst = &this->leftImage;
      dst->width = htons(src->width);
      dst->height = htons(src->height);
      dst->bpp = 24;
      dst->format = PLAYER_CAMERA_FORMAT_RGB888;
      dst->compression = PLAYER_CAMERA_COMPRESS_RAW;
      dst->image_size = htonl(src->left_image_size);
      assert((size_t) src->left_image_size < sizeof(dst->image));
      memcpy(dst->image, src->left_image, src->left_image_size);
      size = sizeof(*dst) - sizeof(dst->image) + src->left_image_size;
      this->PutData(this->leftId, dst, size, &ts);

      // Save frames
      if (this->save)
      {
        snprintf(filename, sizeof(filename), "left_image_%04d.pnm", this->frameno);
        this->SaveFrame(filename, dst);
      }
    }

    if (this->rightId.code)
    {
      // Right image
      dst = &this->rightImage;
      dst->width = htons(src->width);
      dst->height = htons(src->height);
      dst->bpp = 24;
      dst->format = PLAYER_CAMERA_FORMAT_RGB888;
      dst->compression = PLAYER_CAMERA_COMPRESS_RAW;
      dst->image_size = htonl(src->right_image_size);
      assert((size_t) src->right_image_size < sizeof(dst->image));
      memcpy(dst->image, src->right_image, src->right_image_size);
      size = sizeof(*dst) - sizeof(dst->image) + src->right_image_size;
      this->PutData(this->rightId, dst, size, &ts);

      // Save frames
      if (this->save)
      {
        snprintf(filename, sizeof(filename), "right_image_%04d.pnm", this->frameno);
        this->SaveFrame(filename, dst);
      }
    }

    if (this->leftDisparityId.code)
    {
      // Left disparity
      dst = &this->leftDisparity;
      dst->width = htons(src->width);
      dst->height = htons(src->height);
      dst->bpp = 16;
      dst->format = PLAYER_CAMERA_FORMAT_MONO16;
      dst->fdiv = htons(16);
      dst->compression = PLAYER_CAMERA_COMPRESS_RAW;
      dst->image_size = htonl(src->left_disparity_size * 2);
      assert((size_t) src->left_disparity_size < sizeof(dst->image));

      for (j = 0; j < (int) src->height; j++)
      {
        src_pix = src->left_disparity + j * src->width;
        dst_pix = ((uint16_t*) dst->image) + j * src->width;        
        for (i = 0; i < (int) src->width; i++)
          dst_pix[i] = htons((uint16_t) (int16_t) (src_pix[i] * 16));
      }
    
      size = sizeof(*dst) - sizeof(dst->image) + ntohl(dst->image_size);
      this->PutData(this->leftDisparityId, dst, size, &ts);

      // Save frames
      if (this->save)
      {
        snprintf(filename, sizeof(filename), "left_disparity_%04d.pnm", this->frameno);
        this->SaveFrame(filename, dst);
      }
    }

    if (this->rightDisparityId.code)
    {
      // Right disparity
      dst = &this->rightDisparity;
      dst->width = htons(src->width);
      dst->height = htons(src->height);
      dst->bpp = 16;
      dst->format = PLAYER_CAMERA_FORMAT_MONO16;
      dst->fdiv = htons(16);
      dst->compression = PLAYER_CAMERA_COMPRESS_RAW;
      dst->image_size = htonl(src->right_disparity_size * 2);
      assert((size_t) src->right_disparity_size < sizeof(dst->image));

      for (j = 0; j < (int) src->height; j++)
      {
        src_pix = src->right_disparity + j * src->width;
        dst_pix = ((uint16_t*) dst->image) + j * src->width;        
        for (i = 0; i < (int) src->width; i++)
          dst_pix[i] = htons((uint16_t) (int16_t) (src_pix[i] * 16));
      }
    
      size = sizeof(*dst) - sizeof(dst->image) + ntohl(dst->image_size);
      this->PutData(this->rightDisparityId, dst, size, &ts);

      // Save frames
      if (this->save)
      {
        snprintf(filename, sizeof(filename), "right_disparity_%04d.pnm", this->frameno);
        this->SaveFrame(filename, dst);
      }
    }

    if (this->save)
      this->frameno++;
  }

  gz_stereo_unlock(this->iface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Save an image frame
void GzStereo::SaveFrame(const char *filename, player_camera_data_t *data)
{
  int i, j, width, height;
  uint8_t c;
  FILE *file;

  file = fopen(filename, "w+");
  if (!file)
    return;

  width = ntohs(data->width);
  height = ntohs(data->height);

  if (data->format == PLAYER_CAMERA_FORMAT_MONO16)
  {
    // Write pgm
    fprintf(file, "P5\n%d %d\n%d\n", width, height, 255);
    for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
      {
        c = (uint8_t) (ntohs(((uint16_t*) data->image)[i * width + j]) / ntohs(data->fdiv));
        fwrite(&c, 1, 1, file);
      }
    }
  }
  else if (data->format == PLAYER_CAMERA_FORMAT_RGB888)
  {
    // Write pnm 
    fprintf(file, "P6\n%d %d\n%d\n", width, height, 255);
    for (i = 0; i < height; i++)
      fwrite(data->image + i * width * 3, 1, width * 3, file);
  }
  else
  {
    PLAYER_WARN("unsupported format for saving");
  }

  fclose(file);

  return;
}

#endif
