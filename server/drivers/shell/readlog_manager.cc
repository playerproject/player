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
 * Desc: Object for reading log files.
 * Author: Andrew Howard
 * Date: 17 May 2003
 * CVS: $Id$
 *
 * The ReadLogManager synchronizes reads from a data log.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "player.h"
#include "device.h"
#include "deviceregistry.h"

#include "readlog_manager.h"


////////////////////////////////////////////////////////////////////////////
// Pointer to the one-and-only manager
static ReadLogManager *manager;


////////////////////////////////////////////////////////////////////////////
// Instantiate and initialize the manager
int ReadLogManager_Init(const char *filename, double speed)
{
  manager = new ReadLogManager(filename, speed);
  return manager->Startup();
}


////////////////////////////////////////////////////////////////////////////
// Finalize the manager
int ReadLogManager_Fini()
{
  manager->Shutdown();
  delete manager;
  manager = NULL;
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Get the pointer to the one-and-only instance
ReadLogManager *ReadLogManager_Get()
{
  return manager;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
ReadLogManager::ReadLogManager(const char *filename, double speed)
{
  this->filename = strdup(filename);
  this->speed = speed;
  this->file = NULL;
  this->device_count = 0;
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Destructor
ReadLogManager::~ReadLogManager()
{
  free(this->filename);
  
  return;
}



////////////////////////////////////////////////////////////////////////////
// Initialize driver
int ReadLogManager::Startup()
{
  // Reset the time
  this->server_time = 0;

  // Open the file
  this->file = gzopen(this->filename, "r");
  if (this->file == NULL)
  {
    PLAYER_ERROR2("unable to open [%s]: %s\n", this->filename, strerror(errno));
    return -1;
  }

  // Start our own driver thread
  if (pthread_create(&this->thread, NULL, &DummyMain, this) != 0)
  {
    PLAYER_ERROR("unable to create device thread");
    return -1;
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
int ReadLogManager::Shutdown()
{
  // Stop the driver thread
  pthread_cancel(this->thread);
  if (pthread_join(this->thread, NULL) != 0)
    PLAYER_WARN("error joining device thread");

  // Close the file
  gzclose(this->file);
  this->file = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Subscribe to manager
int ReadLogManager::Subscribe(player_device_id_t id, CDevice *device)
{
  // Add this id to the list of subscribed device id's
  // TODO: lock
  assert(this->device_count < (int) (sizeof(this->devices) / sizeof(this->devices[0])));

  this->devices[this->device_count] = device;
  this->device_ids[this->device_count] = id;
  this->device_count++;
  
  // TODO: unlock
  
  return 0;
}



////////////////////////////////////////////////////////////////////////////
// Unsubscribe from the manager
int ReadLogManager::Unsubscribe(player_device_id_t id, CDevice *device)
{
  int i;
  
  // TODO: lock
  
  // Remove this id from the list of subscribed device id's
  for (i = 0; i < this->device_count; i++)
  {
    if (this->devices[i] == device)
    {
      memmove(this->devices + i, this->devices + i + 1,
              (this->device_count - i - 1) * sizeof(this->devices[0]));
      memmove(this->device_ids + i, this->device_ids + i + 1,
              (this->device_count - i - 1) * sizeof(this->device_ids[0]));
      this->device_count--;
      break;
    }
  }

  // TODO: unlock
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Dummy main
void *ReadLogManager::DummyMain(void *_this)
{
  ((ReadLogManager*) _this)->Main();
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Driver thread
void ReadLogManager::Main()
{
  int i, len, linenum;
  char line[8192];
  int token_count;
  char *tokens[4096];

  CDevice *device;
  player_device_id_t device_id;
  player_device_id_t header_id;
  uint64_t stime, dtime;

  // If nobody is subscribed, we will just loop here until they do
  while (this->device_count == 0)
  {
    usleep(100000);
    pthread_testcancel();
    this->server_time += 100000;
  }

  // Now read the file
  linenum = 0;
  while (true)
  {
    pthread_testcancel();

    // Read a line from the file
    if (gzgets(this->file, line, sizeof(line)) == NULL)
      break;

    linenum += 1;

    //printf("line %d\n", linenum);

    // Tokenize the line using whitespace separators
    token_count = 0;
    len = strlen(line);
    for (i = 0; i < len; i++)
    {
      if (isspace(line[i]))
        line[i] = 0;
      else if (i == 0)
      {
        assert(token_count < (int) (sizeof(tokens) / sizeof(tokens[i])));
        tokens[token_count++] = line + i;
      }
      else if (line[i - 1] == 0)
      {
        assert(token_count < (int) (sizeof(tokens) / sizeof(tokens[i])));
        tokens[token_count++] = line + i;
      }
    }

    // Discard comments
    if (token_count >= 1)
    {
      if (strcmp(tokens[0], "#") == 0)
        continue;
      if (strcmp(tokens[0], "##") == 0)
        continue;
    }

    // Parse out the header info
    if (this->ParseHeader(linenum, token_count, tokens, &header_id, &stime, &dtime) != 0)
      continue;

    this->server_time = stime;

    if (header_id.code == PLAYER_PLAYER_CODE)
    {
      usleep((int) (100000 / this->speed));
      continue;
    }
    else
    {
      for (i = 0; i < this->device_count; i++)
      {
        device = this->devices[i];
        device_id = this->device_ids[i];
        
        if (device_id.code == header_id.code && device_id.index == header_id.index)
          this->ParseData(device, linenum, token_count, tokens, dtime / 1000000L, dtime % 1000000L);
      }
    }
  }

  // File is done, so just loop forever
  while (1)
  {
    usleep(100000);
    pthread_testcancel();
    this->server_time += 100000;
  }

  return;
}


////////////////////////////////////////////////////////////////////////////
// Signed int conversion macros
#define NINT16(x) (htons((int16_t)(x)))
#define NUINT16(x) (htons((uint16_t)(x)))
#define NINT32(x) (htonl((int32_t)(x)))
#define NUINT32(x) (htonl((uint32_t)(x)))


////////////////////////////////////////////////////////////////////////////
// Unit conversion macros
#define M_MM(x) ((x) * 1000.0)
#define CM_MM(x) ((x) * 100.0)
#define RAD_DEG(x) ((x) * 180.0 / M_PI)


////////////////////////////////////////////////////////////////////////////
// Parse the header info
int ReadLogManager::ParseHeader(int linenum, int token_count, char **tokens,
                               player_device_id_t *id, uint64_t *stime, uint64_t *dtime)
{
  char *name;
  player_interface_t interface;
  
  if (token_count < 4)
  {
    PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
    return -1;
  }
  
  name = tokens[3];

  if (strcmp(name, "sync") == 0)
  {
    id->code = PLAYER_PLAYER_CODE;
    id->index = 0;
    *stime = (int64_t) (atof(tokens[0]) * 1e6);
    *dtime = 0;
  }
  else if (lookup_interface(name, &interface) == 0)
  {
    id->code = interface.code;
    id->index = atoi(tokens[4]);
    *stime = (uint64_t) (int64_t) (atof(tokens[0]) * 1e6);
    *dtime = (uint64_t) (int64_t) (atof(tokens[5]) * 1e6);
  }
  else
  {
    PLAYER_WARN1("unknown interface name [%s]", name);
    return -1;
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse data
int ReadLogManager::ParseData(CDevice *device, int linenum,
                              int token_count, char **tokens, uint32_t tsec, uint32_t tusec)
{
  if (device->device_id.code == PLAYER_LASER_CODE)
    return this->ParseLaser(device, linenum, token_count, tokens, tsec, tusec);
  else if (device->device_id.code == PLAYER_POSITION_CODE)
    return this->ParsePosition(device, linenum, token_count, tokens, tsec, tusec);
  else if (device->device_id.code == PLAYER_POSITION3D_CODE)
    return this->ParsePosition3d(device, linenum, token_count, tokens, tsec, tusec);
  else if (device->device_id.code == PLAYER_WIFI_CODE)
    return this->ParseWifi(device, linenum, token_count, tokens, tsec, tusec);
  else if (device->device_id.code == PLAYER_GPS_CODE)
    return this->ParseGps(device, linenum, token_count, tokens, tsec, tusec);

  PLAYER_WARN("unknown device code");
  return -1;
}


////////////////////////////////////////////////////////////////////////////
// Parse laser data
int ReadLogManager::ParseLaser(CDevice *device, int linenum,
                               int token_count, char **tokens, uint32_t tsec, uint32_t tusec)
{
  int i, count;
  player_laser_data_t data;

  if (token_count < 12)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.min_angle = NINT16(RAD_DEG(atof(tokens[6])) * 100);
  data.max_angle = NINT16(RAD_DEG(atof(tokens[7])) * 100);
  data.resolution = NUINT16(RAD_DEG(atof(tokens[8])) * 100);
  data.range_count = NUINT16(atoi(tokens[9]));
    
  count = 0;
  for (i = 10; i < token_count; i += 2)
  {
    data.ranges[count] = NUINT16(M_MM(atof(tokens[i + 0])));
    data.intensity[count] = atoi(tokens[i + 1]);
    count += 1;
  }

  if (count != ntohs(data.range_count))
  {
    PLAYER_ERROR2("range count mismatch at %s:%d", this->filename, linenum);
    return -1;
  }

  device->PutData(&data, sizeof(data), tsec, tusec);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse position data
int ReadLogManager::ParsePosition(CDevice *device, int linenum,
                                  int token_count, char **tokens, uint32_t tsec, uint32_t tusec)
{
  player_position_data_t data;

  if (token_count < 11) // 12
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }
  
  data.xpos = NINT32(M_MM(atof(tokens[6])));
  data.ypos = NINT32(M_MM(atof(tokens[7])));
  data.yaw = NINT32(RAD_DEG(atof(tokens[8])));
  data.xspeed = NINT32(M_MM(atof(tokens[9])));
  data.yspeed = NINT32(M_MM(atof(tokens[10])));
  data.yawspeed = NINT32(RAD_DEG(atof(tokens[11])));
  //data.stall = atoi(tokens[12]);

  device->PutData(&data, sizeof(data), tsec, tusec);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse position3d data
int ReadLogManager::ParsePosition3d(CDevice *device, int linenum,
                                    int token_count, char **tokens, uint32_t tsec, uint32_t tusec)
{
 player_position3d_data_t data;

  if (token_count < 19)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }
  
  data.xpos = NINT32(M_MM(atof(tokens[6])));
  data.ypos = NINT32(M_MM(atof(tokens[7])));
  data.zpos = NINT32(M_MM(atof(tokens[8])));

  data.roll = NINT32(RAD_DEG(3600 * atof(tokens[9])));
  data.pitch = NINT32(RAD_DEG(3600 * atof(tokens[10])));
  data.yaw = NINT32(RAD_DEG(3600 * atof(tokens[11])));

  data.xspeed = NINT32(M_MM(atof(tokens[12])));
  data.yspeed = NINT32(M_MM(atof(tokens[13])));
  data.zspeed = NINT32(M_MM(atof(tokens[14])));

  data.rollspeed = NINT32(RAD_DEG(3600 * atof(tokens[15])));
  data.pitchspeed = NINT32(RAD_DEG(3600 * atof(tokens[16])));
  data.yawspeed = NINT32(RAD_DEG(3600 * atof(tokens[17])));
  
  data.stall = atoi(tokens[18]);

  device->PutData(&data, sizeof(data), tsec, tusec);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse wifi data
int ReadLogManager::ParseWifi(CDevice *device, int linenum,
                              int token_count, char **tokens, uint32_t tsec, uint32_t tusec)
{
  player_wifi_data_t data;
  player_wifi_link_t *link;
  int i;

  if (token_count < 6)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.link_count = 0;
  for (i = 6; i < token_count; i += 4)
  {
    link = data.links + data.link_count;
  
    strcpy(link->ip, tokens[i + 0]);
    link->qual = atoi(tokens[i + 1]);
    link->level = atoi(tokens[i + 2]);
    link->noise = atoi(tokens[i + 3]);

    link->qual = htons(link->qual);
    link->level = htons(link->level);
    link->noise = htons(link->noise);    
    
    data.link_count++;
  }
  data.link_count = htons(data.link_count);

  device->PutData(&data, sizeof(data), tsec, tusec);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse GPS data
int ReadLogManager::ParseGps(CDevice *device, int linenum,
                             int token_count, char **tokens, uint32_t tsec, uint32_t tusec)
{
  player_gps_data_t data;

  if (token_count < 17)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.time_sec = NUINT32((int) atof(tokens[6]));
  data.time_usec = NUINT32((int) fmod(atof(tokens[6]), 1.0));

  data.latitude = NINT32((int) (60 * 60 * 60 * atof(tokens[7])));
  data.longitude = NINT32((int) (60 * 60 * 60 * atof(tokens[8])));
  data.altitude = NINT32(M_MM(atof(tokens[9])));

  data.utm_e = NINT32(CM_MM(atof(tokens[10])));
  data.utm_n = NINT32(CM_MM(atof(tokens[11])));

  data.hdop = NINT16((int) (10 * atof(tokens[12])));
  data.err_horz = NUINT32(M_MM(atof(tokens[13])));
  data.err_vert = NUINT32(M_MM(atof(tokens[14])));

  data.quality = atoi(tokens[15]);
  data.num_sats = atoi(tokens[16]);

  device->PutData(&data, sizeof(data), tsec, tusec);

  return 0;
}

