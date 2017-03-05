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
// Desc: OpenCV capture driver
// Author: Paul Osmialowski
// Date: 24 Jun 2008
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_cvcam cvcam
 * @brief OpenCV camera capture

The cvcam driver captures images from cameras through OpenCV infrastructure.

@par Compile-time dependencies

- OpenCV

@par Provides

- @ref interface_camera

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- camindex (integer)
  - Default: CV_CAP_ANY
  - Index of camera image source (OpenCV specific)

- size (integer tuple)
  - Default: [0 0]
  - Desired image size.   This may not be honoured if the underlying driver does
    not support the requested size.
    Size [0 0] denotes default device image size.

- sleep_nsec (integer)
  - Default: 10000000 (=10ms which gives max 100 fps)
  - timespec value for nanosleep()

@par Example

@verbatim
driver
(
  name "cvcam"
  provides ["camera:0"]
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <libplayercore/playercore.h>

#include <cv.h>
#include <highgui.h>

//---------------------------------

class CvCam : public ThreadedDriver
{
  public: CvCam(ConfigFile * cf, int section);
  public: virtual ~CvCam();

  public: virtual int MainSetup();
  public: virtual void MainQuit();

  // This method will be invoked on each incoming message
  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
                                     void * data);

  private: virtual void Main();
  private: int prepareData(player_camera_data_t * data);

  private: CvCapture * capture;
  private: int camindex;
  private: int desired_width;
  private: int desired_height;
  private: int sleep_nsec;
};

Driver * CvCam_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new CvCam(cf, section));
}

void cvcam_Register(DriverTable *table)
{
  table->AddDriver("cvcam", CvCam_Init);
}

CvCam::CvCam(ConfigFile * cf, int section)
  : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_CAMERA_CODE)
{
  this->capture = NULL;
  this->camindex = cf->ReadInt(section, "camindex", CV_CAP_ANY);
  this->desired_width = cf->ReadTupleInt(section, "size", 0, 0);
  this->desired_height = cf->ReadTupleInt(section, "size", 1, 0);
  if ((this->desired_width < 0) || (this->desired_height < 0))
  {
    PLAYER_ERROR("Wrong size");
    this->SetError(-1);
    return;
  }
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 10000000);
}

CvCam::~CvCam()
{
  if (this->capture) cvReleaseCapture(&(this->capture));
}

int CvCam::MainSetup()
{
  if (this->capture) cvReleaseCapture(&(this->capture));
  this->capture = cvCaptureFromCAM(this->camindex);
  if (!(this->capture))
  {
    PLAYER_ERROR("Couldn't create capture device. Something is wrong with your OpenCV.");
    return -1;
  }
  return 0;
}

void CvCam::MainQuit()
{
  if (this->capture) cvReleaseCapture(&(this->capture));
  this->capture = NULL;
}

int CvCam::prepareData(player_camera_data_t * data)
{
  IplImage * frame;
  int i;

  assert(data);
  frame = cvQueryFrame(this->capture);
  if (!frame)
  {
    PLAYER_ERROR("No frame!");
    return -1;
  }
  if (frame->depth != IPL_DEPTH_8U)
  {
    PLAYER_ERROR1("Unsupported depth %d", frame->depth);
    return -1;
  }
  assert((frame->nChannels) > 0);
  data->image_count = frame->width * frame->height * frame->nChannels;
  assert(data->image_count > 0);
  data->image = reinterpret_cast<unsigned char *>(malloc(data->image_count));
  if (!(data->image))
  {
    PLAYER_ERROR("Out of memory");
    return -1;
  }
  data->width = frame->width;
  data->height = frame->height;
  data->bpp = frame->nChannels * 8;
  data->fdiv = 0;
  switch (data->bpp)
  {
  case 8:
    data->format = PLAYER_CAMERA_FORMAT_MONO8;
    memcpy(data->image, frame->imageData, data->image_count);
    break;
  case 24:
    data->format = PLAYER_CAMERA_FORMAT_RGB888;
    for (i = 0; i < static_cast<int>(data->image_count); i += (frame->nChannels))
    {
      data->image[i] = frame->imageData[i + 2];
      data->image[i + 1] = frame->imageData[i + 1];
      data->image[i + 2] = frame->imageData[i];
    }
    break;
  case 32:
    data->format = PLAYER_CAMERA_FORMAT_RGB888;
    for (i = 0; i < static_cast<int>(data->image_count); i += (frame->nChannels))
    {
      data->image[i] = frame->imageData[i + 2];
      data->image[i + 1] = frame->imageData[i + 1];
      data->image[i + 2] = frame->imageData[i];
      data->image[i + 3] = frame->imageData[i + 3];
    }
    break;
  default:
    PLAYER_ERROR1("Unsupported image depth %d", data->bpp);
    free(data->image);
    data->image = NULL;
    return -1;
  }
  data->compression = PLAYER_CAMERA_COMPRESS_RAW;
  return 0;
}

void CvCam::Main()
{
  struct timespec tspec;
  player_camera_data_t * data;

  if ((this->desired_width) > 0)
  {
    PLAYER_WARN1("Setting capture width %.4f", static_cast<double>(this->desired_width));
    cvSetCaptureProperty(this->capture, CV_CAP_PROP_FRAME_WIDTH, static_cast<double>(this->desired_width));
  }
  if ((this->desired_height) > 0)
  {
    PLAYER_WARN1("Setting capture height %.4f", static_cast<double>(this->desired_height));
    cvSetCaptureProperty(this->capture, CV_CAP_PROP_FRAME_HEIGHT, static_cast<double>(this->desired_height));
  }
  PLAYER_WARN2("Achieved capture size %.4f x %.4f", cvGetCaptureProperty(this->capture, CV_CAP_PROP_FRAME_WIDTH), cvGetCaptureProperty(this->capture, CV_CAP_PROP_FRAME_HEIGHT));
  for (;;)
  {
    // Go to sleep for a while (this is a polling loop).
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec;
    nanosleep(&tspec, NULL);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    ProcessMessages();

    pthread_testcancel();

    data = reinterpret_cast<player_camera_data_t *>(malloc(sizeof(player_camera_data_t)));
    if (!data)
    {
      PLAYER_ERROR("Out of memory");
      continue;
    }
    if (this->prepareData(data))
    {
      free(data);
      data = NULL;
      pthread_testcancel();
      continue;
    }
    this->Publish(device_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, reinterpret_cast<void *>(data), 0, NULL, false);
    // copy = false, don't dispose anything here
    data = NULL;
    pthread_testcancel();
  }
}

int CvCam::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_camera_data_t imgData;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                 PLAYER_CAMERA_REQ_GET_IMAGE,
                                 this->device_addr))
  {
    if (this->prepareData(&imgData)) return -1;
    this->Publish(this->device_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_CAMERA_REQ_GET_IMAGE,
                  reinterpret_cast<void *>(&imgData));
    if (imgData.image) free(imgData.image);
    return 0;
  }
  return -1;
}
