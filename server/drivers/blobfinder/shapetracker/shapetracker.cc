/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al
 *                      gerkey@usc.edu    
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
// Desc: Driver for detecting UPC barcodes from camera
// Author: Andrew Howard
// Date: 15 Feb 2004
// CVS: $Id$
//
// Theory of operation:
//   TODO
//
// Requires:
//   Camera device.
//
///////////////////////////////////////////////////////////////////////////

#include "player.h"

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>

#include "device.h"
#include "devicetable.h"
#include "drivertable.h"


// Driver for detecting laser retro-reflectors.
class ShapeTracker : public CDevice
{
  // Constructor
  public: ShapeTracker(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();
  
  // Process any new camera data.
  private: int UpdateCamera();

  // Look for barcodes in the image.  
  private: void ProcessImage();
  
  // Write the device data (the data going back to the client).
  private: void WriteData();

  // Camera stuff
  private: int camera_index;
  private: CDevice *camera;
  private: double camera_time;
  private: player_camera_data_t camera_data;
};


// Initialization function
CDevice* ShapeTracker_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_BLOBFINDER_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"shapetracker\" does not support interface \"%s\"\n",
                  interface);
    return (NULL);
  }
  return ((CDevice*) (new ShapeTracker(interface, cf, section)));
}


// a driver registration function
void ShapeTracker_Register(DriverTable* table)
{
  table->AddDriver("shapetracker", PLAYER_READ_MODE, ShapeTracker_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
ShapeTracker::ShapeTracker(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_blobfinder_data_t), 0, 10, 10)
{
  this->camera_index = cf->ReadInt(section, "camera", 0);
  this->camera = NULL;
  this->camera_time = 0;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int ShapeTracker::Setup()
{
  player_device_id_t id;

  // Subscribe to the camera.
  id.code = PLAYER_CAMERA_CODE;
  id.index = this->camera_index;
  id.port = this->device_id.port;
  this->camera = deviceTable->GetDevice(id);
  if (!this->camera)
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return(-1);
  }
  if (this->camera->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    return(-1);
  }

  // Start the driver thread.
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int ShapeTracker::Shutdown()
{
  // Stop the driver thread.
  StopThread();
  
  // Unsubscribe from devices.
  this->camera->Unsubscribe(this);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void ShapeTracker::Main() 
{
  while (true)
  {
    // Let the camera drive update rate
    this->camera->Wait();

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any new camera data.
    if (this->UpdateCamera())
    {
      this->ProcessImage();
      this->WriteData();
    }

    // Process any pending requests.
    this->HandleRequests();
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int ShapeTracker::HandleRequests()
{
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  int len;
  
  while ((len = GetConfig(&client, &request, sizeof(request))) > 0)
  {
    switch (request[0])
    {
      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Process any new camera data.
int ShapeTracker::UpdateCamera()
{
  size_t size;
  uint32_t timesec, timeusec;
  double time;

  // Get the camera data.
  size = this->camera->GetData(this, (unsigned char*) &this->camera_data,
                               sizeof(this->camera_data), &timesec, &timeusec);
  time = (double) timesec + ((double) timeusec) * 1e-6;

  // Dont do anything if this is old data.
  if (fabs(time - this->camera_time) < 0.001)
    return 0;
  this->camera_time = time;
  
  // Do some byte swapping
  this->camera_data.width = ntohs(this->camera_data.width);
  this->camera_data.height = ntohs(this->camera_data.height); 
  this->camera_data.depth = this->camera_data.depth;
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Look for stuff in the image.
void ShapeTracker::ProcessImage()
{
  // TODO: image processing
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void ShapeTracker::WriteData()
{
  /* TODO
  int i;
  int blob_count, channel_count, channel;
  uint32_t timesec, timeusec;
  blob_t *blob;
  player_blobfinder_data_t data;

  data.width = htons(this->camera_data.width);
  data.height = htons(this->camera_data.height);

  // Reset the header data
  for (i = 0; i < PLAYER_BLOBFINDER_MAX_CHANNELS; i++)
  {
    data.header[i].index = 0;
    data.header[i].num = 0;
  }
  blob_count = 0;

  // Go through the blobs
  for (channel = 0; channel < PLAYER_BLOBFINDER_MAX_CHANNELS; channel++)
  {
    data.header[channel].index = htons(blob_count);
    channel_count = 0;
    
    for (i = 0; i < this->blob_count; i++)
    {
      blob = this->blobs + i;
      if (blob->id != channel)
        continue;

      data.blobs[blob_count].color = 0;  // TODO
      data.blobs[blob_count].area = htonl((int) ((blob->bx - blob->ax) * (blob->by - blob->ay)));
      data.blobs[blob_count].x = htons((int) ((blob->bx + blob->ax) / 2));
      data.blobs[blob_count].y = htons((int) ((blob->by + blob->ay) / 2));
      data.blobs[blob_count].left = htons((int) (blob->ax));
      data.blobs[blob_count].right = htons((int) (blob->ay));
      data.blobs[blob_count].top = htons((int) (blob->bx));
      data.blobs[blob_count].bottom = htons((int) (blob->by));
      data.blobs[blob_count].range = htons(0);
      channel_count++;
      blob_count++;
    }

    data.header[channel].num = htons(channel_count);
  }
  
  // Compute the data timestamp (from camera).
  timesec = (uint32_t) this->camera_time;
  timeusec = (uint32_t) (fmod(this->camera_time, 1.0) * 1e6);
  
  // Copy data to server.
  PutData((unsigned char*) &data, sizeof(data), timesec, timeusec);
  */
  
  return;
}


