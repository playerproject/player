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
// Desc: 1394 camera capture
// Author: Nate Koenig, Andrew Howard
// Date: 03 June 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_camera1394 camera1394

The camera1394 driver captures images from IEEE1394 (Firewire, iLink)
cameras.  

@par Interfaces
- @ref player_interface_camera

@par Supported configuration requests

None


@par Configuration file options

- port 0
  - The 1394 port the camera is attached to.

- node 0
  - The node within the port

- framerate 15
  - Requested frame rate (frames/second)

- mode "320x240_yuv422"
  - Capture mode (size and color layour).  Valid modes are:
    - "320x240_yuv"
    - "640x480_rgb"
    - "640x480_mono"
  - Currently, all of these modes will produce 8-bit monochrome images.
  
- force_raw 0
  - Force the driver to use (slow) memory capture instead of DMA transfer
  (for buggy 1394 drivers).
  
- save 0
  - Debugging option: set this to write each frame as an image file on disk.

  
@par Example 

@verbatim
driver
(
  name "camera1394"
  provides ["camera:0"]
)
@endverbatim


*/
/** @} */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include "player.h"

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>
#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>

#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"
#include "playertime.h"


#define NUM_DMA_BUFFERS 8

// Time for timestamps
extern PlayerTime *GlobalTime;


