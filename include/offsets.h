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
/*
 * $Id$
 *
 *   various constants, mostly related to buffer sizes and offsets
 *   within buffers for various pieces of data.
 *
 *   also, until we get the documentation done, this file is a good
 *   place to look for packet formats
 */
#ifndef OFFSETS
#define OFFSETS

/*
 * this file defines buffer sizes, offsets, and such
 */

// Length specific types
//
#define BYTE unsigned char
#define INT16 signed short
#define UINT16 unsigned short
#define INT32 signed int
#define UINT32 unsigned int


// Notes on stage/player shared memory format.
//
// Each device is allocateed a block of shared memory.
// This block is subdivided into 4 parts:
//      info buffer -- flags (subscribed, new data, new command, new configuration)
//      data buffer
//      command buffer
//      config buffer
// The info buffer is identical for all devices.  It contains the following flags:
//      subscribe : WR : 1 if client has subscribed to device; 0 otherwise
//      data      : RW : 1 if there is new data available; 0 otherwise
//      command   : WR : 1 if there is a new command available; 0 otherwise
//      config    : WR : 1 if there is a new configuration  available; 0 otherwise
// Flags marked WR are written by player and read by stage.
// Flags marked RW are written by stage and read by player.
//
// As at 29 Nov, only the laser supports this format.
//
// ahoward


// Info buffer format
// Each simulated device is allocated a block of shared memory.
//
#define INFO_BUFFER_SIZE    4
#define INFO_SUBSCRIBE_FLAG 0
#define INFO_DATA_FLAG      1
#define INFO_COMMAND_FLAG   2
#define INFO_CONFIG_FLAG    3

// Position device command buffer
//
typedef struct PlayerPositionCommand
{
    INT16 vr, vth;
} __attribute__ ((packed));


// Position device data buffer
//
typedef struct PlayerPositionData
{
    UINT32 time;
    INT32 px, py;
    UINT16 pth;
    INT16 vr, vth;
    UINT16 compass;
    BYTE stall;
} __attribute__ ((packed));


// Memory map for position device
// *** HACK -- name collision with the offset into the PSOS buffer
//
#define SPOSITION_DATA_BUFFER_SIZE sizeof(PlayerPositionData)
#define SPOSITION_COMMAND_BUFFER_SIZE sizeof(PlayerPositionCommand)
#define SPOSITION_CONFIG_BUFFER_SIZE 0
#define SPOSITION_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE \
                              + SPOSITION_DATA_BUFFER_SIZE \
                              + SPOSITION_COMMAND_BUFFER_SIZE \
                              + SPOSITION_CONFIG_BUFFER_SIZE


/* laser stuff */
/*
 * laser data packet is:
 *   361 unsigned shorts, each representing a range sample in mm. 
 *     kasper says the laser give samples from right to left.
 */
#define LASER_SERIAL_PORT_NAME_SIZE 256  // space for a big pathname
#define LASER_NUM_SAMPLES 361
#define LASER_DATA_BUFFER_SIZE (int)(LASER_NUM_SAMPLES*sizeof(unsigned short))
#define LASER_COMMAND_BUFFER_SIZE 0
#define LASER_CONFIG_BUFFER_SIZE 32
#define LASER_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE \
                              + LASER_DATA_BUFFER_SIZE \
                              + LASER_COMMAND_BUFFER_SIZE \
                              + LASER_CONFIG_BUFFER_SIZE


/* vision stuff */
/*
 * vision data packet (it's just an ACTS packet):
 *   64 - 3264 bytes of ACTS data, which is:
 *     64 bytes of header (2 bytes per channel)
 *     sequence of 10-byte blob data (maximum 10 per channel), which is:
 *        check the ACTS manual...
 * NOTE: 
 *   in the CVisionDevice 'data' buffer, there is also an extra leading
 *   short, representing size of ACTS data to follow; this size is NOT 
 *   sent over the network.
 *
 * vision command packet is:
 *   3 shorts:
 *     pan - between -100 and 100 degrees.  positive is clockwise.
 *     tilt - between -25 and 25 degrees. positive is up.
 *     zoom - between 0 and 1023.  0 is wide, 1023 is telefoto.
 *
 * NOTE:
 *   the PTZ commands are actually stored in the CP2OSDevice 'data'
 *   buffer, at an offset defined later.
 */
