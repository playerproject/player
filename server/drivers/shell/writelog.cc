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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"
#include "deviceregistry.h"
#include "playertime.h"


// Utility class for storing per-device info
class WriteLogDevice
{
  public: player_device_id_t id;
  public: CDevice *device;
  public: uint32_t tsec, tusec;
};


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

  // Process configuration requests
  public: virtual int PutConfig(player_device_id_t* device, void* client, 
                                void* data, size_t len);

  // Device thread
  private: virtual void Main(void);

  // Write data to file
  public: void Write(void *data, size_t size,
                     const player_device_id_t *id, uint32_t sec, uint32_t usec);

  // Write position data to file
  private: void WritePosition(player_position_data_t *data, size_t size);

  // Write laser data to file
  private: void WriteLaser(player_laser_data_t *data, size_t size);

  // File to read data from
  private: const char *filename;
  private: FILE *file;

  // Subscribed device list
  private: int device_count;
  private: WriteLogDevice devices[1024];
};


////////////////////////////////////////////////////////////////////////////
// Pointer to the one-and-only time
extern PlayerTime* GlobalTime;


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
  return ((CDevice*) (new WriteLog(interface.code, cf, section)));
}


////////////////////////////////////////////////////////////////////////////
// Device factory registration
void WriteLog_Register(DriverTable* table)
{
  table->AddDriver("writelog", PLAYER_READ_MODE, ReadWriteLog_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
WriteLog::WriteLog(int code, ConfigFile* cf, int section)
    : CDevice(PLAYER_MAX_PAYLOAD_SIZE, PLAYER_MAX_PAYLOAD_SIZE, 1, 1)
{
  int i, index;
  const char *desc;
  char name[64];
  player_interface_t iface;
  WriteLogDevice *device;
  
  this->file = NULL;
  this->filename = cf->ReadString(section, "filename", "writelog.log");

  this->device_count = 0;

  for (i = 0; true; i++)
  {
    desc = cf->ReadTupleString(section, "devices", i, NULL);
    if (!desc)
      break;
      
    // Parse the interface and index
    if (sscanf(desc, "%64[^:]:%d", name, &index) < 2)
    {
      PLAYER_WARN1("invalid device spec; skipping [%s]", desc);
      continue;
    }

    // Lookup the interface
    if (lookup_interface(name, &iface) != 0)
    {
      PLAYER_WARN1("unknown interface spec; skipping [%s]", name);
      continue;
    }

    assert(this->device_count < (int) (sizeof(this->devices) / sizeof(this->devices[0])));
    
    // Add to our device table
    device = this->devices + this->device_count++;
    device->id.code = iface.code;
    device->id.index = index;
    device->device = NULL;
    device->tsec = 0;
    device->tusec = 0;
  }
    
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
  int i;
  WriteLogDevice *device;
  
  // Subscribe to the underlying devices
  for (i = 0; i < this->device_count; i++)
  {
    device = this->devices + i;

    device->device = deviceTable->GetDevice(device->id);
    if (!device->device)
    {
      PLAYER_ERROR("unable to locate device for logging");
      return -1;
    }
    if (device->device->Subscribe(this) != 0)
    {
      PLAYER_ERROR("unable to subscribe to device for logging");
      return -1;
    }
  }

  // Open the file
  this->file = fopen(this->filename, "w+");
  if (this->file == NULL)
  {
    PLAYER_ERROR2("unable to open [%s]: %s\n", this->filename, strerror(errno));
    return -1;
  }

  // Write the file header
  fprintf(this->file, "## Player version %s \n", VERSION);
  fprintf(this->file, "## File version %s \n", "0.0.0");

  // Start device thread
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
int WriteLog::Shutdown()
{
  int i;
  WriteLogDevice *device;

  // Stop the device thread
  this->StopThread();

  // Close the file
  if (this->file)
    fclose(this->file);
  this->file = NULL;

  // Unsubscribe to the underlying devices
  for (i = 0; i < this->device_count; i++)
  {
    device = this->devices + i;

    // Unsbscribe from the underlying device
    device->device->Unsubscribe(this);
    device->device = NULL;
  }
  
  return 0;
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


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void WriteLog::Main(void)
{
  int i;
  size_t size, maxsize;
  uint32_t tsec, tusec;
  void *data;
  WriteLogDevice *device;
  
  maxsize = 8192;
  data = malloc(maxsize);
  
  while (1)
  {
    pthread_testcancel();

    // Walk the device list
    for (i = 0; i < this->device_count; i++)
    {
      device = this->devices + i;
      
      // Read data from underlying device
      size = device->device->GetData(this, (unsigned char*) data, maxsize, &tsec, &tusec);
      assert(size < maxsize);

      // Check for new data
      if (device->tsec == tsec && device->tusec == tusec)
        continue;
      device->tsec = tsec;
      device->tusec = tusec;
  
      // Write data to file
      this->Write(data, size, &device->id, tsec, tusec);
    }
  }

  free(data);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Write data to file
void WriteLog::Write(void *data, size_t size,
                     const player_device_id_t *id, uint32_t sec, uint32_t usec)
{
  char *host;
  int port;
  player_interface_t iface;
  struct timeval stime;

  // Get server time
  GlobalTime->GetTime(&stime);

  // Get interface name
  lookup_interface_code(id->code, &iface);

  host = "";
  port = 0;
  
  // Write header info
  fprintf(this->file, "%014.3f %s %d %s %02d %014.3f ",
           (double) stime.tv_sec + (double) stime.tv_usec * 1e-6,
           host, port, iface.name, id->index,
           (double) sec + (double) usec * 1e-6);

  // Write the data
  switch (id->code)
  {
    case PLAYER_POSITION_CODE:
      this->WritePosition((player_position_data_t*) data, size);
      break;
    case PLAYER_LASER_CODE:
      this->WriteLaser((player_laser_data_t*) data, size);
      break;
  }

  fprintf(this->file, "\n");
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Signed int conversion macros
#define N_INT16(x) ((int16_t) ntohs(x))
#define N_UINT16(x) ((uint16_t) ntohs(x))
#define N_INT32(x) ((int32_t) ntohl(x))
#define N_UINT32(x) ((uint32_t) ntohl(x))


////////////////////////////////////////////////////////////////////////////
// Unit conversion macros
#define MM_M(x) ((x) / 1000.0)
#define DEG_RAD(x) ((x) * M_PI / 180.0)


////////////////////////////////////////////////////////////////////////////
// Write position data to file
void WriteLog::WritePosition(player_position_data_t *data, size_t size)
{
  fprintf(this->file, "%+07.3f %+07.3f %+04.3f %+07.3f %+07.3f %+07.3f %d",
          MM_M(N_INT32(data->xpos)), MM_M(N_INT32(data->ypos)), DEG_RAD(N_INT32(data->yaw)),
          MM_M(N_INT32(data->xspeed)), MM_M(N_INT32(data->yspeed)), DEG_RAD(N_INT32(data->yspeed)),
          data->stall);

  return;
}


////////////////////////////////////////////////////////////////////////////
// Write laser data to file
void WriteLog::WriteLaser(player_laser_data_t *data, size_t size)
{
  int i;
  
  fprintf(this->file, "%+07.4f %+07.4f %+07.4f %04d ",
          DEG_RAD(N_INT16(data->min_angle) * 0.01), DEG_RAD(N_INT16(data->max_angle) * 0.01),
          DEG_RAD(N_UINT16(data->resolution) * 0.01), N_UINT16(data->range_count));

  for (i = 0; i < ntohs(data->range_count); i++)
    fprintf(this->file, "%.3f %2d ", MM_M(N_UINT16(data->ranges[i])), data->intensity[i]);

  return;
}


