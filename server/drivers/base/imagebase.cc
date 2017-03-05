/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al
 *                      gerkey@usc.edu
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: base driver for image processing and transform drivers
// Author: Toby Collett
// Date: 15 Feb 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "imagebase.h"
//#include <libplayerinterface/playerxdr.h>
#include <config.h>
#if HAVE_JPEG
#include <libplayerjpeg/playerjpeg.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// Constructor

ImageBase::ImageBase(ConfigFile *cf, int section, bool overwrite_cmds, size_t queue_maxlen, int interf)
	: ThreadedDriver(cf, section, overwrite_cmds, queue_maxlen, interf)
{
  memset(&this->camera_addr, 0, sizeof(player_devaddr_t));
  stored_data.image = NULL;
  stored_data.image_count = 0;
  HaveData = false;

  // Must have an input camera
  if (cf->ReadDeviceAddr(&this->camera_addr, section, "requires",
                       PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }

}


ImageBase::ImageBase(ConfigFile *cf, int section, bool overwrite_cmds, size_t queue_maxlen)
	: ThreadedDriver(cf, section, overwrite_cmds, queue_maxlen)
{
  memset(&this->camera_addr, 0, sizeof(player_devaddr_t));
  stored_data.image = NULL;
  stored_data.image_count = 0;
  HaveData = false;

  // Must have an input camera
  if (cf->ReadDeviceAddr(&this->camera_addr, section, "requires",
                       PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }

}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int ImageBase::MainSetup()
{
  // Subscribe to the camera.
  if (Device::MatchDeviceAddress (camera_addr, device_addr))
  {
    PLAYER_ERROR ("attempt to subscribe to self");
    return -1;
  }
  if (!(camera_driver = deviceTable->GetDevice (camera_addr)))
  {
    PLAYER_ERROR ("unable to locate suitable camera device");
    return -1;
  }
  if (camera_driver->Subscribe (InQueue) != 0)
  {
    PLAYER_ERROR ("unable to subscribe to camera device");
    return -1;
  }
  return 0;
}

void ImageBase::MainQuit()
{
	if (camera_driver)
		camera_driver->Unsubscribe(InQueue);
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int ImageBase::ProcessMessage (QueuePointer &resp_queue, player_msghdr * hdr, void * data)
{
#if HAVE_JPEG
  uint32_t new_image_count;
#endif

  assert(hdr);

  if(Message::MatchMessage (hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, camera_addr))
  {
	assert(data);
	player_camera_data_t * compdata = reinterpret_cast<player_camera_data_t *>(data);
  	if (!HaveData)
  	{
	    this->stored_data.width = (compdata->width);
	    this->stored_data.height = (compdata->height);
	    this->stored_data.fdiv = (compdata->fdiv);
#if HAVE_JPEG
	    if (compdata->compression != PLAYER_CAMERA_COMPRESS_JPEG)
	    {
#endif
	        this->stored_data.compression = (compdata->compression);
	        this->stored_data.format = (compdata->format);
	        this->stored_data.bpp = (compdata->bpp);
		if ((this->stored_data.image_count) != (compdata->image_count))
	        {
		    this->stored_data.image_count = (compdata->image_count);
		    if (this->stored_data.image) delete [](this->stored_data.image);
		    this->stored_data.image = NULL;
		    if (this->stored_data.image_count)
		    {
		        this->stored_data.image = new uint8_t[this->stored_data.image_count];
			assert(this->stored_data.image);
		    }
		}
		if (this->stored_data.image_count)
		{
		    assert(this->stored_data.image);
		    memcpy(this->stored_data.image, compdata->image, this->stored_data.image_count);
		}
#if HAVE_JPEG
	    } else
	    {
		this->stored_data.compression = PLAYER_CAMERA_COMPRESS_RAW;
		this->stored_data.format = PLAYER_CAMERA_FORMAT_RGB888;
		this->stored_data.bpp = 24;
		new_image_count = (this->stored_data.width) * (this->stored_data.height) * 3;
		if ((this->stored_data.image_count) != new_image_count)
	        {
		    this->stored_data.image_count = new_image_count;
		    if (this->stored_data.image) delete [](this->stored_data.image);
		    this->stored_data.image = NULL;
		    if (this->stored_data.image_count)
		    {
		        this->stored_data.image = new uint8_t[this->stored_data.image_count];
			assert(this->stored_data.image);
		    }
		}
		if (this->stored_data.image_count)
		{
		    assert(this->stored_data.image);
		    jpeg_decompress(reinterpret_cast<unsigned char *>(this->stored_data.image),
		                    this->stored_data.image_count,
		                    reinterpret_cast<unsigned char *>(compdata->image),
		                    compdata->image_count);
		}
	    }
#endif
 	    HaveData = true;
  	}
    return 0;
  }
  return -1;
}

void ImageBase::Main()
{
	for(;;)
	{
		pthread_testcancel();

		InQueue->Wait();

		ProcessMessages();

		if (HaveData)
		{
			ProcessFrame();
			HaveData = false;
		}
	}

}