// Driver for detecting laser retro-reflectors.
class Camera1394 : public Driver
{
  // Constructor
  public: Camera1394( ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Save a frame to memory
  private: int GrabFrame();

  // Convert frame to appropriate output format
  private: int ConvertFrame();

  // Save a frame to disk
  private: int SaveFrame( const char *filename );

  // Update the device data (the data going back to the client).
  private: void WriteData();

  // Video device
  private: unsigned int port;
  private: unsigned int node;
  private: raw1394handle_t handle;
  private: dc1394_cameracapture camera;

  // Camera features
  private: dc1394_feature_set features;

  // Capture method: RAW or VIDEO (DMA)
  private: enum {methodRaw, methodVideo};
  private: int method;
  private: bool forceRaw;

  // Framerate.An  enum defined in libdc1394/dc1394_control.h
  private: unsigned int frameRate;

  // Format & Mode. An enum defined in libdc1394/dc1394_control.h
  private: unsigned int format;
  private: unsigned int mode;

  private: unsigned int width;
  private: unsigned int height;

  // Write frames to disk?
  private: int save;

  // Frame capture buffer and size
  private: unsigned char *frame;
  private: size_t frameSize;

  // Capture timestamp
  private: struct timeval frameTime;
  
  // Data to send to server
  private: player_camera_data_t data;
};


// Initialization function
Driver* Camera1394_Init( ConfigFile* cf, int section)
{
  return ((Driver*) (new Camera1394( cf, section)));
}


// a driver registration function
void Camera1394_Register(DriverTable* table)
{
  table->AddDriver("camera1394", Camera1394_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
Camera1394::Camera1394( ConfigFile* cf, int section)
  : Driver(cf, section, PLAYER_CAMERA_CODE, PLAYER_READ_MODE,
           sizeof(player_camera_data_t), 0, 10, 10)
{
  float fps;
  
  // The port the camera is attached to
  this->port = cf->ReadInt(section, "port", 0);

  // The node inside the port
  this->node = cf->ReadInt(section, "node", 0);

  // Video frame rate
  fps = cf->ReadFloat(section, "framerate", 15);
  if (fps < 3.75)
    this->frameRate = FRAMERATE_1_875;
  else if (fps < 7.5)
    this->frameRate = FRAMERATE_3_75;
  else if (fps < 15)
    this->frameRate = FRAMERATE_7_5;
  else if (fps < 30)
    this->frameRate = FRAMERATE_15;
  else if (fps < 60)
    this->frameRate = FRAMERATE_30;
  else
    this->frameRate = FRAMERATE_60;

  // Get uncompressed video
  this->format = FORMAT_VGA_NONCOMPRESSED;
    
  // Image size. This determines the capture resolution. There are a limited
  // number of options available. At 640x480, a camera can capture at
  // _RGB or _MONO or _MONO16.  
  const char* str;
  str =  cf->ReadString(section, "mode", "320x240_yuv422");
  /*
  if (0==strcmp(str,"160x120_yuv444"))
  {
    this->mode = MODE_160x120_YUV444;
    this->frameSize = 160 * 120 * 2; // Is this correct?
  }
  */
  if (0==strcmp(str,"320x240_yuv422"))
  {
    this->mode = MODE_320x240_YUV422;
    this->frameSize = 320 * 240 * 2;
  }
  /*
  else if (0==strcmp(str,"640x480_yuv411"))
  {
    this->mode = MODE_640x480_YUV411;
  }
  else if (0==strcmp(str,"640x480_yuv422"))
  {
    this->mode = MODE_640x480_YUV422;
  }
  */
  else if (0==strcmp(str,"640x480_rgb"))
  {
    this->mode = MODE_640x480_RGB;
    this->frameSize = 640 * 480 * 3;
  }
  else if (0==strcmp(str,"640x480_mono"))
  {
    this->mode = MODE_640x480_MONO;
    this->frameSize = 640 * 480 * 1;
  }
  /*
  else if (0==strcmp(str,"640x480_mono16"))
  {
    this->mode = MODE_640x480_MONO16;
    assert(false);
  }
  */
  else
  {
    PLAYER_ERROR1("unknown video mode [%s]", str);
    this->SetError(-1);
    return;
  }
  
  // Force into raw mode
  this->forceRaw = cf->ReadInt(section, "force_raw", 0);

  // Create a frame buffer
  this->frame = new unsigned char[this->frameSize];  
     
  // Save frames?
  this->save = cf->ReadInt(section, "save", 0);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int Camera1394::Setup()
{
  unsigned int channel, speed;

  // Create a handle for the given port (port will be zero on most
  // machines)
  this->handle = dc1394_create_handle(this->port);
  if (this->handle == NULL)
  {
    PLAYER_ERROR("Unable to acquire a dc1394 handle");
    return -1;
  }

  this->camera.node = this->node;
  this->camera.port = this->port;

  // Collects the available features for the camera described by node and
  // stores them in features.
  if (DC1394_SUCCESS != dc1394_get_camera_feature_set(this->handle, this->camera.node, 
                                                      &this->features))
  {
    PLAYER_ERROR("Unable to get feature set");
    return -1;
  }

  // TODO: need to indicate what formats the camera supports somewhere
  // Remove; leave?
  dc1394_print_feature_set(&this->features);

  // Get the ISO channel and speed of the video
  if (DC1394_SUCCESS != dc1394_get_iso_channel_and_speed(this->handle, this->camera.node, 
                                                         &channel, &speed))
  {
    PLAYER_ERROR("Unable to get iso data; is the camera plugged in?");
    return -1;
  }


#if DC1394_DMA_SETUP_CAPTURE_ARGS == 11
  // Set camera to use DMA, improves performance.
  if (!this->forceRaw &&
      dc1394_dma_setup_capture(this->handle, this->camera.node, channel,
                               this->format, this->mode, speed,
                               this->frameRate, NUM_DMA_BUFFERS, 1, NULL,
                               &this->camera) == DC1394_SUCCESS)
#elif DC1394_DMA_SETUP_CAPTURE_ARGS == 12
  // Set camera to use DMA, improves performance.
  if (!this->forceRaw &&
      dc1394_dma_setup_capture(this->handle, this->camera.node, channel,
                               this->format, this->mode, speed,
                               this->frameRate, NUM_DMA_BUFFERS, 1, 0, NULL,
                               &this->camera) == DC1394_SUCCESS)
#else
  if (0)
#endif
  {    
    this->method = methodVideo;
  }
  else
  {
    PLAYER_WARN("DMA capture failed; falling back on RAW method");
          
    // Set camera to use RAW method (fallback)
    if (dc1394_setup_capture(this->handle, this->camera.node, channel,
                             this->format, this->mode, SPEED_400, this->frameRate,
                             &this->camera) == DC1394_SUCCESS)
    {
      this->method = methodRaw;
    }
    else
    {
      PLAYER_ERROR("unable to open camera in VIDE0 or RAW modes");
      return -1;
    }
  }

  // Start transmitting camera data
  if (DC1394_SUCCESS != dc1394_start_iso_transmission(this->handle, this->camera.node))
  {
    PLAYER_ERROR("unable to start camera");
    return -1;
  }

  // Start the driver thread.
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int Camera1394::Shutdown()
{
  // Stop the driver thread.
  StopThread();

  // Stop transmitting camera data
  if (dc1394_stop_iso_transmission(this->handle, this->camera.node) != DC1394_SUCCESS) 
    PLAYER_WARN("unable to stop camera");

  // Free resources
  if (this->method == methodRaw)
  {
    dc1394_release_camera(this->handle, &this->camera);
  }
  else if (this->method == methodVideo)
  {
    dc1394_dma_unlisten( this->handle, &this->camera );
    dc1394_dma_release_camera( this->handle, &this->camera );
  }
  dc1394_destroy_handle(this->handle);

  if (this->frame)
    delete [] this->frame;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Camera1394::Main() 
{
  char filename[255];
  int frameno;

  frameno = 0;

  while (true)
  {
    // Go to sleep for a while (this is a polling loop).
    // We shouldn't need to sleep if GrabFrame is blocking.
    // usleep(50000);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    HandleRequests();

    // Get the time
    GlobalTime->GetTime(&this->frameTime);

    // Grab the next frame (blocking)
    this->GrabFrame();

    // Convert frame to appropriate output format
    this->ConvertFrame();
       
    // Write data to server
    this->WriteData();

    // this should go or be replaced
    // Save frames; must be done after writedata (which will byteswap)
    if (this->save)
    {
      //printf("click %d\n", frameno);
      snprintf(filename, sizeof(filename), "click-%04d.ppm", frameno++);
      this->SaveFrame(filename);
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int Camera1394::HandleRequests()
{
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  int len;
  
  while ((len = GetConfig(&client, &request, sizeof(request),NULL)) > 0)
  {
    switch (request[0])
    {
      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Store an image frame into the 'frame' buffer
int Camera1394::GrabFrame()
{
  if (this->method == methodRaw)
  {
    if (dc1394_single_capture(this->handle, &this->camera) != DC1394_SUCCESS)
    {
      PLAYER_ERROR("Unable to capture frame");
      return -1;
    }
    memcpy( this->frame, (unsigned char *) this->camera.capture_buffer, 
            this->frameSize );
  }
  else if (this->method == methodVideo)
  {
    if (dc1394_dma_single_capture(&this->camera) != DC1394_SUCCESS)
    {
      PLAYER_ERROR("Unable to capture frame");
      return -1;
    }
    memcpy( this->frame, (unsigned char *)this->camera.capture_buffer, 
            this->frameSize);
    dc1394_dma_done_with_buffer(&this->camera);
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Convert frame to appropriate output format
int Camera1394::ConvertFrame()
{
  int i;
  unsigned char *src, *dst;
  
  switch (this->mode)
  {
    case MODE_320x240_YUV422:
    {
      this->data.bpp = 8;
      this->data.format = PLAYER_CAMERA_FORMAT_MONO8;
      this->data.image_size = this->frameSize / 2;
      assert(this->data.image_size <= sizeof(this->data.image));

      i = this->frameSize;
      src = this->frame;
      dst = this->data.image;

      for (; i > 0; i -= 4, src += 4, dst += 2)
      {
        dst[0] = src[1];
        dst[1] = src[3];
      }
      break;
    }

    case MODE_640x480_MONO:
    {
      this->data.bpp = 8;
      this->data.format = PLAYER_CAMERA_FORMAT_MONO8;
      this->data.image_size = this->frameSize;
      assert(this->data.image_size <= sizeof(this->data.image));
      memcpy(this->data.image, this->frame, this->data.image_size);
      break;
    }

    case MODE_640x480_RGB:
    {
      this->data.bpp = 24;
      this->data.format = PLAYER_CAMERA_FORMAT_RGB888;
      this->data.image_size = this->frameSize;
      assert(this->data.image_size <= sizeof(this->data.image));
      memcpy(this->data.image, this->frame, this->data.image_size);
      break;
    }

    default:
      assert(false);
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void Camera1394::WriteData()
{
  size_t size;

  // Work out the data size; do this BEFORE byteswapping
  size = sizeof(this->data) - sizeof(this->data.image) + this->data.image_size;

  // Image data and size is filled in ConvertFrame(),
  // so now we just do the byte-swapping
  this->data.width = htons(this->camera.frame_width);
  this->data.height = htons(this->camera.frame_height);
  this->data.compression = PLAYER_CAMERA_COMPRESS_RAW;
  this->data.image_size = htonl(this->data.image_size);

  // Copy data to server
  PutData((void*) &this->data, size, &this->frameTime);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Save a frame to disk
int Camera1394::SaveFrame(const char *filename)
{
  FILE *fp = fopen(filename, "wb");

  if (fp == NULL)
  {
    PLAYER_ERROR("Couldn't create image file");
    return -1;
  }

  switch (this->data.format)
  {
    case PLAYER_CAMERA_FORMAT_MONO8:
      fprintf(fp,"P5\n%u %u\n255\n", ntohs(this->data.width), ntohs(this->data.height));
      fwrite((unsigned char*)this->data.image, 1, ntohl(this->data.image_size), fp);
      break;
    case PLAYER_CAMERA_FORMAT_RGB888:
      fprintf(fp,"P6\n%u %u\n255\n", ntohs(this->data.width), ntohs(this->data.height));
      fwrite((unsigned char*)this->data.image, 1, ntohl(this->data.image_size), fp);
      break;
    default:
      break;
  }

  fclose(fp);

  return 0;
}

