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
// Desc: Blobfinder to dio converter
// Author: Paul Osmialowski
// Date: 18 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_blobtodio blobtodio
 * @brief Blobfinder to dio converter

The blobtodio driver converts blobfinder data to boolean values (true = blobs
found, false = blobs not found)

@par Compile-time dependencies

- none

@par Provides

- @ref interface_dio

@par Requires

- @ref interface_blobfinder
- optionally @ref interface_dio to send dio commands to

@par Configuration requests

- none

@par Configuration file options

- color[n] (integer tuple)
  - Default: none
  - RGB arrays of tracked colors
  - At least one is required
  - n >= 0; n < 32
- threshold (integer)
  - Default: 1
  - Minimal number of matching blobs
  - Greater than 0

@par Example

@verbatim
driver
(
  name "blobtodio"
  provides ["dio:0"]
  requires ["blobfinder:0"]
# red and blue:
  color[0] [255 0 0]
  color[1] [0 0 255]
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL and size_t
#include <string.h> // for memset()
#include <assert.h>
#include <libplayercore/playercore.h>

#if defined (WIN32)
  #define snprintf _snprintf
#endif

class BlobToDio : public Driver
{
  public: BlobToDio(ConfigFile * cf, int section);
  public: virtual ~BlobToDio();
  public: virtual int Setup();
  public: virtual int Shutdown();
  public: virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data);
  private: player_devaddr_t dio_provided_addr;
  private: player_devaddr_t blobfinder_required_addr;
  private: player_devaddr_t dio_required_addr;
  private: Device * blobfinder_required_dev;
  private: Device * dio_required_dev;
  private: int use_dio_cmd;
  private: uint32_t color_count;
  private: uint32_t r[32];
  private: uint32_t g[32];
  private: uint32_t b[32];
  private: int threshold;
};

BlobToDio::BlobToDio(ConfigFile * cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  int i, j;
  char entry[12];

  memset(&(this->dio_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->blobfinder_required_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_required_addr), 0, sizeof(player_devaddr_t));
  this->blobfinder_required_dev = NULL;
  this->dio_required_dev = NULL;
  this->use_dio_cmd = 0;
  this->color_count = 0;
  memset(this->r, 0, sizeof this->r);
  memset(this->g, 0, sizeof this->g);
  memset(this->b, 0, sizeof this->b);
  this->threshold = 0;
  if (cf->ReadDeviceAddr(&(this->dio_provided_addr), section, "provides", PLAYER_DIO_CODE, -1, NULL))
  {
    PLAYER_ERROR("cannot provide dio device");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->dio_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->blobfinder_required_addr), section, "requires", PLAYER_BLOBFINDER_CODE, -1, NULL))
  {
    PLAYER_ERROR("cannot require blobfinder device");
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->dio_required_addr), section, "requires", PLAYER_DIO_CODE, -1, NULL))
  {
    PLAYER_WARN("dio device not required");
    this->use_dio_cmd = 0;
  } else
  {
    PLAYER_WARN("commands will be sent to subscribed dio device");
    this->use_dio_cmd = !0;
  }
  this->color_count = 0;
  for (i = 0; i < 32; i++)
  {
    snprintf(entry, sizeof entry, "color[%d]", i);
    j = cf->GetTupleCount(section, entry);
    if (j <= 0) break;
    if (j != 3)
    {
      PLAYER_ERROR1("Invalid %s tuple", entry);
      this->SetError(-1);
      return;
    }
    j = cf->ReadTupleInt(section, entry, 0, -1);
    if ((j < 0) || (j > 255))
    {
      PLAYER_ERROR1("Invalid %s tuple", entry);
      this->SetError(-1);
      return;
    }
    this->r[i] = (static_cast<uint32_t>(j)) << 16;
    j = cf->ReadTupleInt(section, entry, 1, -1);
    if ((j < 0) || (j > 255))
    {
      PLAYER_ERROR1("Invalid %s tuple", entry);
      this->SetError(-1);
      return;
    }
    this->g[i] = (static_cast<uint32_t>(j)) << 8;
    j = cf->ReadTupleInt(section, entry, 2, -1);
    if ((j < 0) || (j > 255))
    {
      PLAYER_ERROR1("Invalid %s tuple", entry);
      this->SetError(-1);
      return;
    }
    this->b[i] = (static_cast<uint32_t>(j));
    this->color_count++;
  }
  if (!((this->color_count) > 0))
  {
    PLAYER_ERROR("No colors configured");
    this->SetError(-1);
    return;
  }
  PLAYER_WARN1("Configured %d colors", this->color_count);
  this->threshold = cf->ReadInt(section, "threshold", 1);
  if (!((this->threshold) > 0))
  {
    PLAYER_ERROR("invalid threshold value");
    this->SetError(-1);
    return;
  }
}

