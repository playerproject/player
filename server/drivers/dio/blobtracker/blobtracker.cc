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
 * Blob tracker that sends ptz commands in order to make camera follow certain blobs.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_Blobtracker Blobtracker
 * @brief Blob tracker that sends ptz commands in order to make camera follow certain blobs.

@par Compile-time dependencies

- None

@par Provides

- @ref interface_dio - lowest bit of dio bits is set to 1 whenever any of desired blobs is found

@par Requires

- @ref interface_blobfinder
- @ref interface_ptz

@par Configuration requests

- None

@par Configuration file options

- r (integer)
  - Default: 0
  - r value of desired blobs RGB
- g (integer)
  - Default: 0
  - g value of desired blobs RGB
- b (integer)
  - Default: 0
  - b value of desired blobs RGB
- idle_area (double)
  - Default: 0.4
  - Valid value range 0.0 to 1.0
  - Middle part of the image where blob moves are not followed
  - Border cases: 0.0 - no part of the image is idle, 1.0 - whole image
- step (degrees)
  - Default: 1.0
  - How many degrees to move during one iteration 
  - The default value (1.0) is good for Stage,
    however it's too small for Logitech Sphere AF camera
- pan_speed (degrees)
  - Default: 10.0
  - Angular pan speed
- tilt_speed (degrees)
  - Default: 10.0
  - Agular tilt speed
- zoom (degrees)
  - Default: not set
  - Field of view (typically it is not required to set this value)
  - Negative value means 'not set' - field of view will not be changed

@par Example

Look for a green sheet:

@verbatim
driver
(
  name "camerauvc"
  provides ["camera:0"]
  port "/dev/video1"
  size [640 480]
)
driver
(
  name "sphereptz"
  provides ["ptz:0"]
  port "/dev/video1"
  autoreset 0
)
driver
(
  name "cmvision"
  provides ["blobfinder:0"]
  requires ["camera:0"]
  colorfile "colors.txt"
)
driver
(
  name "blobtracker"
  provides ["dio:0"]
  requires ["6665:blobfinder:0" "6665:ptz:0"]
  r 0
  g 255
  b 0
  step 5.0
  alwayson 1
)
@endverbatim

colors.txt file I used for this was:

@verbatim
[Colors]
(255,  0,  0) 0.000000 10 Red
(  0,255,  0) 0.000000 10 Greeen
(  0,  0,255) 0.000000 10 Blue

[Thresholds]
( 25:164, 80:120,150:240)
( 20:220, 50:120, 40:115)
( 15:190,145:255, 40:120)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <libplayercore/playercore.h>

#include <stddef.h>
#include <string.h>
#include <math.h>

#define EPS 0.0000001

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#ifndef DTOR
#define DTOR(d) ((d) * (M_PI) / 180.0)
#endif

class Blobtracker : public Driver
{
public:
  Blobtracker(ConfigFile * cf, int section);
  virtual ~Blobtracker();
  virtual int Setup();
  virtual int Shutdown();
  virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data);
private:
  player_devaddr_t r_blobfinder_addr;
  player_devaddr_t r_ptz_addr;
  player_devaddr_t p_dio_addr;
  Device * r_blobfinder_dev;
  Device * r_ptz_dev;
  player_ptz_data_t ptz_data;
  unsigned char r;
  unsigned char g;
  unsigned char b;
  double idle_area;
  double step;
  double pan_speed;
  double tilt_speed;
  double zoom;
  bool valid_ptz_data;
};

Blobtracker::Blobtracker(ConfigFile * cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  memset(&(this->r_blobfinder_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->r_ptz_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->p_dio_addr), 0, sizeof(player_devaddr_t));
  this->r_blobfinder_dev = NULL;
  this->r_ptz_dev = NULL;
  memset(&(this->ptz_data), 0, sizeof(ptz_data));
  this->ptz_data.pan = 0.0;
  this->ptz_data.tilt = 0.0;
  this->r = 0;
  this->g = 0;
  this->b = 0;
  this->idle_area = 0.0;
  this->step = 0.0;
  this->pan_speed = 0.0;
  this->tilt_speed = 0.0;
  this->valid_ptz_data = false;
  if (cf->ReadDeviceAddr(&(this->p_dio_addr), section, "provides",
                         PLAYER_DIO_CODE, -1, NULL))
  {
    PLAYER_ERROR("cannot provide dio device");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->p_dio_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->r_blobfinder_addr), section, "requires",
                         PLAYER_BLOBFINDER_CODE, -1, NULL))
  {
    PLAYER_ERROR("cannot require blobfinder device");
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->r_ptz_addr), section, "requires",
                         PLAYER_PTZ_CODE, -1, NULL))
  {
    PLAYER_ERROR("cannot require ptz device");
    this->SetError(-1);
    return;
  }
  this->r = static_cast<unsigned char>(cf->ReadInt(section, "r", 0));
  this->g = static_cast<unsigned char>(cf->ReadInt(section, "g", 0));
  this->b = static_cast<unsigned char>(cf->ReadInt(section, "b", 0));
  this->idle_area = cf->ReadFloat(section, "idle_area", 0.4);
  if ((this->idle_area < 0.0) || (this->idle_area > 1.0))
  {
    PLAYER_ERROR("invalid idle_area value");
    this->SetError(-1);
    return;
  }
  this->step = cf->ReadAngle(section, "step", DTOR(1.0));
  if ((this->step) < EPS)
  {
    PLAYER_ERROR("invalid step value");
    this->SetError(-1);
    return;
  }
  this->pan_speed = cf->ReadAngle(section, "pan_speed", DTOR(10.0));
  if ((this->pan_speed) < EPS)
  {
    PLAYER_ERROR("invalid pan_speed value");
    this->SetError(-1);
    return;
  }
  this->tilt_speed = cf->ReadAngle(section, "tilt_speed", DTOR(10.0));
  if ((this->tilt_speed) < EPS)
  {
    PLAYER_ERROR("invalid tilt_speed value");
    this->SetError(-1);
    return;
  }
  this->zoom = cf->ReadAngle(section, "zoom", -1.0);
}

