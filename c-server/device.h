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

#ifndef _DEVICE_H
#define _DEVICE_H

#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>

#include "player.h"

/* forward declaration */
struct player_dev;

typedef struct
{
   int (*subscribe)(struct player_dev* dev, void *client);
   int (*unsubscribe)(struct player_dev* dev, void *client);

   int (*setup)(struct player_dev* dev);
   int (*shutdown)(struct player_dev* dev);

   size_t (*getnumdata)(struct player_dev* dev, void* client);
   size_t (*getdata)(struct player_dev* dev, void* client, 
                     unsigned char* dest, size_t len,
                     struct timeval* timestamp);
   void (*putdata)(struct player_dev* dev, unsigned char* src, size_t len,
                   struct timeval timestamp);

   size_t (*getcommand)(struct player_dev* dev, unsigned char* dest, 
                        size_t len);
   void (*putcommand)(struct player_dev* dev, void* client, 
                      unsigned char* src, size_t len);

   size_t (*getconfig)(struct player_dev* dev, player_device_id_t* device, 
                       void** client, void *data, size_t len);
   int (*putconfig)(struct player_dev* dev, player_device_id_t* device, 
                    void* client, void* data, size_t len);

   int (*getreply)(struct player_dev* dev, player_device_id_t* device, 
                   void* client, unsigned short* type, struct timeval* ts, 
                   void* data, size_t len);
   int (*putreply)(struct player_dev* dev, player_device_id_t* device, 
                   void* client, unsigned short type, struct timeval* ts, 
                   void* data, size_t len);

   void (*lock)(struct player_dev* dev);
   void (*unlock)(struct player_dev* dev);
} player_dev_functable_t;

struct player_dev
{
  pthread_mutex_t access_mutex;

  /* buffers for data and command */
  unsigned char* device_data;
  unsigned char* device_command;

  /* maximum sizes of data and command buffers */
  size_t device_datasize;
  size_t device_commandsize;

  /* amount at last write into each respective buffer */
  size_t device_used_datasize;
  size_t device_used_commandsize;

  /* queues for incoming requests and outgoing replies */
  //PlayerQueue* device_reqqueue;
  //PlayerQueue* device_repqueue;
    
  /* who we are */
  player_device_id_t device_id;
    
  /* to record the time at which the device gathered the data */
  uint32_t data_timestamp_sec;
  uint32_t data_timestamp_usec;
    
  /* number of current subscriptions */
  int subscriptions;

  /* table of driver function pointers */
  player_dev_functable_t functable;
};


#endif
