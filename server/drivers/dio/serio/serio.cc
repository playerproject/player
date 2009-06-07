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
// Desc: Low-level access to the serial port control lines
// Author: Paul Osmialowski
// Date: 22 Aug 2008
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_serio serio
 * @brief low-level access to the serial port control lines

The serio driver provides low-level access to the serial port control lines.

The digout bitfield of dio interface command structure needs lowest two bits
to be filled:
0 - new RTS line state (output line)
1 - new DTR line state (output line)

This driver provides data by filling bits bitfield of dio interface data
structure.
Lowest six bits are filled by the driver:
0 - current RTS line state (output line)
1 - current DTR line state (output line)
2 - currend DCD line state (input line)
3 - current CTS line state (input line)
4 - current DSR line state (input line)
5 - current RI  line state (input line)

@par Compile-time dependencies

- none

@par Provides

- @ref interface_dio

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- port (integer)
  - Default: /dev/ttyS0

- sleep_nsec (integer)
  - Default: 10000000 (=10ms which gives max 100 main loop turns per second)
  - timespec value for nanosleep()

@par Example

@verbatim
driver
(
  name "serio"
  provides ["dio:0"]
  port "/dev/ttyPZ1"
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL
#include <time.h> // for nanosleep(), struct timespec
#include <assert.h>
#include <stdio.h> // for snprintf()
#include <string.h> // for strlen()
#include <fcntl.h> // for open(), O_RDWR
#include <sys/ioctl.h> // for ioctl()
#include <termios.h> // for TIOC* definitions
#include <unistd.h> // for close()
#include <pthread.h>
#include <libplayercore/playercore.h>

class SerIO : public ThreadedDriver
{
  public: SerIO(ConfigFile * cf, int section);
  public: virtual ~SerIO();

  public: virtual int MainSetup();
  public: virtual void MainQuit();

  // This method will be invoked on each incoming message
  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
			             void * data);

  private: virtual void Main();

  private: char port[256];
  private: int fd;
  private: int sleep_nsec;
};

Driver * SerIO_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new SerIO(cf, section));
}

void serio_Register(DriverTable * table)
{
  table->AddDriver("serio", SerIO_Init);
}

SerIO::SerIO(ConfigFile * cf, int section)
  : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_DIO_CODE)
{
  const char * str;

  this->fd = -1;
  str = cf->ReadString(section, "port", "/dev/ttyS0");
  assert(str);
  if (static_cast<int>(strlen(str)) <= 0)
  {
    PLAYER_ERROR("Wrong device port name");
    this->SetError(-1);
    return;
  }
  snprintf(this->port, sizeof this->port, "%s", str);
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 10000000);
}

SerIO::~SerIO()
{
  if ((this->fd) >= 0) close(this->fd);
}

int SerIO::MainSetup()
{
  this->fd = open(this->port, O_RDWR);
  if ((this->fd) < 0)
  {
    PLAYER_ERROR1("Cannot open %d", this->port);
    return -1;
  }
  return 0;
}

void SerIO::MainQuit()
{
  this->StopThread();
  if ((this->fd) >= 0) close(this->fd);
  this->fd = -1;
}

void SerIO::Main()
{
  struct timespec tspec;
  player_dio_data_t data;
  int mcs;

  for (;;)
  {
    // Go to sleep for a while (this is a polling loop).
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec;
    nanosleep(&tspec, NULL);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    this->ProcessMessages();
    pthread_testcancel();

    data.count = 6;
    data.bits = 0;
    if (ioctl(this->fd, TIOCMGET, &mcs) == -1)
    {
      PLAYER_WARN("Cannot get serial port status");
      continue;
    }
    if (mcs & TIOCM_RTS) data.bits |=  1;
    if (mcs & TIOCM_DTR) data.bits |=  2;
    if (mcs & TIOCM_CAR) data.bits |=  4;
    if (mcs & TIOCM_CTS) data.bits |=  8;
    if (mcs & TIOCM_DSR) data.bits |= 16;
    if (mcs & TIOCM_RNG) data.bits |= 32;
    this->Publish(this->device_addr, PLAYER_MSGTYPE_DATA, PLAYER_DIO_DATA_VALUES, reinterpret_cast<void *>(&data), 0, NULL);
  }
}

int SerIO::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_dio_cmd_t cmd;
  int mcs, change;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES, this->device_addr))
  {
    assert(data);
    cmd = *(reinterpret_cast<player_dio_cmd_t *>(data));
    if (cmd.count < 1)
    {
      PLAYER_WARN("Invalid command received");
      return -1;
    }
    change = 0;
    if (ioctl(this->fd, TIOCMGET, &mcs) == -1)
    {
      PLAYER_WARN("Cannot get serial port status");
      return -1;
    }
    if (cmd.digout & 1)
    {
      if (!(mcs & TIOCM_RTS))
      {
        mcs |= TIOCM_RTS;
        change = !0;
      }
    } else
    {
      if (mcs & TIOCM_RTS)
      {
        mcs &= (~TIOCM_RTS);
        change = !0;
      }
    }
    if (cmd.count > 1)
    {
      if (cmd.digout & 2)
      {
        if (!(mcs & TIOCM_DTR))
        {
          mcs |= TIOCM_DTR;
          change = !0;
        }
      } else
      {
        if (mcs & TIOCM_DTR)
        {
          mcs &= (~TIOCM_DTR);
          change = !0;
        }
      }
    }
    if (change)
    {
      if (ioctl(this->fd, TIOCMSET, &mcs))
      {
        PLAYER_WARN("Cannot set new serial port status");
        return -1;
      }
    }
    return 0;
  }
  return -1;
}


