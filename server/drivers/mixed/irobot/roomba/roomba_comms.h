/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006 -
 *     Brian Gerkey
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

#ifndef ROOMBA_COMMS_H
#define ROOMBA_COMMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

/* command opcodes */
#define ROOMBA_OPCODE_START            128
#define ROOMBA_OPCODE_BAUD             129
#define ROOMBA_OPCODE_CONTROL          130
#define ROOMBA_OPCODE_SAFE             131
#define ROOMBA_OPCODE_FULL             132
#define ROOMBA_OPCODE_POWER            133
#define ROOMBA_OPCODE_SPOT             134
#define ROOMBA_OPCODE_CLEAN            135
#define ROOMBA_OPCODE_MAX              136
#define ROOMBA_OPCODE_DRIVE            137
#define ROOMBA_OPCODE_MOTORS           138
#define ROOMBA_OPCODE_LEDS             139
#define ROOMBA_OPCODE_SONG             140
#define ROOMBA_OPCODE_PLAY             141
#define ROOMBA_OPCODE_SENSORS          142
#define ROOMBA_OPCODE_FORCEDOCK        143

#define ROOMBA_DELAY_MODECHANGE_MS      20

#define ROOMBA_MODE_OFF                  0
#define ROOMBA_MODE_PASSIVE              1
#define ROOMBA_MODE_SAFE                 2
#define ROOMBA_MODE_FULL                 3

typedef struct
{
  /* Serial port to which the robot is connected */
  char serial_port[PATH_MAX];
  /* File descriptor associated with serial connection (-1 if no valid
   * connection) */
  int fd;
  /* Current operation mode; one of ROOMBA_MODE_* */
  unsigned char mode;
  /* Integrated odometric position [m m rad] */
  double x, y, theta;
} roomba_comm_t;

roomba_comm_t* roomba_create(const char* serial_port);
int roomba_open(roomba_comm_t* r);
int roomba_init(roomba_comm_t* r);
int roomba_close(roomba_comm_t* r);

#ifdef __cplusplus
}
#endif

#endif

