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
// Author: Nate Koenig 
// Date: 31 Aug 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifndef PLAYER_JPEGSTREAM_H
#define PLAYER_JPEGSTREAM_H

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <math.h>

#include "jpeg.h"

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"
#include "playertime.h"

// Time for timestamps
extern PlayerTime *GlobalTime;

// Converts rawImage into a jpeg image that is stored within a 
// player_camera_data structure. Quality should range from 0-100 with
// 0 being the worst quality (most compression).
/*void jpeg_compress( unsigned char *rawImage,  int width, int height, int quality, unsigned char *outputImage, unsigned int *outputSize );

void jpeg_decompress(unsigned char *jpgImage, unsigned int image_size,
    unsigned char *outputImage, unsigned int *outputSize);
    */


class JpegStream : public Driver
{
  public: JpegStream( ConfigFile *cf, int section);

  public: virtual int Setup();
  public: virtual int Shutdown();

  private: virtual void Main();

  private: int UpdateCamera();
  private: void WriteData();
  private: void SaveFrame(const char *filename);

  // Capture timestamp
  private: uint32_t tsec, tusec;

   // Save image frames?
  private: int save;
  private: int frameno;
 
  private: double imageQuality;

  private: Driver *camera;
  private: int cameraIndex;
  private: double cameraTime;
  private: player_device_id_t camera_id;
  private: player_camera_data_t cameraData;

  private: player_camera_data_t data;
};

Driver *JpegStream_Init(ConfigFile *cf, int section)
{
  return ((Driver*) (new JpegStream(cf, section)));
}

void JpegStream_Register(DriverTable *table)
{
  table->AddDriver("jpegstream", JpegStream_Init);
}

JpegStream::JpegStream( ConfigFile *cf, int section)
  : Driver(cf,section, PLAYER_CAMERA_CODE, PLAYER_READ_MODE,
      sizeof(player_camera_data_t), 0, 10, 10)
{

  this->frameno = 0;

  this->save = cf->ReadInt(section,"save",0);
  this->cameraIndex = cf->ReadInt(section,"camera",0);
  this->imageQuality = cf->ReadFloat(section, "image_quality", 0.8);

  this->camera = NULL;
}

int JpegStream::Setup()
{

  // Subscribe to the camera
  this->camera_id.code = PLAYER_CAMERA_CODE;
  this->camera_id.index = this->cameraIndex;
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

int JpegStream::Shutdown()
{
  // Stop the driver thread
  StopThread();


  // Unsubscribe from devices
  this->camera->Unsubscribe(this->camera_id);

  return 0;
}

void JpegStream::Main()
{
  struct timeval time;
  size_t size;
  char filename[256];

  while (true)
  {
    // Let the camera driver update this thread
    this->camera->Wait();

    // Test if we are suppose to cancel this thread.
    pthread_testcancel();

    // Get the time
    GlobalTime->GetTime(&time);
    this->tsec = time.tv_sec;
    this->tusec = time.tv_usec;

    // Get the latest camera data
    if (this->UpdateCamera())
    {

      this->data.image_size = jpeg_compress( (char*)this->data.image, 
          (char*)this->cameraData.image, this->cameraData.width, 
          this->cameraData.height,PLAYER_CAMERA_IMAGE_SIZE, 
          (int)(this->imageQuality*100));

      if (this->save)
      {
        snprintf(filename, sizeof(filename), "click-%04d.ppm",this->frameno++);
        FILE *fp = fopen(filename, "w+");
        fwrite (this->data.image, 1, this->data.image_size, fp);
        fclose(fp);
      }

      strcpy((char*)(this->data.format),"jpg");

      size = sizeof(this->data) - sizeof(this->data.image) 
        + this->data.image_size;

      this->data.image_size = htonl(this->data.image_size);
      this->data.width = htons(this->cameraData.width);
      this->data.height = htons(this->cameraData.height);
      this->data.depth = this->cameraData.depth;

      PutData((void*) &this->data, size, &time);
    }
  }
}

int JpegStream::UpdateCamera()
{
  size_t size;
  struct timeval ts;
  double time;

  // Get the camera data.
  size = this->camera->GetData(this->camera_id, 
      (unsigned char*) &this->cameraData, sizeof(this->cameraData), &ts);

  time = (double) ts.tv_sec + ((double) ts.tv_usec) * 1e-6;

  // Dont do anything if this is old data.
  if (fabs(time - this->cameraTime) < 0.001)
    return 0;
  this->cameraTime = time;
  
  // Do some byte swapping
  this->cameraData.width = ntohs(this->cameraData.width);
  this->cameraData.height = ntohs(this->cameraData.height); 
  this->cameraData.depth = ntohs(this->cameraData.depth);
  this->cameraData.image_size = ntohl(this->cameraData.image_size); 

  return 1;
}


#endif
