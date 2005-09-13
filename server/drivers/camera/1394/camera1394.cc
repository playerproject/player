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

@par Compile-time dependencies

- libraw1394
- libdc1394_control

@par Provides

- @ref player_interface_camera

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- port (integer)
  - Default: 0
  - The 1394 port the camera is attached to.

- node (integer)
  - Default: 0
  - The node within the port

- framerate (float)
  - Default: 15
  - Requested frame rate (frames/second)

- mode (string)
  - Default: "640x480_yuv422"
  - Capture mode (size and color layour).  Valid modes are:
    - "320x240_yuv422"
    - "640x480_mono"
    - "640x480_yuv422"
    - "640x480_rgb"
    - "800x600_mono"
    - "800x600_yuv422" - will be rescaled to 600x450
    - "1024x768_mono"
    - "1024x768_yuv422" - will be rescaled to 512x384
    - "1280x960_mono"
    - "1280x960_yuv422" - will be rescaled to 640x480
  - Currently, all mono modes will produce 8-bit monochrome images unless 
  a color decoding option is provided (see bayer).
  - All yuv422 modes are converted to RGB24
  
- force_raw (integer)
  - Default: 0
  - Force the driver to use (slow) memory capture instead of DMA transfer
  (for buggy 1394 drivers).
  
- save (integer)
  - Default: 0
  - Debugging option: set this to write each frame as an image file on disk.

- bayer (string)
  - Default: None.
  - Bayer color decoding options for cameras such as the Point Grey Dragonfly and Bummblebee. 
  Option activates color decoding and specifies the Bayer color pattern. Valid modes are:
    - "BGGR"
    - "GRBG"
    - "RGGB"
    - "GBRG"

- method (string)    
  - Default: None (or "DownSample" if bayer option is specified)
  - Determines the algorithm used for Bayer coloro decoding. Valid modes are:
    - "DownSample"
    - "Nearest"
    - "Edge"

- brightness (string or unsigned int)
  - Default: None
  - Sets the camera brightness setting. Valid modes are:
    - "auto"
    - any suitable unsigned integer

- exposure (string or unsigned int)
  - Default: None
  - Sets the camera exposure setting. Valid modes are:
    - "auto"
    - any suitable unsigned integer

- shutter (string or unsigned int)
  - Default: None
  - Sets the camera shutter setting. Valid modes are:
    - "auto"
    - any suitable unsigned integer

- gain (string or unsigned int)
  - Default: None
  - Sets the camera gain setting. Valid modes are:
    - "auto"
    - any suitable unsigned integer

- whitebalance (string)
  - Default: None
  - Sets the manual camera white balance setting. Only valid option:
    - a string containing two suitable blue and red value unsigned integers 

@par Example 

@verbatim
driver
(
  name "camera1394"
  provides ["camera:0"]
)
@endverbatim

@par Authors

Nate Koenig, Andrew Howard
Major code rewrite by Paul Osmialowski, newchief@king.net.pl

*/
/** @} */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <stddef.h>       // for NULL
#include <unistd.h>
#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>

#include <libplayercore/playercore.h>
#include <libplayercore/error.h>

// for color and format conversion (located in cmvision)
#include "conversions.h"

#define NUM_DMA_BUFFERS 4

