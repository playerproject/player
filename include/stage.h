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

// Notes on stage/player shared memory format.
//
// Each device is allocateed a block of shared memory.
// Each simulated device is allocated a block of shared memory.
// This block is subdivided into 4 parts:
//      info buffer -- flags (subscribed, new data, new command, new configuration)
//      data buffer
//      command buffer
//      config buffer


// player/stage info buffer
// available is set by stage and read by player.
// subscribed is set by player and read by stage.
// data_timestamp is set by stage and read by player.
// data_len is set by stage and read by player.
// command_len is set by player.
// config_len is set by player and reset (to zero) by stage.
//
typedef struct
{
    uint8_t available;
    uint8_t subscribed;
    uint32_t data_timestamp_sec;
    uint32_t data_timestamp_usec;
    uint16_t data_len;
    uint16_t command_len;
    uint16_t config_len;
} __attribute__ ((packed)) player_stage_info_t;


#define INFO_BUFFER_SIZE    sizeof(player_stage_info_t)

#define MISC_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE \
                              + MISC_DATA_BUFFER_SIZE \
                              + MISC_COMMAND_BUFFER_SIZE \
                              + MISC_CONFIG_BUFFER_SIZE

#define POSITION_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE \
                              + POSITION_DATA_BUFFER_SIZE \
                              + POSITION_COMMAND_BUFFER_SIZE \
                              + POSITION_CONFIG_BUFFER_SIZE

#define LASER_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE \
                              + LASER_DATA_BUFFER_SIZE \
                              + LASER_COMMAND_BUFFER_SIZE \
                              + LASER_CONFIG_BUFFER_SIZE

#define ACTS_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE \
                             + ACTS_DATA_BUFFER_SIZE \
                             + ACTS_COMMAND_BUFFER_SIZE \
                             + ACTS_CONFIG_BUFFER_SIZE

#define SONAR_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE + \
                                 SONAR_DATA_BUFFER_SIZE + \
                                 SONAR_COMMAND_BUFFER_SIZE + \
                                 SONAR_CONFIG_BUFFER_SIZE 

#define PTZ_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE + \
                              PTZ_DATA_BUFFER_SIZE + \
                              PTZ_COMMAND_BUFFER_SIZE + \
                              PTZ_CONFIG_BUFFER_SIZE

#define LASERBEACON_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE \
                              + LASERBEACON_DATA_BUFFER_SIZE \
                              + LASERBEACON_COMMAND_BUFFER_SIZE \
                              + LASERBEACON_CONFIG_BUFFER_SIZE

#define BROADCAST_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE + \
                              BROADCAST_DATA_BUFFER_SIZE + \
                              BROADCAST_COMMAND_BUFFER_SIZE + \
                              BROADCAST_CONFIG_BUFFER_SIZE 

#define GRIPPER_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE + \
                              GRIPPER_DATA_BUFFER_SIZE + \
                              GRIPPER_COMMAND_BUFFER_SIZE + \
                              GRIPPER_CONFIG_BUFFER_SIZE 

#define GPS_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE + \
                              GPS_DATA_BUFFER_SIZE + \
                              GPS_COMMAND_BUFFER_SIZE + \
                              GPS_CONFIG_BUFFER_SIZE 

// Make space for memory test
//
#define TEST_TOTAL_BUFFER_SIZE 64

// Lay out memory mapped area
//
#define TEST_DATA_START 0
#define MISC_DATA_START TEST_DATA_START + TEST_TOTAL_BUFFER_SIZE
#define POSITION_DATA_START MISC_DATA_START + MISC_TOTAL_BUFFER_SIZE
#define SONAR_DATA_START POSITION_DATA_START + POSITION_TOTAL_BUFFER_SIZE
#define LASER_DATA_START SONAR_DATA_START + SONAR_TOTAL_BUFFER_SIZE
#define PTZ_DATA_START LASER_DATA_START + LASER_TOTAL_BUFFER_SIZE
#define ACTS_DATA_START PTZ_DATA_START + PTZ_TOTAL_BUFFER_SIZE
#define LASERBEACON_DATA_START ACTS_DATA_START + ACTS_TOTAL_BUFFER_SIZE
#define BROADCAST_DATA_START LASERBEACON_DATA_START + LASERBEACON_TOTAL_BUFFER_SIZE
#define GRIPPER_DATA_START BROADCAST_DATA_START + BROADCAST_TOTAL_BUFFER_SIZE
#define GPS_DATA_START GRIPPER_DATA_START + GRIPPER_TOTAL_BUFFER_SIZE

#define TOTAL_SHARED_MEMORY_BUFFER_SIZE GPS_DATA_START + GPS_TOTAL_BUFFER_SIZE

#endif

