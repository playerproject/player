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

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_camerav4l camerav4l

The camerav4l driver captures images from V4l-compatible cameras.  See
below for notes on specific camera/frame grabber combinations.


@par Interfaces
- @ref player_interface_camera

@par Supported configuration requests

None


@par Configuration file options

- port "/dev/video0"
  - Device to read video data from.

- source 3
  - Some capture cards have multiple input sources; use this field to
    select which one is used.

- norm "ntsc"
  - Capture format "ntsc" or "pal"

- size [640 480]
  - Desired image size.   This may not be honoured if the driver does
    not support the requested size).

- depth 24
  - Color depth (8, 16, 24, 32).   

- save 0
  - Debugging option: set this to write each frame as an image file on disk.

Note that some of these options may not be honoured by the underlying
V4L kernel driver (it may not support a given image size, for
example).
  
@par Example 

@verbatim
driver
(
  name "camerav4l"
  devices ["camera:0"]
)
@endverbatim



@par Logitech QuickCam Pro 4000

For the Logitech QuickCam Pro 4000, use:
@verbatim
driver
(
  name "camerav4l"
  devices ["camera:0"]
  port "/dev/video0"
  source 0
  size [160 120]
  palette "YUV420P"
)
@endverbatim

Kernel notes: with a little bit of tweaking, this camera will work with the pwc
(Phillips Web-Cam) driver in the Linux 2.4.20 kernel.  The stock driver recognizes
the QC Pro 3000, but not the QC Pro 4000; to support the latter, you must modify
the kernel source (add a product id in a couple of places in pwc-if.c).  Milage may
vary for other kernel versions.  Also, the binary-only pwcx.o module is needed to
access frame sizes larger than 160x120; good luck finding this and/or getting
it to work (the developer spat the dummy and took down his website).

*/
/** @} */
  

#include "player.h"

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>

#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"
#include "playertime.h"

#include "v4lcapture.h"  // For Gavin's libfg; should integrate this


// Time for timestamps
extern PlayerTime *GlobalTime;


// Driver for detecting laser retro-reflectors.
class CameraV4L : public Driver
{
  // Constructor
  public: CameraV4L( ConfigFile* cf, int section);

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

  // Camera palette
  private: const char *palette;
  
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
Driver* CameraV4L_Init( ConfigFile* cf, int section)
{
  return ((Driver*) (new CameraV4L( cf, section)));
}


// Driver registration function
void CameraV4L_Register(DriverTable* table)
{
  table->AddDriver("camerav4l", CameraV4L_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
CameraV4L::CameraV4L( ConfigFile* cf, int section)
  : Driver(cf, section, PLAYER_CAMERA_CODE, PLAYER_READ_MODE,
           sizeof(player_camera_data_t), 0, 10, 10)
{
  const char *snorm;
  
  // Camera defaults to /dev/video0 and NTSC
  this->device = cf->ReadString(section, "port", "/dev/video0");

  // Input source
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
  else
  {
    this->norm = VIDEO_MODE_AUTO;
    this->width = 320;
    this->height = 240;
  }

  // Size
  this->width = cf->ReadTupleInt(section, "size", 0, this->width);
  this->height = cf->ReadTupleInt(section, "size", 1, this->height);

  // Pixel depth
  this->depth = cf->ReadInt(section, "depth", 24);

  // Palette type
  this->palette = cf->ReadString(section, "palette", "RGB24");

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

  if (strcasecmp(this->palette, "GREY") == 0)
  {
    fg_set_format(this->fg, VIDEO_PALETTE_GREY);
    this->frame = frame_new(this->width, this->height, VIDEO_PALETTE_GREY );
    this->data.format = PLAYER_CAMERA_FORMAT_MONO8;
  }
  else if (strcasecmp(this->palette, "RGB565") == 0)
  {
    fg_set_format(this->fg, VIDEO_PALETTE_RGB565 );
    this->frame = frame_new(this->width, this->height, VIDEO_PALETTE_RGB565 );
    this->data.format = PLAYER_CAMERA_FORMAT_RGB565;
  }
  else if (strcasecmp(this->palette, "RGB24") == 0)
  {
    fg_set_format(this->fg, VIDEO_PALETTE_RGB24 );    
    this->frame = frame_new(this->width, this->height, VIDEO_PALETTE_RGB24 );
    this->data.format = PLAYER_CAMERA_FORMAT_RGB888;
  }
  else if (strcasecmp(this->palette, "RGB32") == 0)
  {
    fg_set_format(this->fg, VIDEO_PALETTE_RGB32 );
    this->frame = frame_new(this->width, this->height, VIDEO_PALETTE_RGB32 );
    this->data.format = PLAYER_CAMERA_FORMAT_RGB888;
  }
  else if (strcasecmp(this->palette, "YUV420P") == 0)
  {
    /// @todo Add support for color (write YUV -> RGB converter)
    // For YUV cameras, we provide the Y component only (greyscale
    // image).  At some point, someone should probably write the
    // planer-to-rgb conversion.
    fg_set_format(this->fg, VIDEO_PALETTE_YUV420P);
    this->frame = frame_new(this->width, this->height, VIDEO_PALETTE_YUV420P );
    this->data.format = PLAYER_CAMERA_FORMAT_MONO8;
    this->depth = 8;
  }
  else
  {
    PLAYER_ERROR2("image depth %d is not supported (add it yourself in %s)",
                  this->depth, __FILE__);
    return 1;
  }

  fg_set_capture_window(this->fg, 0, 0, this->width, this->height);
    
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
      //printf("click %d\n", frameno);
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
  
  while ((len = GetConfig(&client, &request, sizeof(request),NULL)) > 0)
  {
    switch (request[0])
    {
      case PLAYER_FIDUCIAL_GET_GEOM:
        HandleGetGeom(client, request, len);
        break;

      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
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

  if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}



////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void CameraV4L::WriteData()
{
  size_t image_size, size;

  // Compute size of image
  image_size = this->width * this->height * this->depth / 8;
  
  // Set the image properties
  this->data.width = htons(this->width);
  this->data.height = htons(this->height);
  this->data.bpp = this->depth;
  this->data.compression = PLAYER_CAMERA_COMPRESS_RAW;
  this->data.image_size = htonl(image_size);

  assert(image_size <= sizeof(this->data.image));
  assert(image_size <= (size_t) this->frame->size);
  
  // Copy the image pixels
  memcpy(this->data.image, this->frame->data, image_size);

  // Copy data to server
  size = sizeof(this->data) - sizeof(this->data.image) + image_size;

  struct timeval timestamp;
  timestamp.tv_sec = this->tsec;
  timestamp.tv_usec = this->tusec;
  PutData((void*) &this->data, size, &timestamp);

  return;
}


