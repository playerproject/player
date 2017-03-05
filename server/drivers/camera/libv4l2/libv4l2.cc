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
// Desc: libv4l2-based capture driver
// Author: Paul Osmialowski
// Date: 27 Feb 2012
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_libv4l2 libv4l2
 * @brief libv4l2-based capture driver

The libv4l2 driver captures images from various webcams.
Based on v4l-utils (GNU GPL v2).

@par Compile-time dependencies

- libv4l2 (part of v4l-utils, shipped separately as media-libs/libv4l on Gentoo Linux)

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

- sleep_nsec (integer)
  - Default: 10000000 (=10ms which gives max 100 fps)
  - timespec value for nanosleep()

- request_only (integer)
  - Default: 0
  - If set to 1, data will be sent only at PLAYER_CAMEARA_REQ_GET_IMAGE response.

- read_mode (integer)
  - Default: 0
  - Set to 1 if read should be used instead of grab (in most cases it isn't a good idea!)

@par Example

@verbatim
driver
(
  name "libv4l2"
  provides ["camera:0"]
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <time.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <libplayercore/playercore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <libv4l2.h>
#include <libv4lconvert.h>

class Libv4l2 : public ThreadedDriver
{
public:
  Libv4l2(ConfigFile * cf, int section);
  virtual ~Libv4l2();

  virtual int MainSetup();
  virtual void MainQuit();
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr * hdr,
                             void * data);

private:
  virtual void Main();
  void prepareData();
  void cleanup();

  static int instances;
  player_devaddr_t camera_addr;
  const char * port;
  int sleep_nsec;
  int request_only;
  int read_mode;
  int m_fd;
  struct v4lconvert_data * convert;
  struct v4l2_format srcfmt;
  struct v4l2_format dstfmt;
  struct buffer
  {
    void * start;
    size_t length;
  } * m_buffers;
  size_t n_buffers;
  int started;
  player_camera_data_t * data;
};

int Libv4l2::instances = 0;

Libv4l2::Libv4l2(ConfigFile * cf, int section)
    : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  assert(!(this->instances));
  this->instances++;
  memset(&(this->camera_addr), 0, sizeof(player_devaddr_t));
  this->port = NULL;
  this->sleep_nsec = 0;
  this->request_only = 0;
  this->read_mode = 0;
  this->m_fd = -1;
  this->convert = NULL;
  memset(&(this->srcfmt), 0, sizeof this->srcfmt);
  memset(&(this->dstfmt), 0, sizeof this->dstfmt);
  this->m_buffers = NULL;
  this->n_buffers = 0;
  this->started = 0;
  this->data = NULL;
  if (cf->ReadDeviceAddr(&(this->camera_addr), section, "provides", PLAYER_CAMERA_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->camera_addr))
  {
    this->SetError(-1);
    return;
  }
  this->port = cf->ReadString(section, "port", "/dev/video0");
  if (!(this->port))
  {
    PLAYER_ERROR("Port not given");
    this->SetError(-1);
    return;
  }
  if (!(strlen(this->port) > 0))
  {
    PLAYER_ERROR("Empty port name");
    this->SetError(-1);
    return;
  }
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 10000000);
  if ((this->sleep_nsec) < 0)
  {
    PLAYER_ERROR("Invalid sleep_nsec value");
    this->SetError(-1);
    return;
  }
  this->request_only = cf->ReadInt(section, "request_only", 0);
  this->read_mode = cf->ReadInt(section, "read_mode", 0);
}

Libv4l2::~Libv4l2()
{
  this->cleanup();
  assert(!(this->started));
  assert((this->m_fd) < 0);
  this->instances--;
  assert(!(this->instances));
}

