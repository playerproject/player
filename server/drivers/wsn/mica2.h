/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006 Radu Bogdan Rusu (rusu@cs.tum.edu)
 *
 *  Portions copyright (c) 2004 Crossbow Technology, Inc.
 *  Distributed according to the Intel Open Source License.
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
 * TinyOS data structures.
 * Borrowed from the TinyOS project (http://www.tinyos.net).
 * Released under the Intel Open Source License.
 */

#include <vector>

// Change to 19200 for Mica2DOT (!)
#define DEFAULT_MICA2_PORT "/dev/ttyS0"
#define DEFAULT_MICA2_RATE B57600

// ---[ Node calibration values ]---
class NodeCalibrationValues
{
    public:
	unsigned int node_id;           // node identifier
	unsigned int group_id;          // group identifier
	int          c_values[6];       // calibration values
};
typedef std::vector<NodeCalibrationValues> NCV;

// ---[ MTS310 data packet structure ]---
typedef struct
{
    unsigned short vref;
    unsigned short thermistor;
    unsigned short light;
    unsigned short mic;
    unsigned short accelX;
    unsigned short accelY;
    unsigned short magX;
    unsigned short magY;
} MTS310Data;

// ---[ MTS510 data packet structure ]---
typedef struct
{
    unsigned short light;
    unsigned short accelX;
    unsigned short accelY;
    unsigned short sound[5];
} MTS510Data;

// ---[ Generic sensor data packet structure ]---
typedef struct
{
    unsigned char  board_id;        // unique sensorboard id
    unsigned char  packet_id;       // unique packet type for sensorboard
    unsigned char  node_id;         // ID of originating node
    unsigned char  parent;          // ID of node's parent
    unsigned short data[12];        // data payload defaults to 24 bytes
    unsigned char  terminator;      // reserved for null terminator
} SensorPacket;

// ---[ The standard header for all TinyOS active messages ]---
typedef struct
{
    unsigned short addr;
    unsigned char  type;
    unsigned char  group;
    unsigned char  length;
} __attribute__ ((packed)) TOSMsgHeader;

// ---[ Packet structure for XCOMMAND ]---
typedef struct
{
    unsigned short cmd;
    union
    {
	unsigned int  new_rate;    // XCOMMAND_SET_RATE
	unsigned int  node_id;     // XCOMMAND_SET_NODEID
	unsigned char group;       // XCOMMAND_SET_GROUP
	unsigned char rf_power;    // XCOMMAND_SET_RF_POWER
	unsigned char rf_channel;  // XCOMMAND_SET_RF_CHANNEL
	struct
	{
	    unsigned short device; // device: LEDs, speaker, etc
	    unsigned short state;  // state : on/off, etc
	} actuate;
    } param;
} __attribute__ ((packed)) XCommandOp;

typedef struct
{
    TOSMsgHeader tos;
    unsigned short seq_no;
    unsigned short destination_id;  // 0xFFFF for all
    XCommandOp     inst[1];
} __attribute__ ((packed)) XCommandMsg;

// ---[ Health data packet structure ]---
/*typedef struct
{
    unsigned short id;
    unsigned char  hop_count;
    unsigned char  send_est;
} DBGEstEntry;
typedef struct
{
    unsigned short node_id;
    unsigned short origin_addr;
    short          seq_no;
    unsigned char  hop_count;
    // HealthMsg
    unsigned char  est_entries;
    DBGEstEntry    est_list[4];
} HealthData;
*/

