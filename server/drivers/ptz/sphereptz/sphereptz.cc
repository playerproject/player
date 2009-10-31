/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al.
 *
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
 * PTZ for Logitech Sphere AF webcams (based on experiences with real device).
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_SpherePTZ SpherePTZ
 * @brief PTZ for Logitech Sphere AF webcams (based on experiences with real device).

@par Compile-time dependencies

- &lt;sys/types.h&gt;
- &lt;linux/videodev2.h&gt;

@par Provides

- @ref interface_ptz

@par Requires

- None

@par Configuration requests

- None

@par Configuration file options

- port (string)
  - Default: "/dev/video0"
  - %Device to connect to.
- autoreset (integer)
  - Default: 1
  - If set to 1, camera will reset itself whenever it achieve zero position
    (this can be annoying, therefore it can be turned off)

@par Example

Can be safely used together with camerauvc driver:

@verbatim
driver
(
  name "camerauvc"
  provides ["camera:0"]
  port "/dev/video0"
  size [640 480]
)

driver
(
  name "sphereptz"
  provides ["ptz:0"]
  port "/dev/video0"
)
@endverbatim

@author Paul Osmialowski, Paulo Assis

*/
/** @} */

#include <libplayercore/playercore.h>

#if !defined (sun)
  #include <linux/fs.h>
  #include <linux/kernel.h>
#endif
#include <sys/types.h>
#include <linux/videodev.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "v4l2_controls.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#ifndef DTOR
#define DTOR(d) ((d) * (M_PI) / 180.0)
#endif

#ifndef RTOD
#define RTOD(r) ((static_cast<double>(r)) * 180.0 / M_PI)
#endif

class SpherePTZ : public ThreadedDriver
{
  public:
    SpherePTZ(ConfigFile * cf, int section);
    virtual ~SpherePTZ();
    virtual int MainSetup();
    virtual void MainQuit();
    virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data);
  private:
    virtual void Main();
    player_devaddr_t ptz_addr;
    const char * port;
    int autoreset;
    int fd;
    int currentX;
    int currentY;
    int desiredX;
    int desiredY;
};

SpherePTZ::SpherePTZ(ConfigFile * cf, int section) : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  memset(&(this->ptz_addr), 0, sizeof(player_devaddr_t));
  this->port = NULL;
  this->autoreset = 0;
  this->fd = -1;
  this->currentX = 0;
  this->currentY = 0;
  this->desiredX = 0;
  this->desiredY = 0;
  if (cf->ReadDeviceAddr(&(this->ptz_addr), section, "provides", PLAYER_PTZ_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->ptz_addr))
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
  this->autoreset = cf->ReadInt(section, "autoreset", 1);
}

SpherePTZ::~SpherePTZ()
{
  if (this->fd >= 0) close(this->fd);
}

