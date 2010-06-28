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

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_imgcmp imgcmp
 * @brief image comparing driver

The imgcmp driver compares current image frame with previous one in order to detect
any changes on it. Frames that not differ will have the same timestamp.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_camera (key "output" - regular image output)
- @ref interface_dio (to signal change detection)
- (optional) @ref interface_camera (key "diff" - differences map)

Any command sent to provided dio interface forces positive compare result.

@par Requires

- @ref interface_camera
- (optionally) @ref interface_dio (to signal change detection by sending dio commands)

@par Configuration requests

- none

@par Configuration file options

- see properties section

@par Properties

- skip_lines (integer)
  - Default: 0
  - During computations, skip n lines from the top
    (as usually there's nothing interesting in first few lines)
- sleep_nsec (integer)
  - Default: 10000
  - timespec value for additional nanosleep()
- max_lum_dist (integer)
  - Default: 0
  - Allowed values: 0 - 255
  - Maximal luminance distance between two pixels in old and new images; if luminance distance
    computed for two pixels (old and new) is greater than max_lum_dist, these pixels differ.
- max_diff_pixels (double)
  - Default: 0.0
  - Allowed value: 0.0 - 1.0
  - Maximal degree to which two images (old and new) differs (see above) to consider that they
    are actually different.
- idle_publish_interval (double)
  - Default: 0.5
  - Image publish interval (in seconds) whenever differences were not detected.
- keep_buffer (integer)
  - Default: 1
  - If set to non-zero, buffer with previous image is kept until change is detected.
    Otherwise, only two last frames will be compared. Set this property to zero for noisy
    camera image and bad light.

@par Example

@verbatim
driver
(
  name "imgcmp"
  requires ["camera:1"]
  provides ["output:::camera:0" "dio:0"]
  skip_lines 16
  max_lum_dist 128
  max_diff_pixels 0.01
  keep_buffer 0
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
#include <math.h>
#include <libplayercore/playercore.h>
#include <config.h>
#if HAVE_JPEG
  #include <libplayerjpeg/playerjpeg.h>
#endif

#define QUEUE_LEN 1

class ImgCmp : public ThreadedDriver
{
  public: ImgCmp(ConfigFile * cf, int section);
  public: virtual ~ImgCmp();

  public: virtual int MainSetup();
  public: virtual void MainQuit();

  // This method will be invoked on each incoming message
  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
                                     void * data);

  private: virtual void Main();

  // Input camera device
  private: player_devaddr_t camera_provided_addr, dio_provided_addr, diff_provided_addr, camera_id, dio_id;
  private: Device * camera;
  private: Device * dio;
  private: struct
  {
    unsigned char * buffer;
    size_t bufsize;
  } buffers[2];
  private: int currentbuffer;
  private: int use_dio;
  private: int publish_diffs;
  private: int forced;
  private: double lastTStamp;
  private: double lastPublish;
  private: IntProperty skip_lines;
  private: IntProperty sleep_nsec;
  private: IntProperty max_lum_dist;
  private: DoubleProperty max_diff_pixels;
  private: DoubleProperty idle_publish_interval;
  private: IntProperty keep_buffer;
};

Driver * ImgCmp_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new ImgCmp(cf, section));
}

void imgcmp_Register(DriverTable *table)
{
  table->AddDriver("imgcmp", ImgCmp_Init);
}

