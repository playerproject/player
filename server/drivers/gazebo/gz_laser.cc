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
///////////////////////////////////////////////////////////////////////////
//
// Desc: Gazebo (simulator) laser driver
// Author: Andrew Howard
// Date: 6 Apr 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "player.h"
#include "device.h"
#include "drivertable.h"

// UNBELIEVABLE HACK
#include "/home/ahoward/darpa-demo/gazebo-exp/src/gazebo.h"

// Incremental navigation driver
class GzLaser : public CDevice
{
  // Constructor
  public: GzLaser(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzLaser();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Data
  public: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  // Commands
  public: virtual void PutCommand(void* client, unsigned char* src, size_t len);

  // Request/reply
  public: virtual int PutConfig(player_device_id_t* device, void* client, void* data, size_t len);

  // Gazebo id
  private: const char *gz_id;

  // MMap file
  private: int mmap_fd;
  private: gz_laser_t *mmap;
};


// Initialization function
CDevice* GzLaser_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_LASER_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"gz_laser\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new GzLaser(interface, cf, section)));
}


// a driver registration function
void GzLaser_Register(DriverTable* table)
{
  table->AddDriver("gz_laser", PLAYER_ALL_MODE, GzLaser_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzLaser::GzLaser(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_laser_data_t), 0, 10, 10)
{

  // Get the id of the device in Gazebo
  this->gz_id = cf->ReadString(section, "gz_id", 0);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzLaser::~GzLaser()
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzLaser::Setup()
{
  char filename[128];

  snprintf(filename, sizeof(filename), "/tmp/gazebo.%s.mmap", this->gz_id);
  printf("opening %s\n", filename);

  // Open the mmap file
  this->mmap_fd = ::open(filename, O_RDWR);
  if (this->mmap_fd <= 0)
  {
    PLAYER_ERROR1("error opening device file: %s", strerror(errno));
    return -1;
  }

  // Map the mmap file
  this->mmap = (gz_laser_t*) ::mmap(0, sizeof(gz_laser_t), PROT_READ | PROT_WRITE,
                                       MAP_SHARED, this->mmap_fd, 0);
  if ((int) this->mmap <= 0)
  {
    PLAYER_ERROR1("error mapping device file: %s", strerror(errno));
    exit(1);
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzLaser::Shutdown()
{
  ::munmap(this->mmap, sizeof(gz_laser_t));
  ::close(this->mmap_fd);

  this->mmap = NULL;
  this->mmap_fd = 0;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Data
size_t GzLaser::GetData(void* client, unsigned char* dest, size_t len,
                        uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  int i;
  player_laser_data_t data;

  data.min_angle = htons((int) (this->mmap->min_angle * 100 * 180 / M_PI));
  data.max_angle = htons((int) (this->mmap->max_angle * 100 * 180 / M_PI));
  data.resolution = htons((int) (this->mmap->resolution * 100 * 180 / M_PI));
  data.range_count = htons((int) (this->mmap->range_count));
  
  for (i = 0; i < this->mmap->range_count; i++)
  {
    data.ranges[i] = htons((int) (this->mmap->ranges[i] * 1000));
    data.intensity[i] = htons(0);
  }

  assert(len >= sizeof(data));
  memcpy(dest, &data, sizeof(data));

  if (timestamp_sec)
    *timestamp_sec = (int) (this->mmap->time);
  if (timestamp_usec)
    *timestamp_usec = (int) (fmod(this->mmap->time, 1) * 1e6);
  
  return sizeof(data);
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzLaser::PutCommand(void* client, unsigned char* src, size_t len)
{  
  return;
}



////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzLaser::PutConfig(player_device_id_t* device, void* client, void* data, size_t len)
{

  // TODO

  if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
    PLAYER_ERROR("PutReply() failed");
  
  return 0;
}
