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
// Author: Nate Koenig 
// Date: 03 June 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include "player.h"

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>
#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>

#include "device.h"
#include "devicetable.h"
#include "drivertable.h"
#include "playertime.h"

#define NUM_DMA_BUFFERS 8

// Time for timestamps
extern PlayerTime *GlobalTime;


// Driver for detecting laser retro-reflectors.
class Camera1394 : public CDevice
{
  // Constructor
  public: Camera1394(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Save a frame to memory
  private: int GrabFrame();

  // Save a frame to disk
  private: int SaveFrame( const char *filename );

  /*public: virtual size_t GetData( void *client, unsigned char *dest,
              size_t maxsize, uint32_t *timesatamp_sec,
              uint32_t *timestamp_usec);
              */

  // Update the device data (the data going back to the client).
  private: void WriteData();

  // Video device
  private: const char *device;

  private: raw1394handle_t handle;
  private: dc1394_cameracapture camera;

  private: unsigned int port;
  private: unsigned int node;

  // Camera features
  private: dc1394_feature_set features;

  // Framerate.An  enum defined in libdc1394/dc1394_control.h
  private: unsigned int frameRate;

  // Resolution. An enum defined in libdc1394/dc1394_control.h
  private: unsigned int resolution;

  // Write frames to disk?
  private: int save;

  // Frame capture buffer and size
  private: unsigned char *frame;

  // Capture timestamp
  private: uint32_t tsec, tusec;
  
  // Data to send to server
  private: player_camera_data_t data;
};


// Initialization function
CDevice* Camera1394_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_CAMERA_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"camera1394\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new Camera1394(interface, cf, section)));
}


// a driver registration function
void Camera1394_Register(DriverTable* table)
{
  table->AddDriver("camera1394", PLAYER_READ_MODE, Camera1394_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
Camera1394::Camera1394(char* interface, ConfigFile* cf, int section)
  : CDevice(sizeof(player_camera_data_t), 0, 10, 10)
{
  float fps;
  int width, height;
  
  // Camera defaults to /dev/video0 and NTSC
  this->device = cf->ReadString(section, "device", "/dev/video1394");

  // The port the camera is attached to
  this->port = cf->ReadInt(section, "port", 0);

  // The node inside the port
  this->node = cf->ReadInt(section, "node", 0);

  // Video frame rate
  fps = atof(cf->ReadString(section, "framerate", "15"));
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

  // Image size. This determines the capture resolution. There are a limited
  // number of options available. At 640x480, a camera can capture at
  // _RGB or _MONO or _MONO16. Right now if 640x480 is selected, we default
  // to _RGB.
  width = cf->ReadInt(section, "width", 640);
  height = cf->ReadInt(section, "height", 480);
  if (width < 320)
    this->resolution = MODE_160x120_YUV444;
  else if (width < 640)
    this->resolution = MODE_320x240_YUV422;
  else
    this->resolution = MODE_640x480_RGB;

  // Save frames?
  this->save = cf->ReadInt(section, "save", 0);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int Camera1394::Setup()
{
  unsigned int channel, speed;

  // Create a raw1394 handle. Port should be 0 unless you have multiple
  // firewire cards in your system.
  this->handle = dc1394_create_handle(this->port);

  if (this->handle == NULL)
  {
    PLAYER_ERROR("Camera1394 : Unable to acquire a raw1394 handle");
    return -1;
  }

  this->camera.node = this->node;
  this->camera.port = this->port;


  // Collects the available features for the camera described by node and
  // stores them in features.
  if (dc1394_get_camera_feature_set(this->handle, this->camera.node, 
        &this->features) != DC1394_SUCCESS)
  {
    PLAYER_ERROR("Camera1394 : Unable to get feature set");
    return -1;
  }


  // Get the ISO channel and speed of the video
  if (dc1394_get_iso_channel_and_speed(this->handle, this->camera.node, 
        &channel, &speed) != DC1394_SUCCESS)
  {
    PLAYER_ERROR("Camera1394 : Unable to get iso channel and speed");
    return -1;
  } 



  // Set camera to use DMA, improves performance.
  // The '1' parameters is the dropFrames parameter.
  if(dc1394_dma_setup_capture(this->handle, this->camera.node, channel,
        FORMAT_VGA_NONCOMPRESSED, this->resolution, SPEED_400, this->frameRate, 
        NUM_DMA_BUFFERS, 1, this->device, &this->camera)
      != DC1394_SUCCESS)
  {
    PLAYER_ERROR("Camera1394 : Unable to setup camera.");
    return -1;
  }

  if (dc1394_start_iso_transmission(this->handle, this->camera.node) 
      != DC1394_SUCCESS)
  {
    PLAYER_ERROR("Camera1394 : Unable to start camera iso transmission.");
    return -1;
  }

  this->frame = new unsigned char[this->camera.dma_frame_size];

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

  // Free resources
  dc1394_dma_unlisten( this->handle, &this->camera );
  dc1394_dma_release_camera( this->handle, &this->camera );

  dc1394_destroy_handle(this->handle);

  if (this->frame)
    delete [] this->frame;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Camera1394::Main() 
{
  struct timeval time;
  char filename[255];
  int frameno;

  frameno = 0;

  while (true)
  {
    // Go to sleep for a while (this is a polling loop).
    usleep(50000);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    //HandleRequests();

    // Get the time
    GlobalTime->GetTime(&time);
    this->tsec = time.tv_sec;
    this->tusec = time.tv_usec;

    // Grab the next frame (blocking)
    this->GrabFrame();
       
    // Write data to server
    this->WriteData();
  
    // Save frames
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
  
  while ((len = GetConfig(&client, &request, sizeof(request))) > 0)
  {
    switch (request[0])
    {
      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
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
  if (dc1394_dma_single_capture(&this->camera) != DC1394_SUCCESS)
  {
    PLAYER_ERROR("Unable to capture frame");
    return -1;
  }

  memcpy( this->frame, (unsigned char *)this->camera.capture_buffer, 
          this->camera.dma_frame_size);

  dc1394_dma_done_with_buffer(&this->camera);

  return 0;
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

  fprintf(fp,"P6\n%u %u\n255\n",this->camera.frame_width, this->camera.frame_height);

  fwrite ((unsigned char*)this->frame, 1, this->camera.dma_frame_size, fp);

  fclose(fp);

  return 0;
}

/*size_t Camera1394::GetData( void *client, unsigned char *dest,
              size_t maxsize, uint32_t *timesatamp_sec,
              uint32_t *timestamp_usec)
{
  printf("Len[%d] [%d]\n",maxsize,sizeof(player_camera_data_t));
  return 0;
}
*/


////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void Camera1394::WriteData()
{
  size_t size;
  //printf("%d %06d\n", this->tsec, this->tusec);
  
  // Set the image properties
  this->data.width = htons(this->camera.frame_width);
  this->data.height = htons(this->camera.frame_height);
  this->data.depth = htons(8);
  this->data.image_size = htonl(this->camera.dma_frame_size);

  // Set the image pixels
  assert((size_t) this->camera.dma_frame_size <= sizeof(this->data.image));
  memcpy(this->data.image, this->frame, this->camera.dma_frame_size);
 
  // Copy data to server.
  size = sizeof(this->data) - sizeof(this->data.image) + this->camera.dma_frame_size;

  PutData((void*) &this->data, size, this->tsec, this->tusec);

  return;
}