ImgCmp::ImgCmp(ConfigFile * cf, int section)
  : ThreadedDriver(cf, section, true, QUEUE_LEN),
  skip_lines("skip_lines", 0, false),
  sleep_nsec("sleep_nsec", 10000, false),
  max_lum_dist("max_lum_dist", 0, false),
  max_diff_pixels("max_diff_pixels", 0.0, false),
  idle_publish_interval("idle_publish_interval", 0.5, false),
  keep_buffer("keep_buffer", 1, false)
{
  memset(&(this->camera_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->diff_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->camera_id), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_id), 0, sizeof(player_devaddr_t));
  this->camera = NULL;
  this->dio = NULL;
  memset(this->buffers, 0, sizeof this->buffers);
  this->buffers[0].buffer = NULL;
  this->buffers[0].bufsize = 0;
  this->buffers[1].buffer = NULL;
  this->buffers[1].bufsize = 0;
  this->currentbuffer = 0;
  this->use_dio = 0;
  this->publish_diffs = 0;
  this->forced = 0;
  this->lastTStamp = 0.0;
  this->lastPublish = 0.0;
  if (cf->ReadDeviceAddr(&(this->camera_provided_addr), section, "provides", PLAYER_CAMERA_CODE, -1, "output"))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->camera_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->dio_provided_addr), section, "provides", PLAYER_DIO_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->dio_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->diff_provided_addr), section, "provides", PLAYER_CAMERA_CODE, -1, "diff"))
  {
    this->publish_diffs = 0;
  } else
  {
    if (this->AddInterface(this->diff_provided_addr))
    {
      this->SetError(-1);
      return;
    }
    this->publish_diffs = !0;
  }
  if (cf->ReadDeviceAddr(&(this->camera_id), section, "requires", PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->dio_id), section, "requires", PLAYER_DIO_CODE, -1, NULL) != 0)
  {
    this->use_dio = 0;
  } else
  {
    this->use_dio = !0;
  }
  if (!(this->RegisterProperty("skip_lines", &(this->skip_lines), cf, section)))
  {
    this->SetError(-1);
    return;
  }
  if (this->skip_lines.GetValue() < 0)
  {
    this->SetError(-1);
    return;
  }
  if (!(this->RegisterProperty("sleep_nsec", &(this->sleep_nsec), cf, section)))
  {
    this->SetError(-1);
    return;
  }
  if (this->sleep_nsec.GetValue() < 0)
  {
    this->SetError(-1);
    return;
  }
  if (!(this->RegisterProperty("max_lum_dist", &(this->max_lum_dist), cf, section)))
  {
    this->SetError(-1);
    return;
  }
  if ((this->max_lum_dist.GetValue() < 0) || (this->max_lum_dist.GetValue() > 255))
  {
    this->SetError(-1);
    return;
  }
  if (!(this->RegisterProperty("max_diff_pixels", &(this->max_diff_pixels), cf, section)))
  {
    this->SetError(-1);
    return;
  }
  if ((this->max_diff_pixels.GetValue() < 0.0) || (this->max_diff_pixels.GetValue() > 1.0))
  {
    this->SetError(-1);
    return;
  }
  if (!(this->RegisterProperty("idle_publish_interval", &(this->idle_publish_interval), cf, section)))
  {
    this->SetError(-1);
    return;
  }
  if (this->idle_publish_interval.GetValue() < 0.0)
  {
    this->SetError(-1);
    return;
  }
  if (!(this->RegisterProperty("keep_buffer", &(this->keep_buffer), cf, section)))
  {
    this->SetError(-1);
    return;
  }
}

ImgCmp::~ImgCmp()
{
  if (this->buffers[0].buffer) free(this->buffers[0].buffer);
  this->buffers[0].buffer = NULL;
  if (this->buffers[1].buffer) free(this->buffers[1].buffer);
  this->buffers[1].buffer = NULL;
}

