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
// Desc: Low-level access to the hardware I/O ports
// Author: Paul Osmialowski
// Date: 04 Sep 2009
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_portio portio
 * @brief low-level access to the hardware I/O ports

The portio driver provides low-level access to the hardware I/O ports.

The digout bitfield of dio interface command structure needs lowest eight bits
to be filled.

This driver provides data by filling lowest eight bit of dio interface bitfield.

@par Compile-time dependencies

- sys/io.h or hw/inout.h
- System Administrator rights

@par Provides

- @ref interface_dio

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- port (integer)
  - Default: 888 (=378h - LPT1: 8-bit data output)

- sleep_nsec (integer)
  - Default: 10000000 (=10ms which gives max 100 main loop turns per second)
  - timespec value for nanosleep()

@par Example

@verbatim
driver
(
  name "portio"
  provides ["dio:0"]
  port 890
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL
#include <time.h> // for nanosleep(), struct timespec
#include <string.h> // for memset()
#include <assert.h>
#include <pthread.h>
#ifdef __QNXNTO__
#include <hw/inout.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <stdint.h>
#else
#include <sys/io.h>
#endif
#include <libplayercore/playercore.h>

class PortIO : public ThreadedDriver
{
  public: PortIO(ConfigFile * cf, int section);
  public: virtual ~PortIO();
  public: virtual void MainQuit();

  // This method will be invoked on each incoming message
  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
			             void * data);

  private: virtual void Main();

  private: player_devaddr_t dio_provided_addr;
  private: unsigned short port;
  private: int sleep_nsec;
  private: unsigned char init_val;
#ifdef __QNXNTO__
  private: uintptr_t portptr;
#endif
};

Driver * PortIO_Init(ConfigFile * cf, int section)
{	
  return reinterpret_cast<Driver *>(new PortIO(cf, section));
}

void portio_Register(DriverTable * table)
{
  table->AddDriver("portio", PortIO_Init);
}

PortIO::PortIO(ConfigFile * cf, int section)
  : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  memset(&(this->dio_provided_addr), 0, sizeof(player_devaddr_t));
  if (cf->ReadDeviceAddr(&(this->dio_provided_addr), section, "provides",
                         PLAYER_DIO_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->dio_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  this->port = static_cast<unsigned short>(cf->ReadInt(section, "port", 888));
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 10000000);
#ifdef __QNXNTO__
  if (ThreadCtl(_NTO_TCTL_IO, NULL) == -1)
  {
    PLAYER_ERROR("Only root can do that");
    this->SetError(-1);
    return;
  }
  this->portptr = mmap_device_io(1, this->port);
  if ((this->portptr) == reinterpret_cast<uintptr_t>(MAP_FAILED))
  {
    PLAYER_ERROR("mmap_device_io() failed");
    this->SetError(-1);
    return;    
  }
#else
  if (iopl(3) == -1)
  {
    PLAYER_ERROR("Only root can do that");
    this->SetError(-1);
    return;
  }
#endif
}

PortIO::~PortIO()
{
#ifdef __QNXNTO__
  munmap_device_io(this->portptr, 1);
#endif
}

void PortIO::MainQuit()
{
#ifdef __QNXNTO__
  out8(this->portptr, this->init_val);
#else
  outb_p(this->init_val, this->port);
#endif
}

void PortIO::Main()
{
  struct timespec tspec;
  player_dio_data_t data;

#ifdef __QNXNTO__
  this->init_val = in8(this->portptr);
#else
  this->init_val = inb_p(this->port);
#endif
  for (;;)
  {
    pthread_testcancel();
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec;
    nanosleep(&tspec, NULL);

    pthread_testcancel();
    this->ProcessMessages();
    pthread_testcancel();

    data.count = 8;
#ifdef __QNXNTO__
    data.bits = in8(this->portptr);
#else
    data.bits = inb_p(this->port);
#endif
    this->Publish(this->dio_provided_addr, PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES, reinterpret_cast<void *>(&data), 0, NULL);
  }
}

int PortIO::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_dio_cmd_t cmd;
  unsigned char u, v;
  const unsigned char masks[] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
  size_t digits;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, this->dio_provided_addr))
  {
    assert(data);
    cmd = *(reinterpret_cast<player_dio_cmd_t *>(data));
    digits = cmd.count;
    if (digits > 8) digits = 8;
    if (digits < 1)
    {
      PLAYER_WARN("Invalid command received");
      return -1;
    }
#ifdef __QNXNTO__
    u = in8(this->portptr);
#else
    u = inb_p(this->port);
#endif
    v = (u & (~(masks[digits - 1]))) | (cmd.digout & (masks[digits - 1]));
    if (u != v)
    {
#ifdef __QNXNTO__
      out8(this->portptr, v);
#else
      outb_p(v, this->port);
#endif
    }
    return 0;
  }
  return -1;
}
