/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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

/*
 * $Id$
 *
 * Relevant constants for the so-called "Trogdor" robots, by Botrics.
 * These values are taken from the 'cerebellum' module of CARMEN; thanks to
 * the authors of that module.
 */

#include <math.h>

#define TROGDOR_DEFAULT_PORT "/dev/usb/ttyUSB1"

// might need to define a longer delay to wait for acks
#define TROGDOR_DELAY_US 10000

/************************************************************************/
/* Physical constants, in meters, radians, seconds (unless otherwise noted) */
#define TROGDOR_LOOP_FREQUENCY  300.0
#define TROGDOR_AXLE_LENGTH    0.317
#define TROGDOR_WHEEL_DIAM     0.10795  /* 4.25 inches */
#define TROGDOR_WHEEL_CIRCUM   (TROGDOR_WHEEL_DIAM * M_PI)
#define TROGDOR_TICKS_PER_REV  5800.0
#define TROGDOR_M_PER_TICK     (TROGDOR_WHEEL_CIRCUM / TROGDOR_TICKS_PER_REV)
/* there's some funky timing loop constant for converting to/from speeds */
#define TROGDOR_MPS_PER_TICK   (TROGDOR_M_PER_TICK * 300.0)

/* assuming that the counts can use the full space of a signed 32-bit int */
#define TROGDOR_MAX_TICS 2147483648U

/* for safety */
#define TROGDOR_MAX_WHEELSPEED   1.0

/************************************************************************/
/* Comm protocol values */
#define TROGDOR_ACK   6 // if command acknowledged
#define TROGDOR_NACK 21 // if garbled message

#define TROGDOR_INIT1 253 // The init commands are used in sequence(1,2,3)
#define TROGDOR_INIT2 252 // to initialize a link to a cerebellum.
#define TROGDOR_INIT3 251 // It will then blink green and start accepting other 
                  // commands.

#define TROGDOR_DEINIT 250

#define TROGDOR_SET_VELOCITIES 118 // 'v'(left_vel, right_vel) as 16-bit signed ints
#define TROGDOR_SET_ACCELERATIONS 97 // 'a'(left_accel, right_accel) as 16-bit unsigned ints
#define TROGDOR_ENABLE_VEL_CONTROL  101 // 'e'()
#define TROGDOR_DISABLE_VEL_CONTROL 100 // 'd'()
#define TROGDOR_GET_ODOM            111 // 'o'()->(left_count, right_count, left_vel, right_vel)
#define TROGDOR_GET_VOLTAGE          98 // 'b'()->(batt_voltage)
#define TROGDOR_STOP                115 // 's'()  [shortcut for set_velocities(0,0)]
#define TROGDOR_KILL                107 // 'k'()  [shortcut for disable_velocity_control]
#define TROGDOR_HEARTBEAT           104 // 'h'() sends keepalive
/************************************************************************/


