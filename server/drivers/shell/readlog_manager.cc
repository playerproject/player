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
#include "driver.h"
#include "deviceregistry.h"
#include "clientmanager.h"
#include "encode.h"

#include "readlog_manager.h"

// Pointer to the clientmanager, so we can reset timestamps in the clients,
// should the logfile be rewound.
extern ClientManager* clientmanager;

////////////////////////////////////////////////////////////////////////////
// Pointer to the one-and-only manager
static ReadLogManager *manager = NULL;


////////////////////////////////////////////////////////////////////////////
// Instantiate and initialize the manager
int ReadLogManager_Init(const char *filename, double speed)
{
  manager = new ReadLogManager(filename, speed);
  return manager->Init();
}


////////////////////////////////////////////////////////////////////////////
// Finalize the manager
int ReadLogManager_Fini()
{
  manager->Fini();
  delete manager;
  manager = NULL;
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Start the reader
int ReadLogManager_Startup()
{
  assert(manager);
  return manager->Startup();
}


////////////////////////////////////////////////////////////////////////////
// Stop the reader
int ReadLogManager_Shutdown()
{
  return manager->Shutdown();
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
  this->format = strdup("unknown");
  this->speed = speed;
  this->file = NULL;
  this->device_count = 0;
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Destructor
ReadLogManager::~ReadLogManager()
{
  free(this->format);
  free(this->filename);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Initialize driver
int ReadLogManager::Init()
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
  
  // Playback enabled by default
  this->enable = true;

  // Rewind not requested by default
  this->rewind_requested = false;

  // Autorewind not on by default
  this->autorewind = false;

  // Make some space for parsing data from the file.  This size is not
  // an exact upper bound; it's just my best guess.
  this->line_size = 4 * PLAYER_MAX_MESSAGE_SIZE;
  this->line = (char*) malloc(this->line_size);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
int ReadLogManager::Fini()
{
  // Free allocated mem
  free(this->line);
  
  // Close the file
  gzclose(this->file);
  this->file = NULL;
  
  return 0;
}



////////////////////////////////////////////////////////////////////////////
// Start the driver
int ReadLogManager::Startup()
{
  // Start our own driver thread
  if (pthread_create(&this->thread, NULL, &DummyMain, this) != 0)
  {
    PLAYER_ERROR("unable to create device thread");
    return -1;
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Stop the driver
int ReadLogManager::Shutdown()
{
  // Stop the driver thread
  pthread_cancel(this->thread);
  if (pthread_join(this->thread, NULL) != 0)
    PLAYER_WARN("error joining device thread");
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Subscribe to manager
int ReadLogManager::Subscribe(player_device_id_t id, Driver *device)
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
int ReadLogManager::Unsubscribe(player_device_id_t id, Driver *device)
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
  int token_count;
  char *tokens[4096];

  Driver *device;
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

    // If we're not supposed to playback data, sleep and loop
    if(!this->enable)
    {
      usleep(10000);
      continue;
    }

    // If a client has requested that we rewind, then do so
    if(this->rewind_requested)
    {
      // back up to the beginning of the file
      if(gzseek(this->file,0,SEEK_SET) < 0)
      {
        // oh well, warn the user and keep going
        PLAYER_WARN1("while rewinding logfile, gzseek() failed: %s",
                     strerror(errno));
      }
      else
      {
        linenum = 0;

        // reset the time
        this->server_time = 0;

        // reset time-of-last-write in all clients
        //
        // FIXME: It's not really thread-safe to call this here, because it
        //        writes to a bunch of fields that are also being read and/or
        //        written in the server thread.  But I'll be damned if I'm
        //        going to add a mutex just for this.
        clientmanager->ResetClientTimestamps();

        // reset the flag
        this->rewind_requested = false;

        puts("ReadLog: logfile rewound");

        continue;
      }
    }

    // Read a line from the file
    if (gzgets(this->file, this->line, this->line_size) == NULL)
    {
      // File is done, so just loop forever, unless we're on auto-rewind,
      // or until a client requests rewind.
      while(!this->autorewind && !this->rewind_requested)
      {
        usleep(100000);
        pthread_testcancel();
        this->server_time += 100000;
      }

      // request a rewind and start again
      this->rewind_requested = true;
      continue;
    }

    // Possible buffer overflow, so bail
    assert(strlen(this->line) < this->line_size);

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

    if (token_count >= 1)
    {
      // Discard comments
      if (strcmp(tokens[0], "#") == 0)
        continue;

      // Parse meta-data
      if (strcmp(tokens[0], "##") == 0)
      {
        if (token_count == 4)
        {
          free(this->format);
          this->format = strdup(tokens[3]);
        }
        continue;
      }
    }

    // Parse out the header info
    if (this->ParseHeader(linenum, token_count, tokens, &header_id, &stime, &dtime) != 0)
      continue;

    this->server_time = stime;

    if (header_id.code == PLAYER_PLAYER_CODE)
    {
      usleep((int) (100000 / this->speed) - 20000);
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
int ReadLogManager::ParseData(Driver *device, int linenum,
                              int token_count, char **tokens, uint32_t tsec, uint32_t tusec)
{
  if (device->device_id.code == PLAYER_CAMERA_CODE)
    return this->ParseCamera(device, linenum, token_count, tokens, tsec, tusec);
  else if (device->device_id.code == PLAYER_GPS_CODE)
    return this->ParseGps(device, linenum, token_count, tokens, tsec, tusec);
  else if (device->device_id.code == PLAYER_LASER_CODE)
    return this->ParseLaser(device, linenum, token_count, tokens, tsec, tusec);
  else if (device->device_id.code == PLAYER_POSITION_CODE)
    return this->ParsePosition(device, linenum, token_count, tokens, tsec, tusec);
  else if (device->device_id.code == PLAYER_POSITION3D_CODE)
    return this->ParsePosition3d(device, linenum, token_count, tokens, tsec, tusec);
  else if (device->device_id.code == PLAYER_WIFI_CODE)
    return this->ParseWifi(device, linenum, token_count, tokens, tsec, tusec);

  PLAYER_WARN("unknown device code");
  return -1;
}


////////////////////////////////////////////////////////////////////////////
// Parse camera data
int ReadLogManager::ParseCamera(Driver *device, int linenum,
                               int token_count, char **tokens, uint32_t tsec, uint32_t tusec)
{
  player_camera_data_t *data;
  size_t src_size, dst_size;
  
  if (token_count < 13)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data = (player_camera_data_t*) malloc(sizeof(player_camera_data_t));
  assert(data);
    
  data->width = NUINT16(atoi(tokens[6]));
  data->height = NUINT16(atoi(tokens[7]));
  data->depth = atoi(tokens[8]);
  data->format = atoi(tokens[9]);
  data->compression = atoi(tokens[10]);
  data->image_size = NUINT32(atoi(tokens[11]));
  
  // Check sizes
  src_size = strlen(tokens[12]);
  dst_size = ::DecodeHexSize(src_size);
  assert(dst_size = NUINT32(data->image_size));
  assert(dst_size < sizeof(data->image));

  // REMOVE printf("%d %d\n", src_size, dst_size);

  // Decode string
  ::DecodeHex(data->image, dst_size, tokens[12], src_size);
              
  struct timeval ts;
  ts.tv_sec = tsec;
  ts.tv_usec = tusec;
  device->PutData(data, sizeof(*data) - sizeof(data->image) + dst_size, &ts);

  free(data);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse laser data
int ReadLogManager::ParseLaser(Driver *device, int linenum,
                               int token_count, char **tokens, uint32_t tsec, uint32_t tusec)
{
  int i, count, angle;
  player_laser_data_t data;

  if (token_count < 12)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  if (strcmp(this->format, "0.0.0") == 0)
  {
    data.min_angle = NINT16(RAD_DEG(atof(tokens[6])) * 100);
    data.max_angle = NINT16(RAD_DEG(atof(tokens[7])) * 100);
    data.resolution = NUINT16(RAD_DEG(atof(tokens[8])) * 100);
    data.range_res = NUINT16(1);
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
  }
  else
  {
    data.min_angle = +18000;
    data.max_angle = -18000;
    
    count = 0;
    for (i = 6; i < token_count; i += 3)
    {
      data.ranges[count] = NUINT16(M_MM(atof(tokens[i + 0])));      
      data.intensity[count] = atoi(tokens[i + 2]);

      angle = (int) (atof(tokens[i + 1]) * 180 / M_PI * 100);
      if (angle < data.min_angle)
        data.min_angle = angle;
      if (angle > data.max_angle)
        data.max_angle = angle;

      count += 1;
    }

    data.resolution = (data.max_angle - data.min_angle) / (count - 1);
    data.range_count = count;

    data.range_count = NUINT16(data.range_count);
    data.min_angle = NINT16(data.min_angle);
    data.max_angle = NINT16(data.max_angle);
    data.resolution = NUINT16(data.resolution);
    data.range_res = NUINT16(1);
  }

  struct timeval ts;
  ts.tv_sec = tsec;
  ts.tv_usec = tusec;
  device->PutData(&data, sizeof(data), &ts);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse position data
int ReadLogManager::ParsePosition(Driver *device, int linenum,
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

  struct timeval ts;
  ts.tv_sec = tsec;
  ts.tv_usec = tusec;
  device->PutData(&data, sizeof(data), &ts);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse position3d data
int ReadLogManager::ParsePosition3d(Driver *device, int linenum,
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

  struct timeval ts;
  ts.tv_sec = tsec;
  ts.tv_usec = tusec;
  device->PutData(&data, sizeof(data), &ts);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse wifi data
int ReadLogManager::ParseWifi(Driver *device, int linenum,
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

  struct timeval ts;
  ts.tv_sec = tsec;
  ts.tv_usec = tusec;
  device->PutData(&data, sizeof(data), &ts);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse GPS data
int ReadLogManager::ParseGps(Driver *device, int linenum,
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

  struct timeval ts;
  ts.tv_sec = tsec;
  ts.tv_usec = tusec;
  device->PutData(&data, sizeof(data), &ts);

  return 0;
}

