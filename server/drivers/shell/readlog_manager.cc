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
 * The logfile driver will read or write data from a log file.  In
 * write mode, the driver can be used to record data from any other
 * Player driver.  In read mode, the driver can be use to replay
 * stored data as if it was coming from a real device.
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
  char line[4096];
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
          this->ParseData(device, linenum, token_count, tokens, dtime);
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
                             int token_count, char **tokens, uint64_t dtime)
{
  if (device->device_id.code == PLAYER_LASER_CODE)
    return this->ParseLaser(device, linenum, token_count, tokens, dtime);
  else if (device->device_id.code == PLAYER_POSITION_CODE)
    return this->ParsePosition(device, linenum, token_count, tokens, dtime);
  else if (device->device_id.code == PLAYER_WIFI_CODE)
    return this->ParseWifi(device, linenum, token_count, tokens, dtime);

  PLAYER_WARN("unknown device code");
  return -1;
}


////////////////////////////////////////////////////////////////////////////
// Parse laser data
int ReadLogManager::ParseLaser(CDevice *device, int linenum,
                              int token_count, char **tokens, uint64_t dtime)
{
  player_laser_data_t data;
  uint32_t dtime_sec, dtime_usec;
    
  int i;
  double sr, sb;
  int si;

  if (token_count < 12)
  {
    PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.range_count = 0;
  for (i = 6; i < token_count; i += 3)
  {
    sr = atof(tokens[i + 0]);
    sb = atof(tokens[i + 1]);
    si = atoi(tokens[i + 2]);

    if (data.range_count == 0)
      data.min_angle = (int) (sb * 180 / M_PI * 100);
    data.max_angle = (int) (sb * 180 / M_PI * 100);

    data.ranges[data.range_count] = htons((int) (sr * 1000));
    data.intensity[data.range_count] = si;
    data.range_count += 1;
  }

  data.resolution = 100; // HACK (int) ((data.max_angle - data.min_angle) / data.range_count);

  //printf("%d %d %d %d\n", data.resolution, data.min_angle, data.max_angle, data.range_count);
  
  data.resolution = htons(data.resolution);
  data.min_angle = htons(data.min_angle);
  data.max_angle = htons(data.max_angle);
  data.range_count = htons(data.range_count);

  dtime_sec = dtime / 1000000;
  dtime_usec = dtime % 1000000;

  device->PutData(&data, sizeof(data), dtime_sec, dtime_usec);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse position data
int ReadLogManager::ParsePosition(CDevice *device, int linenum,
                                 int token_count, char **tokens, uint64_t dtime)
{
  player_position_data_t data;
  uint32_t dtime_sec, dtime_usec;
  
  double px, py, pa;
  double vx, vy, va;

  if (token_count < 12)
  {
    PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
    return -1;
  }
  
  px = atof(tokens[6]);
  py = atof(tokens[7]);
  pa = atof(tokens[8]);

  vx = atof(tokens[9]);
  vy = atof(tokens[10]);
  va = atof(tokens[11]);

  data.xpos = htonl((int) (px * 1000));
  data.ypos = htonl((int) (py * 1000));
  data.yaw = htonl((int) (pa * 180 / M_PI));

  data.xspeed = htonl((int) (vx * 1000));
  data.yspeed = htonl((int) (vy * 1000));
  data.yawspeed = htonl((int) (va * 180 / M_PI));
  
  data.stall = 0;

  dtime_sec = dtime / 1000000;
  dtime_usec = dtime % 1000000;

  device->PutData(&data, sizeof(data), dtime_sec, dtime_usec);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse wifi data
int ReadLogManager::ParseWifi(CDevice *device, int linenum,
                              int token_count, char **tokens, uint64_t dtime)
{
  player_wifi_data_t data;
  player_wifi_link_t *link;
  uint32_t dtime_sec, dtime_usec;
  int i;

  if (token_count < 6)
  {
    PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
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

  dtime_sec = dtime / 1000000;
  dtime_usec = dtime % 1000000;

  device->PutData(&data, sizeof(data), dtime_sec, dtime_usec);

  return 0;
}

