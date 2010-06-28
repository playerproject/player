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
 * Localization device that treats blobs as object markers that can be used
 * to retrieve real position of given object.
 *
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_Blobposition Blobposition
 * @brief Localization device that treats blobs as object markers that can be used
to retrieve real position of given object.

This device computes real position of some object found by blobfinder that denotes it
by a blob of given color key.
If only one color key is given in configuration file options, only px and py coordinates
will be computed leaving pa filled with zero. If two color keys are given, two
differently coloured blobs (with each of those color keys) must be found by blobfinder
device in order to compute object position. In such a case position of the object
is found in the middle of the line segment between those two blobs. Knowing which blob
is which this driver can compute complete information about object position (px, py, pa).
If more than one blob with any of given color keys is found, no position is computed.
If position cannot be computed, this fact will be indicated by 'stall' field set to 1;
other fields will be filled with previously computed values.

When this driver is started, camera device from which blobfinder reads data
should remain static all the time. Moving the camera distorts computation results.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_position2d

@par Requires

- @ref interface_blobfinder

@par Configuration requests

- None

@par Configuration file options

- x_ppm (integer)
  - Default: 100
  - X-axis pixels per meter
- y_ppm (integer)
  - Default: 100
  - Y-axis pixels per meter
- min_area (integer)
  - Default: 1
  - Minimal size of blob in pixels (noise reduction)
- stall_when_lost (integer)
  - Default: 1
  - If set to non-zero, whenever position cannot be computed, this fact will be indicated
    by 'stall' field set to 1; other fields will be filled with previously computed values
  - If set to zero, whenever position cannot be computed, no position2d data will be
    published
- expected_size (integer tuple)
  - Default: [640 480]
  - Expected size of image reported by blobfinder device;
    position will not be computed if the size does not match
- offset (integer tuple)
  - Default: [320 240]
  - Offset of (0.0, 0.0) point (given in pixels)
- color_key (string tuple)
  - Default: None, must be set
  - Tuple of one or two color keys, each 8-digit hex value (0x prefixed)
  - First color key denotes rightmost (top) blob

@par Example

@verbatim
driver
(
  name "blobposition"
  provides ["position2d:0"]
  requires ["blobfinder:0"]
  x_ppm 99
  y_ppm 97
  expected_size [640 480]
  offset [358 258]
  colorkeys ["0x00ff0000" "0x0000ff00"]
)
@endverbatim

@author Paul Osmialowski

*/

/** @} */

#include <stddef.h>
#include <libplayercore/playercore.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif
#ifndef DTOR
#define DTOR(d) ((d) * M_PI / 180.0)
#endif

#define EPS 0.000001

class Blobposition: public Driver
{
  public:
    Blobposition(ConfigFile * cf, int section);
    virtual ~Blobposition();
    virtual int Setup();
    virtual int Shutdown();
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);
  private:
    player_devaddr_t p_position2d_addr;
    player_devaddr_t r_blobfinder_addr;
    Device * r_blobfinder;
    int x_ppm;
    int y_ppm;
    int min_area;
    int stall_when_lost;
    int size_x;
    int size_y;
    int offset_x;
    int offset_y;
    uint32_t colorkeys[2];
    int num_colorkeys;
    double prev_px;
    double prev_py;
    double prev_pa;
};