#define VISION_CONFIGFILE_NAME_SIZE 256 // space for a big pathname
#define VISION_PORT_NAME_SIZE 16 // space for a 15-digit number...that's big
/* ACTS size stuff */
#define ACTS_NUM_CHANNELS 32
#define ACTS_HEADER_SIZE 2*ACTS_NUM_CHANNELS  
#define ACTS_BLOB_SIZE 10
#define ACTS_MAX_BLOBS_PER_CHANNEL 10
#define ACTS_MAX_BLOB_DATA_SIZE \
  ACTS_NUM_CHANNELS*ACTS_BLOB_SIZE*ACTS_MAX_BLOBS_PER_CHANNEL
#define ACTS_TOTAL_MAX_SIZE \
  ACTS_MAX_BLOB_DATA_SIZE+ACTS_HEADER_SIZE

// Players needs 2 bytes to store the packet length
#define ACTS_DATA_BUFFER_SIZE 2 + ACTS_TOTAL_MAX_SIZE
#define ACTS_COMMAND_BUFFER_SIZE 0
#define ACTS_CONFIG_BUFFER_SIZE 0
#define ACTS_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE \
                             + ACTS_DATA_BUFFER_SIZE \
                             + ACTS_COMMAND_BUFFER_SIZE \
                             + ACTS_CONFIG_BUFFER_SIZE

/*
 * P2OS device stuff
 *
 *   this device's 'data' buffer is shared among many devices.  here
 *   is the layout (in this order):
 *     'position' data:
 *       3 ints: time X Y
 *       4 shorts: heading, forwardvel, turnrate, compass
 *       1 chars: stalls
 *     'sonar' data:
 *       16 shorts: 16 sonars
 *     'gripper' data:
 *       2 chars: gripstate,gripbeams
 *     'misc' data:
 *       2 chars: frontbumper,rearbumpers
 *       1 char:  voltage
 */
#define POSITION_DATA_BUFFER_SIZE 3*sizeof(int)+ \
                                  4*sizeof(unsigned short)+ \
                                  1*sizeof(char)
#define SONAR_DATA_BUFFER_SIZE 16*sizeof(unsigned short)
#define GRIPPER_DATA_BUFFER_SIZE 2*sizeof(char)
#define MISC_DATA_BUFFER_SIZE 3*sizeof(char)
#define P2OS_DATA_BUFFER_SIZE POSITION_DATA_BUFFER_SIZE + \
                              SONAR_DATA_BUFFER_SIZE + \
                              GRIPPER_DATA_BUFFER_SIZE + \
                              MISC_DATA_BUFFER_SIZE
#define POSITION_DATA_OFFSET 0
#define SONAR_DATA_OFFSET POSITION_DATA_OFFSET + POSITION_DATA_BUFFER_SIZE
#define GRIPPER_DATA_OFFSET SONAR_DATA_OFFSET + SONAR_DATA_BUFFER_SIZE
#define MISC_DATA_OFFSET GRIPPER_DATA_OFFSET + GRIPPER_DATA_BUFFER_SIZE


/*
 * the P2OS device 'command' buffer is shared by several devices.
 * here is the layout (in this order):
 *    'position' command:
 *       2 shorts: forwardspeed (mm/sec), turnspeed (deg/sec)
 *    'gripper' command:
 *       2 chars: gripcommand, optional gripcommand
 *    'vision' command:
 *       3 shorts: pan, tilt, zoom
 */
