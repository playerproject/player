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

#ifndef _PLAYERQUEUE_H
#define _PLAYERQUEUE_H

//#include <clientdata.h>
#include <messages.h>
#include <sys/time.h>

// a queue contains elements of the following type.
typedef struct
{
  char valid;  // is this entry used?
  void* client;  // pointer to the client who is expecting a reply
  unsigned short type;    // player message type (only really used for replies)
  struct timeval timestamp;  // time that configuration was made (only replies)
  int size;             // size (in bytes) of the request/reply
  unsigned char data[PLAYER_MAX_REQREP_SIZE]; // the request/reply
} __attribute__ ((packed)) playerqueue_elt_t;

class PlayerQueue
{
  private:
    // the queue itself
    playerqueue_elt_t* queue;
    // the size of the queue (i.e., # of elements)
    int len;

  public:
    // basic constructor; makes a PlayerQueue that will dynamically allocate
    // memory for the queue
    PlayerQueue(int tmpqueuelen);

    // constructor for Stage; creates a PlayerQueue with a chunk of memory
    // already set aside
    PlayerQueue(unsigned char* tmpqueue, int tmpqueuelen);

    // push a new element on the queue.  returns the index of the new
    // element in the queue, or -1 if the queue is full
    int Push(void* client, unsigned short type, struct timeval* ts,
             unsigned char* data, int size);

    // another form of Push, this one doesn't set the client pointer
    int Push(unsigned char* data, int size);

    // pop an element off the queue. returns the size of the element,
    // or -1 if the queue is empty
    int Pop(void** client, unsigned char* data, int size);
    
    // another form of Pop, this one doesn't set the client pointer
    int Pop(unsigned char* data, int size);

    // a slightly different kind of Pop, this one searches the queue for an
    // element in which the client pointer matches the one provided.  Pops
    // the first such element and returns its size, or -1 if no such element
    // is found
    int Match(void* client, unsigned short* type, struct timeval* ts,
              unsigned char* data, int size);

    // clear the queue; returns 0 on success; -1 on failure
    int Flush();

    // is the queue empty?
    bool Empty();
};

#endif
