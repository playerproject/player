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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: jpeg compression and decompression routines
// Author: Nate Koenig, Andrew Howard
// Date: 31 Aug 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_cameracompress cameracompress
 * @brief Image compression

The cameracompress driver accepts data from another camera device,
compresses it, and makes the compressed data available on a new
interface.

@par Compile-time dependencies

- libjpeg

@par Provides

- Compressed image data is provided via a @ref interface_camera
  device.

@par Requires

- Image data to be compressed is read from a @ref interface_camera
  device.

@par Configuration requests

- none

@par Configuration file options

- check_timestamps (integer)
  - Default: 0
  - If non-zero, timestamps are checked so only new images are compressed
    and published.

- save (integer)
  - Default: 0
  - If non-zero, uncompressed images are saved to disk (with a .jpeg extension)

- image_quality (float)
  - Default: 0.8
  - Image quality for JPEG compression

- request_only (integer)
  - Default: 0
  - If set to 1, data will be sent only at PLAYER_CAMEARA_REQ_GET_IMAGE response.

@par Example

@verbatim
driver
(
  name "cameracompress"
  provides ["camera:1"]
  requires ["camera:0"]  # Compress data from device camera:0
)
@endverbatim

@author Nate Koenig, Andrew Howard

*/
/** @} */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stddef.h>
#include <stdlib.h>       // for atoi(3)
#include <math.h>
#include <assert.h>

#include <libplayercore/playercore.h>
#include <libplayerjpeg/playerjpeg.h>

class CameraCompress : public ThreadedDriver
{
  // Constructor
  public: CameraCompress( ConfigFile* cf, int section);
  // Destructor
  public: virtual ~CameraCompress();

  // Setup/shutdown routines.
  public: virtual int MainSetup();
  public: virtual void MainQuit();

  // This method will be invoked on each incoming message
  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
                                     void * data);

  // Main function for device thread.
  private: virtual void Main();

  private: int ProcessImage(player_camera_data_t & rawdata);

  // Input camera device
  private:

    // Camera device info
    Device *camera;
    player_devaddr_t camera_id;
    double camera_time;
    bool camera_subscribed;

    // Output (compressed) camera data
    private: player_camera_data_t imgdata;

    // Image quality for JPEG compression
    private: double quality;

    // Save image frames?

    private: int save;
    private: int frameno;
    private: int check_timestamps;
    private: int request_only;
};

Driver *CameraCompress_Init(ConfigFile *cf, int section)
{
  return ((Driver*) (new CameraCompress(cf, section)));
}

void cameracompress_Register(DriverTable *table)
{
  table->AddDriver("cameracompress", CameraCompress_Init);
}

