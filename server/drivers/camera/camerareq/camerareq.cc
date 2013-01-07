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
/////////////////////////////////////////////////////////////////////////////
//
// Desc: Keeps on emitting PLAYER_CAMERA_REQ_GET_IMAGE request with given
//       interval; all received image frames are published on provided camera
//       interface. Typically used with point-and-shoot digicam devices
//       to simulate live image.
// Author: Paul Osmialowski
// Date: 25 Jul 2010
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_camerareq camerareq
 * @brief PLAYER_CAMERA_REQ_GET_IMAGE request emitter

The camerareq driver keeps on emitting PLAYER_CAMERA_REQ_GET_IMAGE request with
given interval; all received image frames are published on provided camera
interface. Typically used with point-and-shoot digicam devices to simulate live
image.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_camera

@par Requires

- @ref interface_camera

@par Configuration requests

- none

@par Configuration file options

- interval (float)
  - Default: 10.0 (seconds)

- sleep_nsec (integer)
  - Default: 100000000 (=100ms which gives max 10 fps)
  - timespec value for nanosleep()

@par Example

@verbatim
driver
(
  name "camerareq"
  requires ["camera:1"]
  provides ["camera:0"]
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <libplayercore/playercore.h>
#if !HAVE_NANOSLEEP
  #include <replace/replace.h>
#endif

class Camerareq : public ThreadedDriver
{
public:
  // Constructor; need that
  Camerareq(ConfigFile * cf, int section);
  virtual ~Camerareq();

  virtual int MainSetup();
  virtual void MainQuit();

  // This method will be invoked on each incoming message
  virtual int ProcessMessage(QueuePointer & resp_queue,
	                     player_msghdr * hdr,
	                     void * data);

private:
  virtual void Main();
  player_devaddr_t p_camera_addr;
  player_devaddr_t r_camera_addr;
  Device * r_camera_dev;
  double interval;
  int sleep_nsec;
};

Camerareq::Camerareq(ConfigFile * cf, int section)
    : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  memset(&(this->p_camera_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->r_camera_addr), 0, sizeof(player_devaddr_t));
  this->r_camera_dev = NULL;
  this->interval = 0.0;
  this->sleep_nsec = 0;
  if (cf->ReadDeviceAddr(&(this->p_camera_addr), section, "provides", PLAYER_CAMERA_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->p_camera_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&this->r_camera_addr, section, "requires", PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  this->interval = cf->ReadFloat(section, "interval", 10.0);
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 100000000);
  if ((this->sleep_nsec) <= 0)
  {
    PLAYER_ERROR("Invalid sleep_nsec value");
    this->SetError(-1);
    return;
  }
}

Camerareq::~Camerareq() { }

int Camerareq::MainSetup()
{
  if (Device::MatchDeviceAddress(this->r_camera_addr, this->p_camera_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return -1;
  }
  this->r_camera_dev = deviceTable->GetDevice(this->r_camera_addr);
  if (!this->r_camera_dev)
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return -1;
  }
  if (this->r_camera_dev->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    this->r_camera_dev = NULL;
    return -1;
  }
  return 0;
}

void Camerareq::MainQuit()
{
  if (this->r_camera_dev) this->r_camera_dev->Unsubscribe(this->InQueue);
  this->r_camera_dev = NULL;
}

Driver * Camerareq_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new Camerareq(cf, section));
}

void Camerareq::Main()
{
  struct timespec ts;
  double last_time = 0.0;
  double t;
  Message * msg = NULL;
  player_camera_data_t * img = NULL;

  for (;;)
  {
    ts.tv_sec = 0;
    ts.tv_nsec = this->sleep_nsec;
    nanosleep(&ts, NULL);
    pthread_testcancel();
    this->ProcessMessages();
    pthread_testcancel();
    GlobalTime->GetTimeDouble(&t);
    if ((t - last_time) > (this->interval))
    {
      msg = this->r_camera_dev->Request(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_CAMERA_REQ_GET_IMAGE, NULL, 0, NULL, true); // threaded = true
      if (!msg)
      {
        PLAYER_WARN("failed to send request");
        pthread_testcancel();
        continue;
      }
      if (!(msg->GetDataSize() > 0))
      {
        PLAYER_ERROR("empty data received");
        delete msg;
        msg = NULL;
        pthread_testcancel();
        continue;
      }
      img = reinterpret_cast<player_camera_data_t *>(msg->GetPayload());
      if (!img)
      {
        PLAYER_WARN("NULL image received");
        delete msg;
        msg = NULL;
        pthread_testcancel();
        continue;
      }
      if (((img->width) > 0) && ((img->height) > 0) && ((img->image_count) > 0) && (img->image))
      {
        this->Publish(this->p_camera_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, reinterpret_cast<void *>(img), 0, &(msg->GetHeader()->timestamp), true); // copy = true
      }
      delete msg;
      msg = NULL;
      img = NULL;
      last_time = t;
    }
    pthread_testcancel();
  }
}

int Camerareq::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_msghdr_t newhdr;
  Message * msg;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, this->r_camera_addr))
  {
    assert(data);
    this->Publish(this->p_camera_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, data, 0, &(hdr->timestamp), true); // copy = true
    return 0;
  } else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->p_camera_addr))
  {
    hdr->addr = this->r_camera_addr;
    msg = this->r_camera_dev->Request(this->InQueue, hdr->type, hdr->subtype, data, 0, NULL, true); // threaded = true
    if (!msg)
    {
      PLAYER_WARN("failed to forward request");
      return -1;
    }
    newhdr = *(msg->GetHeader());
    newhdr.addr = this->p_camera_addr;
    this->Publish(resp_queue, &newhdr, msg->GetPayload(), true); // copy = true, do not dispose published data as we're disposing whole source message in the next line
    delete msg;
    return 0;
  }
  return -1;
}

void camerareq_Register(DriverTable *table)
{
  table->AddDriver("camerareq", Camerareq_Init);
}
