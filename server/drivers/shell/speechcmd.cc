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
// Desc: Speech commands sender
// Author: Paul Osmialowski
// Date: 27 Feb 2010
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_speechcmd speechcmd
 * @brief Speech commands sender

The speechcmd driver keeps on repeating configured speech command.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_opaque

@par Requires

- @ref interface_speech

@par Configuration requests

- none

@par Configuration file options

- sleep_sec (integer)
  - Default: 0
  - timespec value for nanosleep() (sec resolution).
- sleep_nsec (integer)
  - Default: 100000000 (however, when sleep_sec is greater than 0, default sleep_nsec is 0)
  - timespec value for nanosleep() (nanosec resolution).
- message (string)
  - Default: "foo"
  - Message to be spoken.

@par Example

@verbatim
driver
(
  name "speechcmd"
  provides ["opaque:100"]
  requires ["speech:0"]
  message "hello world"
  alwayson 1
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h> // for NULL and size_t
#include <stdlib.h> // for malloc() and free()
#include <string.h> // for memset()
#include <stdio.h>  // for snprintf()
#include <pthread.h>
#include <libplayercore/playercore.h>

#define MAX_MSG_LEN 255

class SpeechCmd : public ThreadedDriver
{
  public:
    // Constructor; need that
    SpeechCmd(ConfigFile * cf, int section);

    virtual ~SpeechCmd();

    // Must implement the following methods.
    virtual int MainSetup();
    virtual void MainQuit();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);

  private:
    virtual void Main();
    player_devaddr_t provided_opaque_addr;
    player_devaddr_t required_speech_addr;
    Device * required_speech_dev;

    int sleep_sec;
    int sleep_nsec;
    char message[MAX_MSG_LEN + 1];
};

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
//
SpeechCmd::SpeechCmd(ConfigFile * cf, int section) : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  int default_nsec;
  const char * str;

  memset(&(this->provided_opaque_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->required_speech_addr), 0, sizeof(player_devaddr_t));
  this->required_speech_dev = NULL;
  this->sleep_sec = 0;
  this->sleep_nsec = 0;
  this->message[0] = '\0';
  if (cf->ReadDeviceAddr(&(this->provided_opaque_addr), section, "provides",
                         PLAYER_OPAQUE_CODE, -1, NULL))
  {
    PLAYER_ERROR("Nothing is provided");
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->provided_opaque_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->required_speech_addr), section, "requires",
                         PLAYER_SPEECH_CODE, -1, NULL))
  {
    PLAYER_ERROR("Cannot require speech device");
    this->SetError(-1);
    return;
  }
  this->sleep_sec = cf->ReadInt(section, "sleep_sec", 0);
  if ((this->sleep_sec) < 0)
  {
    PLAYER_ERROR("Invalid sleep_sec value");
    this->SetError(-1);
    return;
  }
  default_nsec = 100000000;
  if ((this->sleep_sec) > 0) default_nsec = 0;
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", default_nsec);
  if ((this->sleep_nsec) < 0)
  {
    PLAYER_ERROR("Invalid sleep_nsec value");
    this->SetError(-1);
    return;
  }
  str = cf->ReadString(section, "message", "foo");
  if (!str)
  {
    PLAYER_ERROR("NULL message");
    this->SetError(-1);
    return;
  }
  if (!(strlen(str) > 0))
  {
    PLAYER_ERROR("Message too short");
    this->SetError(-1);
    return;
  }
  if (strlen(str) >= (sizeof this->message))
  {
    PLAYER_ERROR("Message too long");
    this->SetError(-1);
    return;
  }
#if defined (WIN32)
  _snprintf(this->message, sizeof this->message, "%s", str);
#else
  snprintf(this->message, sizeof this->message, "%s", str);
#endif
}

SpeechCmd::~SpeechCmd() { }

int SpeechCmd::MainSetup()
{
  this->required_speech_dev = deviceTable->GetDevice(this->required_speech_addr);
  if (!(this->required_speech_dev))
  {
    PLAYER_ERROR("unable to locate suitable speech device");
    return -1;
  }
  if (this->required_speech_dev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to speech device");
    this->required_speech_dev = NULL;
    return -1;
  }
  return 0;
}

void SpeechCmd::MainQuit()
{
  if (this->required_speech_dev) this->required_speech_dev->Unsubscribe(this->InQueue);
  this->required_speech_dev = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void SpeechCmd::Main()
{
  struct timespec tspec;
  player_opaque_data_t data;
  player_speech_cmd_t cmd;

  cmd.string_count = strlen(this->message) + 1;
  cmd.string = this->message;
  for (;;)
  {
    // Test if we are supposed to cancel
    pthread_testcancel();

    // Process incoming messages
    this->ProcessMessages();

    // Test if we are supposed to cancel
    pthread_testcancel();

    this->required_speech_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_CMD, PLAYER_SPEECH_CMD_SAY, reinterpret_cast<void *>(&cmd), 0, NULL);

    // Test if we are supposed to cancel
    pthread_testcancel();

    memset(&data, 0, sizeof data);
    data.data_count = 0;
    data.data = NULL;
    this->Publish(this->provided_opaque_addr,
                  PLAYER_MSGTYPE_DATA, PLAYER_OPAQUE_DATA_STATE,
                  reinterpret_cast<void *>(&data), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
    // Test if we are supposed to cancel
    pthread_testcancel();

    if (((this->sleep_sec) > 0) || ((this->sleep_nsec) > 0))
    {
      // sleep for a while
      tspec.tv_sec = this->sleep_sec;
      tspec.tv_nsec = this->sleep_nsec;
      nanosleep(&tspec, NULL);
    }
  }
}

int SpeechCmd::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * SpeechCmd_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new SpeechCmd(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void speechcmd_Register(DriverTable * table)
{
  table->AddDriver("speechcmd", SpeechCmd_Init);
}
