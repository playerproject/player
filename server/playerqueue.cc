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
 * a general queue.  could do anything, i suppose, but it's meant for shifting
 * configuration requests and replies between devices and the client read/write
 * threads.  it can be used either intra-process with real devices or
 * inter-process (through shared memory) with simulated Stage devices.
 */

#include <playerqueue.h>
#include <stdlib.h> // for exit(3)
#include <string.h> // for bzero(3)

#if HAVE_STRINGS_H
  #include <strings.h>
#endif

//#include <playertime.h>
//extern PlayerTime* GlobalTime;
    
// basic constructor; makes a PlayerQueue that will dynamically allocate
// memory for the queue
PlayerQueue::PlayerQueue(int tmpqueuelen)
{
  len = tmpqueuelen;
  queue = new playerqueue_elt_t[len];
  assert(queue);
  bzero(queue, sizeof(playerqueue_elt_t)*len);
}

// constructor for Stage; creates a PlayerQueue with a chunk of memory
// already set aside
PlayerQueue::PlayerQueue(void* tmpqueue, int tmpqueuelen)
{
  queue = (playerqueue_elt_t*)tmpqueue;
  len = tmpqueuelen;
  bzero(queue, sizeof(playerqueue_elt_t)*len);
}

// push a new element on the queue.  returns the index of the new
// element in the queue, or -1 if the queue is full
int 
PlayerQueue::Push(player_device_id_t* device, void* client, 
                  unsigned short type, struct timeval* ts, 
                  void* data, int size)
{
  // search for an empty spot, from front to back
  for(int i=0;i<len;i++)
  {
    if(!queue[i].valid)
    {
      queue[i].client = client;
      if(size > PLAYER_MAX_REQREP_SIZE)
      {
        queue[i].size = PLAYER_MAX_REQREP_SIZE;
        fprintf(stderr, "PlayerQueue::Push(): WARNING: truncating %d byte "
                "request/reply to %d bytes\n", size, queue[i].size);
      }
      else
        queue[i].size = size;

      if(device)
        queue[i].device = *device;

      bzero(queue[i].data, PLAYER_MAX_REQREP_SIZE);
      memcpy(queue[i].data,data,queue[i].size);
      queue[i].type = type;

      if(ts) {
	//        queue[i].timestamp = *ts;
	// LPR. on the ARM, the previous line seemed to munge the high-order byte
	// of the type member. why?
	queue[i].timestamp.tv_sec = ts->tv_sec;
	queue[i].timestamp.tv_usec = ts->tv_usec;
      }
      else
      {
        queue[i].timestamp.tv_sec = 0;
        queue[i].timestamp.tv_usec = 0;
      }

      queue[i].valid = 1;
      return(i);
    }
  }
  return(-1);
}

// another form of Push, this one doesn't set the client pointer
int 
PlayerQueue::Push(void* data, int size)
{
  return(Push(NULL,NULL,0,NULL,data,size));
}

// pop an element off the queue. returns the size of the element,
// or -1 if the queue is empty
int 
PlayerQueue::Pop(player_device_id_t* device, void** client, 
                 void* data, int size)
{
  int tmpsize;

  if(queue[0].valid)
  {
    // first, get the data out (check to see if there's a valid clientp first)
    if(client)
      *client = queue[0].client;
    if(size < queue[0].size)
    {
      fprintf(stderr, "PlayerQueue::Pop(): WARNING: truncating %d byte "
              "request/reply to %d bytes\n", queue[0].size, size);
      tmpsize = size;
    }
    else
      tmpsize = queue[0].size;

    if(device)
      *device = queue[0].device;

    memcpy(data, queue[0].data, tmpsize);

    queue[0].valid = 0;

    // now, move the others up in the queue
    for(int i=0;i<(len-1);i++)
    {
      if(!queue[i].valid && queue[i+1].valid)
      {
        queue[i].client = queue[i+1].client;
        queue[i].size = queue[i+1].size;
        memcpy(queue[i].data,queue[i+1].data,queue[i].size);
        queue[i].valid = 1;
        queue[i+1].valid = 0;
      }
    }

    return(tmpsize);
  }
  else
    return(-1);
}
    
// another form of Pop, this one doesn't set the client pointer
int 
PlayerQueue::Pop(void* data, int size)
{
  return(Pop(NULL,NULL,data,size));
}
    
// clear the queue; returns 0 on success; -1 on failure
int 
PlayerQueue::Flush()
{
  unsigned char dummybuf[PLAYER_MAX_REQREP_SIZE];
  int dummysize = PLAYER_MAX_REQREP_SIZE;

  while(Pop(dummybuf, dummysize) >= 0);
  return(0);
}

// is the queue empty?
bool 
PlayerQueue::Empty()
{
  return(!(queue[0].valid));
}

// a slightly different kind of Pop, this one searches the queue for an
// element in which the client pointer matches the one provided.  Pops
// the first such element and returns its size, or -1 if no such element
// is found
int 
PlayerQueue::Match(player_device_id_t* device, void* client, 
                   unsigned short* type, struct timeval* ts, 
                   void* data, int size)
{
  int tmpsize;

  // look for the one we want
  for(int i=0;i<len;i++)
  {
    // assume that they're packed to the front, so we can bail on the
    // first invalid entry
    if(!queue[i].valid)
      return(-1);

    if(queue[i].client == client)
    {
      if(size < queue[i].size)
      {
        fprintf(stderr, "PlayerQueue::Match(): WARNING: truncating %d byte "
                "request/reply to %d bytes\n", queue[i].size, size);
        tmpsize = size;
      }
      else
        tmpsize = queue[i].size;

      if(device)
        *device = queue[i].device;
      memcpy(data, queue[i].data, tmpsize);
      *type = queue[i].type;
      //*ts = queue[i].timestamp;
      ts->tv_sec = queue[i].timestamp.tv_sec;
      ts->tv_usec = queue[i].timestamp.tv_usec;

      queue[i].valid = 0;
    
      // now, move the others up in the queue
      while(i<(len-1))
      {
        if(!queue[i].valid && queue[i+1].valid)
        {
          queue[i].client = queue[i+1].client;
          queue[i].size = queue[i+1].size;
          memcpy(queue[i].data,queue[i+1].data,queue[i].size);
          queue[i].valid = 1;
          queue[i+1].valid = 0;
        }
        i++;
      }

      return(tmpsize);
    }
  }

  // didn't find one
  return(-1);
}

