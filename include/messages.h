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
 *   definitive source for the various message internal structures.
 */

#ifndef _MESSAGES_H
#define _MESSAGES_H

#include <playercommon.h>
#include <defaults.h>
#include <stdint.h>

/*********************************************************/

/* need to put this stuff somewhere else? maybe run-time config? */
#define PLAYER_NUM_SONAR_SAMPLES 16
#define PLAYER_NUM_LASER_SAMPLES 401
#define PLAYER_NUM_IDAR_SAMPLES  8 

/* the message start signifier */
#define PLAYER_STXX ((uint16_t) 0x5878)

/* the player message types */
#define PLAYER_MSGTYPE_DATA      ((uint16_t)1)
#define PLAYER_MSGTYPE_CMD       ((uint16_t)2)
#define PLAYER_MSGTYPE_REQ       ((uint16_t)3)
#define PLAYER_MSGTYPE_RESP_ACK  ((uint16_t)4)
#define PLAYER_MSGTYPE_SYNCH     ((uint16_t)5)
#define PLAYER_MSGTYPE_RESP_NACK ((uint16_t)6)
#define PLAYER_MSGTYPE_RESP_ERR  ((uint16_t)7)

/* strings to match the currently assigned devices (used for pretty-printing 
 * and command-line parsing) */
#define PLAYER_MAX_DEVICE_STRING_LEN 64

#define PLAYER_PLAYER_STRING         "player"
#define PLAYER_MISC_STRING           "misc"
#define PLAYER_GRIPPER_STRING        "gripper"
#define PLAYER_POSITION_STRING       "position"
#define PLAYER_SONAR_STRING          "sonar"
#define PLAYER_LASER_STRING          "laser"
#define PLAYER_VISION_STRING         "vision"
#define PLAYER_PTZ_STRING            "ptz"
#define PLAYER_AUDIO_STRING          "audio"
#define PLAYER_LASERBEACON_STRING    "laserbeacon"
#define PLAYER_BROADCAST_STRING      "broadcast"
#define PLAYER_SPEECH_STRING         "speech"
#define PLAYER_GPS_STRING            "gps"
#define PLAYER_BPS_STRING            "bps"
#define PLAYER_DESCARTES_STRING      "descartes"
#define PLAYER_IDAR_STRING           "idar"
#define PLAYER_MOTE_STRING           "mote"

/* the currently assigned device codes */
#define PLAYER_PLAYER_CODE         ((uint16_t)1)
#define PLAYER_MISC_CODE           ((uint16_t)2)
#define PLAYER_GRIPPER_CODE        ((uint16_t)3)
#define PLAYER_POSITION_CODE       ((uint16_t)4)
#define PLAYER_SONAR_CODE          ((uint16_t)5)
#define PLAYER_LASER_CODE          ((uint16_t)6)
#define PLAYER_VISION_CODE         ((uint16_t)7)
#define PLAYER_PTZ_CODE            ((uint16_t)8)
#define PLAYER_AUDIO_CODE          ((uint16_t)9)
#define PLAYER_LASERBEACON_CODE    ((uint16_t)10)
#define PLAYER_BROADCAST_CODE      ((uint16_t)11)
#define PLAYER_SPEECH_CODE         ((uint16_t)12)
#define PLAYER_GPS_CODE            ((uint16_t)13)
#define PLAYER_OCCUPANCY_CODE      ((uint16_t)14) // broken?
#define PLAYER_TRUTH_CODE          ((uint16_t)15)
#define PLAYER_BPS_CODE            ((uint16_t)16) // broken?
#define PLAYER_IDAR_CODE           ((uint16_t)17)
#define PLAYER_DESCARTES_CODE      ((uint16_t)18)
#define PLAYER_MOTE_CODE           ((uint16_t)19)

/* the access modes */
#define PLAYER_READ_MODE 'r'
#define PLAYER_WRITE_MODE 'w'
#define PLAYER_ALL_MODE 'a'
#define PLAYER_CLOSE_MODE 'c'
#define PLAYER_ERROR_MODE 'e'

/* the largest possible message that the server will currently send
 * or receive */
#define PLAYER_MAX_MESSAGE_SIZE 8192 /*8KB*/

