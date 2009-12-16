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
// Desc: Bumper to dio converter
// Author: Paul Osmialowski
// Date: 07 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_bumpertodio bumpertodio
 * @brief Bumper to dio converter

The bumpertodio driver converts bumper readings to boolean values.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_dio

@par Requires

- @ref interface_bumper
- optionally @ref interface_dio to send dio commands to

@par Configuration requests

- none

@par Configuration file options

- start_idx (integer)
  - Default: 0
  - Index of the first usable element in scans array
- end_idx (integer)
  - Default: -1
  - Index of the last usable element in scans array (-1 = the end of array)
  - end_idx should be greater or equal to start_idx
- bits (integer)
  - Default: 32
  - Number of bits to set (1..32)

@par Example

@verbatim
driver
(
  name "bumpertodio"
  provides ["dio:0"]
  requires ["bumper:0"]
  bits 2
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL and size_t
#include <string.h> // for memset()
#include <assert.h>
#include <libplayercore/playercore.h>

class BumperToDio : public Driver
{
  public: BumperToDio(ConfigFile * cf, int section);
  public: virtual ~BumperToDio();
  public: virtual int Setup();
  public: virtual int Shutdown();
  public: virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data);
  private: player_devaddr_t dio_provided_addr;
  private: player_devaddr_t bumper_required_addr;
  private: player_devaddr_t dio_required_addr;
  private: Device * bumper_required_dev;
  private: Device * dio_required_dev;
  private: int use_dio_cmd;
  private: int start_idx;
  private: int end_idx;
  private: size_t bits;
  private: static int process(uint8_t * bumps, int first, int last);
};

BumperToDio::BumperToDio(ConfigFile * cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  memset(&(this->dio_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->bumper_required_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_required_addr), 0, sizeof(player_devaddr_t));
  this->bumper_required_dev = NULL;
  this->dio_required_dev = NULL;
  this->use_dio_cmd = 0;
  this->start_idx = 0;
  this->end_idx = 0;
  this->bits = 0;
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
  if (cf->ReadDeviceAddr(&(this->bumper_required_addr), section, "requires", PLAYER_BUMPER_CODE, -1, NULL))
  {
    PLAYER_ERROR("cannot require bumper device");
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
  this->start_idx = cf->ReadInt(section, "start_idx", 0);
  if ((this->start_idx) < 0)
  {
    PLAYER_ERROR("invalid start_idx value");
    this->SetError(-1);
    return;
  }
  this->end_idx = cf->ReadInt(section, "end_idx", -1);
  if (!(((this->end_idx) == -1) || (this->end_idx >= this->start_idx)))
  {
    PLAYER_ERROR("invalid end_idx value");
    this->SetError(-1);
    return;
  }
  this->bits = static_cast<size_t>(cf->ReadInt(section, "bits", 32));
  if (((this->bits) < 1) || ((this->bits) > 32))
  {
    PLAYER_ERROR("invalid number of bits");
    this->SetError(-1);
    return;
  }
}

BumperToDio::~BumperToDio() { }

int BumperToDio::Setup()
{
  this->bumper_required_dev = deviceTable->GetDevice(this->bumper_required_addr);
  if (!(this->bumper_required_dev)) return -1;
  if (this->bumper_required_dev->Subscribe(this->InQueue))
  {
    this->bumper_required_dev = NULL;
    return -1;
  }
  if (this->use_dio_cmd)
  {
    if (Device::MatchDeviceAddress(this->dio_required_addr, this->dio_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self");
      if (this->bumper_required_dev) this->bumper_required_dev->Unsubscribe(this->InQueue);
      this->bumper_required_dev = NULL;
      return -1;
    }
    this->dio_required_dev = deviceTable->GetDevice(this->dio_required_addr);
    if (!(this->dio_required_dev))
    {
      if (this->bumper_required_dev) this->bumper_required_dev->Unsubscribe(this->InQueue);
      this->bumper_required_dev = NULL;
      return -1;
    }
    if (this->dio_required_dev->Subscribe(this->InQueue))
    {
      this->dio_required_dev = NULL;
      if (this->bumper_required_dev) this->bumper_required_dev->Unsubscribe(this->InQueue);
      this->bumper_required_dev = NULL;
      return -1;
    }
  }
  return 0;
}

int BumperToDio::Shutdown()
{
  if (this->dio_required_dev) this->dio_required_dev->Unsubscribe(this->InQueue);
  this->dio_required_dev = NULL;
  if (this->bumper_required_dev) this->bumper_required_dev->Unsubscribe(this->InQueue);
  this->bumper_required_dev = NULL;
  return 0;
}

int BumperToDio::process(uint8_t * bumps, int first, int last)
{
  int count = 0;
  int num = (last - first) + 1;
  int i;

  assert(last >= first);
  assert(num > 0);
  for (i = first; i <= last; i++)
  {
    if (bumps[i]) count++;
  }
  assert(count <= num);
  return !((static_cast<double>(count) - (static_cast<double>(num) / 2.0)) < 0.0);
}

int BumperToDio::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_dio_data_t dio_data;
  player_dio_cmd_t dio_cmd;
  player_bumper_data_t * bumps;
  double x, d;
  int size;
  int i;
  int first, last, endidx;
  uint32_t u;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_BUMPER_DATA_STATE, this->bumper_required_addr))
  {
    assert(data);
    bumps = reinterpret_cast<player_bumper_data_t *>(data);
    assert(bumps);
    endidx = -1;
    if ((this->end_idx) == -1)
    {
      if ((static_cast<int>(bumps->bumpers_count)) <= (this->start_idx))
      {
        PLAYER_WARN("Not enough data");
        return -1;
      }
      endidx = static_cast<int>(bumps->bumpers_count) - 1;
    } else
    {
      if ((static_cast<int>(bumps->bumpers_count)) <= (this->end_idx))
      {
        PLAYER_WARN("Not enough data");
        return -1;
      }
      endidx = this->end_idx;
    }
    assert(endidx >= 0);
    if (endidx < (this->start_idx))
    {
      PLAYER_WARN("Wrong indices");
      return -1;
    }
    size = (endidx - this->start_idx) + 1;
    assert(size > 0);
    memset(&dio_data, 0, sizeof dio_data);
    memset(&dio_cmd, 0, sizeof dio_cmd);
    assert(this->bits > 0);
    dio_data.count = this->bits;
    dio_data.bits = 0;
    dio_cmd.count = this->bits;
    dio_cmd.digout = 0;
    x = (static_cast<double>(size)) / (static_cast<double>(this->bits));
    d = 0.0;
    u = 1;
    for (i = 0; i < static_cast<int>(this->bits); i++)
    {
      first = static_cast<int>(d);
      d += x;
      last = static_cast<int>(d);
      if (d > (static_cast<double>(last))) last++;
      last--;
      if (last == size)
      {
        assert((i + 1) == static_cast<int>(this->bits));
        last--;
      }
      assert((first >= 0) && (first < size));
      assert((last >= 0) && (last < size));
      assert(last >= first);
      assert((this->start_idx + first) < (static_cast<int>(bumps->bumpers_count)));
      assert((this->start_idx + last) < (static_cast<int>(bumps->bumpers_count)));
      if (BumperToDio::process(bumps->bumpers, this->start_idx + first, this->start_idx + last))
      {
        dio_data.bits |= u;
        dio_cmd.digout |= u;
      }
      u <<= 1;
    }
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
Driver * BumperToDio_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new BumperToDio(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void bumpertodio_Register(DriverTable * table)
{
  table->AddDriver("bumpertodio", BumperToDio_Init);
}
