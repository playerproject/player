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
 * $Id$
 *
 * defines information necessary for the Stage/Player interaction
 */

#ifndef _STAGE_H
#define _STAGE_H

/* get types and device-specific packet formats */
#include <messages.h>
#include <sys/time.h> // for struct timeval
#include <semaphore.h> // for sem_t

// the largest number of unique ports player can bind
// this is only used for a temporary buffer
// and can easily be replaced with dynamic allocation if necessary
#define MAXPORTS 2048

// Notes on stage/player shared memory format.
//
// Each device is allocateed a block of shared memory.
// Each simulated device is allocated a block of shared memory.
// This block is subdivided into 5 parts:
//      info buffer -- device ids, plus timestamps and available byte-counts
//      data buffer
//      command buffer
//      config buffer
//      truth buffer

// these notes might be a little wrong - check stage's entity.cc behavior:
// player/stage info buffer
// available is set by stage and read by player.
// subscribed is set by player and read by stage.
// data_timestamp is set by stage and read by player.
// data_len is set by stage and read by player.
// command_len is set by player.
// config_len is set by player and reset (to zero) by stage.
//

typedef struct player_stage_info
{
  sem_t lock; // POSIX semaphore used to protect this structure

  player_id_t player_id;  // identify this entity to Player
  uint32_t len;           // total size of this struct + all the buffers
  uint8_t subscribed;     // the number of Players connected to this device
  uint8_t local;

  // the type-specific stuff is stored in variable length buffers
  // after this header - we store useful info about the availability
  // and freshness of that data here
  uint32_t data_len;
  uint32_t data_avail;
  uint32_t data_timestamp_sec;
  uint32_t data_timestamp_usec;

  uint32_t command_len;
  uint32_t command_avail;
  uint32_t command_timestamp_sec;
  uint32_t command_timestamp_usec;

  uint32_t config_len;
  uint32_t config_avail;
  uint32_t config_timestamp_sec;
  uint32_t config_timestamp_usec;

} __attribute__ ((packed)) player_stage_info_t;



typedef struct stage_clock
{
  sem_t lock; // POSIX semaphore used to protect this structure
  struct timeval time;
}  __attribute__ ((packed)) stage_clock_t;

#endif // _STAGE_H

