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
// Desc: position2d stall state to dio converter
// Author: Paul Osmialowski
// Date: 07 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_stalltodio stalltodio
 * @brief position2d stall state to dio converter

The stalltodio driver converts position2d stall state to the boolean value.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_dio

@par Requires

- @ref interface_position2d
- optionally @ref interface_dio to send dio commands to

@par Configuration requests

- none

@par Configuration file options

- neg (integer)
  - Default: 0
  - If set to 1, negate stall state before use

@par Example

@verbatim
driver
(
  name "stalltodio"
  provides ["dio:0"]
  requires ["position2d:0" "dio:100"]
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL and size_t
#include <string.h> // for memset()
#include <assert.h>
#include <libplayercore/playercore.h>

class StallToDio : public Driver
{
  public: StallToDio(ConfigFile * cf, int section);
  public: virtual ~StallToDio();
  public: virtual int Setup();
  public: virtual int Shutdown();
  public: virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data);
  private: player_devaddr_t dio_provided_addr;
  private: player_devaddr_t position2d_required_addr;
  private: player_devaddr_t dio_required_addr;
  private: Device * position2d_required_dev;
  private: Device * dio_required_dev;
  private: int use_dio_cmd;
  private: int neg;
};

StallToDio::StallToDio(ConfigFile * cf, int section) : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  memset(&(this->dio_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->position2d_required_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->dio_required_addr), 0, sizeof(player_devaddr_t));
  this->position2d_required_dev = NULL;
  this->dio_required_dev = NULL;
  this->use_dio_cmd = 0;
  this->neg = 0;
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
  if (cf->ReadDeviceAddr(&(this->position2d_required_addr), section, "requires", PLAYER_POSITION2D_CODE, -1, NULL))
  {
    PLAYER_ERROR("cannot require position2d device");
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
  this->neg = cf->ReadInt(section, "neg", 0);
}

StallToDio::~StallToDio() { }

int StallToDio::Setup()
{
  this->position2d_required_dev = deviceTable->GetDevice(this->position2d_required_addr);
  if (!(this->position2d_required_dev)) return -1;
  if (this->position2d_required_dev->Subscribe(this->InQueue))
  {
    this->position2d_required_dev = NULL;
    return -1;
  }
  if (this->use_dio_cmd)
  {
    if (Device::MatchDeviceAddress(this->dio_required_addr, this->dio_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self");
      if (this->position2d_required_dev) this->position2d_required_dev->Unsubscribe(this->InQueue);
      this->position2d_required_dev = NULL;
      return -1;
    }
    this->dio_required_dev = deviceTable->GetDevice(this->dio_required_addr);
    if (!(this->dio_required_dev))
    {
      if (this->position2d_required_dev) this->position2d_required_dev->Unsubscribe(this->InQueue);
      this->position2d_required_dev = NULL;
      return -1;
    }
    if (this->dio_required_dev->Subscribe(this->InQueue))
    {
      this->dio_required_dev = NULL;
      if (this->position2d_required_dev) this->position2d_required_dev->Unsubscribe(this->InQueue);
      this->position2d_required_dev = NULL;
      return -1;
    }
  }
  return 0;
}

int StallToDio::Shutdown()
{
  if (this->dio_required_dev) this->dio_required_dev->Unsubscribe(this->InQueue);
  this->dio_required_dev = NULL;
  if (this->position2d_required_dev) this->position2d_required_dev->Unsubscribe(this->InQueue);
  this->position2d_required_dev = NULL;
  return 0;
}

int StallToDio::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_dio_data_t dio_data;
  player_dio_cmd_t dio_cmd;
  player_position2d_data_t * pos_data;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, this->position2d_required_addr))
  {
    assert(data);
    pos_data = reinterpret_cast<player_position2d_data_t *>(data);
    assert(pos_data);
    memset(&dio_data, 0, sizeof dio_data);
    memset(&dio_cmd, 0, sizeof dio_cmd);
    dio_data.count = 1;
    dio_cmd.count = 1;
    if (this->neg)
    {
      dio_data.bits = pos_data->stall ? 0 : 1;
      dio_cmd.digout = pos_data->stall ? 0 : 1;
    } else
    {
      dio_data.bits = pos_data->stall ? 1 : 0;
      dio_cmd.digout = pos_data->stall ? 1 : 0;
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
Driver * StallToDio_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new StallToDio(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void stalltodio_Register(DriverTable * table)
{
  table->AddDriver("stalltodio", StallToDio_Init);
}
