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

/*
 *
 * This driver captures images from V4L2-compatible cameras.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_camerav4l2 camerav4l2
 * @brief Video4Linux2 camera capture

The camerav4l2 driver captures images from V4L2-compatible cameras.

@par Compile-time dependencies

- &lt;sys/types.h&gt;
- &lt;linux/videodev2.h&gt;

@par Provides

- @ref interface_camera

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- port (string)
  - Default: "/dev/video0"
  - Device to read video data from.

- sources (integer tuple)
  - Default: NONE
  - Some capture cards have few multiple input sources; use this field to
    select which ones are used. You should define as many provided camera
    interfaces as the number of sources is given here. Source channel
    numbers are used as keys ('ch' prefixed) in 'provides' tuple.
  - If not given, channel 0 alone will be used by defauilt.
  - Note that switching between channels takes time. Framerate drops
    dramatically whenever more than one channel is used.

- norm (string)
  - Default: "NTSC"
  - Capture format; "NTSC", "PAL" or "UNKNOWN"
  - Case sensitive!

- size (integer tuple)
  - Default: varies with norm
    - PAL: [768 576]
    - NTSC: [640 480]
    - other: [320 240]
  - Desired image size.   This may not be honoured if the driver does
    not support the requested size).

- mode (string)
  - Default: "BGR3"
  - Desired capture mode.  Can be one of:
    - GREY (8-bit monochrome)
    - RGBP (16-bit packed; will produce 24-bit color images)
    - BGR3, RGB3 (24-bit RGB)
    - BGR4, RGB4 (32-bit RGB)
    - BA81 (for sn9c1xx-based USB webcams; will produce 24-bit color images)
    - MJPG (for webcams producing MJPEG streams not decompressed by V4L2 driver)

- buffers (integer)
  - Default: 2
  - Number of buffers to use for grabbing. This reduces latency, but also
  	potentially reduces throughput. Use this if you are reading slowly
  	from the player driver and do not want to get stale frames.

- sleep_nsec (integer)
  - Default: 10000000 (=10ms which gives max 100 fps)
  - timespec value for nanosleep()

- settle_time (double)
  - Default: 0.5
  - Time (in seconds) to wait after switching to next channel; images grabbed
    right after channel switching are too bright or messy, so it is better
    to wait a little while before start grabbing.

- skip_frames (integer)
  - Default: 10
  - See 'settle_time' - during this settle time frames are grabbed anyway
    and they are counted. We can decide to stop waiting for stable image
    after given number of frames was skipped.

- failsafe (integer)
  - Default: 0
  - If after few days of grabbing you experience unexpected failures you can
    try to set this to 1. WARNING! Such failures may (or may not) suggest
    something wrong is going on with your system kernel. Reboot of whole
    system is a better solution, although it may not always be desired.
    This feature is turned off by default - use at your own risk.

Note that some of these options may not be honoured by the underlying
V4L2 kernel driver (it may not support a given image size, for
example).

@par Example

@verbatim
driver
(
  name "camerav4l2"
  provides ["camera:0"]
)
@endverbatim

@par Channel 2 explicitly selected:

@verbatim
driver
(
  name "camerav4l2"
  sources [2]
  norm "PAL"
  size [384 288]
  mode "BGR4"
  buffers 4
  sleep_nsec 10000
  provides ["ch2:::camera:1"]
)
driver
(
  name "cameracompress"
  requires "camera:1"
  provides "camera:0"
)
@endverbatim

@par Two channels at the same time:

@verbatim
driver
(
  name "camerav4l2"
  sources [0 2]
  norm "PAL"
  provides ["ch2:::camera:0" "ch0:::camera:1"]
)
@endverbatim

@par sn9c1xx-based USB webcam (it accepts one buffer and only 352x288 size!):

@verbatim
driver
(
  name "camerav4l2"
  norm "UNKNOWN"
  mode "BA81"
  size [352 288]
  buffers 1
  provides ["camera:1"]
)
driver
(
  name "cameracompress"
  requires ["camera:1"]
  provides ["camera:0"]
)
@endverbatim

@author Paul Osmialowski, Takafumi Mizuno

*/
/** @} */