void Libv4l2::cleanup()
{
  int i;
  struct v4l2_encoder_cmd cmd;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  struct v4l2_requestbuffers reqbuf;

  if (this->data)
  {
    if (this->data->image) free(this->data->image);
    this->data->image = NULL;
    this->data->image_count = 0;
    free(this->data);
  }
  this->data = NULL;
  if (this->started)
  {
    assert(!((this->m_fd) < 0));
    if (this->read_mode)
    {
      memset(&cmd, 0, sizeof cmd);
      cmd.cmd = V4L2_ENC_CMD_STOP;
      v4l2_ioctl(this->m_fd, VIDIOC_ENCODER_CMD, &cmd);
    } else
    {
      v4l2_ioctl(this->m_fd, VIDIOC_STREAMOFF, &type);
    }
  }
  this->started = 0;
  if (this->m_buffers)
  {
    assert(!(this->read_mode));
    assert(!((this->m_fd) < 0));
    for (i = 0; i < static_cast<int>(this->n_buffers); i++)
    {
      if (this->m_buffers[i].start) v4l2_munmap(this->m_buffers[i].start, this->m_buffers[i].length);
      this->m_buffers[i].start = NULL;
    }
    memset(&reqbuf, 0, sizeof reqbuf);
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 1;
    v4l2_ioctl(this->m_fd, VIDIOC_REQBUFS, &reqbuf);
    memset(&reqbuf, 0, sizeof reqbuf);
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 0;
    v4l2_ioctl(this->m_fd, VIDIOC_REQBUFS, &reqbuf);
    free(this->m_buffers);
  }
  this->m_buffers = NULL;
  this->n_buffers = 0;
  if (this->convert) v4lconvert_destroy(this->convert);
  this->convert = NULL;
  if (!((this->m_fd) < 0)) v4l2_close(this->m_fd);
  this->m_fd = -1;
}

int Libv4l2::MainSetup()
{
  assert(!(this->started));
  assert((this->m_fd) < 0);
  return 0;
}

void Libv4l2::MainQuit()
{
  this->cleanup();
  assert(!(this->started));
  assert((this->m_fd) < 0);
}

