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
  this->file = gzopen(this->filename, "w+");
  if (this->file == NULL)
  {
    PLAYER_ERROR2("unable to open [%s]: %s\n", this->filename, strerror(errno));
    return -1;
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
int WriteLogManager::Shutdown()
{
  // Close the file
  gzclose(this->file);
  this->file = NULL;
  
  return 0;
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
  gzprintf(this->file, "%14.3f %s %d %s %02d %14.3f ",
           (double) stime.tv_sec + (double) stime.tv_usec * 1e-6,
           host, port, iface.name, id->index,
           (double) sec + (double) usec * 1e-6);
  
  return;
}