/* maximum size for request/reply.
 * this is a convenience so that the PlayerQueue can used fixed size elements.
 * need to think about this a little
 */
#define PLAYER_MAX_REQREP_SIZE 1024 /*1KB*/

/* the default player port */
#define PLAYER_PORTNUM 6665

/*
 * info that is spit back as a banner on connection
 */
#define PLAYER_IDENT_STRING "Player v."
#define PLAYER_IDENT_STRLEN 32

#define PLAYER_KEYLEN 32

/* generic message header */
typedef struct
{
  uint16_t stx;     /* always equal to "xX" (0x5878) */
  uint16_t type;    /* message type */
  uint16_t device;  /* what kind of device */
  uint16_t device_index; /* which device of that kind */
  uint32_t time_sec;  /* server's current time (seconds since epoch) */
  uint32_t time_usec; /* server's current time (microseconds since epoch) */
  uint32_t timestamp_sec;  /* time when the current data/response was generated */
  uint32_t timestamp_usec; /* time when the current data/response was generated */
  uint32_t reserved;  /* for extension */
  uint32_t size;  /* size in bytes of the payload to follow */
} __attribute__ ((packed)) player_msghdr_t;

/*************************************************************************/
/*
 * The "Player" device
 */

/* the format of a general iotcl to Player */
/*
typedef struct
{
  uint16_t subtype;
} __attribute__ ((packed)) player_device_ioctl_t;
*/


/* the format of a "device request" ioctl to Player */
typedef struct
{
  uint16_t subtype;
  uint16_t code;
  uint16_t index;
  uint8_t access;
  //uint8_t consume;
} __attribute__ ((packed)) player_device_req_t;
  
/* the valid datamode codes */
#define PLAYER_DATAMODE_PUSH_ALL 0 // all data at fixed frequency
#define PLAYER_DATAMODE_PULL_ALL 1 // all data on demand
#define PLAYER_DATAMODE_PUSH_NEW 2 // only new new data at fixed freq
#define PLAYER_DATAMODE_PULL_NEW 3 // only new data on demand

/* the format of a "datamode change" ioctl to Player */
typedef struct
{
  uint16_t subtype;
  uint8_t mode;
} __attribute__ ((packed)) player_device_datamode_req_t;

/* the format of a "frequency change" ioctl to Player */
typedef struct
{
  // frequency in Hz
  uint16_t subtype;
  uint16_t frequency;
} __attribute__ ((packed)) player_device_datafreq_req_t;

/* the format of an authentication request */
typedef struct
{
  uint16_t subtype;
  char auth_key[PLAYER_KEYLEN];
} __attribute__ ((packed)) player_device_auth_req_t;

/* the format of a request for data (no args) */
typedef struct
{
  uint16_t subtype;
} __attribute__ ((packed)) player_device_data_req_t;

#define PLAYER_PLAYER_DEV_REQ     ((uint16_t)1)
#define PLAYER_PLAYER_DATA_REQ     ((uint16_t)2)
#define PLAYER_PLAYER_DATAMODE_REQ ((uint16_t)3)
#define PLAYER_PLAYER_DATAFREQ_REQ ((uint16_t)4)
#define PLAYER_PLAYER_AUTH_REQ     ((uint16_t)5)

/*************************************************************************/

/*************************************************************************/
/*
 * Position Device
 */

/* Position device command buffer */
typedef struct
{
  int16_t speed, sidespeed, turnrate;
} __attribute__ ((packed)) player_position_cmd_t;

/* Position device data buffer */
typedef struct 
{
  int32_t xpos,ypos;
  uint16_t theta;
  int16_t speed, sidespeed, turnrate;
  uint16_t compass;
  uint8_t stalls;
} __attribute__ ((packed)) player_position_data_t;


// RTV - don't much like this config model - it should be a single
// structured packet that contains all the state at once - but the c++ proxy
// is set up to do it this way. i guess the real P2OS device does it this way

/* the various configuration commands 
 * NOTE: these must not be the same as any other P2OS device! */
#define PLAYER_POSITION_MOTOR_POWER_REQ       ((uint8_t)1)
#define PLAYER_POSITION_VELOCITY_CONTROL_REQ ((uint8_t)2)
#define PLAYER_POSITION_RESET_ODOM_REQ        ((uint8_t)3)

