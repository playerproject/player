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
// Desc: Gazebo (simulator) camera driver
// Author: Pranav Srivastava
// Date: 16 Sep 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#if INCLUDE_GAZEBO_CAMERA

#include "gz_camera.h"
#define RGBSIZE 3

#include <unistd.h>
#include <fcntl.h>

// Constructor
CMGzCamera::CMGzCamera(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_camera_data_t), 0, 10, 10)
{

 
  // Get the id of the device in Gazebo
  this->gz_id = cf->ReadString(section, "gz_id", 0);

  this->width = cf->ReadInt(section,"width",0);
  this->height= cf->ReadInt(section,"height",0);

  if(width==0 || height ==0)
    {   
        this->width=320;
        this->height=240;
    }
  // Get the globally defined  Gazebo client (one per instance of Player)
  this->client = GzClient::client;
  
  // Create an interface
  this->iface = gz_camera_alloc();
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
CMGzCamera::~CMGzCamera()
{
  gz_camera_free(this->iface);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int CMGzCamera::Setup()
{ 
  // Open the interface
  if (gz_camera_open(this->iface, this->client, this->gz_id) != 0)
    return -1;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int CMGzCamera::Shutdown()
{
  gz_camera_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Data
size_t CMGzCamera::GetData(void* client, unsigned char* dest, size_t len,
                        uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
// player_camera_data_t data;

//dest=(unsigned char *) malloc(width*height*RGBSIZE*sizeof(unsigned char));

  //  data.image =(unsigned char *)((this->iface->data->image));
  if (this->iface->data->image!=NULL){
    memcpy(dest,this->iface->data->image,width*height*RGBSIZE);
    
  
    if (timestamp_sec)
      *timestamp_sec = (int) (this->iface->data->time);
    if (timestamp_usec)
      *timestamp_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);
  }

  /*  
    int fd = creat("tmpimage.raw",O_RDWR);
    write(fd,dest,width*height*RGBSIZE);
    close(fd);
  */

  // free(data.image);

  // don't really have to do this... 
  // return sizeof(data);
  return 0;
}

#endif