#include "v4l2.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <libplayercore/playercore.h>

#define MAX_CHANNELS 10

#define IS_JPEG(ptr) ((((ptr)[0]) == 0xff) && (((ptr)[1]) == 0xd8))

class CameraV4L2: public ThreadedDriver
{
  public:
    // Constructor; need that
    CameraV4L2(ConfigFile * cf, int section);

    virtual ~CameraV4L2();

    // Must implement the following methods.
    virtual int MainSetup();
    virtual void MainQuit();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);
  private:
    // Main function for device thread.
    virtual void Main();
    int setSource();

    int started;
    const char * port;
    const char * mode;
    int buffers;
    int sources_count;
    int sources[MAX_CHANNELS];
    int current_source;
    player_devaddr_t camera_addrs[MAX_CHANNELS];
    void * fg;
    const char * norm;
    int width;
    int height;
    int bpp;
    int sleep_nsec;
    double settle_time;
    int skip_frames;
    uint32_t format;
    int failsafe;
    int jpeg;
};

CameraV4L2::CameraV4L2(ConfigFile * cf, int section)
    : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  int i;
  char key[24];

  this->started = 0;
  this->port = NULL;
  this->mode = NULL;
  this->buffers = 0;
  this->sources_count = 0;
  this->current_source = 0;
  this->fg = NULL;
  this->norm = NULL;
  this->width = 0;
  this->height = 0;
  this->bpp = 0;
  this->sleep_nsec = 0;
  this->settle_time = 0.0;
  this->skip_frames = 0;
  this->format = 0;
  this->failsafe = 0;
  this->jpeg = 0;
  memset(this->sources, 0, sizeof this->sources);
  memset(this->camera_addrs, 0, sizeof this->camera_addrs);
  this->sources_count = cf->GetTupleCount(section, "sources");
  if (((this->sources_count) <= 0) || ((this->sources_count) > MAX_CHANNELS))
  {
    PLAYER_WARN("Implicitly using channel 0");
    this->sources_count = 1;
    this->sources[0] = 0;
    if (cf->ReadDeviceAddr(&(this->camera_addrs[0]), section, "provides",
                           PLAYER_CAMERA_CODE, -1, NULL))
    {
      this->SetError(-1);
      return;
    }
    if (this->AddInterface(this->camera_addrs[0]))
    {
      this->SetError(-1);
      return;
    }
  } else
  {
    for (i = 0; i < (this->sources_count); i++)
    {
      this->sources[i] = cf->ReadTupleInt(section, "sources", i, -1);
      if ((this->sources[i]) < 0)
      {
        PLAYER_ERROR2("Invalid channel number %d for source %d", this->sources[i], i);
        this->SetError(-1);
        return;
      }
      snprintf(key, sizeof key, "ch%d", this->sources[i]);
      if (cf->ReadDeviceAddr(&(this->camera_addrs[i]), section, "provides",
                             PLAYER_CAMERA_CODE, -1, key))
      {
        PLAYER_ERROR1("Cannot provide device %s", key);
        this->SetError(-1);
        return;
      }
      if (this->AddInterface(this->camera_addrs[i]))
      {
        this->SetError(-1);
        return;
      }
    }
  }
  if (((this->sources_count) <= 0) || ((this->sources_count) > MAX_CHANNELS))
  {
    this->SetError(-1);
    return;
  }
  this->port = cf->ReadString(section, "port", "/dev/video0");
  if (!(this->port))
  {
    this->SetError(-1);
    return;
  }
  this->mode = cf->ReadString(section, "mode", "BGR3");
  if (!(this->mode))
  {
    this->SetError(-1);
    return;
  }
  if (!(strcmp(this->mode, "GREY")))
  {
    this->format = PLAYER_CAMERA_FORMAT_MONO8;
    this->bpp = 8;
  } else if (!(strcmp(this->mode, "RGBP")))
  {
    this->format = PLAYER_CAMERA_FORMAT_RGB888;
    this->bpp = 24;
  } else if (!(strcmp(this->mode, "BGR3")))
  {
    this->format = PLAYER_CAMERA_FORMAT_RGB888;
    this->bpp = 24;
  } else if (!(strcmp(this->mode, "BGR4")))
  {
    this->format = PLAYER_CAMERA_FORMAT_RGB888;
    this->bpp = 32;
  } else if (!(strcmp(this->mode, "RGB3")))
  {
    this->format = PLAYER_CAMERA_FORMAT_RGB888;
    this->bpp = 24;
  } else if (!(strcmp(this->mode, "RGB4")))
  {
    this->format = PLAYER_CAMERA_FORMAT_RGB888;
    this->bpp = 32;
  } else if (!(strcmp(this->mode, "BA81")))
  {
    this->format = PLAYER_CAMERA_FORMAT_RGB888;
    this->bpp = 24;
  } else if (!(strcmp(this->mode, "MJPG")))
  {
    this->format = PLAYER_CAMERA_FORMAT_RGB888;
    this->bpp = 24;
    this->jpeg = !0;
  } else
  {
    PLAYER_ERROR("Unknown pixel format");
    this->SetError(-1);
    return;
  }
  if ((this->bpp) <= 0)
  {
    this->SetError(-1);
    return;
  }
  // NTSC, PAL or UNKNOWN
  this->norm = cf->ReadString(section, "norm", "NTSC");
  if (!(strcmp(this->norm, "NTSC")))
  {
    this->width = 640;
    this->height = 480;
  } else if (!(strcmp(this->norm, "PAL")))
  {
    this->width = 768;
    this->height = 576;
  } else
  {
    this->width = 320;
    this->height = 240;
  }
  if (cf->GetTupleCount(section, "size") == 2)
  {
    this->width = cf->ReadTupleInt(section, "size", 0, this->width);
    this->height = cf->ReadTupleInt(section, "size", 1, this->height);
  }
  if (((this->width) <= 0) || ((this->height) <= 0))
  {
    this->SetError(-1);
    return;
  }
  this->buffers = cf->ReadInt(section, "buffers", 2);
  if ((this->buffers) <= 0)
  {
    this->SetError(-1);
    return;
  }
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 10000000);
  if ((this->sleep_nsec) < 0)
  {
    this->SetError(-1);
    return;
  }
  this->settle_time = cf->ReadFloat(section, "settle_time", 0.5);
  this->skip_frames = cf->ReadInt(section, "skip_frames", 10);
  this->failsafe = cf->ReadInt(section, "failsafe", 0);
}