typedef struct
{
  uint8_t request; // one of the above request types
  uint8_t value;  // value for the request (usually 0 or 1)
} __attribute__ ((packed)) player_position_config_t;
 
/*************************************************************************/


/*************************************************************************/
/*
 * Sonar Device
 */

/* the sonar data packet */
typedef struct
{
  uint16_t ranges[16]; /* start at the front left sonar and number clockwise */
} __attribute__ ((packed)) player_sonar_data_t;

typedef struct
{
  uint8_t cmd;
  uint8_t arg;
} __attribute__ ((packed)) player_sonar_config_t;

/* the various configuration commands 
 * NOTE: these must not be the same as any other P2OS device! */
#define PLAYER_SONAR_POWER_REQ      ((uint8_t)4)

/*************************************************************************/


/*************************************************************************/
/*
 * Gripper Device
 */

/* the gripper command packet */
typedef struct
{
  /* cmd is the command & arg is an optional arg used for some commands */
  uint8_t cmd, arg; 
} __attribute__ ((packed)) player_gripper_cmd_t;

/* the gripper data packet */
typedef struct
{
  uint8_t state, beams;
} __attribute__ ((packed)) player_gripper_data_t;


/*************************************************************************/


/*************************************************************************/
/*
 * Miscellaneous Device
 */

/* miscellaneous data packet */
typedef struct
{
  uint8_t frontbumpers, rearbumpers; /* bitfields; panels number clockwise */
  uint8_t voltage;  /* battery voltage in decivolts */
  uint8_t analog;
  uint8_t digin;
} __attribute__ ((packed)) player_misc_data_t;


/*************************************************************************/


/*************************************************************************/
/*
 * Laser Device
 */

/* The laser data packet. */
typedef struct
{
  /* Start and end angles for the laser scan (in units of 0.01
   * degrees). */
  int16_t min_angle;
  int16_t max_angle;

  /* Angular resolution (in units of 0.01 degrees).  */
  uint16_t resolution;

  /* Range readings.  <range_count> specifies the number of valid
   * readings.  Reflectivity data is stored in the top three bits of
   * each range reading.  */
  uint16_t range_count;
  uint16_t ranges[PLAYER_NUM_LASER_SAMPLES];
  
} __attribute__ ((packed)) player_laser_data_t;


/* Laser request subtypes. */
#define PLAYER_LASER_SET_CONFIG 0x01
#define PLAYER_LASER_GET_CONFIG 0x02


/* Laser configuration packet. */
typedef struct
{
  /* The packet subtype.  Set this to PLAYER_LASER_SET_CONFIG to set
   * the laser configuration; or set to PLAYER_LASER_GET_CONFIG to get
   * the laser configuration.  */
  uint8_t subtype;

  /* Start and end angles for the laser scan (in units of 0.01
   * degrees).  Valid range is -9000 to +9000.  */
  int16_t min_angle;
  int16_t max_angle;

  /* Scan resolution (in units of 0.01 degrees).  Valid resolutions
   * are 25, 50, 100.  */
  uint16_t resolution;

  /* Enable reflection intensity data. */
  uint8_t  intensity;
  
} __attribute__ ((packed)) player_laser_config_t;


/*************************************************************************/


/*************************************************************************/
/*
 * PTZ Device
 */

/*
 * the ptz command packet
 */
typedef struct
{
  int16_t pan;  /* -100 to 100 degrees. increases counterclockwise */
  int16_t tilt; /* -25 to 25 degrees. increases up */
  uint16_t zoom; /* 0 to 1023. 0 is wide, 1023 is telefoto */
} __attribute__ ((packed)) player_ptz_cmd_t;

/*
 * the ptz data packet
 */
typedef struct
{
  int16_t pan;  /* -100 to 100 degrees. increases counterclockwise */
  int16_t tilt; /* -25 to 25 degrees. increases up */
  uint16_t zoom; /* 0 to 1023. 0 is wide, 1023 is telefoto */
} __attribute__ ((packed)) player_ptz_data_t;

/*************************************************************************/


/*************************************************************************/
/*
 * Vision Device
 */