int SpherePTZ::MainSetup()
{
  struct v4l2_capability cap;
  struct v4l2_ext_control xctrls[1];
  struct v4l2_ext_controls ctrls;
  struct timespec tspec;

  this->currentX = 0;
  this->currentY = 0;
  this->desiredX = 0;
  this->desiredY = 0;
  PLAYER_WARN1("opening %s", this->port);
  this->fd = open(this->port, O_RDWR);
  if (this->fd < 0)
  {
    PLAYER_ERROR1("Cannot open %s", this->port);
    return -1;
  }
  memset(&cap, 0, sizeof cap);
  if (ioctl(this->fd, VIDIOC_QUERYCAP, &cap) == -1)
  {
    PLAYER_ERROR("VIDIOC_QUERYCAP failed");
    close(this->fd);
    this->fd = -1;
    return -1;
  }
  if (!(cap.capabilities & V4L2_CAP_STREAMING))
  {
    PLAYER_WARN("V4L2_CAP_READWRITE check failed (ignored)");
  }
  if (!(cap.capabilities & V4L2_CAP_READWRITE))
  {
    PLAYER_WARN("V4L2_CAP_READWRITE check failed (ignored)");
  }
  memset(&ctrls, 0, sizeof ctrls);
  memset(&xctrls, 0, sizeof xctrls);
  xctrls[0].id = V4L2_CID_PAN_RESET_NEW;
  xctrls[0].value = 1;
  ctrls.count = 1;
  ctrls.controls = xctrls;
  if (ioctl(this->fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1)
  {
    PLAYER_ERROR("VIDIOC_S_EXT_CTRLS failed on V4L2_CID_PAN_RESET_NEW");
    close(this->fd);
    this->fd = -1;
    return -1;
  }
  tspec.tv_sec = 2;
  tspec.tv_nsec = 0;
  nanosleep(&tspec, NULL);
  memset(&ctrls, 0, sizeof ctrls);
  memset(&xctrls, 0, sizeof xctrls);
  xctrls[0].id = V4L2_CID_TILT_RESET_NEW;
  xctrls[0].value = 1;
  ctrls.count = 1;
  ctrls.controls = xctrls;
  if (ioctl(this->fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1)
  {
    PLAYER_ERROR("VIDIOC_S_EXT_CTRLS failed on V4L2_CID_TILT_RESET_NEW");
    close(this->fd);
    this->fd = -1;
    return -1;
  }
  tspec.tv_sec = 2;
  tspec.tv_nsec = 0;
  nanosleep(&tspec, NULL);
  return 0;
}

void SpherePTZ::MainQuit()
{
  if (this->fd >= 0) close(this->fd);
  this->fd = -1;
}

void SpherePTZ::Main()
{
  struct timespec tspec;
  player_ptz_data_t mPtzData;
  struct v4l2_ext_control xctrls[2];
  struct v4l2_ext_controls ctrls;
  int reset_pan, reset_tilt;

  for (;;)
  {
    tspec.tv_sec = 0;
    tspec.tv_nsec = 10000000;
    nanosleep(&tspec, NULL);
    pthread_testcancel();
    this->ProcessMessages();
    reset_pan = 0;
    reset_tilt = 0;
    if ((this->desiredX) > (this->currentX))
    {
      memset(xctrls, 0, sizeof xctrls);
      memset(&ctrls, 0, sizeof ctrls);
      xctrls[0].id = V4L2_CID_PAN_RELATIVE_NEW;
      xctrls[0].value = 128;
      xctrls[1].id = V4L2_CID_TILT_RELATIVE_NEW;
      xctrls[1].value = 0;
      ctrls.count = 2;
      ctrls.controls = xctrls;
      if (ioctl(this->fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1)
      {
        PLAYER_ERROR("VIDIOC_S_EXT_CTRLS failed while panning right");
      } else
      {
        this->currentX++;
        if ((!(this->currentX)) && (!(this->currentY))) reset_pan = !0;
      }
    } else if ((this->desiredX) < (this->currentX))
    {
      memset(xctrls, 0, sizeof xctrls);
      memset(&ctrls, 0, sizeof ctrls);
      xctrls[0].id = V4L2_CID_PAN_RELATIVE_NEW;
      xctrls[0].value = -128;
      xctrls[1].id = V4L2_CID_TILT_RELATIVE_NEW;
      xctrls[1].value = 0;
      ctrls.count = 2;
      ctrls.controls = xctrls;
      if (ioctl(this->fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1)
      {
        PLAYER_ERROR("VIDIOC_S_EXT_CTRLS failed while panning left");
      } else
      {
        this->currentX--;
        if ((!(this->currentX)) && (!(this->currentY))) reset_pan = !0;
      }
    }
    if ((this->desiredY) > (this->currentY))
    {
      memset(xctrls, 0, sizeof xctrls);
      memset(&ctrls, 0, sizeof ctrls);
      xctrls[0].id = V4L2_CID_PAN_RELATIVE_NEW;
      xctrls[0].value = 0;
      xctrls[1].id = V4L2_CID_TILT_RELATIVE_NEW;
      xctrls[1].value = -128;
      ctrls.count = 2;
      ctrls.controls = xctrls;
      if (ioctl(this->fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1)
      {
        PLAYER_ERROR("VIDIOC_S_EXT_CTRLS failed while tilting up");
      } else
      {
        this->currentY++;
        if ((!(this->currentX)) && (!(this->currentY))) reset_tilt = !0;
      }
    } else if ((this->desiredY) < (this->currentY))
    {
      memset(xctrls, 0, sizeof xctrls);
      memset(&ctrls, 0, sizeof ctrls);
      xctrls[0].id = V4L2_CID_PAN_RELATIVE_NEW;
      xctrls[0].value = 0;
      xctrls[1].id = V4L2_CID_TILT_RELATIVE_NEW;
      xctrls[1].value = 128;
      ctrls.count = 2;
      ctrls.controls = xctrls;
      if (ioctl(this->fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1)
      {
        PLAYER_ERROR("VIDIOC_S_EXT_CTRLS failed while tilting down");
      } else
      {
        this->currentY--;
        if ((!(this->currentX)) && (!(this->currentY))) reset_tilt = !0;
      }
    }
    pthread_testcancel();
    if ((!(this->currentX)) && (!(this->currentY)))
    {
      if ((reset_pan) && (this->autoreset))
      {
        memset(xctrls, 0, sizeof xctrls);
        memset(&ctrls, 0, sizeof ctrls);
        xctrls[0].id = V4L2_CID_PAN_RESET_NEW;
        xctrls[0].value = 1;
        ctrls.count = 1;
        ctrls.controls = xctrls;
        if (ioctl(this->fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1)
        {
          PLAYER_ERROR("VIDIOC_S_EXT_CTRLS failed on V4L2_CID_PAN_RESET_NEW");
        } else
        {
          tspec.tv_sec = 2;
          tspec.tv_nsec = 0;
          nanosleep(&tspec, NULL);
          pthread_testcancel();
        }
      }
      if ((reset_tilt) && (this->autoreset))
      {
        memset(xctrls, 0, sizeof xctrls);
        memset(&ctrls, 0, sizeof ctrls);
        xctrls[0].id = V4L2_CID_TILT_RESET_NEW;
        xctrls[0].value = 1;
        ctrls.count = 1;
        ctrls.controls = xctrls;
        if (ioctl(this->fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1)
        {
          PLAYER_ERROR("VIDIOC_S_EXT_CTRLS failed on V4L2_CID_TILT_RESET_NEW");
        } else
        {
          tspec.tv_sec = 2;
          tspec.tv_nsec = 0;
          nanosleep(&tspec, NULL);
          pthread_testcancel();
        }
      }
    }
    memset(&mPtzData, 0, sizeof mPtzData);
    mPtzData.pan = DTOR(this->currentX * 2.5);
    mPtzData.tilt = DTOR(this->currentY * 2.5);
    mPtzData.zoom = -1.0;
    mPtzData.panspeed = -1.0;
    mPtzData.tiltspeed = -1.0;
    this->Publish(this->ptz_addr, PLAYER_MSGTYPE_DATA, PLAYER_PTZ_DATA_STATE, reinterpret_cast<void *>(&mPtzData));
    pthread_testcancel();
  }
}

int SpherePTZ::ProcessMessage(QueuePointer & resp_queue,
                              player_msghdr * hdr,
                              void * data)
{
  player_ptz_cmd_t * cmd;

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_PTZ_CMD_STATE, this->ptz_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL data");
      return -1;
    }
    cmd = reinterpret_cast<player_ptz_cmd_t *>(data);
    if (!cmd)
    {
      PLAYER_ERROR("NULL cmd");
      return -1;
    }
    this->desiredX = static_cast<int>(RTOD(cmd->pan) / 2.5);
    if ((this->desiredX) > 36) this->desiredX = 36;
    if ((this->desiredX) < -36) this->desiredX = -36;
    this->desiredY = static_cast<int>(RTOD(cmd->tilt) / 2.5);
    if ((this->desiredY) > 12) this->desiredY = 12;
    if ((this->desiredY) < -12) this->desiredY = -12;
    return 0;
  }
  return -1;
}

Driver * SpherePTZ_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new SpherePTZ(cf, section));
}

void sphereptz_Register(DriverTable* table)
{
  table->AddDriver("sphereptz", SpherePTZ_Init);
}