CameraCompress::CameraCompress( ConfigFile *cf, int section)
  : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_CAMERA_CODE)
{
  this->imgdata.image_count = 0;
  this->imgdata.image = NULL;
  this->frameno = 0;

  this->camera = NULL;
  // Must have a camera device
  if (cf->ReadDeviceAddr(&this->camera_id, section, "requires",
                       PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  this->camera_time = 0.0;

  this->check_timestamps = cf->ReadInt(section, "check_timestamps", 0);
  this->save = cf->ReadInt(section, "save", 0);
  this->quality = cf->ReadFloat(section, "image_quality", 0.8);
  this->request_only = cf->ReadInt(section, "request_only", 0);

  return;
}

CameraCompress::~CameraCompress()
{
  if (this->imgdata.image)
  {
    delete []this->imgdata.image;
    this->imgdata.image = NULL;
    this->imgdata.image_count = 0;
  }
}

int CameraCompress::MainSetup()
{
  // Subscribe to the camera.
  if(Device::MatchDeviceAddress(this->camera_id, this->device_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return(-1);
  }
  if(!(this->camera = deviceTable->GetDevice(this->camera_id)))
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return(-1);
  }
  if(this->camera->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    return(-1);
  }

  return 0;
}

void CameraCompress::MainQuit()
{
  camera->Unsubscribe(InQueue);

  if (this->imgdata.image)
  {
    delete []this->imgdata.image;
    this->imgdata.image = NULL;
    this->imgdata.image_count = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int CameraCompress::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr,
                               void * data)
{
  player_msghdr_t newhdr;
  player_camera_data_t camdata, * rqdata;
  Message * msg;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, this->camera_id))
  {
    assert(data);
    if (this->request_only) return 0;
    if ((!(this->check_timestamps)) || (this->camera_time != hdr->timestamp))
    {
      this->camera_time = hdr->timestamp;
      if (!(this->ProcessImage(*(reinterpret_cast<player_camera_data_t *>(data))))) this->Publish(this->device_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, reinterpret_cast<void *>(&(this->imgdata)), 0, &(this->camera_time));
      // don't delete anything here! this->imgdata.image is required and is deleted somewhere else
    }
    return 0;
  } else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_CAMERA_REQ_GET_IMAGE, this->device_addr))
  {
    hdr->addr = this->camera_id;
    msg = this->camera->Request(this->InQueue, hdr->type, hdr->subtype, data, 0, NULL, true); // threaded = true
    if (!msg)
    {
      PLAYER_WARN("failed to forward request");
      return -1;
    }
    if (!(msg->GetDataSize() > 0))
    {
      PLAYER_WARN("Wrong size of request reply");
      delete msg;
      return -1;
    }
    rqdata = reinterpret_cast<player_camera_data_t *>(msg->GetPayload());
    if (!rqdata)
    {
      PLAYER_WARN("No image data from forwarded request");
      delete msg;
      return -1;
    }
    if (!((rqdata->width > 0) && (rqdata->height > 0) && (rqdata->bpp > 0) && (rqdata->image_count > 0) && (rqdata->image)))
    {
      newhdr = *(msg->GetHeader());
      newhdr.addr = this->device_addr;
      this->Publish(resp_queue, &newhdr, reinterpret_cast<void *>(rqdata), true); // copy = true, do not dispose published data as we're disposing whole source message in the next line
      delete msg;
      return 0;
    }
    camdata = *rqdata;
    camdata.image = NULL;
    assert((camdata.width > 0) && (camdata.height > 0) && (camdata.bpp > 0) && (camdata.image_count > 0));
    camdata.image = reinterpret_cast<uint8_t *>(malloc(camdata.image_count));
    if (!(camdata.image))
    {
      PLAYER_ERROR("Out of memory");
      delete msg;
      return -1;
    }
    assert((camdata.image_count) == (rqdata->image_count));
    memcpy(camdata.image, rqdata->image, camdata.image_count);
    if (this->ProcessImage(camdata))
    {
      free(camdata.image);
      delete msg;
      return -1;
    }
    free(camdata.image);
    newhdr = *(msg->GetHeader());
    newhdr.addr = this->device_addr;
    this->Publish(resp_queue, &newhdr, reinterpret_cast<void *>(&(this->imgdata)), true); // copy = true
    delete msg;
    return 0;
  } else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->device_addr))
  {
    hdr->addr = this->camera_id;
    msg = this->camera->Request(this->InQueue, hdr->type, hdr->subtype, data, 0, NULL, true); // threaded = true
    if (!msg)
    {
      PLAYER_WARN("failed to forward request");
      return -1;
    }
    newhdr = *(msg->GetHeader());
    newhdr.addr = this->device_addr;
    this->Publish(resp_queue, &newhdr, msg->GetPayload(), true); // copy = true, do not dispose published data as we're disposing whole source message in the next line
    delete msg;
    return 0;
  }
  return -1;
}

void CameraCompress::Main()
{
  while (true)
  {
    // Let the camera driver update this thread
    InQueue->Wait();

    // Test if we are suppose to cancel this thread.
    pthread_testcancel();

    this->ProcessMessages();
  }
}

