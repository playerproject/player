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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "player.h"
#include "device.h"
#include "drivertable.h"
#include "deviceregistry.h"

#include "readlog_manager.h"


// The logfile driver
class ReadLog: public CDevice 
{
  // Constructor
  public: ReadLog(int code, ConfigFile* cf, int section);

  // Destructor
  public: ~ReadLog();

  // Initialize the driver
  public: virtual int Setup();

  // Finalize the driver
  public: virtual int Shutdown();

  // Process configuration requests
  public: virtual int PutConfig(player_device_id_t* device, void* client, 
                                void* data, size_t len);

  // Our manager
  private: ReadLogManager *manager;
  
  // The part of the log file to read
  private: player_device_id_t read_id;
};



////////////////////////////////////////////////////////////////////////////
// Create a driver for reading log files
CDevice* ReadReadLog_Init(char* name, ConfigFile* cf, int section)
{
  player_interface_t interface;

  if (lookup_interface(name, &interface) != 0)
  {
    PLAYER_ERROR1("interface \"%s\" is not supported", name);
    return NULL;
  }
  if (ReadLogManager_Get() == NULL)
  {
    PLAYER_ERROR("no log file specified; did you forget to use -r <filename>?");
    return NULL;
  }
  return ((CDevice*) (new ReadLog(interface.code, cf, section)));
}


////////////////////////////////////////////////////////////////////////////
// Device factory registration
void ReadLog_Register(DriverTable* table)
{
  table->AddDriver("readlog", PLAYER_READ_MODE, ReadReadLog_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
ReadLog::ReadLog(int code, ConfigFile* cf, int section)
    : CDevice(PLAYER_MAX_PAYLOAD_SIZE, PLAYER_MAX_PAYLOAD_SIZE, 1, 1)
{
  // Get our manager
  this->manager = ReadLogManager_Get();
  assert(this->manager != NULL);
  
  // Get the part of the log file we wish to read
  this->read_id.code = code;
  this->read_id.index = cf->ReadInt(section, "index", 0);

  if(code == PLAYER_LOG_CODE)
  {
    if(cf->ReadInt(section, "enable", 1) > 0)
      this->manager->enable = true;
    else
      this->manager->enable = false;
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
  if(this->device_id.code != PLAYER_LOG_CODE)
  {
    if(this->manager->Subscribe(this->read_id, this) != 0)
      return -1;
  }

  // Clear the data buffer
  this->PutData(NULL, 0, 0, 0);
    
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
int ReadLog::PutConfig(player_device_id_t* device, void* client,
                       void* data, size_t len)
{
  player_log_set_read_state_t sreq;
  player_log_get_state_t greq;
  uint8_t subtype;

  if(device->code != PLAYER_LOG_CODE)
  {
    if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return(0);
  }

  if(len < sizeof(sreq.subtype))
  {
    PLAYER_WARN2("request was too small (%d < %d)",
                  len, sizeof(sreq.subtype));
    if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  subtype = ((player_log_set_read_state_t*)data)->subtype;
  switch(subtype)
  {
    case PLAYER_LOG_SET_READ_STATE_REQ:
      if(len != sizeof(sreq))
      {
        PLAYER_WARN2("request wrong size (%d != %d)", len, sizeof(sreq));
        if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      sreq = *((player_log_set_read_state_t*)data);
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
      if (this->PutReply(client, PLAYER_MSGTYPE_RESP_ACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    case PLAYER_LOG_GET_STATE_REQ:
      if(len != sizeof(greq.subtype))
      {
        PLAYER_WARN2("request wrong size (%d != %d)", 
                     len, sizeof(greq.subtype));
        if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      greq = *((player_log_get_state_t*)data);
      greq.type = PLAYER_LOG_TYPE_READ;
      if(this->manager->enable)
        greq.state = 1;
      else
        greq.state = 0;

      if(this->PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL,
                        &greq, sizeof(greq)) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    default:
      PLAYER_WARN1("got request of unknown subtype %u", subtype);
      if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
  }

  return 0;
}


