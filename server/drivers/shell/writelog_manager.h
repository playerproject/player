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
 * Desc: Driver for reading/writing log files.
 * Author: Andrew Howard
 * Date: 17 May 2003
 * CVS: $Id$
 *
 * The logfile driver will read or write data from a log file.  In
 * write mode, the driver can be used to record data from any other
 * Player driver.  In read mode, the driver can be use to replay
 * stored data as if it was coming from a real device.
 */

#ifndef READLOG_MANAGER_H
#define READLOG_MANAGER_H

#include <stdio.h>
#include <zlib.h>

#include "device.h"


// Forward declarations
class WriteLogManager;


// Instantiate and initialize the manager
int WriteLogManager_Init(const char *filename);

// Finalize the manager
int WriteLogManager_Fini();

// Get the pointer to the one-and-only instance
WriteLogManager *WriteLogManager_Get();


class WriteLogManager
{
  // Constructor
  public: WriteLogManager(const char *filename);

  // Destructor
  public: virtual ~WriteLogManager();  

  // Start the reader; read data from the given device
  public: int Startup();

  // Stop the reader
  public: int Shutdown();
  
  // Subscribe 
  public: int Subscribe(player_device_id_t id, CDevice *device);

  // Unsubscribe
  public: int Unsubscribe(player_device_id_t id, CDevice *device);

  // Write data to file
  public: void Write(void *data, size_t size,
                     const player_device_id_t *id, uint32_t sec, uint32_t usec);

  // File to read data from
  private: char *filename;
  private: gzFile file;
};


#endif