Blobtracker::~Blobtracker() { }

int Blobtracker::Setup()
{
  memset(&(this->ptz_data), 0, sizeof(ptz_data));
  this->valid_ptz_data = false;
  this->r_blobfinder_dev = deviceTable->GetDevice(this->r_blobfinder_addr);
  if (!(this->r_blobfinder_dev)) return -1;
  if (this->r_blobfinder_dev->Subscribe(this->InQueue))
  {
    this->r_blobfinder_dev = NULL;
    return -1;
  }
  this->r_ptz_dev = deviceTable->GetDevice(this->r_ptz_addr);
  if (!(this->r_ptz_dev))
  {
    this->r_blobfinder_dev->Unsubscribe(this->InQueue);
    this->r_blobfinder_dev = NULL;
    return -1;
  }
  if (this->r_ptz_dev->Subscribe(this->InQueue))
  {
    this->r_blobfinder_dev->Unsubscribe(this->InQueue);
    this->r_blobfinder_dev = NULL;
    this->r_ptz_dev = NULL;
    return -1;
  }
  return 0;
}

int Blobtracker::Shutdown()
{
  if (this->r_blobfinder_dev) this->r_blobfinder_dev->Unsubscribe(this->InQueue);
  this->r_blobfinder_dev = NULL;
  if (this->r_ptz_dev) this->r_ptz_dev->Unsubscribe(this->InQueue);
  this->r_ptz_dev = NULL;
  return 0;
}

int Blobtracker::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  uint32_t idle_width, idle_height, u, v;
  player_blobfinder_data_t * bdata;
  player_dio_data_t ddata;
  player_ptz_cmd_t cmd;
  int i, idx;
  double dPan, dTilt;

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            PLAYER_BLOBFINDER_DATA_BLOBS,
                            this->r_blobfinder_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL data");
      return -1;
    }
    bdata = reinterpret_cast<player_blobfinder_data_t *>(data);
    idle_width = static_cast<uint32_t>(static_cast<double>(bdata->width) * (this->idle_area));
    idle_height = static_cast<uint32_t>(static_cast<double>(bdata->height) * (this->idle_area));
    if ((idle_width > 0) && (idle_height > 0))
    {
      idx = -1;
      for (i = 0; i < static_cast<int>(bdata->blobs_count); i++)
      {
        if ((((bdata->blobs[i].color >> 16) & 0xff) == (this->r)) && (((bdata->blobs[i].color >> 8) & 0xff) == (this->g)) && ((bdata->blobs[i].color & 0xff) == (this->b)))
        {
          idx = i;
          break;
        }
      }
      memset(&ddata, 0, sizeof ddata);
      ddata.count = 1;
      dPan = 0.0;
      dTilt = 0.0;
      if (!(idx < 0))
      {
        u = (bdata->width - idle_width) / 2;
        v = u + idle_width;
        if ((bdata->blobs[idx].x) < u) dPan = step;
        if (((bdata->blobs[idx].x) >= u) && ((bdata->blobs[idx].x) <= v)) dPan = 0.0;
        if ((bdata->blobs[idx].x) > v) dPan = -step;
        u = (bdata->height - idle_height) / 2;
        v = u + idle_height;
        if ((bdata->blobs[idx].y) < u) dTilt = step;
        if (((bdata->blobs[idx].y) >= u) && ((bdata->blobs[idx].y) <= v)) dTilt = 0.0;
        if ((bdata->blobs[idx].y) > v) dTilt = -step;
        ddata.bits = 1;
      } else ddata.bits = 0;
      this->Publish(this->p_dio_addr,
                    PLAYER_MSGTYPE_DATA,
                    PLAYER_DIO_DATA_VALUES,
                    reinterpret_cast<void *>(&ddata),
                    0, NULL, true); // copy = true, do not dispose data placed on local stack!
      if ((this->valid_ptz_data) && ((fabs(dPan) > EPS) || (fabs(dTilt) > EPS)))
      {
        this->valid_ptz_data = false;
        memset(&cmd, 0, sizeof cmd);
        cmd.pan = this->ptz_data.pan + dPan;
        cmd.tilt = this->ptz_data.tilt + dTilt;
        if (this->zoom < 0.0) cmd.zoom = this->ptz_data.zoom;
        else cmd.zoom = this->zoom;
        cmd.panspeed = this->pan_speed;
        cmd.tiltspeed = this->tilt_speed;
        this->r_ptz_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_PTZ_CMD_STATE, reinterpret_cast<void *>(&cmd), 0, NULL);
      }
    }
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            PLAYER_PTZ_DATA_STATE,
                            this->r_ptz_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL data");
      return -1;
    }
    this->ptz_data = *(reinterpret_cast<player_ptz_data *>(data));
    this->valid_ptz_data = true;
    return 0;
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * Blobtracker_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new Blobtracker(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void blobtracker_Register(DriverTable * table)
{
  table->AddDriver("blobtracker", Blobtracker_Init);
}
