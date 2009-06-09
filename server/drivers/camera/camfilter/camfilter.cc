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
/////////////////////////////////////////////////////////////////////////////
//
// Desc: Multiband thresholding for camera interface,
//       see http://en.wikipedia.org/wiki/Thresholding_%28image_processing%29
// Author: Paul Osmialowski
// Date: 24 Jun 2008
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_camfilter camfilter
 * @brief camera image filtering driver

The camfilter driver filters colors of pixels on whole given camera image.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_camera

@par Requires

- @ref interface_camera

@par Configuration requests

- none

@par Configuration file options

- max_color_only (integer)
  - Default: 0 (no effect)
  - when set to 1, only max from R G B is passed, other values are changed to 0

- r_min (integer)
  - Default: -1 (no filter)
  - R minimal threshold value
- g_min (integer)
  - Default: -1 (no filter)
  - G minimal threshold value
- b_min (integer)
  - Default: -1 (no filter)
  - B minimal threshold value
- grey_min (integer)
  - Default: -1 (no filter)
  - GREY minimal threshold value

- r_max (integer)
  - Default: -1 (no filter)
  - R maximal threshold value
- g_max (integer)
  - Default: -1 (no filter)
  - G maximal threshold value
- b_max (integer)
  - Default: -1 (no filter)
  - B maximal threshold value
- grey_max (integer)
  - Default: -1 (no filter)
  - GREY maximal threshold value

- r_above (integer)
  - Default: 255
  - new value for each R value above the R maximal threshold
- g_above (integer)
  - Default: 255
  - new value for each G value above the G maximal threshold
- b_above (integer)
  - Default: 255
  - new value for each B value above the B maximal threshold
- grey_above (integer)
  - Default: 255
  - new value for each RGB value above the GREY maximal threshold

- r_below (integer)
  - Default: 0
  - new value for each R value below the R minimal threshold
- g_below (integer)
  - Default: 0
  - new value for each G value below the G minimal threshold
- b_below (integer)
  - Default: 0
  - new value for each B value below the B minimal threshold
- grey_below (integer)
  - Default: 0
  - new value for each RGB value below the GREY minimal threshold

- r_passed (integer)
  - Default: -1 (no change)
  - new value for each R value between the R minimal and the R maximal thresholds
- g_passed (integer)
  - Default: -1 (no change)
  - new value for each G value between the G minimal and the G maximal thresholds
- b_passed (integer)
  - Default: -1 (no change)
  - new value for each B value between the B minimal and the B maximal thresholds
- grey_passed (integer)
  - Default: -1 (no change)
  - new value for each RGB value between the GREY minimal and the GREY maximal thresholds (-1 = no change, -2 = RGB->GREY conversion)
  - this setting overrides other *_passed settings

@par Example

@verbatim
driver
(
  name "camfilter"
  requires ["camera:1"]
  provides ["camera:0"]
  r_min 128
  r_below 0
  r_passed 255
  g_passed 0
  b_passed 0
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <libplayercore/playercore.h>
#include <config.h>
#if HAVE_JPEG
  #include <libplayerjpeg/playerjpeg.h>
#endif

class CamFilter : public ThreadedDriver
{
  public: CamFilter(ConfigFile * cf, int section);
  public: virtual ~CamFilter();

  public: virtual int MainSetup();
  public: virtual void MainQuit();

  // This method will be invoked on each incoming message
  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
			             void * data);

  private: virtual void Main();

  // Input camera device
  private: player_devaddr_t camera_provided_addr, camera_id;
  private: Device * camera;
  private: unsigned char * buffer;
  private: size_t bufsize;

  private: int max_color_only;
  private: int r_min;
  private: int g_min;
  private: int b_min;
  private: int grey_min;
  private: int r_max;
  private: int g_max;
  private: int b_max;
  private: int grey_max;
  private: int r_above;
  private: int g_above;
  private: int b_above;
  private: int grey_above;
  private: int r_below;
  private: int g_below;
  private: int b_below;
  private: int grey_below;
  private: int r_passed;
  private: int g_passed;
  private: int b_passed;
  private: int grey_passed;
};

Driver * CamFilter_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new CamFilter(cf, section));
}

void camfilter_Register(DriverTable *table)
{
  table->AddDriver("camfilter", CamFilter_Init);
}

