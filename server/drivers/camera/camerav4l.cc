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
// Desc: Video for Linux capture driver
// Author: Andrew Howard
// Date: 9 Jan 2004
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

#include "device.h"
#include "devicetable.h"
#include "drivertable.h"
#include "playertime.h"

#include "v4lcapture.h"  // For Gavin's libfg; should integrate this


// Time for timestamps
extern PlayerTime *GlobalTime;


// Driver for detecting laser retro-reflectors.
class CameraV4L : public CDevice
{
  // Constructor
  public: CameraV4L(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Update the device data (the data going back to the client).
  private: void WriteData();

  // Video device
  private: const char *device;
  
  // Input source
  private: int source;
  
  // The signal norm: VIDEO_NORM_PAL or VIDEO_NORM_NTSC.
  private: int norm;

  // Pixel depth
  private: int depth;
  
  // Image dimensions
  private: int width, height;
  
  // Frame grabber interface
  private: FRAMEGRABBER* fg;

  // The current image (local copy)
  private: FRAME* frame;

  // Write frames to disk?
  private: int save;

  // Capture timestamp
  private: uint32_t tsec, tusec;
  
  // Data to send to server
  private: player_camera_data_t data;
};


// Initialization function
CDevice* CameraV4L_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_CAMERA_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"camerav4l\" does not support interface \"%s\"\n",
                  interface);
    return (NULL);
  }
  return ((CDevice*) (new CameraV4L(interface, cf, section)));
}


// a driver registration function
void CameraV4L_Register(DriverTable* table)
{
  table->AddDriver("camerav4l", PLAYER_READ_MODE, CameraV4L_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
CameraV4L::CameraV4L(char* interface, ConfigFile* cf, int section)
  : CDevice(sizeof(player_camera_data_t), 0, 10, 10)
{
  const char *snorm;
  
  // Camera defaults to /dev/video0 and NTSC
  this->device = cf->ReadString(section, "device", "/dev/video0");

  // Source
  this->source = cf->ReadInt(section, "source", 3);
  
  // NTSC or PAL
  snorm = cf->ReadString(section, "norm", "ntsc");
  if (strcasecmp(snorm, "ntsc") == 0)
  {
    this->norm = VIDEO_MODE_NTSC;
    this->width = 640;
    this->height = 480;
  }
  else if (strcasecmp(snorm, "pal") == 0)
  {
    this->norm = VIDEO_MODE_PAL;
    this->width = 786;
    this->height = 576;
  }

  // Size
  this->width = cf->ReadInt(section, "width", this->width);
  this->height = cf->ReadInt(section, "height", this->height);

  // Color
  this->depth = cf->ReadInt(section, "depth", 24);

  // Save frames?
  this->save = cf->ReadInt(section, "save", 0);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int CameraV4L::Setup()
{
  this->fg = fg_open(this->device);
  if (this->fg == NULL)
  {
    PLAYER_ERROR1("unable to open %s", this->device);
    return -1;
  }
  
  fg_set_source(this->fg, this->source);
  fg_set_source_norm(this->fg, this->norm);
  fg_set_capture_window(this->fg, 0, 0, this->width, this->height);

  if (this->depth == 8)
  {
    fg_set_format(this->fg, VIDEO_PALETTE_GREY);
    this->frame = frame_new(this->width, this->height, VIDEO_PALETTE_GREY );
  }
  else if (this->depth == 16)
  {
    fg_set_format(this->fg, VIDEO_PALETTE_RGB565 );
    this->frame = frame_new(this->width, this->height, VIDEO_PALETTE_RGB565 );
  }
  else if (this->depth == 24)
  {
    fg_set_format(this->fg, VIDEO_PALETTE_RGB24 );
    this->frame = frame_new(this->width, this->height, VIDEO_PALETTE_RGB24 );
  }
  else if (this->depth == 32)
  {
    fg_set_format(this->fg, VIDEO_PALETTE_RGB32 );
    this->frame = frame_new(this->width, this->height, VIDEO_PALETTE_RGB32 );
  }
  else
  {
    PLAYER_ERROR2("image depth %d is not supported (add it yourself in %s)",
                  this->depth, __FILE__);
    return 1;
  }

  // Start the driver thread.
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int CameraV4L::Shutdown()
{
  // Stop the driver thread.
  StopThread();

  // Free resources
  frame_release(this->frame);
  fg_close(this->fg);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void CameraV4L::Main() 
{
  struct timeval time;
  int frameno;
  char filename[256];

  frameno = 0;
      
  while (true)
  {
    // Go to sleep for a while (this is a polling loop).
    usleep(50000);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    HandleRequests();

    // Get the time
    GlobalTime->GetTime(&time);
    this->tsec = time.tv_sec;
    this->tusec = time.tv_usec;

    // Grab the next frame (blocking)
    fg_grab_frame(this->fg, this->frame);
        
    // Write data to server
    this->WriteData();
    
    // Save frames
    if (this->save)
    {
      printf("click %d\n", frameno);
      snprintf(filename, sizeof(filename), "click-%04d.ppm", frameno++);
      frame_save(this->frame, filename);
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int CameraV4L::HandleRequests()
{
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  int len;
  
  while ((len = GetConfig(&client, &request, sizeof(request))) > 0)
  {
    switch (request[0])
    {
      case PLAYER_FIDUCIAL_GET_GEOM:
        HandleGetGeom(client, request, len);
        break;

      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Handle geometry requests.
void CameraV4L::HandleGetGeom(void *client, void *request, int len)
{

  // TODO

  if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}



////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void CameraV4L::WriteData()
{
  size_t size;
  //printf("%d %06d\n", this->tsec, this->tusec);
  
  // Set the image properties
  this->data.width = htons(this->width);
  this->data.height = htons(this->height);
  this->data.depth = this->depth;
  this->data.image_size = htonl(this->frame->size);

  // Set the image pixels
  assert((size_t) this->frame->size <= sizeof(this->data.image));
  memcpy(this->data.image, this->frame->data, this->frame->size);
  
  // Copy data to server.
  size = sizeof(this->data) - sizeof(this->data.image) + this->frame->size;
  PutData((void*) &this->data, size, this->tsec, this->tusec);

  return;
}


