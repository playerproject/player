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
 * Desc: Driver for writing log files.
 * Author: Andrew Howard
 * Date: 14 Jun 2003
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
#include "devicetable.h"
#include "drivertable.h"
#include "deviceregistry.h"

#include "writelog_manager.h"


// The logfile driver
class WriteLog: public CDevice 
{
  // Constructor
  public: WriteLog(int code, ConfigFile* cf, int section);

  // Destructor
  public: ~WriteLog();

  // Initialize the driver
  public: virtual int Setup();

  // Finalize the driver
  public: virtual int Shutdown();

  // Process data requests
  public: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  // Process configuration requests
  public: virtual int PutConfig(player_device_id_t* device, void* client, 
                                void* data, size_t len);

  // Our manager
  private: WriteLogManager *manager;

  // Id if the device whose data we are writing
  private: player_device_id_t write_id;

  // The device whose data we are writing
  private: CDevice *device;
};



////////////////////////////////////////////////////////////////////////////
// Create a driver for reading log files
CDevice* ReadWriteLog_Init(char* name, ConfigFile* cf, int section)
{
  player_interface_t interface;

  if (lookup_interface(name, &interface) != 0)
  {
    PLAYER_ERROR1("interface \"%s\" is not supported", name);
    return NULL;
  }
  if (WriteLogManager_Get() == NULL)
  {
    PLAYER_ERROR("no log file specified; did you forget to use -r <filename>?");
    return NULL;
  }
  return ((CDevice*) (new WriteLog(interface.code, cf, section)));
}


////////////////////////////////////////////////////////////////////////////
// Device factory registration
void WriteLog_Register(DriverTable* table)
{
  table->AddDriver("writelogfile", PLAYER_READ_MODE, ReadWriteLog_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
WriteLog::WriteLog(int code, ConfigFile* cf, int section)
    : CDevice(PLAYER_MAX_PAYLOAD_SIZE, PLAYER_MAX_PAYLOAD_SIZE, 1, 1)
{
  // Get our manager
  this->manager = WriteLogManager_Get();
  assert(this->manager != NULL);
  
  // Get the part of the log file we wish to read
  this->write_id.code = code;
  this->write_id.index = cf->ReadInt(section, "index", 0);

  this->device = NULL;
    
  return;
}


////////////////////////////////////////////////////////////////////////////
// Destructor
WriteLog::~WriteLog()
{
  return;
}


////////////////////////////////////////////////////////////////////////////
// Initialize driver
int WriteLog::Setup()
{
  // Subscribe to the underlying writer
  if (this->manager->Subscribe(this->write_id, this) != 0)
    return -1;

  // Subscribe to the underlying device
  this->device = deviceTable->GetDevice(this->write_id);
  if (!this->device)
  {
    PLAYER_ERROR("unable to locate underlying device");
    return -1;
  }
  if (this->device->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to underlying device");
    return -1;
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
int WriteLog::Shutdown()
{
  // Unsbscribe from the underlying device
  this->device->Unsubscribe(this);
  this->device = NULL;
  
  // Unsubscribe from the underlying reader
  this->manager->Unsubscribe(this->write_id, this);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Process get data requests by seeing if the underlying device has
// any new data.
size_t WriteLog::GetData(void* client, unsigned char* dest, size_t len,
                         uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  size_t size;

  // Read data from underlying device
  size = this->device->GetData(client, dest, len, timestamp_sec, timestamp_usec);

  // Write data to file
  this->manager->Write(dest, size, &this->write_id, *timestamp_sec, *timestamp_usec);

  return size;
}


////////////////////////////////////////////////////////////////////////////
// Process configuration requests
int WriteLog::PutConfig(player_device_id_t* device, void* client,
                       void* data, size_t len)
{
  if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
    PLAYER_ERROR("PutReply() failed");

  return 0;
}