int ImgCmp::MainSetup()
{
  if (Device::MatchDeviceAddress(this->camera_id, this->camera_provided_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return -1;
  }
  if (this->use_dio)
  {
    if (Device::MatchDeviceAddress(this->dio_id, this->dio_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self");
      return -1;
    }
  }
  if (this->publish_diffs)
  {
    if (Device::MatchDeviceAddress(this->camera_id, this->diff_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self");
      return -1;
    }
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
  if (this->use_dio)
  {
    this->dio = deviceTable->GetDevice(this->dio_id);
    if (!this->dio)
    {
      PLAYER_ERROR("unable to locate suitable dio device");
      if (this->camera) this->camera->Unsubscribe(this->InQueue);
      this->camera = NULL;
      return -1;
    }
    if (this->dio->Subscribe(this->InQueue) != 0)
    {
      PLAYER_ERROR("unable to subscribe to dio device");
      if (this->camera) this->camera->Unsubscribe(this->InQueue);
      this->camera = NULL;
      this->dio = NULL;
      return -1;
    }
  }
  return 0;
}

void ImgCmp::MainQuit()
{
  if (this->camera) this->camera->Unsubscribe(this->InQueue);
  this->camera = NULL;
  if (this->use_dio)
  {
    if (this->dio) this->dio->Unsubscribe(this->InQueue);
    this->dio = NULL;
  }
}

void ImgCmp::Main()
{
  struct timespec tspec;

  for (;;)
  {
    this->InQueue->Wait();
    pthread_testcancel();
    this->ProcessMessages();
    pthread_testcancel();
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec.GetValue();
    if (tspec.tv_nsec > 0)
    {
      nanosleep(&tspec, NULL);
      pthread_testcancel();
    }
  }
}

int ImgCmp::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_camera_data_t * output = NULL;
  int i, j;
  size_t new_size;
  unsigned char * ptr, * ptr1, * ptr2;
  player_camera_data_t * rawdata;
  int diff, diffs;
  int nextbuffer = ((this->currentbuffer) ? 0 : 1);
  double lum, lum1;
  int skip_lines = this->skip_lines.GetValue();
  int differ;
  player_dio_data_t dio_data;
  player_dio_cmd_t dio_cmd;
  double t;
  int d;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, -1, this->dio_id))
  {
    assert(data);
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, -1, this->dio_provided_addr))
  {
    assert(data);
    this->forced = !0;
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, this->camera_id))
  {
    assert(data);
    rawdata = reinterpret_cast<player_camera_data_t *>(data);
    if ((static_cast<int>(rawdata->width) <= 0) || (static_cast<int>(rawdata->height) <= 0))
    {
      return -1;
    } else
    {
      new_size = rawdata->width * rawdata->height * 3;
      if ((this->buffers[this->currentbuffer].bufsize) != new_size)
      {
        if (this->buffers[this->currentbuffer].buffer) free(this->buffers[this->currentbuffer].buffer);
        this->buffers[this->currentbuffer].buffer = NULL;
        this->buffers[this->currentbuffer].bufsize = 0;
      }
      if (!(this->buffers[this->currentbuffer].buffer))
      {
        this->buffers[this->currentbuffer].bufsize = 0;
        this->buffers[this->currentbuffer].buffer = reinterpret_cast<unsigned char *>(malloc(new_size));
        if (!(this->buffers[this->currentbuffer].buffer))
        {
          PLAYER_ERROR("Out of memory");
          return -1;
        }
        this->buffers[this->currentbuffer].bufsize = new_size;
      }
      switch (rawdata->compression)
      {
      case PLAYER_CAMERA_COMPRESS_RAW:
        switch (rawdata->bpp)
        {
        case 8:
          ptr = this->buffers[this->currentbuffer].buffer;
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
          break;
        case 24:
          memcpy(this->buffers[this->currentbuffer].buffer, rawdata->image, this->buffers[this->currentbuffer].bufsize);
          break;
        case 32:
          ptr = this->buffers[this->currentbuffer].buffer;
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
          break;
        default:
          PLAYER_WARN("unsupported image depth (not good)");
          return -1;
        }
        break;
#if HAVE_JPEG
      case PLAYER_CAMERA_COMPRESS_JPEG:
        jpeg_decompress(this->buffers[this->currentbuffer].buffer, this->buffers[this->currentbuffer].bufsize, rawdata->image, rawdata->image_count);
        break;
#endif
      default:
        PLAYER_WARN("unsupported compression scheme (not good)");
        return -1;
      }
      assert(skip_lines >= 0);
      assert(skip_lines < static_cast<int>(rawdata->height));
      assert(this->buffers[this->currentbuffer].buffer);
      if ((this->buffers[nextbuffer].buffer) && ((this->buffers[this->currentbuffer].bufsize) == (this->buffers[nextbuffer].bufsize)))
      {
        if (this->forced)
        {
          this->forced = 0;
          differ = !0;
        } else
        {
          if (this->publish_diffs)
          {
            output = reinterpret_cast<player_camera_data_t *>(malloc(sizeof(player_camera_data_t)));
            if (!output)
            {
              PLAYER_ERROR("Out of memory");
              return -1;
            }
            memset(output, 0, sizeof(player_camera_data_t));
            output->bpp = 8;
            output->compression = PLAYER_CAMERA_COMPRESS_RAW;
            output->format = PLAYER_CAMERA_FORMAT_MONO8;
            output->fdiv = rawdata->fdiv;
            output->width = rawdata->width;
            output->height = rawdata->height;
            output->image_count = ((rawdata->width) * (rawdata->height));
            output->image = reinterpret_cast<uint8_t *>(malloc(output->image_count));
            if (!(output->image))
            {
              free(output);
              PLAYER_ERROR("Out of memory");
              return -1;
            }
            memset(output->image, 0, output->image_count);
            ptr2 = output->image + (skip_lines * (rawdata->width));
          } else ptr2 = NULL;
          ptr = (this->buffers[this->currentbuffer].buffer) + (skip_lines * (rawdata->width) * 3);
          assert(ptr);
          ptr1 = this->buffers[nextbuffer].buffer + (skip_lines * (rawdata->width) * 3);
          assert(ptr1);
          diffs = 0;
          differ = 0;
          d = this->max_lum_dist.GetValue();
          for (i = skip_lines; i < static_cast<int>(rawdata->height); i++)
          {
            for (j = 0; j < static_cast<int>(rawdata->width); j++)
            {
              lum = (0.299 * static_cast<double>(ptr[0])) + (0.587 * static_cast<double>(ptr[1])) + (0.114 * static_cast<double>(ptr[2]));
              lum1 = (0.299 * static_cast<double>(ptr1[0])) + (0.587 * static_cast<double>(ptr1[1])) + (0.114 * static_cast<double>(ptr1[2]));
              diff = static_cast<int>(fabs(lum - lum1));
              assert(diff >= 0);
              if (diff > 255)
              {
                PLAYER_WARN("difference too big");
                diff = 255;
              }
              if (diff > d)
              {
                diffs++;
                diff = 255;
              }
              if (this->publish_diffs)
              {
                assert(ptr2);
                *ptr2 = static_cast<unsigned char>(diff);
                ptr2++;
              }
              ptr += 3; ptr1 += 3;
            }
          }
          if (this->publish_diffs)
          {
            this->Publish(this->diff_provided_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, reinterpret_cast<void *>(output), 0, &(hdr->timestamp), false); // copy = false
            // I assume that Publish() (with copy = false) freed those output data!
            output = NULL;
          }
          differ = ((static_cast<double>(diffs) / static_cast<double>((rawdata->width) * (rawdata->height))) > (this->max_diff_pixels.GetValue()));
        }
        memset(&dio_data, 0, sizeof dio_data);
        dio_data.count = 1;
        dio_data.bits = differ ? 1 : 0;
        this->Publish(this->dio_provided_addr, PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES, reinterpret_cast<void *>(&dio_data), 0, &(hdr->timestamp), true); // copy = true
        if (this->use_dio)
        {
          memset(&dio_cmd, 0, sizeof dio_cmd);
          dio_cmd.count = 1;
          dio_cmd.digout = differ ? 1 : 0;
          this->dio->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, reinterpret_cast<void *>(&dio_cmd), 0, &(hdr->timestamp));
        }
        GlobalTime->GetTimeDouble(&t);
        if ((differ) || (fabs(t - (this->lastPublish)) > (this->idle_publish_interval.GetValue())))
        {
          output = reinterpret_cast<player_camera_data_t *>(malloc(sizeof(player_camera_data_t)));
          if (!output)
          {
            PLAYER_ERROR("Out of memory");
            return -1;
          }
          memset(output, 0, sizeof(player_camera_data_t));
          output->bpp = 24;
          output->compression = PLAYER_CAMERA_COMPRESS_RAW;
          output->format = PLAYER_CAMERA_FORMAT_RGB888;
          output->fdiv = rawdata->fdiv;
          output->width = rawdata->width;
          output->height = rawdata->height;
          output->image_count = this->buffers[this->currentbuffer].bufsize;
          output->image = reinterpret_cast<uint8_t *>(malloc(output->image_count));
          if (!(output->image))
          {
            free(output);
            PLAYER_ERROR("Out of memory");
            return -1;
          }
          memcpy(output->image, this->buffers[this->currentbuffer].buffer, output->image_count);
          if (differ) this->lastTStamp = hdr->timestamp;
          this->Publish(this->camera_provided_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, reinterpret_cast<void *>(output), 0, &(this->lastTStamp), false); // copy = false
          // I assume that Publish() (with copy = false) freed those output data!
          output = NULL;
          this->lastPublish = t;
        }
        if ((differ) || (!(this->keep_buffer.GetValue()))) this->currentbuffer = nextbuffer;
      } else
      {
        this->lastTStamp = hdr->timestamp;
        this->currentbuffer = nextbuffer;
      }
    }
    return 0;
  }
  return -1;
}