/*
 * vision data packet (it's more or less an ACTS v1.2 packet):
 *   128 - 5248 bytes of ACTS data, which is:
 *     128 bytes of header (4 bytes per channel)
 *     sequence of 16-byte blob data (maximum 10 per channel), which is:
 *        check the ACTS manual...
 * NOTE: 
 *   in the CVisionDevice 'data' buffer, there is also an extra leading
 *   short, representing size of ACTS data to follow; this size is NOT 
 *   sent over the network.
 */

/* ACTS size stuff; this stuff is only used between ACTS and Player */
#define ACTS_NUM_CHANNELS 32
#define ACTS_HEADER_SIZE_1_0 2*ACTS_NUM_CHANNELS  
#define ACTS_HEADER_SIZE_1_2 4*ACTS_NUM_CHANNELS  
#define ACTS_BLOB_SIZE_1_0 10
#define ACTS_BLOB_SIZE_1_2 16
#define ACTS_MAX_BLOBS_PER_CHANNEL 10

/* Vision device info; this defines Player's external interface */
#define VISION_NUM_CHANNELS ACTS_NUM_CHANNELS
#define VISION_MAX_BLOBS_PER_CHANNEL ACTS_MAX_BLOBS_PER_CHANNEL

typedef struct
{
  uint16_t index, num;
} __attribute__ ((packed)) player_vision_header_elt_t;

#define VISION_HEADER_SIZE \
  (sizeof(player_vision_header_elt_t)*VISION_NUM_CHANNELS)

typedef struct
{
  uint32_t area;
  uint16_t x, y;
  uint16_t left, right, top, bottom;
} __attribute__ ((packed)) player_vision_blob_elt_t;

#define VISION_BLOB_SIZE sizeof(player_vision_blob_elt_t)

typedef struct
{
  player_vision_header_elt_t header[VISION_NUM_CHANNELS];
  player_vision_blob_elt_t 
          blobs[VISION_MAX_BLOBS_PER_CHANNEL*VISION_NUM_CHANNELS];
} __attribute__ ((packed)) player_vision_data_t;

// Player needs 2 bytes to store the packet length
//typedef struct
//{
  //uint16_t size;
  //player_vision_data_t data;
//} __attribute__ ((packed)) player_internal_vision_data_t;
//
/*************************************************************************/



/*************************************************************************/
/*
 * Broadcast device
 */

/* Request packet sub-types */
#define PLAYER_BROADCAST_SUBTYPE_SEND 1
#define PLAYER_BROADCAST_SUBTYPE_RECV 2

/* Broadcast request/reply packet. */
typedef struct
{
  /* Packet subtype.  Set to PLAYER_BROADCAST_SUBTYPE_SEND to send a
   * broadcast messages.  Set to PLAYER_BROADCAST_SUBTYPE_RECV to read
   * the next message in the incoming message queue. */
  uint8_t subtype;

  /* The message to send, or the message that was received*/
  uint8_t data[PLAYER_MAX_REQREP_SIZE - 1];
  
} __attribute__ ((packed)) player_broadcast_msg_t;


/*************************************************************************/


/*************************************************************************/
/*
 * Speech Device
 */

/*
 * Speech data packet:
 *   uint8_t dummy: a dummy byte (because i think a device that returns
 *                  no data won't work very well right now - should fix
 *                  that)
 */
typedef struct
{
  uint8_t dummy;
} __attribute__ ((packed)) player_speech_data_t;

/*
 * Speech command packet:
 *   ASCII string to say  (max length is in defaults.h)
 */
typedef struct
{
  uint8_t string[SPEECH_MAX_STRING_LEN];
} __attribute__ ((packed)) player_speech_cmd_t;

/*************************************************************************/


/*************************************************************************/
/*
 * GPS Device
 *
 *   A global positioning device.
 *   Use the config command to teleport the robot in the simulator.
 */

/*
 * GPS data packet:
 *   xpos, ypos: current global position (in mm).
 *   heading: current global heading (in degrees).
 */
typedef struct
{
  int32_t xpos,ypos;
  int32_t heading;
} __attribute__ ((packed)) player_gps_data_t;


/*
 * GPS config packet:
 *   xpos, ypos: new global position (in mm).
 *   heading: new global heading (in degrees).
 */