CamFilter::CamFilter(ConfigFile * cf, int section)
  : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  memset(&(this->camera_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->camera_id), 0, sizeof(player_devaddr_t));
  this->camera = NULL;
  this->buffer = NULL;
  this->bufsize = 0;
  if (cf->ReadDeviceAddr(&(this->camera_provided_addr), section, "provides", PLAYER_CAMERA_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->camera_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&this->camera_id, section, "requires", PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  this->max_color_only = cf->ReadInt(section, "max_color_only", 0);
  this->r_min = cf->ReadInt(section, "r_min", -1);
  this->g_min = cf->ReadInt(section, "g_min", -1);
  this->b_min = cf->ReadInt(section, "b_min", -1);
  this->grey_min = cf->ReadInt(section, "grey_min", -1);
  this->r_max = cf->ReadInt(section, "r_max", -1);
  this->g_max = cf->ReadInt(section, "g_max", -1);
  this->b_max = cf->ReadInt(section, "b_max", -1);
  this->grey_max = cf->ReadInt(section, "grey_max", -1);
  this->r_above = cf->ReadInt(section, "r_above", 255);
  this->g_above = cf->ReadInt(section, "g_above", 255);
  this->b_above = cf->ReadInt(section, "b_above", 255);
  this->grey_above = cf->ReadInt(section, "grey_above", 255);
  this->r_below = cf->ReadInt(section, "r_below", 0);
  this->g_below = cf->ReadInt(section, "g_below", 0);
  this->b_below = cf->ReadInt(section, "b_below", 0);
  this->grey_below = cf->ReadInt(section, "grey_below", 0);
  this->r_passed = cf->ReadInt(section, "r_passed", -1);
  this->g_passed = cf->ReadInt(section, "g_passed", -1);
  this->b_passed = cf->ReadInt(section, "b_passed", -1);
  this->grey_passed = cf->ReadInt(section, "grey_passed", -1);
}

CamFilter::~CamFilter()
{
  if (this->buffer) free(this->buffer);
}

int CamFilter::MainSetup()
{
  if (Device::MatchDeviceAddress(this->camera_id, this->camera_provided_addr))
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
    this->camera = NULL;
    return -1;
  }
  return 0;
}

void CamFilter::MainQuit()
{
  if (this->camera) this->camera->Unsubscribe(this->InQueue);
  this->camera = NULL;
}

void CamFilter::Main()
{
  for (;;)
  {
    this->InQueue->Wait();
    pthread_testcancel();
    this->ProcessMessages();
    pthread_testcancel();
  }
}

