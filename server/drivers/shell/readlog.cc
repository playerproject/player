/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
 * Desc: Driver for reading log files.
 * Author: Andrew Howard
 * Date: 17 May 2003
 * CVS: $Id$
 *
 * The writelog driver will write data from another device to a log file.
 * The readlog driver will replay the data as if it can from the real sensors.
 */
/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_readlog Readlog driver

The readlog driver can be used to ``replay'' data stored in a log
file.  This is particularly useful for debugging client programs,
since users may run their clients against the same data set over and
over again.  Suitable log files can be generated using the @ref
player_driver_writelog driver.

To make use of log file data, Player must be started in a special
mode:

@verbatim
  $ player -r <logfile> <configfile>
@endverbatim

The '-r' switch instructs Player to load the given log file, and
replay the data according the configuration specified in the
configuration file.  See the below for an example configuration file.



@par Interfaces

- @ref player_interface_log
- @ref player_interface_camera
- @ref player_interface_gps
- @ref player_interface_laser
- @ref player_interface_position
- @ref player_interface_position3d
- @ref player_interface_wifi


@par Supported configuration requests

- TODO

@par Configuration file options

- index 0
  - If there is more than one entry in the log file for a given
    interface type (e.g., two different position devices have recorded data),
    the index option can be used to specify which of these will be read.

      
@par Example 

@verbatim
# Use this device to get laser data from the log
driver
(
  name "readlog"
  devices ["laser:0"]
)

# Use this device to get position data from the log
driver
(
  name "readlog"
  devices ["position:0"]
)

# Use this device to control the replay through the log interface.
driver
(
  name "readlog"
  devices ["log:0"]
  index 0
  enable 1
  alwayson 1
)
@endverbatim
*/
/** @} */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "player.h"
#include "driver.h"
#include "drivertable.h"
#include "deviceregistry.h"

#include "readlog_manager.h"


// The logfile driver
class ReadLog: public Driver 
{
  // Constructor
  public: ReadLog(ConfigFile* cf, int section);

  // Destructor
  public: ~ReadLog();

  // Initialize the driver
  public: virtual int Setup();

  // Finalize the driver
  public: virtual int Shutdown();

  // Process configuration requests
  public: virtual int PutConfig(player_device_id_t id, void *client, 
                                void* src, size_t len,
                                struct timeval* timestamp);

  // Our manager
  private: ReadLogManager *manager;

  // Our local device id
  private: player_device_id_t local_id;
  
  // The part of the log file to read
  private: player_device_id_t read_id;
};



////////////////////////////////////////////////////////////////////////////
// Create a driver for reading log files
Driver* ReadReadLog_Init(ConfigFile* cf, int section)
{
  if (ReadLogManager_Get() == NULL)
  {
    PLAYER_ERROR("no log file specified; did you forget to use -r <filename>?");
    return NULL;
  }

  return ((Driver*) (new ReadLog(cf, section)));
}


////////////////////////////////////////////////////////////////////////////
// Device factory registration
void ReadLog_Register(DriverTable* table)
{
  table->AddDriver("readlog", ReadReadLog_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
ReadLog::ReadLog(ConfigFile* cf, int section)
    : Driver(cf, section, -1, PLAYER_ALL_MODE,
             PLAYER_MAX_PAYLOAD_SIZE, PLAYER_MAX_PAYLOAD_SIZE, 1, 1)
{
  this->local_id = this->device_id;

  // Get our manager
  this->manager = ReadLogManager_Get();
  assert(this->manager != NULL);
  
  // Get the part of the log file we wish to read
  this->read_id.code = this->local_id.code;
  this->read_id.index = cf->ReadInt(section, "index", 0);

  if(this->local_id.code == PLAYER_LOG_CODE)
  {
    if(cf->ReadInt(section, "enable", 1) > 0)
      this->manager->enable = true;
    else
      this->manager->enable = false;

    if(cf->ReadInt(section, "autorewind", 0) > 0)
      this->manager->autorewind = true;
    else
      this->manager->autorewind = false;
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Destructor
ReadLog::~ReadLog()
{
  return;
}


////////////////////////////////////////////////////////////////////////////
// Initialize driver
int ReadLog::Setup()
{
  // Subscribe to the underlying reader, unless we're the 'log' device,
  // which doesn't produce any data, but rather allows the client to
  // start/stop data playback.
  if(this->local_id.code != PLAYER_LOG_CODE)
  {
    if(this->manager->Subscribe(this->read_id, this) != 0)
      return -1;
  }

  // Clear the data buffer
  // TODO: reinstate? this->PutData(NULL, 0, NULL);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
int ReadLog::Shutdown()
{
  // Unsubscribe from the underlying reader
  this->manager->Unsubscribe(this->read_id, this);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Process configuration requests
int ReadLog::PutConfig(player_device_id_t id, void *client, 
                       void* src, size_t len,
                       struct timeval* timestamp)
{
  player_log_set_read_rewind_t rreq;
  player_log_set_read_state_t sreq;
  player_log_get_state_t greq;
  uint8_t subtype;

  if(id.code != PLAYER_LOG_CODE)
  {
    if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
    return(0);
  }

  if(len < sizeof(sreq.subtype))
  {
    PLAYER_WARN2("request was too small (%d < %d)",
                  len, sizeof(sreq.subtype));
    if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  subtype = ((player_log_set_read_state_t*)src)->subtype;
  switch(subtype)
  {
    case PLAYER_LOG_SET_READ_STATE_REQ:
      if(len != sizeof(sreq))
      {
        PLAYER_WARN2("request wrong size (%d != %d)", len, sizeof(sreq));
        if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      sreq = *((player_log_set_read_state_t*)src);
      if(sreq.state)
      {
        puts("ReadLog: start playback");
        this->manager->enable = true;
      }
      else
      {
        puts("ReadLog: stop playback");
        this->manager->enable = false;
      }
      if (this->PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    case PLAYER_LOG_GET_STATE_REQ:
      if(len != sizeof(greq.subtype))
      {
        PLAYER_WARN2("request wrong size (%d != %d)", 
                     len, sizeof(greq.subtype));
        if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      greq = *((player_log_get_state_t*)src);
      greq.type = PLAYER_LOG_TYPE_READ;
      if(this->manager->enable)
        greq.state = 1;
      else
        greq.state = 0;

      if(this->PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
                        &greq, sizeof(greq),NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    case PLAYER_LOG_SET_READ_REWIND_REQ:
      if(len != sizeof(rreq.subtype))
      {
        PLAYER_WARN2("request wrong size (%d != %d)", 
                     len, sizeof(rreq.subtype));
        if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }

      // set the appropriate flag in the manager
      this->manager->rewind_requested = true;

      if(this->PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;

    default:
      PLAYER_WARN1("got request of unknown subtype %u", subtype);
      if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
  }

  return 0;
}


