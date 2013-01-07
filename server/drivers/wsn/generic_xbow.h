/********************************************************************
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 ********************************************************************/
/*********************************************************************
 * TinyOS data structures.
 * Portions borrowed from the TinyOS project (http://www.tinyos.net), 
 * distributed according to the Intel Open Source License.
 *********************************************************************/
/***************************************************************************
 * Desc: Driver for generic Crossbow WSN nodes
 * Author: Adrian Jimenez Gonzalez, Jose Manuel Sanchez Matamoros
 * Date: 15 Aug 2011
 **************************************************************************/

#ifndef _GENERIC_XBOW_TYPES_H
#define _GENERIC_XBOW_TYPES_H

#define DEFAULT_GENERICXBOW_PORT "/dev/ttyUSB0"
#define DEFAULT_GENERICXBOW_PLATFORM "telosb"
#define DEFAULT_GENERICXBOW_OS "tos2x"

#ifndef UNIQUE_TIMER
#define UNIQUE_TIMER unique("Timer")
#endif

#define FIXED_UPDATE_INTERVAL 20

#define STATIC_DELAY 40

#define MAX_PAYLOAD 42
#define MAX_TOS_PAYLOAD 35
#define MAX_TRANSP_SIZE 100

#define WSN_PLAYER_HEADER_COUNT 4

#include <cstdio>
#include <iostream>
#include "mote/MoteIF.h"

using namespace mote;
using namespace std;

enum { MICA2DOT, MICA2, MICAZ, IRIS, TELOS, TELOSB, TMOTE, EYES, INTELMOTE2 };

// AM for different message interface
enum{
	AM_MOTE_MESSAGE = 10, // Node to pc messages
	AM_BASE_MESSAGE = 11, // Node to base messages
 	AM_PLAYER_TO_WSN = 11, // PC to Node messages	
};

// appID for different message origins
enum{
	ID_MOBILE_DATA = 1,		// Mobile node message appId
	ID_HEALTH = 2,			// Health message appId
	ID_FIXED_DATA = 3,		// Fixed node message appId
};

//// Message definitions

// XMesh header.
typedef struct {
	uint16_t orig;
	uint16_t source;
	uint16_t seq;
	uint8_t appId;
} __attribute__ ((packed)) XMeshHeader;

//! Messages between wsn and a robot

// Health message
typedef struct {
	XMeshHeader header;
	uint16_t id;
	uint16_t parent_id;
} __attribute__((packed)) HealthMsg;

// Beacon message
typedef struct {
	uint8_t type;
	uint8_t node_id;
	uint8_t sender_id; // RSSI sender
	uint16_t rssi;
	uint16_t stamp;
	uint32_t timelow;
 	uint32_t timehigh;
	float x;
	float y;
	float z;
} __attribute__((packed)) RSSIBeaconMsg;

typedef struct {
	uint8_t type;
	uint16_t id;
	uint16_t parent_id;
	float x;
	float y;
	float z;
	uint8_t status;
	
} __attribute__((packed)) PositionMsg;


// Individual sensor description
typedef struct {
	uint8_t type;
	int16_t value;
} __attribute__((packed)) sensor_t;

// Sensor or Alarm message
typedef struct {
	uint8_t type;
	uint16_t id;
	uint16_t parent_id;
	uint8_t sensor_count;
	sensor_t *sensor;
} __attribute__((packed)) SensorMsg;

// User defined data message
typedef struct {
	uint8_t type;
	uint16_t id;
	uint16_t parent_id;
	uint8_t data_size;
	uint8_t *data;
} __attribute__((packed)) UserDataMsg;

// Request Message 
typedef struct {
	uint8_t type;
	uint16_t id;
	uint16_t parent_id;
	uint8_t request;
	uint8_t parameters_size;
	uint8_t *parameters;
} __attribute__((packed)) RequestMsg;

// Command Message 
typedef struct {
	uint8_t type;
	uint16_t id;
	uint16_t parent_id;
	uint8_t command;
	uint8_t parameters_size;
	uint8_t *parameters;
} __attribute__((packed)) CommandMsg;

#endif
