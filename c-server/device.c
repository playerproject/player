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


/*
 * Player's internal device API
 *
 * $Id$
 */

#include <assert.h>

#include "device.h"

struct player_dev*
player_dev_create(size_t datasize, size_t commandsize, 
                  int reqqueuelen, int repqueuelen)
{
  struct player_dev* dev;

  assert(dev = (struct player_dev*)malloc(sizeof(struct player_dev)));

  assert(dev->device_data = (unsigned char*)malloc(datasize));
  dev->device_datasize = dev->device_used_datasize = datasize;
  
  assert(dev->device_command = (unsigned char*)malloc(commandsize));
  dev->device_commandsize = dev->device_used_commandsize = commandsize;
  
  /*
  device_reqqueue = new PlayerQueue(reqqueuelen);
  device_repqueue = new PlayerQueue(repqueuelen);
  */

  dev->subscriptions = 0;

  pthread_mutex_init(&dev->access_mutex,NULL);

  return(dev);
}

player_dev_destroy(struct player_dev* dev)
{
  free(dev->device_data);
  free(dev->device_command);

  free(dev);
}

int 
player_dev_subscribe(struct player_dev* dev, void *client)
{
  int setupResult;

  if(dev->subscriptions == 0) 
  {
    setupResult = (dev->functable.setup)(dev);
    if (setupResult == 0) 
      dev->subscriptions++; 
  }
  else 
  {
    dev->subscriptions++;
    setupResult = 0;
  }
  
  return(setupResult);
}

int 
player_dev_unsubscribe(struct player_dev* dev, void *client)
{
  int shutdownResult;

  if(dev->subscriptions == 0) 
    shutdownResult = -1;
  else if(dev->subscriptions == 1) 
  {
    shutdownResult = (dev->functable.shutdown)(dev);
    if (shutdownResult == 0 ) 
      dev->subscriptions--;
    /* do we want to unsubscribe even though the shutdown went bad? */
  }
  else 
  {
    dev->subscriptions--;
    shutdownResult = 0;
  }
  
  return(shutdownResult);
}


int 
player_dev_setup(struct player_dev* dev)
{
  PLAYER_ERROR("You really must define your own setup()!");
  return(-1);
}

int
player_dev_shutdown(struct player_dev* dev)
{
  PLAYER_ERROR("You really must define your own setup()!");
  return(-1);
}


size_t
player_dev_getnumdata(struct player_dev* dev, void* client)
{
  return(1);
}

size_t 
player_dev_getdata(struct player_dev* dev, void* client, 
                   unsigned char* dest, size_t len,
                   struct timeval* timestamp)
{
  int size;
  (dev->functable.lock)(dev);

  assert(len >= dev->device_used_datasize);
  memcpy(dest,dev->device_data,dev->device_used_datasize);
  size = dev->device_used_datasize;
  if(timestamp)
  {
    timestamp->tv_sec = dev->data_timestamp_sec;
    timestamp->tv_usec = dev->data_timestamp_usec;
  }

  (dev->functable.unlock)(dev);
  return(size);
}

void
player_dev_putdata(struct player_dev* dev, unsigned char* src, size_t len,
                   struct timeval timestamp)
{
  if(timestamp.tv_sec == 0)
  {
    struct timeval curr;
    /*
    if(GlobalTime->GetTime(&curr) == -1)
      fputs("CDevice::PutData(): GetTime() failed!!!!\n", stderr);
     */
    gettimeofday(&curr,NULL);
    timestamp = curr;
  }

  (dev->functable.lock)(dev);
  assert(len <= dev->device_datasize);
  memcpy(dev->device_data,src,len);
  dev->data_timestamp_sec = timestamp.tv_sec;
  dev->data_timestamp_usec = timestamp.tv_usec;

  /* store the amount we copied, for later reference */
  dev->device_used_datasize = len;
  (dev->functable.unlock)(dev);
}


size_t
player_dev_getcommand(struct player_dev* dev, unsigned char* dest, 
                      size_t len)
{
  int size;
  (dev->functable.lock)(dev);
  assert(len >= dev->device_used_commandsize);
  memcpy(dest,dev->device_command,dev->device_used_commandsize);
  size = dev->device_used_commandsize;
  (dev->functable.unlock)(dev);
  return(size);
}

void
player_dev_putcommand(struct player_dev* dev, void* client, 
                      unsigned char* src, size_t len)
{
  (dev->functable.lock)(dev);
  assert(len <= dev->device_commandsize);
  memcpy(dev->device_command,src,len);
  /* store the amount we wrote */
  dev->device_used_commandsize = len;
  (dev->functable.unlock)(dev);
}

size_t
player_dev_getconfig(struct player_dev* dev, player_device_id_t* device, 
                     void** client, void *data, size_t len)
{
  int size = 0;

  (dev->functable.lock)(dev);

  /*
  if((size = device_reqqueue->Pop(device, client, data, len)) < 0)
  {
    (dev->functable.unlock)(dev);
    return(0);
  }
  */

  (dev->functable.unlock)(dev);
  return(size);
}

int
player_dev_putconfig(struct player_dev* dev, player_device_id_t* device, 
                     void* client, void* data, size_t len)
{
  (dev->functable.lock)(dev);

  /*
  if(device_reqqueue->Push(device, client, PLAYER_MSGTYPE_REQ, NULL, 
                           data, len) < 0)
  {
    // queue was full
    (dev->functable.unlock)(dev);
    return(-1);
  }
  */

  (dev->functable.unlock)(dev);
  return(0);
}

int
player_dev_getreply(struct player_dev* dev, player_device_id_t* device, 
                    void* client, unsigned short* type, struct timeval* ts, 
                    void* data, size_t len)
{
  int size = 0;

  (dev->functable.lock)(dev);
  /*
  size = device_repqueue->Match(device, (void*)client, type, ts, data, len);
  */
  (dev->functable.unlock)(dev);

  return(size);
}

int
player_dev_putreply(struct player_dev* dev, player_device_id_t* device, 
                    void* client, unsigned short type, struct timeval* ts, 
                    void* data, size_t len)
{
  struct timeval curr;
  player_device_id_t id;

  if(ts)
    curr = *ts;
  else
    gettimeofday(&curr,NULL);
    /*
    GlobalTime->GetTime(&curr);
    */

  if(!device)
  {
    /* 
     * stick a dummy device code on it; when the server calls GetReply,
     * it will know what to do
     */
    id.code = id.index = id.port = 0;
  }
  else
    id = *device;


  (dev->functable.lock)(dev);
  /*device_repqueue->Push(&id, client, type, &curr, data, len);*/
  (dev->functable.unlock)(dev);

  return(0);
}

void
player_dev_lock(struct player_dev* dev)
{
  pthread_mutex_lock(&dev->access_mutex);
}

void
player_dev_unlock(struct player_dev* dev)
{
  pthread_mutex_unlock(&dev->access_mutex);
}