Blobposition::Blobposition(ConfigFile * cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  const char * hexbuf;
  int i;

  memset(&(this->p_position2d_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->r_blobfinder_addr), 0, sizeof(player_devaddr_t));
  this->r_blobfinder = NULL;
  this->x_ppm = 0;
  this->y_ppm = 0;
  this->min_area = 0;
  this->stall_when_lost = 0;
  this->size_x = 0;
  this->size_y = 0;
  this->offset_x = 0;
  this->offset_y = 0;
  this->prev_px = 0.0;
  this->prev_py = 0.0;
  this->prev_pa = 0.0;
  memset(this->colorkeys, 0, sizeof this->colorkeys);
  this->num_colorkeys = 0;
  if (cf->ReadDeviceAddr(&(this->p_position2d_addr), section, "provides",
                           PLAYER_POSITION2D_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->p_position2d_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->r_blobfinder_addr), section, "requires", PLAYER_BLOBFINDER_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  this->x_ppm = cf->ReadInt(section, "x_ppm", 100);
  if ((this->x_ppm) <= 0)
  {
    PLAYER_ERROR("Invalid x_ppm value");
    this->SetError(-1);
    return;
  }
  this->y_ppm = cf->ReadInt(section, "y_ppm", 100);
  if ((this->y_ppm) <= 0)
  {
    PLAYER_ERROR("Invalid y_ppm value");
    this->SetError(-1);
    return;
  }
  this->min_area = cf->ReadInt(section, "min_area", 1);
  if ((this->min_area) <= 0)
  {
    PLAYER_ERROR("Invalid min_area value");
    this->SetError(-1);
    return;
  }
  this->stall_when_lost = cf->ReadInt(section, "stall_when_lost", 1);
  if (cf->GetTupleCount(section, "expected_size") != 2)
  {
    PLAYER_ERROR("Invalid tuple expected_size");
    this->SetError(-1);
    return;
  }
  this->size_x = cf->ReadTupleInt(section, "expected_size", 0, 640);
  if ((this->size_x) <= 0)
  {
    PLAYER_ERROR("Invalid expected_size x value");
    this->SetError(-1);
    return;
  }
  this->size_y = cf->ReadTupleInt(section, "expected_size", 1, 480);
  if ((this->size_y) <= 0)
  {
    PLAYER_ERROR("Invalid expected_size y value");
    this->SetError(-1);
    return;
  }
  if (cf->GetTupleCount(section, "offset") != 2)
  {
    PLAYER_ERROR("Invalid tuple offset");
    this->SetError(-1);
    return;
  }
  this->offset_x = cf->ReadTupleInt(section, "offset", 0, 320);
  if ((this->offset_x) <= 0)
  {
    PLAYER_ERROR("Invalid offset x value");
    this->SetError(-1);
    return;
  }
  this->offset_y = cf->ReadTupleInt(section, "offset", 1, 240);
  if ((this->offset_y) <= 0)
  {
    PLAYER_ERROR("Invalid offset y value");
    this->SetError(-1);
    return;
  }
  this->num_colorkeys = cf->GetTupleCount(section, "colorkeys");
  switch (this->num_colorkeys)
  {
  case 1:
  case 2:
    break;
  default:
    PLAYER_ERROR("Invalid colorkeys tuple");
    this->SetError(-1);
    return;
  }
  for (i = 0; i < (this->num_colorkeys); i++)
  {
    hexbuf = cf->ReadTupleString(section, "colorkeys", i, "");
    if (!hexbuf)
    {
      PLAYER_ERROR("internal error: NULL");
      this->SetError(-1);
      return;
    }
    if (strlen(hexbuf) != 10)
    {
      PLAYER_ERROR("invalid colorkeys tuple");
      this->SetError(-1);
      return;
    }
    if ((hexbuf[0] != '0') || (hexbuf[1] != 'x'))
    {
      PLAYER_ERROR("invalid colorkeys tuple");
      this->SetError(-1);
      return;
    }
    sscanf(hexbuf + 2, "%x", &(this->colorkeys[i]));
  }
}

Blobposition::~Blobposition() { }

int Blobposition::Setup()
{
  this->prev_px = 0.0;
  this->prev_py = 0.0;
  this->prev_pa = 0.0;
  this->r_blobfinder = deviceTable->GetDevice(this->r_blobfinder_addr);
  if (!this->r_blobfinder)
  {
    PLAYER_ERROR("unable to locate suitable blobfinder device");
    return -1;
  }
  if (this->r_blobfinder->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to blobfinder device");
    this->r_blobfinder = NULL;
    return -1;
  }
  return 0;
}

int Blobposition::Shutdown()
{
  if (this->r_blobfinder) this->r_blobfinder->Unsubscribe(this->InQueue);
  this->r_blobfinder = NULL;
  return 0;
}

int Blobposition::ProcessMessage(QueuePointer &resp_queue,
                                 player_msghdr * hdr,
                                 void * data)
{
  player_blobfinder_data_t * blobs;
  player_position2d_data_t pos_data;
  int i, j, d, found;
  int counters[2];
  int indexes[2];
  double x0, y0;
  double x1, y1;

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
    blobs = reinterpret_cast<player_blobfinder_data_t *>(data);
    if (!blobs)
    {
      PLAYER_ERROR("internal error");
      return -1;
    }
    if ((static_cast<int>(blobs->width) != (this->size_x)) || (static_cast<int>(blobs->height) != (this->size_y)))
    {
      PLAYER_WARN("wrong image size");
      return -1;
    }
    for (i = 0; i < (this->num_colorkeys); i++)
    {
      counters[i] = 0;
      indexes[i] = 0;
    }
    for (i = 0; i < static_cast<int>(blobs->blobs_count); i++)
    {
      if (!(blobs->blobs))
      {
        PLAYER_ERROR("internal error");
        return -1;
      }
      if (static_cast<int>((blobs->blobs[i].area)) < (this->min_area)) continue;
      for (j = 0; j < (this->num_colorkeys); j++)
      {
        if ((blobs->blobs[i].color) == (this->colorkeys[j]))
        {
          counters[j]++;
          indexes[j] = i;
        }
      }
    }
    found = !0;
    for (i = 0; i < (this->num_colorkeys); i++)
    {
      if (counters[i] != 1)
      {
        found = 0;
        break;
      }
    }
    memset(&pos_data, 0, sizeof pos_data);
    if (found)
    {
      switch (this->num_colorkeys)
      {
      case 1:
        pos_data.pos.px = (static_cast<double>((static_cast<int>(blobs->blobs[indexes[0]].x)) - (this->offset_x)) / static_cast<double>(this->x_ppm));
        pos_data.pos.py = (static_cast<double>((((this->size_y) - 1) - static_cast<int>(blobs->blobs[indexes[0]].y)) - (((this->size_y) - 1) - (this->offset_y))) / static_cast<double>(this->y_ppm));
        pos_data.pos.pa = 0.0;
        break;
      case 2:
        d = MAX(static_cast<int>(blobs->blobs[indexes[0]].x), static_cast<int>(blobs->blobs[indexes[1]].x)) - MIN(static_cast<int>(blobs->blobs[indexes[0]].x), static_cast<int>(blobs->blobs[indexes[1]].x));
        if (d < 0)
        {
          PLAYER_ERROR("invalid blob");
          return -1;
        }
        d >>= 1;
        j = (MIN(static_cast<int>(blobs->blobs[indexes[0]].x), static_cast<int>(blobs->blobs[indexes[1]].x)) + d);
        pos_data.pos.px = (static_cast<double>(j - (this->offset_x)) / static_cast<double>(this->x_ppm));
        d = MAX(static_cast<int>(blobs->blobs[indexes[0]].y), static_cast<int>(blobs->blobs[indexes[1]].y)) - MIN(static_cast<int>(blobs->blobs[indexes[0]].y), static_cast<int>(blobs->blobs[indexes[1]].y));
        if (d < 0)
        {
          PLAYER_ERROR("invalid blob");
          return -1;
        }
        d >>= 1;
        j = (MIN(static_cast<int>(blobs->blobs[indexes[0]].y), static_cast<int>(blobs->blobs[indexes[1]].y)) + d);
        pos_data.pos.py = (static_cast<double>((((this->size_y) - 1) - j) - (((this->size_y) - 1) - (this->offset_y))) / static_cast<double>(this->y_ppm));
        x0 = (static_cast<double>(static_cast<int>(blobs->blobs[indexes[1]].x) - (this->offset_x)) / static_cast<double>(this->x_ppm));
        y0 = (static_cast<double>((((this->size_y) - 1) - static_cast<int>(blobs->blobs[indexes[1]].y)) - (((this->size_y) - 1) - (this->offset_y))) / static_cast<double>(this->y_ppm));
        x1 = (static_cast<double>(static_cast<int>(blobs->blobs[indexes[0]].x) - (this->offset_x)) / static_cast<double>(this->x_ppm));
        y1 = (static_cast<double>((((this->size_y) - 1) - static_cast<int>(blobs->blobs[indexes[0]].y)) - (((this->size_y) - 1) - (this->offset_y))) / static_cast<double>(this->y_ppm));
        if (fabs(x1 - x0) < EPS)
        {
          pos_data.pos.pa = DTOR((y0 > y1) ? -90.0 : 90.0);
        } else
        {
          pos_data.pos.pa = atan2(y1 - y0, x1 - x0);
        }
        break;
      default:
        PLAYER_ERROR("internal error");
        return -1;
      }
      pos_data.stall = 0;
    } else
    {
      pos_data.pos.px = this->prev_px;
      pos_data.pos.py = this->prev_py;
      pos_data.pos.pa = this->prev_pa;
      pos_data.stall = !0;
    }
    this->prev_px = pos_data.pos.px;
    this->prev_py = pos_data.pos.py;
    this->prev_pa = pos_data.pos.pa;
    if ((found) || (this->stall_when_lost))
    {
      this->Publish(this->p_position2d_addr,
                      PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
                      reinterpret_cast<void *>(&pos_data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
    }
    return 0;
  }
  return -1;
}

Driver * Blobposition_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new Blobposition(cf, section));
}

void blobposition_Register(DriverTable * table)
{
  table->AddDriver("blobposition", Blobposition_Init);
}
