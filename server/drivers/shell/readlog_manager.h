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
class ReadLogManager;


// Instantiate and initialize the manager
int ReadLogManager_Init(const char *filename, double speed);

// Finalize the manager
int ReadLogManager_Fini();

// Get the pointer to the one-and-only instance
ReadLogManager *ReadLogManager_Get();


class ReadLogManager
{
  // Constructor
  public: ReadLogManager(const char *filename, double speed);

  // Destructor
  public: virtual ~ReadLogManager();  

  // Start the reader; read data from the given device
  public: int Startup();

  // Stop the reader
  public: int Shutdown();
  
  // Subscribe 
  public: int Subscribe(player_device_id_t id, CDevice *device);

  // Unsubscribe
  public: int Unsubscribe(player_device_id_t id, CDevice *device);

  // Dummy main
  private: static void *DummyMain(void *_this);

  // Main loop
  private: void Main();

  // Parse the header info
  private: int ParseHeader(int linenum, int token_count, char **tokens,
                           player_device_id_t *id, uint64_t *stime, uint64_t *dtime);

  // Parse some data
  private: int ParseData(CDevice *device, int linenum,
                         int token_count, char **tokens, uint32_t tsec, uint32_t tusec);

  // Parse laser data
  private: int ParseLaser(CDevice *device, int linenum,
                          int token_count, char **tokens, uint32_t tsec, uint32_t tusec);

  // Parse position data
  private: int ParsePosition(CDevice *device, int linenum,
                             int token_count, char **tokens, uint32_t tsec, uint32_t tusec);

  // Parse position3d data
  private: int ParsePosition3d(CDevice *device, int linenum,
                               int token_count, char **tokens, uint32_t tsec, uint32_t tusec);

  // Parse wifi data
  private: int ParseWifi(CDevice *device, int linenum,
                         int token_count, char **tokens, uint32_t tsec, uint32_t tusec);

  // Parse gps data
  private: int ParseGps(CDevice *device, int linenum,
                        int token_count, char **tokens, uint32_t tsec, uint32_t tusec);

  // File to read data from
  private: char *filename;
  private: gzFile file;

  // File format
  private: char *format;

  // Playback speed (1 = real time)
  private: double speed;

  // Subscribed device list
  private: int device_count;
  private: CDevice *devices[1024];
  private: player_device_id_t device_ids[1024];

  // Device thread
  private: pthread_t thread;

  // Server time
  public: uint64_t server_time;
};


#endif
