/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     David Feil-Seifer
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
 *
 * Relevant constants for the "ER" robots, made by Evolution Robotics.    
 *
 * Some of this code is borrowed and/or adapted from the player
 * module of trogdor; thanks to the author of that module.
 */

#include <math.h>

#define ER_DEFAULT_PORT "/dev/usb/ttyUSB0"
#define ER_DEFAULT_LEFT_MOTOR 0
#define ER_DELAY_US 1000

#define ER_MOTOR_0_INIT		0x0071008F

/* all of these commands are for motor 0, need to be translated
	to motor 1 */
	
/* I believe that this sequence resets the odometry counts
 on the wheels to zero */
#define ER_ODOM_RESET_1		0x00C70039
#define ER_ODOM_RESET_2_1	0x00890077
#define ER_ODOM_RESET_2_2	0x0000
#define ER_ODOM_RESET_3_1	0x00800080
#define ER_ODOM_RESET_3_2	0x0000
#define ER_ODOM_RESET_4_1	0x005F00A0
#define ER_ODOM_RESET_4_2	0x0001
#define ER_ODOM_RESET_5_1	0x00EF0011
#define ER_ODOM_RESET_5_2	0x00000000
#define ER_ODOM_RESET_6_1	0x00700090
#define ER_ODOM_RESET_6_2	0x00000000
#define ER_ODOM_RESET_7_1	0x006F0091
#define ER_ODOM_RESET_7_2	0x00000000

/* asks for a 6 byte wheel respnse */
#define ER_ODOM_PROBE		0x00E3001D

/* execute command given to motor */
#define ER_MOTOR_EXECUTE_1	0x00E6001A

/* command string to maintain current speed */
#define ER_STOP_1_1	0x00CC0034
#define ER_STOP_1_2	0x0000
#define ER_STOP_2_1	0x00890077
#define ER_STOP_2_2	0x0000
#define ER_STOP_3_1	0x005F00A0
#define ER_STOP_3_2	0x0001
#define ER_STOP_4_1	0x002F00D0
#define ER_STOP_4_2	0x0001
#define ER_STOP_5_1	0x00EF0011
#define ER_STOP_5_2	0x00000000
#define ER_STOP_6_1	0x00700090
#define ER_STOP_6_2	0x00000000
#define ER_STOP_7_1	0x006F0091
#define ER_STOP_7_2	0x00000000
/* then execute, then*/
#define ER_STOP_8_1	0x00C70039

/*set velocity */
#define ER_SET_SPEED_1			0x00CF0031
#define ER_SET_SPEED_2_1		0x00C90034
#define ER_SET_SPEED_2_2		0x0300
#define ER_SET_SPEED_3_1		0x00940077
#define ER_SET_SPEED_3_2		0x6590
#define ER_SET_SPEED_4_1		0x005F00A0
#define ER_SET_SPEED_4_2		0x0001
/* fourth instruction created depending on the speed */
#define ER_SET_SPEED_6_1		0x00F20090
#define ER_SET_SPEED_6_2		0x0000007E


#define ER_SET_ACCL_1			0x00CF0031
#define ER_SET_ACCL_2_1			0x00C50034
#define ER_SET_ACCL_2_2			0x0700
#define ER_SET_ACCL_3_1			0x00F20090
#define ER_SET_ACCL_3_2			0x0000007A

/************************************************************************/
/* robot-specific info */

#define ER_DEFAULT_MOTOR_0_DIR		-1
#define ER_DEFAULT_MOTOR_1_DIR		1
#define ER_DEFAULT_AXLE_LENGTH		.38



#define ER_MAX_TICKS 3000

#define ER_WHEEL_RADIUS		.055
#define ER_WHEEL_CIRC		.345575197
#define ER_WHEEL_STEP		.45
#define ER_M_PER_TICK		.00000602836879

/* for safety */
#define ER_MAX_WHEELSPEED   500
#define ER_MPS_PER_TICK		1

/************************************************************************/
/* Comm protocol values */
#define ER_ACK		0x0000 // if command acknowledged
#define ER_NACK		0x1111 // if garbled message ??? This is wrong

                  // commands.

#define ER_GET_VOLTAGE_LOW		0x010F00EF
#define ER_GET_VOLTAGE_HIGH		0x0001

#define ER_ODOM_CHECK			0x00E3001D

#define FULL_STOP	0
#define STOP		1

/************************************************************************/