typedef struct
{
  int32_t xpos,ypos;
  int32_t heading;
} __attribute__ ((packed)) player_gps_config_t;


/*************************************************************************/


/*************************************************************************/
/*
 * Laser beacon device
 */

/* The laser beacon data packet (one beacon). */
typedef struct
{
  /* The beacon id.  Beacons that cannot be identified get id 0. */
  uint8_t id;

  /* Beacon range (in mm) relative to the laser. */
  uint16_t range;

  /* Beacon bearing and orientation (in degrees) relative to laser. */
  int16_t bearing;
  int16_t orient;
  
} __attribute__ ((packed)) player_laserbeacon_item_t;


#define PLAYER_MAX_LASERBEACONS 32

/* The laser beacon data packet (all beacons). */
typedef struct 
{
  /* List of detected beacons */
  uint16_t count;
  player_laserbeacon_item_t beacon[PLAYER_MAX_LASERBEACONS];
  
} __attribute__ ((packed)) player_laserbeacon_data_t;


/* Request packet subtypes */
#define PLAYER_LASERBEACON_SUBTYPE_SETCONFIG 0x01
#define PLAYER_LASERBEACON_SUBTYPE_GETCONFIG 0x02


/* Laser beacon request/reply packet. */
typedef struct
{
  /* Packet subtype.  Set to PLAYER_LASERBEACON_SUBTYPE_SETCONFIG to
   *  set the device configuration.  Set to
   *  PLAYER_LASERBEACON_SUBTYPE_GETCONFIG to get the device
   *  configuration. */
  uint8_t subtype;

  /* The number of bits in the beacon, including start and end
   * markers. */
  uint8_t bit_count;

  /* The width of each bit, in mm. */
  uint16_t bit_size;

  /* Bit detection thresholds.  <zero_thresh> is the minimum threshold
   * for declaring a bit is zero (0-100).  <one_thresh> is the minimum
   * threshold for declaring a bit is one (0-100). */
  uint16_t zero_thresh;
  uint16_t one_thresh;

} __attribute__ ((packed)) player_laserbeacon_config_t;


/*************************************************************************/


/*************************************************************************/
/*
 * BPS Device
 *
 *   A global positioning device using laser beacons (hence *B*PS)
 *   Dont forget to specify the beacon positions (using the setbeacon request).
 */

/* BPS data packet:
 *   (px, py, pa): current global pose (mm, mm, degrees).
 *   (ux, uy, ua): uncertainty (mm, mm, degrees).
 *   (err): residual error in estimate (x 1e6)
 */
typedef struct
{
  int32_t px, py, pa;
  int32_t ux, uy, ua;
  int32_t err;
} __attribute__ ((packed)) player_bps_data_t;


/* BPS request packet: set gain terms.
 * subtype : must be PLAYER_BPS_SUBTYPE_GAIN
 * gain : gain * 1e6
 */
typedef struct
{
    uint8_t subtype;
    uint32_t gain;
} __attribute__ ((packed)) player_bps_setgain_t;


/* BPS request packet: set the pose of the laser
 * (relative to robot origin in odometric cs)
 * subtype : PLAYER_BPS_SUBTYPE_SETLASER
 * px, py, pa : laser pose (mm, mm, degrees)
 */
typedef struct
{
    uint8_t subtype;
    int32_t px, py, pa;
} __attribute__ ((packed)) player_bps_setlaser_t;


/* BPS request packet: set the pose of a beacon.
 * subtype : must by PLAYER_BPS_SUBTYPE_SETBEACON
 * id : beacon id
 * px, py, pa : beacon pose (mm, mm, degrees)
 * ux, uy, ua : unceratinty in beacon pose (mm, mm, degrees)
 */
typedef struct
{
  uint8_t subtype;
  uint8_t id;
  int32_t px, py, pa;
  int32_t ux, uy, ua;
} __attribute__ ((packed)) player_bps_setbeacon_t;


/* Request packet subtypes
 */
#define PLAYER_BPS_SUBTYPE_SETGAIN 1
#define PLAYER_BPS_SUBTYPE_SETBEACON 2
#define PLAYER_BPS_SUBTYPE_SETLASER 3


/*************************************************************************/


/*************************************************************************/
/*
 * IDAR device - HRL's infrared data and ranging turret
 */