int CamFilter::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_camera_data_t * output;
  int i, j;
  size_t new_size;
  unsigned char * ptr, * ptr1;
  player_camera_data_t * rawdata;
  unsigned char r, g, b, grey, max;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, this->camera_id))
  {
    assert(data);
    rawdata = reinterpret_cast<player_camera_data_t *>(data);
    if ((rawdata->width <= 0) || (rawdata->height <= 0))
    {
      return -1;
    } else
    {
      new_size = rawdata->width * rawdata->height * 3;
      ptr = NULL;
      switch (rawdata->compression)
      {
      case PLAYER_CAMERA_COMPRESS_RAW:
        switch (rawdata->bpp)
        {
        case 8:
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
	  ptr1 = reinterpret_cast<unsigned char *>(rawdata->image);
	  for (i = 0; i < static_cast<int>(rawdata->height); i++)
	  {
	    for (j = 0; j < static_cast<int>(rawdata->width); j++)
	    {
	      ptr[0] = *ptr1;
	      ptr[1] = *ptr1;
	      ptr[2] = *ptr1;
	      ptr += 3; ptr1++;
	    }
	  }
	  ptr = this->buffer;
	  break;
        case 24:
	  ptr = reinterpret_cast<unsigned char *>(rawdata->image);
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
          ptr1 = reinterpret_cast<unsigned char *>(rawdata->image);
	  for (i = 0; i < static_cast<int>(rawdata->height); i++)
	  {
	    for (j = 0; j < static_cast<int>(rawdata->width); j++)
	    {
              ptr[0] = ptr1[0];
              ptr[1] = ptr1[1];
              ptr[2] = ptr1[2];
	      ptr += 3; ptr1 += 4;
	    }
          }
          ptr = this->buffer;
          break;
        default:
          PLAYER_WARN("unsupported image depth (not good)");
          return -1;
	}
        break;
#if HAVE_JPEG
      case PLAYER_CAMERA_COMPRESS_JPEG:
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
	jpeg_decompress(this->buffer, this->bufsize, rawdata->image, rawdata->image_count);
	ptr = this->buffer;
        break;
#endif
      default:
        PLAYER_WARN("unsupported compression scheme (not good)");
        return -1;
      }
      assert(ptr);
      output = reinterpret_cast<player_camera_data_t *>(malloc(sizeof(player_camera_data_t)));
      if (!output)
      {
        PLAYER_ERROR("Out of memory");
	return -1;
      }
      memset(output, 0, sizeof(player_camera_data_t));
      output->bpp = 24;
      output->format = PLAYER_CAMERA_FORMAT_RGB888;
      output->fdiv = rawdata->fdiv;
      output->width = rawdata->width;
      output->height = rawdata->height;
      output->image_count = output->width * output->height * 3;
      output->image = reinterpret_cast<uint8_t *>(malloc(output->image_count));
      if (!(output->image))
      {
	free(output);
        PLAYER_ERROR("Out of memory");
	return -1;
      }
      ptr1 = ptr;
      ptr = reinterpret_cast<unsigned char *>(output->image);
      for (i = 0; i < static_cast<int>(rawdata->height); i++)
      {
	for (j = 0; j < static_cast<int>(rawdata->width); j++)
	{
	  r = ptr1[0];
	  g = ptr1[1];
	  b = ptr1[2];
	  if (this->max_color_only)
	  {
	    max = r;
	    if (g > max) max = g;
	    if (b > max) max = b;
	    if (r < max) r = 0;
	    if (g < max) g = 0;
	    if (b < max) b = 0;
	  }
	  grey = static_cast<unsigned char>((0.299 * static_cast<double>(r)) + (0.587 * static_cast<double>(g)) + (0.114 * static_cast<double>(b)));
	  switch (this->grey_passed)
	  {
	  case -1:
            ptr[0] = (this->r_passed != -1) ? static_cast<unsigned char>(this->r_passed) : r;
	    ptr[1] = (this->g_passed != -1) ? static_cast<unsigned char>(this->g_passed) : g;
	    ptr[2] = (this->b_passed != -1) ? static_cast<unsigned char>(this->b_passed) : b;
	    break;
	  case -2:
	    ptr[0] = grey;
	    ptr[1] = grey;
	    ptr[2] = grey;
	    break;
	  default:
	    ptr[0] = static_cast<unsigned char>(this->grey_passed);
	    ptr[1] = static_cast<unsigned char>(this->grey_passed);
	    ptr[2] = static_cast<unsigned char>(this->grey_passed);
	  }
	  if ((this->r_min >= 0) && (r < static_cast<unsigned char>(this->r_min))) ptr[0] = static_cast<unsigned char>(this->r_below);
	  if ((this->g_min >= 0) && (g < static_cast<unsigned char>(this->g_min))) ptr[1] = static_cast<unsigned char>(this->g_below);
	  if ((this->b_min >= 0) && (b < static_cast<unsigned char>(this->b_min))) ptr[2] = static_cast<unsigned char>(this->b_below);
	  if ((this->grey_min >= 0) && (grey < static_cast<unsigned char>(this->grey_min)))
	  {
	    ptr[0] = static_cast<unsigned char>(this->grey_below);
	    ptr[1] = static_cast<unsigned char>(this->grey_below);
	    ptr[2] = static_cast<unsigned char>(this->grey_below);
	  }
	  if ((this->r_max >= 0) && (r > static_cast<unsigned char>(this->r_max))) ptr[0] = static_cast<unsigned char>(this->r_above);
	  if ((this->g_max >= 0) && (g > static_cast<unsigned char>(this->g_max))) ptr[1] = static_cast<unsigned char>(this->g_above);
	  if ((this->b_max >= 0) && (b > static_cast<unsigned char>(this->b_max))) ptr[2] = static_cast<unsigned char>(this->b_above);
	  if ((this->grey_max >= 0) && (grey > static_cast<unsigned char>(this->grey_max)))
	  {
	    ptr[0] = static_cast<unsigned char>(this->grey_above);
	    ptr[1] = static_cast<unsigned char>(this->grey_above);
	    ptr[2] = static_cast<unsigned char>(this->grey_above);
	  }
	  ptr += 3; ptr1 += 3;
	}
      }
      Publish(this->camera_provided_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, reinterpret_cast<void *>(output), 0, &(hdr->timestamp), false);
      // I assume that Publish() freed those output data!
      output = NULL;
    }
    return 0;
  }
  return -1;
}