// Driver for detecting laser retro-reflectors.
class Camera1394 : public Driver
{
  // Constructor
  public: Camera1394( ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();
  private: void SafeCleanup();

  // Main function for device thread.
  private: virtual void Main();

  // This method will be invoked on each incoming message
  public: virtual int ProcessMessage(MessageQueue* resp_queue,
                                     player_msghdr * hdr,
                                     void * data);

  // Save a frame to memory
  private: int GrabFrame();

  private: unsigned char resized[1280 * 960 * 3];

  // Save a frame to disk
  private: int SaveFrame( const char *filename );

  // Update the device data (the data going back to the client).
  private: void RefreshData();

  // Video device
  private: unsigned int port;
  private: unsigned int node;
  private: raw1394handle_t handle;
  private: dc1394_cameracapture camera;

  // Camera features
  private: dc1394_feature_set features;

  // Capture method: RAW or VIDEO (DMA)
  private: enum {methodRaw, methodVideo, methodNone};
  private: int method;
  private: bool forceRaw;

  // Framerate.An  enum defined in libdc1394/dc1394_control.h
  private: unsigned int frameRate;

  // Format & Mode. An enum defined in libdc1394/dc1394_control.h
  private: unsigned int format;
  private: unsigned int mode;

  // Write frames to disk?
  private: int save;

  // Frame capture size
  private: size_t frameSize;

  // Capture timestamp
  private: struct timeval frameTime;
  
  // Data to send to server
  private: player_camera_data_t data;

  // Bayer Colour Conversion
  private: bool DoBayerConversion;
  private: int BayerPattern;
  private: int BayerMethod;

  // Camera settings
  private: bool setBrightness, setExposure, setWhiteBalance, setShutter, setGain;
  private: bool autoBrightness, autoExposure, autoShutter, autoGain;
  private: unsigned int brightness, exposure, redBalance, blueBalance, shutter, gain;

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
Camera1394::Camera1394(ConfigFile* cf, int section)
  : Driver(cf,
           section,
           true,
           PLAYER_MSGQUEUE_DEFAULT_MAXLEN,
           PLAYER_CAMERA_CODE)
{
  float fps;

  this->handle = NULL;
  this->method = methodNone;

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
  str =  cf->ReadString(section, "mode", "640x480_yuv422");
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
    this->frameSize = 320 * 240 * 3;
  }
  /*
  else if (0==strcmp(str,"640x480_mono16"))
  {
    this->mode = MODE_640x480_MONO16;
    assert(false);
  }
  else if (0==strcmp(str,"640x480_yuv411"))
  {
    this->mode = MODE_640x480_YUV411;
  }
  */
  else if (0==strcmp(str,"640x480_mono"))
  {
    this->mode = MODE_640x480_MONO;
    this->frameSize = 640 * 480 * 1;
  }
  else if (0==strcmp(str,"640x480_yuv422"))
  {
    this->mode = MODE_640x480_YUV422;
    this->frameSize = 640 * 480 * 3;
  }
  else if (0==strcmp(str,"640x480_rgb"))
  {
    this->mode = MODE_640x480_RGB;
    this->frameSize = 640 * 480 * 3;
  }
  else if (0==strcmp(str,"800x600_mono"))
  {
    this->mode = MODE_800x600_MONO;
    this->format = FORMAT_SVGA_NONCOMPRESSED_1;
    this->frameSize = 800 * 600 * 1;
  }
  else if (0==strcmp(str,"800x600_yuv422"))
  {
    this->mode = MODE_800x600_YUV422;
    this->format = FORMAT_SVGA_NONCOMPRESSED_1;
    this->frameSize = 600 * 450 * 3;
  }
  else if (0==strcmp(str,"1024x768_mono"))
  {
    this->mode = MODE_1024x768_MONO;
    this->format = FORMAT_SVGA_NONCOMPRESSED_1;
    this->frameSize = 1024 * 768 * 1;
  }
  else if (0==strcmp(str,"1024x768_yuv422"))
  {
    this->mode = MODE_1024x768_YUV422;
    this->format = FORMAT_SVGA_NONCOMPRESSED_1;
    this->frameSize = 512 * 384 * 3;
  }
  else if (0==strcmp(str,"1280x960_mono"))
  {
    this->mode = MODE_1280x960_MONO;
    this->format = FORMAT_SVGA_NONCOMPRESSED_2;
    this->frameSize = 1280 * 960 * 1;
  }
  else if (0==strcmp(str,"1280x960_yuv422"))
  {
    this->mode = MODE_1280x960_YUV422;
    this->format = FORMAT_SVGA_NONCOMPRESSED_2;
    this->frameSize = 640 * 480 * 3;
  }
  else
  {
    PLAYER_ERROR1("unknown video mode [%s]", str);
    this->SetError(-1);
    return;
  }

  // Many cameras such as the Pt Grey Dragonfly and Bumblebee devices
  // don't do onchip colour conversion. Various Bayer colour encoding 
  // patterns and decoding methods exist. They are now presented to the 
  // user as config file options.
  // check Bayer colour decoding option
  str =  cf->ReadString(section, "bayer", "NONE");
  this->DoBayerConversion = false;
  if (strcmp(str,"NONE"))
       {
	    if (!strcmp(str,"BGGR"))
		 {
		      this->DoBayerConversion = true;
		      this->BayerPattern = BAYER_PATTERN_BGGR;
		 }
	    else if (!strcmp(str,"GRBG"))
		 {
		      this->DoBayerConversion = true;
		      this->BayerPattern = BAYER_PATTERN_GRBG;
		 }
	    else if (!strcmp(str,"RGGB"))
		 {
		      this->DoBayerConversion = true;
		      this->BayerPattern = BAYER_PATTERN_RGGB;
		 }
	    else if (!strcmp(str,"GBRG"))
		 {
		      this->DoBayerConversion = true;
		      this->BayerPattern = BAYER_PATTERN_GBRG;
		 }
	    else
		 {
		      PLAYER_ERROR1("unknown bayer pattern [%s]", str);
		      this->SetError(-1);
		      return;
		 }
       }
  // Set default Bayer decoding method
  if (this->DoBayerConversion)
       this->BayerMethod = BAYER_DECODING_DOWNSAMPLE;
  else
       this->BayerMethod = NO_BAYER_DECODING;
  // Check for user selected method
  str =  cf->ReadString(section, "method", "NONE");
  if (strcmp(str,"NONE"))
       {
	    if (!this->DoBayerConversion)
		 {
		      PLAYER_ERROR1("bayer method [%s] specified without enabling bayer conversion", str);
		      this->SetError(-1);
		      return;
		 }
	    if (!strcmp(str,"DownSample"))
		 {
		      this->BayerMethod = BAYER_DECODING_DOWNSAMPLE;
		 }
	    else if (!strcmp(str,"Nearest"))
		 {
		      this->BayerMethod = BAYER_DECODING_NEAREST;
		 }
	    else if (!strcmp(str,"Edge"))
		 {
		      this->BayerMethod = BAYER_DECODING_EDGE_SENSE;
		 }
	    else
		 {
		      PLAYER_ERROR1("unknown bayer method [%s]", str);
		      this->SetError(-1);
		      return;
		 }
       }

  // Allow the user to set useful camera setting options
  // brightness, exposure, redBalance, blueBalance, shutter and gain
  // Parse camera settings - default is to leave them alone. 
  str =  cf->ReadString(section, "brightness", "NONE");
  if (strcmp(str,"NONE"))
       if (!strcmp(str,"auto"))
	    {
		 this->setBrightness=true;
		 this->autoBrightness=true;
	    }
       else
	    {
		 this->setBrightness=true;
		 this->autoBrightness=false;
		 this->brightness = atoi(str);		 
	    }
  str =  cf->ReadString(section, "exposure", "NONE");
  if (strcmp(str,"NONE"))
       if (!strcmp(str,"auto"))
	    {
		 this->setExposure=true;
		 this->autoExposure=true;
	    }
       else
	    {
		 this->setExposure=true;
		 this->autoExposure=false;
		 this->exposure = atoi(str);		 
	    }
  str =  cf->ReadString(section, "shutter", "NONE");
  if (strcmp(str,"NONE"))
       if (!strcmp(str,"auto"))
	    {
		 this->setShutter=true;
		 this->autoShutter=true;
	    }
       else
	    {
		 this->setShutter=true;
		 this->autoShutter=false;
		 this->shutter = atoi(str);		 
	    }
  str =  cf->ReadString(section, "gain", "NONE");
  if (strcmp(str,"NONE"))
       if (!strcmp(str,"auto"))
	    {
		 this->setGain=true;
		 this->autoGain=true;
	    }
       else
	    {
		 this->setGain=true;
		 this->autoGain=false;
		 this->gain = atoi(str);		 
	    }
  str =  cf->ReadString(section, "whitebalance", "NONE");
  if (strcmp(str,"NONE"))
       if(sscanf(str,"%u %u",&this->blueBalance,&this->redBalance)==2)
	    this->setWhiteBalance=true;
       else
	    PLAYER_ERROR1("didn't understand white balance values [%s]", str);

  // Force into raw mode
  this->forceRaw = cf->ReadInt(section, "force_raw", 0);
 
  // Save frames?
  this->save = cf->ReadInt(section, "save", 0);

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Safe Cleanup
void Camera1394::SafeCleanup()
{
  if (this->handle)
  {
    switch (this->method)
    {
    case methodRaw:
      dc1394_release_camera(this->handle, &this->camera);
      break;
    case methodVideo:
      dc1394_dma_unlisten(this->handle, &this->camera);
      dc1394_dma_release_camera(this->handle, &this->camera);
      break;
    }
    dc1394_destroy_handle(this->handle);
  }
  this->handle = NULL;
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
    this->SafeCleanup();
    return -1;
  }

  this->camera.node = this->node;
  this->camera.port = this->port;


  // apply user config file provided camera settings
  if (this->setBrightness)
       {
	    if (DC1394_SUCCESS != dc1394_auto_on_off(this->handle, this->camera.node,FEATURE_BRIGHTNESS,this->autoBrightness))
		 {
		      PLAYER_ERROR("Unable to set Brightness mode");
		      this->SafeCleanup();
		      return -1;
		 }
	    if (!this->autoBrightness)
		 if (DC1394_SUCCESS != dc1394_set_brightness(this->handle, this->camera.node,this->brightness))
		 {
		      PLAYER_ERROR("Unable to set Brightness value");
		      this->SafeCleanup();
		      return -1;
		 }
       }
  if (this->setExposure)
       {
	    if (DC1394_SUCCESS != dc1394_auto_on_off(this->handle, this->camera.node,FEATURE_EXPOSURE,this->autoExposure))
		 {
		      PLAYER_ERROR("Unable to set Exposure mode");
		      this->SafeCleanup();
		      return -1;
		 }
	    if (!this->autoExposure)
		 if (DC1394_SUCCESS != dc1394_set_exposure(this->handle, this->camera.node,this->exposure))
		 {
		      PLAYER_ERROR("Unable to set Exposure value");
		      this->SafeCleanup();
		      return -1;
		 }
       }
  if (this->setShutter)
       {
	    if (DC1394_SUCCESS != dc1394_auto_on_off(this->handle, this->camera.node,FEATURE_SHUTTER,this->autoShutter))
		 {
		      PLAYER_ERROR("Unable to set Shutter mode");
		      this->SafeCleanup();
		      return -1;
		 }
	    if (!this->autoShutter)
		 if (DC1394_SUCCESS != dc1394_set_shutter(this->handle, this->camera.node,this->shutter))
		 {
		      PLAYER_ERROR("Unable to set Shutter value");
		      this->SafeCleanup();
		      return -1;
		 }
       }
  if (this->setGain)
       {
	    if (DC1394_SUCCESS != dc1394_auto_on_off(this->handle, this->camera.node,FEATURE_GAIN,this->autoGain))
		 {
		      PLAYER_ERROR("Unable to set Gain mode");
		      this->SafeCleanup();
		      return -1;
		 }
	    if (!this->autoShutter)
		 if (DC1394_SUCCESS != dc1394_set_gain(this->handle, this->camera.node,this->gain))
		 {
		      PLAYER_ERROR("Unable to Gain value");
		      this->SafeCleanup();
		      return -1;
		 }
       }
  if (this->setWhiteBalance)
	    if (DC1394_SUCCESS != dc1394_set_white_balance(this->handle, this->camera.node,this->blueBalance,this->redBalance))
		 {
		      PLAYER_ERROR("Unable to set White Balance");
		      this->SafeCleanup();
		      return -1;
		 }


  // Collects the available features for the camera described by node and
  // stores them in features.
  if (DC1394_SUCCESS != dc1394_get_camera_feature_set(this->handle, this->camera.node, 
                                                      &this->features))
  {
    PLAYER_ERROR("Unable to get feature set");
    this->SafeCleanup();
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
    this->SafeCleanup();
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
      this->SafeCleanup();
      return -1;
    }
  }

