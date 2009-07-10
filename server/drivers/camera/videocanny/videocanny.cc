/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004  Brian Gerkey gerkey@stanford.edu
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

/*
 *
 * OpenCV-based edge detection.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_VideoCanny VideoCanny
 * @brief OpenCV-based edge detection.

@par Provides

- @ref interface_camera

@par Requires

- @ref interface_camera

@par Configuration requests

- None

@par Configuration file options

- function (string)
  - Default: "canny"
  - One of: "canny", "laplace", "sobel"

- canny_threshold_1 (integer)
  - Default: 50

- canny_threshold_2 (integer)
  - Default: 90

- sobel_xorder (integer)
  - Default: 2

- sobel_yorder (integer)
  - Default: 1

- apsize (integer)
  - Default: 3

@par Example

@verbatim
driver
(
    name "videocanny"
    function "sobel"
    apsize 7
    requires ["camera:0"]
    provides ["camera:1"]
)
@endverbatim

@author Piotr A. Dutt, Paul Osmialowski

*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <libplayercore/playercore.h>
#include <libplayerjpeg/playerjpeg.h>

#include <cv.h>
#include <highgui.h>

typedef enum { canny = 0, sobel, laplace } func_t;

//---------------------------------

class VideoCanny : public ThreadedDriver
{
  public: VideoCanny(ConfigFile * cf, int section);
  public: virtual ~VideoCanny();

  public: virtual int MainSetup();
  public: virtual void MainQuit();

  // This method will be invoked on each incoming message
  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
			             void * data);

  private: virtual void Main();
  
  // Input camera device
  private: player_devaddr_t camera_id;
  private: Device * camera;

  // Output camera data
  private: player_camera_data_t data;

  private: unsigned char * buffer;
  private: size_t bufsize;
  private: unsigned char * decompress_buffer;
  private: size_t decompress_bufsize;
  private: IplImage * img8;
  private: IplImage * img16;

  // cvCanny Thresholds
  private: int canny_threshold_1;
  private: int canny_threshold_2;

  // cvSobel settings
  private: int sobel_xorder;
  private: int sobel_yorder;

  private: int apsize;
  private: func_t function;
};

Driver * VideoCanny_Init(ConfigFile * cf, int section)
{	
  return reinterpret_cast<Driver *>(new VideoCanny(cf, section));
}

void videocanny_Register(DriverTable * table)
{
  table->AddDriver("videocanny", VideoCanny_Init);
}

VideoCanny::VideoCanny( ConfigFile *cf, int section)
  : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_CAMERA_CODE)
{
  const char * func;

  memset(&(this->camera_id), 0, sizeof(player_devaddr_t));
  memset(&(this->data), 0, sizeof this->data);
  this->data.image = NULL;
  this->camera = NULL;
  this->buffer = NULL;
  this->bufsize = 0;
  this->decompress_buffer = NULL;
  this->decompress_bufsize = 0;
  this->img8 = NULL;
  this->img16 = NULL;
  if (cf->ReadDeviceAddr(&this->camera_id, section, "requires", PLAYER_CAMERA_CODE, -1, NULL) != 0)
    { this->SetError(-1); return; }
  if ((this->canny_threshold_1 = cf->ReadInt(section,"canny_threshold_1", 50)) < 0)
  {   
    PLAYER_WARN("Invalid value for canny_threshold_1 in .cfg file, using 50");
    this->canny_threshold_1 = 50;
  }
  if ((this->canny_threshold_2 = cf->ReadInt(section,"canny_threshold_2", 90)) < 0)
  {
    PLAYER_WARN("Invalid value for canny_threshold_2 in .cfg file, using 90");
    this->canny_threshold_2 = 90;
  }
  if ((this->sobel_xorder = cf->ReadInt(section,"sobel_xorder", 2)) < 0)
  {
    PLAYER_WARN("Invalid value for sobel_xorder in .cfg file, using 2");
    this->sobel_xorder = 2;
  }
  if ((this->sobel_yorder = cf->ReadInt(section,"sobel_yorder", 1)) < 0)
  {
    PLAYER_WARN("Invalid value for sobel_yorder in .cfg file, using 1");
    this->sobel_yorder = 1;
  }
  if ((this->apsize = cf->ReadInt(section,"apsize", 3)) < 0)
  {
    PLAYER_WARN("Invalid value for apsize using 3");
    this->apsize = 3;
  }
  func = cf->ReadString(section, "function", "canny");
  if (!func)
  {
    PLAYER_ERROR("error");
    this->SetError(-1);
    return;
  }
  if (!strcmp(func, "canny")) this->function = canny;
  else if (!strcmp(func, "sobel")) this->function = sobel;
  else if (!strcmp(func, "laplace")) this->function = laplace;
  else
  {
    PLAYER_ERROR("unknown function name given");
    this->SetError(-1);
    return;
  }
}

VideoCanny::~VideoCanny()
{
  if (this->data.image) free(this->data.image);
  if (this->decompress_buffer) free(this->decompress_buffer);
  if (this->buffer) free(this->buffer);
  if (this->img8) cvReleaseImage(&(this->img8));
  if (this->img16) cvReleaseImage(&(this->img16));
}

int VideoCanny::MainSetup()
{
  if (Device::MatchDeviceAddress(this->camera_id, this->device_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return -1;
  }
  this->camera = deviceTable->GetDevice(this->camera_id);  
  if (!this->camera)
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return -1;
  }
  if (this->camera->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    return -1;
  }
  return 0;
}

void VideoCanny::MainQuit()
{
  if (this->camera) this->camera->Unsubscribe(this->InQueue);
  this->camera = NULL;
}

void VideoCanny::Main()
{
  for (;;)
  {
    this->InQueue->Wait();
    pthread_testcancel();
    this->ProcessMessages();
    pthread_testcancel();
  }
}

int VideoCanny::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  int i, j;
  size_t new_size;
  unsigned char * raw, * ptr, * ptr1;
  player_camera_data_t * rawdata;
  int bpp;

  assert(hdr);
  assert(data);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, this->camera_id))
  {
    rawdata = reinterpret_cast<player_camera_data_t *>(data);
    if ((rawdata->width <= 0) || (rawdata->height <= 0))
    {
      if (!(this->data.image)) return -1;
    } else
    {
      switch(rawdata->compression)
      {
      case PLAYER_CAMERA_COMPRESS_RAW:
        raw = reinterpret_cast<unsigned char *>(rawdata->image);
	bpp = rawdata->bpp;
        break;
      case PLAYER_CAMERA_COMPRESS_JPEG:
        new_size = rawdata->width * rawdata->height * 3;
        if (this->decompress_bufsize != new_size)
	{
	  if (this->decompress_buffer) free(this->decompress_buffer);
	  this->decompress_buffer = NULL;
	  this->decompress_bufsize = 0;
	}
	if (!(this->decompress_buffer))
	{
	  this->decompress_bufsize = 0;
	  this->decompress_buffer = reinterpret_cast<unsigned char *>(malloc(new_size));
	  if (!(this->decompress_buffer))
	  {
	    PLAYER_ERROR("Out of memory");
	    return -1;
	  }
	  this->decompress_bufsize = new_size;
	}
	jpeg_decompress(this->decompress_buffer, this->decompress_bufsize, rawdata->image, rawdata->image_count);
	raw = this->decompress_buffer;
	bpp = 24;
        break;
      default:
        PLAYER_WARN("unsupported compression scheme (not good)");
        return -1;
      }
      new_size = rawdata->width * rawdata->height * 1;
      ptr = NULL;
      switch (bpp)
      {
      case 8:
        ptr = raw;
	break;
      case 24:
        if (this->bufsize != new_size)
	{
	  if (this->buffer) free(this->buffer);
	  this->buffer = NULL;
	  this->bufsize = 0;
	}
	if (!(this->buffer))
	{
	  this->bufsize = 0;
	  this->buffer = reinterpret_cast<unsigned char *>(malloc(new_size));
	  if (!(this->buffer))
	  {
	    PLAYER_ERROR("Out of memory");
	    return -1;
	  }
	  this->bufsize = new_size;
	}
	ptr = this->buffer;
	ptr1 = raw;
	for (i = 0; i < static_cast<int>(rawdata->height); i++)
	{
	  for (j = 0; j < static_cast<int>(rawdata->width); j++)
	  {
	    *ptr = static_cast<unsigned char>(static_cast<int>(0.299 * static_cast<double>(ptr1[0]) + 0.587 * static_cast<double>(ptr1[1]) + 0.114 * static_cast<double>(ptr1[2])));
	    ptr++; ptr1 += 3;
	  }
	}
	ptr = this->buffer;
	break;
      case 32:
        if (this->bufsize != new_size)
	{
	  if (this->buffer) free(this->buffer);
	  this->buffer = NULL;
	  this->bufsize = 0;
	}
	if (!(this->buffer))
	{
	  this->bufsize = 0;
	  this->buffer = reinterpret_cast<unsigned char *>(malloc(new_size));
	  if (!(this->buffer))
	  {
	    PLAYER_ERROR("Out of memory");
	    return -1;
	  }
	  this->bufsize = new_size;
	}
	ptr = this->buffer;
	ptr1 = raw;
	for (i = 0; i < static_cast<int>(rawdata->height); i++)
	{
	  for (j = 0; j < static_cast<int>(rawdata->width); j++)
	  {
	    *ptr = static_cast<unsigned char>(static_cast<int>(0.299 * static_cast<double>(ptr1[0]) + 0.587 * static_cast<double>(ptr1[1]) + 0.114 * static_cast<double>(ptr1[2])));
	    ptr++; ptr1 += 4;
	  }
	}
	ptr = this->buffer;
	break;
      default:
        PLAYER_WARN("unsupported image depth (not good)");
        return -1;
      }
      assert(ptr);
      if (this->img8)
      {
        if ((this->img8->width != static_cast<int>(rawdata->width)) || (this->img8->height != static_cast<int>(rawdata->height)))
        {
          if (this->img8) cvReleaseImage(&(this->img8));
  	  this->img8 = NULL;
        }
      }
      if (!(this->img8))
      {
        this->img8 = cvCreateImage(cvSize(rawdata->width, rawdata->height), IPL_DEPTH_8U, 1);
	if (!(this->img8))
	{
	  PLAYER_ERROR("Cannot create cvImage");
	  return -1;
	}
      }
      memcpy(this->img8->imageData, ptr, new_size);
      switch (this->function)
      {
      case canny:
        cvCanny(this->img8, this->img8, this->canny_threshold_1, this->canny_threshold_2, this->apsize);
	break;
      case sobel:
        cvSobel(this->img8, this->img8, this->sobel_xorder, this->sobel_yorder, this->apsize);
        break;
      case laplace:
        if (this->img16)
        {
          if ((this->img16->width != static_cast<int>(rawdata->width)) || (this->img16->height != static_cast<int>(rawdata->height)))
          {
            if (this->img16) cvReleaseImage(&(this->img16));
  	    this->img16 = NULL;
          }
        }
        if (!(this->img16))
        {
          this->img16 = cvCreateImage(cvSize(rawdata->width, rawdata->height), IPL_DEPTH_16S, 1);
	  if (!(this->img16))
	  {
	    PLAYER_ERROR("Cannot create cvImage");
	    return -1;
	  }
        }
        cvLaplace(this->img8, this->img16, this->apsize);
	cvConvertScaleAbs(this->img16, this->img8);
        break;
      default:
        PLAYER_ERROR("Internal error!");
	return -1;
      }
      new_size = this->img8->width * this->img8->height * 1;
      if (this->data.image_count != new_size)
      {
        if (this->data.image) free(this->data.image);
	this->data.image = NULL;
	this->data.image_count = 0;
      }
      if (!(this->data.image))
      {
        this->data.image_count = 0;
        this->data.image = reinterpret_cast<unsigned char *>(malloc(new_size));
	if (!(this->data.image))
	{
	  PLAYER_ERROR("Out of memory");
	  return -1;
	}
	this->data.image_count = new_size;
      }
      memcpy(this->data.image, this->img8->imageData, this->data.image_count);
      this->data.width = this->img8->width;
      this->data.height = this->img8->height;
      this->data.bpp = 8;
      this->data.fdiv = 0;
      this->data.format = PLAYER_CAMERA_FORMAT_MONO8;
      this->data.compression = PLAYER_CAMERA_COMPRESS_RAW;
    }
    this->Publish(device_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, reinterpret_cast<void *>(&(this->data)), 0, &(hdr->timestamp), true);
    // don't delete anything here! this->data.image is required and is deleted somewhere else
    return 0;
  }
  return -1;
}