CameraV4L2::~CameraV4L2()
{
  if (this->fg)
  {
    if (this->started) stop_grab(this->fg);
    this->started = 0;
    close_fg(this->fg);
  }
  this->fg = NULL;
}

int CameraV4L2::setSource()
{
  int dropped = 0;
  double start_time, t;
  struct timespec tspec;
  static int next_source = 0;

  if (!(this->fg)) return -1;
  if (this->started) stop_grab(this->fg);
  this->started = 0;
  if ((next_source < 0) || (next_source >= (this->sources_count))) return -1;
  if (set_channel(this->fg, this->sources[next_source], this->norm) < 0)
  {
    PLAYER_ERROR1("Cannot set channel %d", this->sources[next_source]);
    return -1;
  }
  this->current_source = next_source;
  next_source++;
  if (next_source >= (this->sources_count)) next_source = 0;
  if (((this->current_source) < 0) || ((this->current_source) >= (this->sources_count))) return -1;
  if (start_grab(this->fg) < 0)
  {
    PLAYER_ERROR1("Cannot start grab on channel %d", this->sources[this->current_source]);
    return -1;
  }
  this->started = !0;
  GlobalTime->GetTimeDouble(&start_time);
  for (;;)
  {
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec;
    nanosleep(&tspec, NULL);
    GlobalTime->GetTimeDouble(&t);
    if (((t - start_time) >= (this->settle_time)) && (dropped >= (this->skip_frames))) break;
    if (!(get_image(this->fg))) PLAYER_WARN("No frame grabbed");
    dropped++;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int CameraV4L2::MainSetup()
{
  assert((!(this->fg)) && (!(this->started)));
  this->fg = open_fg(this->port, this->mode, this->width, this->height, (this->bpp) / 8, this->buffers);
  if (!(this->fg)) return -1;
  return this->setSource();
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void CameraV4L2::MainQuit()
{
  if (this->fg)
  {
    if (this->started) stop_grab(this->fg);
    this->started = 0;
    close_fg(this->fg);
  }
  this->fg = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void CameraV4L2::Main()
{
  struct timespec tspec;
  const unsigned char * img;
  player_camera_data_t * data = NULL;
  int i;

  for (;;)
  {
    // Go to sleep for a while (this is a polling loop).
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec;
    nanosleep(&tspec, NULL);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    ProcessMessages();

    assert(this->fg);
    assert(this->started);
    // Grab the next frame (blocking)
    img = get_image(this->fg);
    if (this->failsafe)
    {
      if (!img)
      {
        PLAYER_ERROR("Cannot grab frame");
        pthread_testcancel();
        if (this->started) stop_grab(this->fg);
        this->started = 0;
        close_fg(this->fg);
        this->fg = NULL;
        tspec.tv_sec = 1;
        tspec.tv_nsec = 0;
        nanosleep(&tspec, NULL);
        pthread_testcancel();
        this->fg = open_fg(this->port, this->mode, this->width, this->height, (this->bpp) / 8, this->buffers);
        assert(this->fg);
        this->setSource();
        assert(this->started);
        pthread_testcancel();
        continue;
      }
    } else
    {
      assert(img);
    }
    data = reinterpret_cast<player_camera_data_t *>(malloc(sizeof(player_camera_data_t)));
    if (!data)
    {
      PLAYER_ERROR("Out of memory");
      continue;
    }
    memset(data, 0, sizeof *data);
    // Set the image properties
    data->width       = this->width;
    data->height      = this->height;
    data->bpp         = this->bpp;
    data->format      = this->format;
    data->fdiv        = 0;
    data->image_count = 0;
    data->image       = NULL;
    if (!(this->jpeg))
    {
      data->compression = PLAYER_CAMERA_COMPRESS_RAW;
      data->image_count = this->width * this->height * ((this->bpp) / 8);
      assert(data->image_count > 0);
      data->image = reinterpret_cast<uint8_t *>(malloc(data->image_count));
      if (!(data->image))
      {
        PLAYER_ERROR("Out of memory!");
        free(data);
        data = NULL;
        continue;
      }
      memcpy(data->image, img, data->image_count);
    } else
    {
      data->compression = PLAYER_CAMERA_COMPRESS_JPEG;
      memcpy(&i, img, sizeof(int));
      data->image_count = i;
      assert(data->image_count > 1);
      if (!(IS_JPEG(img + sizeof(int))))
      {
        PLAYER_ERROR("Not a JPEG image...");
        free(data);
        data = NULL;
        continue;
      }
      data->image = reinterpret_cast<uint8_t *>(malloc(data->image_count));
      if (!(data->image))
      {
        PLAYER_ERROR("Out of memory!");
        free(data);
        data = NULL;
        continue;
      }
      memcpy(data->image, img + sizeof(int), data->image_count);
    }
    assert(data->image_count > 0);
    assert(data->image);
    Publish(this->camera_addrs[this->current_source],
            PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE,
            reinterpret_cast<void *>(data), 0, NULL, false);
            // copy = false, don't dispose anything here!
    data = NULL;

    if ((this->sources_count) > 1)
    {
      pthread_testcancel();
      if (this->failsafe)
      {
        this->setSource();
      } else
      {
        assert(!(this->setSource()));
      }
    }

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();
  }
}

int CameraV4L2::ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data)
{
  assert(hdr);
  return -1;
}

Driver * CameraV4L2_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new CameraV4L2(cf, section));
}

////////////////////////////////////////////////////////////////////////////////
// a driver registration function
void camerav4l2_Register(DriverTable* table)
{
  table->AddDriver("camerav4l2", CameraV4L2_Init);
}