  // Start transmitting camera data
  if (DC1394_SUCCESS != dc1394_start_iso_transmission(this->handle, this->camera.node))
  {
    PLAYER_ERROR("unable to start camera");
    this->SafeCleanup();
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
  this->SafeCleanup();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Camera1394::Main() 
{
  char filename[255];
  int frameno;

  frameno = 0;
  //struct timeval now,old;
  while (true)
  {
    
    // Go to sleep for a while (this is a polling loop).
    // We shouldn't need to sleep if GrabFrame is blocking.    
    //nanosleep(&NSLEEP_TIME, NULL);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    ProcessMessages();

    // Grab the next frame (blocking)
    this->GrabFrame();

    // Write data to server
    this->RefreshData();

    // this should go or be replaced
    // Save frames; must be done after writedata (which will byteswap)
    if (this->save)
    {
      //printf("click %d\n", frameno);
      snprintf(filename, sizeof(filename), "click-%04d.ppm", frameno++);
      this->SaveFrame(filename);
    }
    /*
      gettimeofday(&now,NULL);
      printf("dt = %lf\n",now.tv_sec-old.tv_sec+(now.tv_usec-old.tv_usec)*1.0e-6);
      old=now;
    */
 }
  printf("Camera1394::main() exited\n");
  
}


////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int Camera1394::ProcessMessage(MessageQueue* resp_queue,
                              player_msghdr * hdr,
                              void * data)
{
  assert(resp_queue);
  assert(hdr);
  assert(data);

  /* We currently don't support any messages, but if we do...
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                           PLAYER_FIDUCIAL_GET_GEOM, this->device_addr))
  {
    assert(hdr->size == sizeof(player_position2d_data_t));
    ProcessOdom(hdr, *reinterpret_cast<player_position2d_data_t *> (data));
    return(0);

  }
  */

  return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Store an image frame into the 'frame' buffer
int Camera1394::GrabFrame()
{
  uint f, c;
  int  i, j;
  unsigned char * ptr1, * ptr2, * dst;

  switch (this->method)
  {
  case methodRaw:
    if (dc1394_single_capture(this->handle, &this->camera) != DC1394_SUCCESS)
    {
      PLAYER_ERROR("Unable to capture frame");
      return -1;
    }
    break;
  case methodVideo:
    if (dc1394_dma_single_capture(&this->camera) != DC1394_SUCCESS)
    {
      PLAYER_ERROR("Unable to capture frame");
      return -1;
    }
    break;
  default:
    PLAYER_ERROR("Unknown grab method");
    return -1;
  }

  switch (this->mode)
  {
  case MODE_320x240_YUV422:
  case MODE_640x480_YUV422:
    this->data.bpp = 24;
    this->data.format = PLAYER_CAMERA_FORMAT_RGB888;
    this->data.image_count = this->frameSize;
    this->data.width = this->camera.frame_width;
    this->data.height = this->camera.frame_height;
    assert(this->data.image_count <= sizeof(this->data.image));
    uyvy2rgb((unsigned char *)this->camera.capture_buffer, this->data.image, (this->camera.frame_width) * (this->camera.frame_height));
    break;
  case MODE_1024x768_YUV422:
  case MODE_1280x960_YUV422:
    this->data.bpp = 24;
    this->data.format = PLAYER_CAMERA_FORMAT_RGB888;
    this->data.image_count = this->frameSize;
    this->data.width = this->camera.frame_width / 2;
    this->data.height = this->camera.frame_height / 2;
    assert(this->data.image_count <= sizeof(this->data.image));
    uyvy2rgb((unsigned char *)this->camera.capture_buffer, this->resized, (this->camera.frame_width) * (this->camera.frame_height));
    ptr1 = this->resized;
    ptr2 = this->data.image;
    for (f = 0; f < (this->data.height); f++)
    {
      for (c = 0; c < (this->data.width); c++)
      {
	ptr2[0] = ptr1[0];
	ptr2[1] = ptr1[1];
	ptr2[2] = ptr1[2];
	ptr1 += (3 * 2);
	ptr2 += 3;
      }
      ptr1 += ((this->camera.frame_width) * 3);
    }
    break;
  case MODE_800x600_YUV422:
    this->data.bpp = 24;
    this->data.format = PLAYER_CAMERA_FORMAT_RGB888;
    this->data.image_count = this->frameSize;
    this->data.width = 600;
    this->data.height = 450;
    assert(this->data.image_count <= sizeof(this->data.image));
    uyvy2rgb((unsigned char *)this->camera.capture_buffer, this->resized, (this->camera.frame_width) * (this->camera.frame_height));
    ptr1 = this->resized;
    ptr2 = this->data.image;
    i = 3; j = 3;
    for (f = 0; f < (this->data.height); f++)
    {
      for (c = 0; c < (this->data.width); c++)
      {
	ptr2[0] = ptr1[0];
	ptr2[1] = ptr1[1];
	ptr2[2] = ptr1[2];
	j--;
	if (!j) ptr1 += (3 * 2), j = 3;
	else ptr1 += 3;
	ptr2 += 3;
      }
      i--;
      if (!i) ptr1 += ((this->camera.frame_width) * 3), i = 3;
    }
    break;  
  case MODE_640x480_RGB:
    this->data.bpp = 24;
    this->data.format = PLAYER_CAMERA_FORMAT_RGB888;
    this->data.image_count = this->frameSize;
    this->data.width = this->camera.frame_width;
    this->data.height = this->camera.frame_height;
    assert(this->data.image_count <= sizeof(this->data.image));
    memcpy(this->data.image, (unsigned char *)this->camera.capture_buffer, this->data.image_count);
    break;
  case MODE_640x480_MONO:
  case MODE_800x600_MONO:
  case MODE_1024x768_MONO:
  case MODE_1280x960_MONO:
    if (!DoBayerConversion)
    {
      this->data.bpp = 8;
      this->data.format = PLAYER_CAMERA_FORMAT_MONO8;
      this->data.image_count = this->frameSize;
      this->data.width = this->camera.frame_width;
      this->data.height = this->camera.frame_height;
      assert(this->data.image_count <= sizeof(this->data.image));
      memcpy(this->data.image, (unsigned char *)this->camera.capture_buffer, this->data.image_count);
    } else
    {
      this->data.bpp = 24;
      this->data.format = PLAYER_CAMERA_FORMAT_RGB888;
      if ((this->camera.frame_width) > PLAYER_CAMERA_IMAGE_WIDTH) dst = this->resized;
      else dst = this->data.image;
      switch (this->BayerMethod)
      {
      case BAYER_DECODING_DOWNSAMPLE:
        // quarter of the image but 3 bytes per pixel
	this->data.image_count = this->frameSize/4*3;
	assert(this->data.image_count <= sizeof(this->data.image));
	BayerDownsample((unsigned char *)this->camera.capture_buffer, this->data.image,
					this->camera.frame_width/2, this->camera.frame_height/2,
					(bayer_pattern_t)this->BayerPattern);
	break;
      case BAYER_DECODING_NEAREST:
        if ((this->camera.frame_width) > PLAYER_CAMERA_IMAGE_WIDTH) this->data.image_count = this->frameSize/4*3;
	else this->data.image_count = this->frameSize * 3;
	assert(this->data.image_count <= sizeof(this->data.image));
	BayerNearestNeighbor((unsigned char *)this->camera.capture_buffer, dst,
					this->camera.frame_width, this->camera.frame_height,
					(bayer_pattern_t)this->BayerPattern);
	break;
      case BAYER_DECODING_EDGE_SENSE:
        if ((this->camera.frame_width) > PLAYER_CAMERA_IMAGE_WIDTH) this->data.image_count = this->frameSize/4*3;
        else this->data.image_count = this->frameSize * 3;
	assert(this->data.image_count <= sizeof(this->data.image));
	BayerEdgeSense((unsigned char *)this->camera.capture_buffer, dst,
					this->camera.frame_width, this->camera.frame_height,
					(bayer_pattern_t)this->BayerPattern);
	break;
      default:
        PLAYER_ERROR("camera1394: Unknown Bayer Method");
	return -1;
      }
      if (this->BayerMethod != BAYER_DECODING_DOWNSAMPLE)
      {
        if ((this->camera.frame_width) > PLAYER_CAMERA_IMAGE_WIDTH)
	{
	  this->data.width = this->camera.frame_width/2;
	  this->data.height = this->camera.frame_height/2;
          ptr1 = this->resized;
          ptr2 = this->data.image;
          for (f = 0; f < (this->data.height); f++)
          {
            for (c = 0; c < (this->data.width); c++)
            {
	      ptr2[0] = ptr1[0];
	      ptr2[1] = ptr1[1];
	      ptr2[2] = ptr1[2];
	      ptr1 += (3 * 2);
	      ptr2 += 3;
            }
            ptr1 += ((this->camera.frame_width) * 3);
          }
	} else
	{
	  this->data.width = this->camera.frame_width;
	  this->data.height = this->camera.frame_height;
	}
      } else
      {    //image is half the size of grabbed frame
	this->data.width = this->camera.frame_width/2;
	this->data.height = this->camera.frame_height/2;
      }
    }
    break;
  default:
    PLAYER_ERROR("camera1394: Unknown mode");
    return -1;
  }

  if (this->method == methodVideo) dc1394_dma_done_with_buffer(&this->camera);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void Camera1394::RefreshData()
{
  size_t size;

  // Work out the data size; do this BEFORE byteswapping
  size = sizeof(this->data) - sizeof(this->data.image) + this->data.image_count;

  // now we just do the byte-swapping
  this->data.width = this->data.width;
  this->data.height = this->data.height;

  this->data.compression = PLAYER_CAMERA_COMPRESS_RAW;
  this->data.image_count = this->data.image_count;

  /* We should do this to be efficient */
  Publish(this->device_addr, NULL,
          PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE,
          reinterpret_cast<void*>(&this->data), size, NULL);  
  
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
      fprintf(fp,"P5\n%u %u\n255\n", this->data.width, this->data.height);
      fwrite((unsigned char*)this->data.image, 1, this->data.image_count, fp);
      break;
    case PLAYER_CAMERA_FORMAT_RGB888:
      fprintf(fp,"P6\n%u %u\n255\n", this->data.width, this->data.height);
      fwrite((unsigned char*)this->data.image, 1, this->data.image_count, fp);
      break;
    default:
      break;
  }

  fclose(fp);

  return 0;
}