BlobToDio::~BlobToDio() { }

int BlobToDio::Setup()
{
  this->blobfinder_required_dev = deviceTable->GetDevice(this->blobfinder_required_addr);
  if (!(this->blobfinder_required_dev)) return -1;
  if (this->blobfinder_required_dev->Subscribe(this->InQueue))
  {
    this->blobfinder_required_dev = NULL;
    return -1;
  }
  if (this->use_dio_cmd)
  {
    if (Device::MatchDeviceAddress(this->dio_required_addr, this->dio_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self");
      if (this->blobfinder_required_dev) this->blobfinder_required_dev->Unsubscribe(this->InQueue);
      this->blobfinder_required_dev = NULL;
      return -1;
    }
    this->dio_required_dev = deviceTable->GetDevice(this->dio_required_addr);
    if (!(this->dio_required_dev))
    {
      if (this->blobfinder_required_dev) this->blobfinder_required_dev->Unsubscribe(this->InQueue);
      this->blobfinder_required_dev = NULL;
      return -1;
    }
    if (this->dio_required_dev->Subscribe(this->InQueue))
    {
      this->dio_required_dev = NULL;
      if (this->blobfinder_required_dev) this->blobfinder_required_dev->Unsubscribe(this->InQueue);
      this->blobfinder_required_dev = NULL;
      return -1;
    }
  }
  return 0;
}

int BlobToDio::Shutdown()
{
  if (this->dio_required_dev) this->dio_required_dev->Unsubscribe(this->InQueue);
  this->dio_required_dev = NULL;
  if (this->blobfinder_required_dev) this->blobfinder_required_dev->Unsubscribe(this->InQueue);
  this->blobfinder_required_dev = NULL;
  return 0;
}

int BlobToDio::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_dio_data_t dio_data;
  player_dio_cmd_t dio_cmd;
  player_blobfinder_data_t * blobs;
  int counters[32];
  int i, j;
  uint32_t u, v;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_BLOBFINDER_DATA_BLOBS, this->blobfinder_required_addr))
  {
    assert(data);
    blobs = reinterpret_cast<player_blobfinder_data_t *>(data);
    assert(blobs);
    assert(this->color_count <= 32);
    for (i = 0; i < static_cast<int>(this->color_count); i++) counters[i] = 0;
    for (i = 0; i < static_cast<int>(blobs->blobs_count); i++)
    {
      u = blobs->blobs[i].color;
      for (j = 0; j < static_cast<int>(this->color_count); j++)
      {
        if (((u & 0xff0000) == (this->r[j])) && ((u & 0xff00) == (this->g[j])) && ((u & 0xff) == (this->b[j]))) counters[j]++;
      }
    }
    u = 1; v = 0;
    for (i = 0; i < static_cast<int>(this->color_count); i++)
    {
      if (counters[i] >= threshold) v |= u;
      u <<= 1;
    }
    memset(&dio_data, 0, sizeof dio_data);
    memset(&dio_cmd, 0, sizeof dio_cmd);
    assert(this->color_count > 0);
    dio_data.count = this->color_count;
    dio_data.bits = v;
    dio_cmd.count = this->color_count;
    dio_cmd.digout = v;
    this->Publish(this->dio_provided_addr,
                  PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES,
                  reinterpret_cast<void *>(&dio_data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
    if (this->use_dio_cmd)
    {
      this->dio_required_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, reinterpret_cast<void *>(&dio_cmd), 0, NULL);
    }
    return 0;
  }
  if (this->use_dio_cmd)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES, this->dio_required_addr))
    {
      assert(data);
      return 0;
    }
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * BlobToDio_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new BlobToDio(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void blobtodio_Register(DriverTable * table)
{
  table->AddDriver("blobtodio", BlobToDio_Init);
}