#define POSITION_COMMAND_BUFFER_SIZE 2*sizeof(short)
#define GRIPPER_COMMAND_BUFFER_SIZE 2*sizeof(char)
//#define VISION_COMMAND_BUFFER_SIZE 3*sizeof(short)

#define P2OS_COMMAND_BUFFER_SIZE POSITION_COMMAND_BUFFER_SIZE + \
                                 GRIPPER_COMMAND_BUFFER_SIZE
// max size for a P2OS config request (should be big enough
// for a raw P2OS packet)
#define P2OS_CONFIG_BUFFER_SIZE 256


#define POSITION_COMMAND_OFFSET 0
#define GRIPPER_COMMAND_OFFSET POSITION_COMMAND_OFFSET + POSITION_COMMAND_BUFFER_SIZE


/*
 * the PTZ device.  accepts commands for the PTZ camera,
 * and gives feedback on current position.  currently,
 * only pan, tilt, and zoom (shorts in that order) are supported.  
 * could change in the future.
 */
#define PTZ_COMMAND_BUFFER_SIZE 3*sizeof(short)
#define PTZ_DATA_BUFFER_SIZE 3*sizeof(short)
#define PTZ_CONFIG_BUFFER_SIZE 0
#define PTZ_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE + \
                              PTZ_DATA_BUFFER_SIZE + \
                              PTZ_COMMAND_BUFFER_SIZE + \
                              PTZ_CONFIG_BUFFER_SIZE 

// RTV
/*
 * SSonar (Separate Sonar) stuff for stage compatibility 
 * and consistency with other devices
 * this is slotted in the shared buffer after the P2OS sonar device
 */

#define SSONAR_CONFIG_BUFFER_SIZE 0
#define SSONAR_COMMAND_BUFFER_SIZE 0
#define SSONAR_DATA_BUFFER_SIZE  16*sizeof( unsigned short )
#define SSONAR_TOTAL_BUFFER_SIZE INFO_BUFFER_SIZE + \
                                 SSONAR_DATA_BUFFER_SIZE + \
                                 SSONAR_COMMAND_BUFFER_SIZE + \
                                 SSONAR_CONFIG_BUFFER_SIZE 

// RTV -----------------------------------------------------------------------
// Player/arena interface shared memory locations

// subscription flags for player/arena interface 
// - stored at the top of the memory map
#define SUB_MOTORS 0
#define SUB_SONAR  SUB_MOTORS + 1
#define SUB_LASER  SUB_SONAR + 2
#define SUB_VISION SUB_MOTORS + 3     
#define SUB_GRIPPER SUB_MOTORS + 4    //   NOT YET IMPLEMENTED IN ARENA
#define SUB_MISC SUB_MOTORS + 5       //              "
#define SUB_PTZ  SUB_MOTORS + 6

#define SUB_BUFFER_SIZE 7

#define ARENA_SUB_START 0
//*** remove #define P2OS_DATA_START ARENA_SUB_START + SUB_BUFFER_SIZE
//*** remove #define P2OS_COMMAND_START P2OS_DATA_START + P2OS_DATA_BUFFER_SIZE
#define SPOSITION_DATA_START ARENA_SUB_START + SUB_BUFFER_SIZE
//*** #define SONAR_DATA_START SPOSITION_COMMAND_START + SPOSITION_TOTAL_BUFFER_SIZE
#define SSONAR_DATA_START SPOSITION_DATA_START + SPOSITION_TOTAL_BUFFER_SIZE
#define LASER_DATA_START SSONAR_DATA_START + SSONAR_TOTAL_BUFFER_SIZE
#define PTZ_DATA_START LASER_DATA_START + LASER_TOTAL_BUFFER_SIZE
#define ACTS_DATA_START PTZ_DATA_START + PTZ_TOTAL_BUFFER_SIZE

// ACTS is the last thing in the shared memory
#define TOTAL_SHARED_MEMORY_BUFFER_SIZE ACTS_DATA_START + ACTS_TOTAL_BUFFER_SIZE

// !RTV ---------------------------------------------------------------------

#endif