void Libv4l2::prepareData()
{
  ssize_t s;
  unsigned char * frame = NULL;
  struct v4l2_buffer buf;

  assert(this->data);
  if (this->data->image) free(this->data->image);
  this->data->image = NULL;
  this->data->image_count = 0;
  s = 0;
  if (this->read_mode)
  {
    assert((this->srcfmt.fmt.pix.sizeimage) > 0);
    frame = reinterpret_cast<unsigned char *>(malloc(this->srcfmt.fmt.pix.sizeimage));
    assert(frame);
    s = v4l2_read(this->m_fd, frame, this->srcfmt.fmt.pix.sizeimage);
    if (!s)
    {
      PLAYER_ERROR("Cannot capture frame");
      free(frame);
      return;
    }
    if (s < 0)
    {
      free(frame);
      return;
    }
  } else
  {
    memset(&buf, 0, sizeof buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (v4l2_ioctl(this->m_fd, VIDIOC_DQBUF, &buf) < 0) return;
    s = buf.bytesused;
    if (s <= 0)
    {
      PLAYER_ERROR("No data (or other error)");
      return;
    }
    frame = reinterpret_cast<unsigned char *>(malloc(s));
    assert(frame);
    assert(this->m_buffers);
    memcpy(frame, this->m_buffers[buf.index].start, s);
    assert(v4l2_ioctl(this->m_fd, VIDIOC_QBUF, &buf) >= 0);
  }
  assert(s > 0);
  assert(frame);
  assert((this->dstfmt.fmt.pix.pixelformat) == V4L2_PIX_FMT_RGB24);
  memset(this->data, 0, sizeof(player_camera_data_t));
  this->data->width = this->dstfmt.fmt.pix.width;
  this->data->height = this->dstfmt.fmt.pix.height;
  this->data->bpp = 24;
  this->data->format = PLAYER_CAMERA_FORMAT_RGB888;
  this->data->fdiv = 0;
  this->data->compression = PLAYER_CAMERA_COMPRESS_RAW;
  this->data->image_count = s;
  this->data->image = frame;
}

void Libv4l2::Main()
{
  struct v4l2_capability cap;
  struct v4l2_fract interval;
  struct v4l2_streamparm parm;
  struct v4l2_requestbuffers req;
  struct v4l2_buffer buf;
  struct timespec ts;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int interval_ok = 0;
  int i;

  assert(!(this->started));
  assert(!(this->data));
  assert(!(this->convert));
  assert(!(this->m_buffers));
  assert(!(this->n_buffers));
  assert((this->m_fd) < 0);
  this->m_fd = open(this->port, O_RDWR | O_NONBLOCK);
  assert(!((this->m_fd) < 0));
  memset(&cap, 0, sizeof cap);
  assert(v4l2_ioctl(this->m_fd, VIDIOC_QUERYCAP, &cap) >= 0);
  assert(v4l2_fd_open(this->m_fd, V4L2_ENABLE_ENUM_FMT_EMULATION) >= 0);
  this->convert = v4lconvert_create(this->m_fd);
  assert(this->convert);
  memset(&(this->srcfmt), 0, sizeof this->srcfmt);
  this->srcfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  assert(v4l2_ioctl(this->m_fd, VIDIOC_G_FMT, &(this->srcfmt)) >= 0);
  if ((this->srcfmt.fmt.pix.pixelformat) != V4L2_PIX_FMT_RGB24)
  {
    memset(&interval, 0, sizeof interval);
    memset(&parm, 0, sizeof parm);
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    interval_ok = 0;
    if (v4l2_ioctl(this->m_fd, VIDIOC_G_PARM, &parm) >= 0) if (parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
    {
      interval = parm.parm.capture.timeperframe;
      interval_ok = !0;
    }
    this->srcfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    v4l2_ioctl(this->m_fd, VIDIOC_S_FMT, &(this->srcfmt)); /* ignore the result! */
    memset(&(this->srcfmt), 0, sizeof this->srcfmt);
    this->srcfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    assert(v4l2_ioctl(this->m_fd, VIDIOC_G_FMT, &(this->srcfmt)) >= 0);
    if (interval_ok)
    {
      memset(&parm, 0, sizeof parm);
      parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (!(v4l2_ioctl(this->m_fd, VIDIOC_G_PARM, &parm) < 0)) if (parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
      {
        parm.parm.capture.timeperframe = interval;
        v4l2_ioctl(this->m_fd, VIDIOC_S_PARM, &parm); /* ignore the result! */
      }
    }
  }
  assert((this->srcfmt.fmt.pix.sizeimage) > 0);
  memcpy(&(this->dstfmt), &(this->srcfmt), sizeof this->dstfmt);
  this->dstfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
  if (!(this->read_mode))
  {
    memset(&req, 0, sizeof req);
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    req.count = 3;
    assert(v4l2_ioctl(this->m_fd, VIDIOC_REQBUFS, &req) >= 0);
    assert((req.count) > 0);
    this->m_buffers = reinterpret_cast<buffer *>(calloc(req.count, sizeof(struct Libv4l2::buffer)));
    assert(this->m_buffers);
    this->n_buffers = req.count;
    for (i = 0; i < static_cast<int>(this->n_buffers); i++)
    {
      memset(&buf, 0, sizeof buf);
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = i;
      assert(v4l2_ioctl(this->m_fd, VIDIOC_QUERYBUF, &buf) >= 0);
      this->m_buffers[i].length = buf.length;
      this->m_buffers[i].start = v4l2_mmap(NULL, this->m_buffers[i].length, PROT_READ | PROT_WRITE, MAP_SHARED, this->m_fd, buf.m.offset);
      assert(this->m_buffers[i].start != MAP_FAILED);
      assert(this->m_buffers[i].start);
    }
    for (i = 0; i < static_cast<int>(this->n_buffers); i++)
    {
      memset(&buf, 0, sizeof buf);
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = i;
      assert(v4l2_ioctl(this->m_fd, VIDIOC_QBUF, &buf) >= 0);
    }
    assert(v4l2_ioctl(this->m_fd, VIDIOC_STREAMON, &type) >= 0);
  }
  this->started = !0;
  this->data = reinterpret_cast<player_camera_data_t *>(malloc(sizeof(player_camera_data_t)));
  assert(this->data);
  memset(this->data, 0, sizeof(player_camera_data_t));
  this->data->image = NULL;
  this->data->image_count = 0;
  for (;;)
  {
    if ((this->sleep_nsec) > 0)
    {
      ts.tv_sec = 0;
      ts.tv_nsec = this->sleep_nsec;
      nanosleep(&ts, NULL);
    }
    pthread_testcancel();
    this->prepareData();
    pthread_testcancel();
    assert(this->data);
    if (!(this->data->image)) continue;
    assert((this->data->image_count) > 0);
    if (!(this->request_only))
    {
      this->Publish(this->camera_addr,
                    PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE,
                    reinterpret_cast<void *>(this->data), 0, NULL, true);
                    // copy = true, we need to preserve the data
    }
    pthread_testcancel();
    this->ProcessMessages();
    pthread_testcancel();
  }
}

int Libv4l2::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_CAMERA_REQ_GET_IMAGE, this->camera_addr))
  {
    if (!(this->started)) return -1;
    if (!(this->data)) return -1;
    if (!(this->data->image)) return -1;
    if (!((this->data->image_count) > 0)) return -1;
    this->Publish(this->camera_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_CAMERA_REQ_GET_IMAGE,
                  reinterpret_cast<void *>(this->data));
    return 0;
  }
  return -1;
}

Driver * Libv4l2_Init(ConfigFile * cf, int section)
{
  return (Driver *)(new Libv4l2(cf, section));
}

void libv4l2_Register(DriverTable * table)
{
  table->AddDriver("libv4l2", Libv4l2_Init);
}
