/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
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

/* Desc: Driver for getting signal strengths for AODV ah-hoc network software.
 * Author: Andrew Howard ahoward@usc.edu
 * Date: 26 Nov 2002
 * $Id$
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include <driver.h>
#include <configfile.h>
#include <playertime.h>
#include <drivertable.h>
#include <player.h>

extern PlayerTime *GlobalTime;

#define AODV_INFO_FILE "/proc/aodv/route_table"

class Aodv : public Driver
{
  public: Aodv( ConfigFile *cf, int section);

  // Initialize driver
  public: virtual int Setup();

  // Finalize driver
  public: virtual int Shutdown();

  // Get the current readings
  public: virtual size_t GetData(player_device_id_t id,
                                 void* dest, size_t len,
                                 struct timeval* timestamp);

  // File handle for the /proc file system entry
  protected: FILE *file;
};


// Instantiate driver for given interface
Driver * Aodv_Init( ConfigFile *cf, int section)
{ 
  return ((Driver*)(new Aodv(cf, section)));
}


// Register driver type
void Aodv_Register(DriverTable *table)
{
  table->AddDriver("aodv", Aodv_Init);
  return;
}


// Constructor
Aodv::Aodv( ConfigFile *cf, int section)
    : Driver(cf, section, PLAYER_WIFI_CODE, PLAYER_READ_MODE, 0, 0, 0, 0)
{
  return;
}


// Initialize driver
int Aodv::Setup()
{
  // Just open the file
  this->file = fopen(AODV_INFO_FILE, "r");
  if (this->file == NULL)
  {
    PLAYER_ERROR2("unable to open [%s]; error [%s]",
                  AODV_INFO_FILE, strerror(errno));
    return -1;
  }
  return 0;
}


// Finalize driver
int Aodv::Shutdown()
{
  fclose(this->file);
  return 0;
}


// Get new data
size_t Aodv::GetData(player_device_id_t id,
                     void* dest, size_t maxsize,
                     struct timeval* timestamp)
{
  int n, link_count;
  player_wifi_link_t *link;
  player_wifi_data_t data;
  char ip[16], next_ip[16];
  int seq, hop;
  int qual, level, noise;

  struct timeval curr;

  // Rewind to start of file
  rewind(this->file);

  // Skip header
  for (n = 0; n < 5;)
    if (fgetc(this->file) == '\n')
      n++;

  link_count = 0;
  while (TRUE)
  {
    n = fscanf(this->file, " %16s %d %d %16s ( %d ) ", ip, &seq, &hop, next_ip, &level);
    if (n == EOF)
      break;
    if (n < 5)
      continue;

    qual = 0;
    noise = 0;
    
    printf("aodv %s : %d\n", ip, level);

    link = data.links + link_count++;
    strncpy(link->ip, ip, sizeof(link->ip));
    //link->qual_type = PLAYER_WIFI_QUAL_UNKNOWN;
    link->qual = htons(qual);
    link->level = htons(level);
    link->noise = htons(noise);
  }
  data.link_count = htons(link_count);

  // Copy data to the server's buffer
  assert(sizeof(data) < maxsize);
  memcpy(dest, &data, sizeof(data));

  // Set the data timestamp
  GlobalTime->GetTime(&curr);
  *timestamp = curr;

  return (sizeof(data));
}