#define IDARBUFLEN 16
#define RAYS_PER_SENSOR 5

#define IDAR_INSTRUCTION_TRANSMIT 0
#define IDAR_INSTRUCTION_RECEIVE 1
#define IDAR_INSTRUCTION_RECEIVE_NOFLUSH 2

typedef struct
{
  unsigned char mesg[IDARBUFLEN];
  uint8_t len; //0-255
  uint8_t intensity; //0-255
  uint8_t directions; // each set bit means send in that direction
} __attribute__ ((packed)) idartx_t;

// WARNING - if( PLAYER_NUM_IDAR_SAMPLES > 8 ) you need to increase
// the number of bits in the directions member...
typedef struct
{
  unsigned char mesg[IDARBUFLEN];
  uint8_t len; //0-255
  uint8_t intensity; //0-255
  uint8_t reflection; // true/false
  uint32_t timestamp_sec;
  uint32_t timestamp_usec;
  uint16_t ranges[ RAYS_PER_SENSOR ]; // useful for debugging & visualization
} __attribute__ ((packed)) idarrx_t; 

// IDAR data packet - contains messages received and messages sent
// since last read
typedef struct
{
  idarrx_t rx[PLAYER_NUM_IDAR_SAMPLES];
  //idartx_t tx[PLAYER_NUM_IDAR_SAMPLES];
} __attribute__ ((packed)) player_idar_data_t;

// IDAR command packet - contains a message (possibly null) for each
//typedef  struct
//{
//idartx_t tx[PLAYER_NUM_IDAR_SAMPLES];
//} __attribute__ ((packed)) player_idar_cmd_t;

// IDRAR config packet - 
// has room for a message in case this is a transmit command
// we use config because it is consumed by default
// and the messages must only be sent once
typedef  struct
{
  uint8_t instruction;
  idartx_t tx;
} __attribute__ ((packed)) player_idar_config_t;


/*************************************************************************/
/*
 * Descartes Device - a small holonomic robot with bumpers
 */

/* command buffer */
typedef struct
{
  int16_t speed, heading, distance; // UNITS: mm/sec, degrees, mm
} __attribute__ ((packed)) player_descartes_cmd_t;

/* data buffer */
typedef struct 
{
  int32_t xpos,ypos; // mm, mm
  int16_t theta; //  degrees
  uint8_t bumpers[2]; // booleans
} __attribute__ ((packed)) player_descartes_data_t;

// no configs for descartes

/*************************************************************************/

/*************************************************************************/
/*
 * Truth device, used for getting and setting data about all entities in Stage
 * for example by a GUI or a ground-truth logging system
 */

// identify an entity to Player
typedef struct
{
  uint16_t port;
  uint16_t index;
  uint16_t type;
} player_id_t;  

// packet for truth device
typedef struct
{
  player_id_t id;
  player_id_t parent;
  
  uint32_t x, y; // mm, mm
  uint16_t th, w, h; // degrees, mm, mm
} __attribute__ ((packed)) player_generic_truth_t;

/*************************************************************************/

/*************************************************************************/
/*
 * Occupancy device, exports the world background as an occupancy grid
 */

typedef struct
{
  uint16_t width, height, ppm;
  uint32_t num_pixels;
  //uint16_t num_truths;
} __attribute__ ((packed)) player_occupancy_data_t;

typedef struct
{
  uint16_t x, y, color;
} __attribute__ ((packed)) pixel_t;

/**************************************************************************
 * the Audio device; recognizes and generates fixed-frequency tones with
 * sound hardware
 */
#define AUDIO_DATA_BUFFER_SIZE 20
#define AUDIO_COMMAND_BUFFER_SIZE 3*sizeof(short)
/**************************************************************************/



/*************************************************************************
*
*  Mote radio device
* 
*/

#define MAX_MOTE_DATA_SIZE 32
#define MAX_MOTE_Q_LEN     10

typedef struct
{
  uint8_t len;
  uint8_t buf[MAX_MOTE_DATA_SIZE];
  float   rssi;
} __attribute__ ((packed)) player_mote_data_t;

typedef struct
{
  uint8_t strength;
} __attribute__ ((packed)) player_mote_config_t;


#endif


