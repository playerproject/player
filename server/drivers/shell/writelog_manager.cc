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
 * Desc: Object for writing log files.
 * Author: Andrew Howard
 * Date: 17 May 2003
 * CVS: $Id$
 *
 * The WriteLogManager syncronizes writes to a log file.
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
#include "deviceregistry.h"
#include "playertime.h"

#include "writelog_manager.h"


////////////////////////////////////////////////////////////////////////////
// Pointer to the one-and-only time
extern PlayerTime* GlobalTime;


////////////////////////////////////////////////////////////////////////////
// Pointer to the one-and-only manager
static WriteLogManager *manager;


////////////////////////////////////////////////////////////////////////////
// Instantiate and initialize the manager
int WriteLogManager_Init(const char *filename)
{
  manager = new WriteLogManager(filename);
  return manager->Startup();
}


////////////////////////////////////////////////////////////////////////////
// Finalize the manager
int WriteLogManager_Fini()
{
  manager->Shutdown();
  delete manager;
  manager = NULL;
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Get the pointer to the one-and-only instance
WriteLogManager *WriteLogManager_Get()
{
  return manager;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
WriteLogManager::WriteLogManager(const char *filename)
{
  this->filename = strdup(filename);
  this->file = NULL;
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Destructor
WriteLogManager::~WriteLogManager()
{
  free(this->filename);
  
  return;
}



////////////////////////////////////////////////////////////////////////////
// Initialize driver
int WriteLogManager::Startup()
{
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
int WriteLogManager::Shutdown()
{
  // Stop the driver thread
  pthread_cancel(this->thread);
  if (pthread_join(this->thread, NULL) != 0)
    PLAYER_WARN("error joining device thread");

  // Close the file
  fclose(this->file);
  this->file = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Dummy main
void *WriteLogManager::DummyMain(void *_this)
{
  ((WriteLogManager*) _this)->Main();
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Driver thread
void WriteLogManager::Main()
{
  // TODO
}


////////////////////////////////////////////////////////////////////////////
// Write data to file
void WriteLogManager::Write(void *data, size_t size,
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
  fprintf(this->file, "%14.3f %s %d %s %02d %14.3f ",
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
void WriteLogManager::WritePosition(player_position_data_t *data, size_t size)
{
  fprintf(this->file, "%+07.3f %+07.3f %+04.3f %+07.3f %+07.3f %+07.3f %d",
          MM_M(N_INT32(data->xpos)), MM_M(N_INT32(data->ypos)), DEG_RAD(N_INT32(data->yaw)),
          MM_M(N_INT32(data->xspeed)), MM_M(N_INT32(data->yspeed)), DEG_RAD(N_INT32(data->yspeed)),
          data->stall);

  return;
}


////////////////////////////////////////////////////////////////////////////
// Write laser data to file
void WriteLogManager::WriteLaser(player_laser_data_t *data, size_t size)
{
  int i;
  
  fprintf(this->file, "%+07.4f %+07.4f %+07.4f %04d ",
          DEG_RAD(N_INT16(data->min_angle) * 0.01), DEG_RAD(N_INT16(data->max_angle) * 0.01),
          DEG_RAD(N_UINT16(data->resolution) * 0.01), N_UINT16(data->range_count));

  for (i = 0; i < ntohs(data->range_count); i++)
    fprintf(this->file, "%.3f %2d ", MM_M(N_UINT16(data->ranges[i])), data->intensity[i]);

  return;
}