int CameraCompress::ProcessImage(player_camera_data_t & rawdata)
{
  char filename[256];
  unsigned char * ptr, * ptr1;
  int i, l, ret;
  FILE * fp;
  unsigned char * buffer = NULL;

  if ((rawdata.width <= 0) || (rawdata.height <= 0))
  {
    if (!(this->imgdata.image)) return -1;
  } else if (rawdata.compression == PLAYER_CAMERA_COMPRESS_RAW)
  {
    switch (rawdata.bpp)
    {
    case 8:
      l = (rawdata.width) * (rawdata.height);
      ptr = buffer = new unsigned char[(rawdata.width) * (rawdata.height) * 3];
      assert(buffer);
      ptr1 = reinterpret_cast<unsigned char *>(rawdata.image);
      for (i = 0; i < l; i++)
      {
        ptr[0] = *ptr1;
        ptr[1] = *ptr1;
        ptr[2] = *ptr1;
        ptr += 3; ptr1++;
      }
      ptr = buffer;
      break;
    case 24:
      ptr = reinterpret_cast<unsigned char *>(rawdata.image);
      break;
    case 32:
      l = (rawdata.width) * (rawdata.height);
      ptr = buffer = new unsigned char[(rawdata.width) * (rawdata.height) * 3];
      assert(buffer);
      ptr1 = reinterpret_cast<unsigned char *>(rawdata.image);
      for (i = 0; i < l; i++)
      {
        ptr[0] = ptr1[0];
        ptr[1] = ptr1[1];
        ptr[2] = ptr1[2];
        ptr += 3; ptr1 += 4;
      }
      ptr = buffer;
      break;
    default:
      PLAYER_WARN("unsupported image depth (not good)");
      return -1;
    }
    if (this->imgdata.image) delete []this->imgdata.image;
    this->imgdata.image = new unsigned char[(rawdata.width) * (rawdata.height) * 3];
    assert(this->imgdata.image);
    this->imgdata.image_count = jpeg_compress(reinterpret_cast<char *>(this->imgdata.image),
                                              reinterpret_cast<char *>(ptr),
                                              rawdata.width,
                                              rawdata.height,
                                              (rawdata.width) * (rawdata.height) * 3,
                                              static_cast<int>(this->quality * 100));
    this->imgdata.width = (rawdata.width);
    this->imgdata.height = (rawdata.height);
    this->imgdata.bpp = 24;
    this->imgdata.format = PLAYER_CAMERA_FORMAT_RGB888;
    this->imgdata.fdiv = (rawdata.fdiv);
    this->imgdata.compression = PLAYER_CAMERA_COMPRESS_JPEG;
    this->imgdata.image_count = (this->imgdata.image_count);
  } else
  {
    if (this->imgdata.image) delete []this->imgdata.image;
    this->imgdata.image = new unsigned char[rawdata.image_count];
    assert(this->imgdata.image);
    memcpy(this->imgdata.image, rawdata.image, rawdata.image_count);
    this->imgdata.width = (rawdata.width);
    this->imgdata.height = (rawdata.height);
    this->imgdata.bpp = (rawdata.bpp);
    this->imgdata.format = (rawdata.format);
    this->imgdata.fdiv = (rawdata.fdiv);
    this->imgdata.compression = (rawdata.compression);
    this->imgdata.image_count = (rawdata.image_count);
  }
  if (buffer) delete []buffer;
  buffer = NULL;

  if (this->save)
  {
#ifdef WIN32
    _snprintf(filename, sizeof(filename), "click-%04d.jpeg",this->frameno++);
#else
    snprintf(filename, sizeof(filename), "click-%04d.jpeg",this->frameno++);
#endif
    fp = fopen(filename, "w+");
    if (fp)
    {
      ret = fwrite(this->imgdata.image, 1, this->imgdata.image_count, fp);
      if (ret < 0) PLAYER_ERROR("Failed to save frame");
      fclose(fp);
    }
  }
  return 0;
}
