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

/*********************************************************/
/* various device-specific defaults */

#define DEFAULT_LASER_PORT "/dev/ttyS1"

#define DEFAULT_ACTS_PORT 5001
#define DEFAULT_ACTS_CONFIGFILE "/usr/local/acts/actsconfig"

#define DEFAULT_PTZ_PORT "/dev/ttyS2"

#define DEFAULT_P2OS_PORT "/dev/ttyS0"

// don't change this unless you change the Festival init scripts as well
#define DEFAULT_FESTIVAL_PORTNUM 1314
// change this if Festival is installed somewhere else
#define DEFAULT_FESTIVAL_LIBDIR "/usr/local/festival/lib"
/*********************************************************/

/* need to put this stuff somewhere else? maybe run-time config? */
#define PLAYER_NUM_SONAR_SAMPLES 16
#define PLAYER_NUM_LASER_SAMPLES 401

/* the message start signifier */
#define PLAYER_STXX ((uint16_t) 0x5878)

/* the player message types */
#define PLAYER_MSGTYPE_DATA  ((uint16_t)1)
#define PLAYER_MSGTYPE_CMD   ((uint16_t)2)
#define PLAYER_MSGTYPE_REQ   ((uint16_t)3)
#define PLAYER_MSGTYPE_RESP  ((uint16_t)4)

/* the currently assigned device codes */
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
#define PLAYER_LASERBEACON_CODE   ((uint16_t)10)
#define PLAYER_BROADCAST_CODE     ((uint16_t)11)
#define PLAYER_SPEECH_CODE        ((uint16_t)12)
#define PLAYER_GPS_CODE           ((uint16_t)13) // broken?
#define PLAYER_OCCUPANCY_CODE     ((uint16_t)14) // broken?
#define PLAYER_TRUTH_CODE         ((uint16_t)15)
#define PLAYER_BPS_CODE           ((uint16_t)16) // broken?

/* the access modes */
#define PLAYER_READ_MODE 'r'
#define PLAYER_WRITE_MODE 'w'
#define PLAYER_ALL_MODE 'a'
#define PLAYER_CLOSE_MODE 'c'

/* the largest possible message that the server will currently send
 * or receive */
#define PLAYER_MAX_MESSAGE_SIZE 8192 /*8KB*/
//#define PLAYER_MAX_MESSAGE_SIZE 1048575 /*1MB*/

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
typedef struct
{
  uint16_t subtype;
} __attribute__ ((packed)) player_device_ioctl_t;


/* the format of a "device request" ioctl to Player */
typedef struct
{
  uint16_t code;
  uint16_t index;
  uint8_t access;
} __attribute__ ((packed)) player_device_req_t;

/* the format of a "datamode change" ioctl to Player */
typedef struct
{
  // 0 = continuous (default)
  // 1 = request/reply
  uint8_t mode;
} __attribute__ ((packed)) player_device_datamode_req_t;

/* the format of a "frequency change" ioctl to Player */
typedef struct
{
  // frequency in Hz
  uint16_t frequency;
} __attribute__ ((packed)) player_device_datafreq_req_t;

/* the format of an authentication request */
typedef struct
{
  char auth_key[PLAYER_KEYLEN];
} __attribute__ ((packed)) player_device_auth_req_t;

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
  int16_t speed, turnrate;
} __attribute__ ((packed)) player_position_cmd_t;

/* Position device data buffer */
typedef struct 
{
  int32_t xpos,ypos;
  uint16_t theta;
  int16_t speed, turnrate;
  uint16_t compass;
  uint8_t stalls;
} __attribute__ ((packed)) player_position_data_t;


/* TODO: need to make structured buffers for config requests */

/* the various configuration commands 
 * NOTE: these must not be the same as any other P2OS device! */
#define PLAYER_POSITION_MOTOR_POWER_REQ       ((uint8_t)1)
#define PLAYER_POSITION_VELOCITY_CONTROL_REQ ((uint8_t)2)
#define PLAYER_POSITION_RESET_ODOM_REQ        ((uint8_t)3)
 
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
} __attribute__ ((packed)) player_misc_data_t;


/*************************************************************************/


/*************************************************************************/
/*
 * Laser Device
 */

/*
   the laser data packet
   
   <min_angle> and <max_angle> specify the start and end angles
   (in units of 0.01 degrees).  Valid range is -9000 to +9000.
   <resolution> specifies the resolution (in units of 0.01 degrees).
   Valid resolutions are 25, 50, 100.
   <samples> indicates the total nuumber of samples.
   <ranges> are in mm and start from <min_angle>.
 */
typedef struct
{
  int16_t min_angle;
  int16_t max_angle;
  uint16_t resolution;
  uint16_t range_count;
  uint16_t ranges[PLAYER_NUM_LASER_SAMPLES];
} __attribute__ ((packed)) player_laser_data_t;

/*
   the laser configuration packet

   <min_angle> and <max_angle> specify the start and end angles
   (in units of 0.01 degrees).  Valid range is -9000 to +9000.
   <resolution> specifies the resolution (in units of 0.01 degrees).
   Valid resolutions are 25, 50, 100.
   If <intensity> is set reflection intensity values will be returned
   in the top 3 bits of the range data.
*/
typedef struct
{
  int16_t min_angle;
  int16_t max_angle;
  uint16_t resolution;
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
 * vision data packet (it's just an ACTS packet):
 *   64 - 3264 bytes of ACTS data, which is:
 *     64 bytes of header (2 bytes per channel)
 *     sequence of 10-byte blob data (maximum 10 per channel), which is:
 *        check the ACTS manual...
 * NOTE: 
 *   in the CVisionDevice 'data' buffer, there is also an extra leading
 *   short, representing size of ACTS data to follow; this size is NOT 
 *   sent over the network.
 */

/* ACTS size stuff */
#define ACTS_NUM_CHANNELS 32
#define ACTS_HEADER_SIZE 2*ACTS_NUM_CHANNELS  
#define ACTS_BLOB_SIZE 10
#define ACTS_MAX_BLOBS_PER_CHANNEL 10
#define ACTS_MAX_BLOB_DATA_SIZE \
  ACTS_NUM_CHANNELS*ACTS_BLOB_SIZE*ACTS_MAX_BLOBS_PER_CHANNEL
#define ACTS_TOTAL_MAX_SIZE \
  ACTS_MAX_BLOB_DATA_SIZE+ACTS_HEADER_SIZE

typedef struct
{
  uint8_t header[ACTS_HEADER_SIZE];
  uint8_t blobs[ACTS_MAX_BLOB_DATA_SIZE];
} __attribute__ ((packed)) player_vision_data_t;

// Player needs 2 bytes to store the packet length
typedef struct
{
  uint16_t size;
  player_vision_data_t data;
} __attribute__ ((packed)) player_internal_vision_data_t;

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
 *   1-256 byte string: ASCII string to say
 */

/* Festival size stuff */
#define SPEECH_MAX_STRING_LEN 256
#define SPEECH_MAX_QUEUE_LEN 1

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

/*
 * the laser beacon data packet (one beacon)
 */
typedef struct
{
    uint8_t id;
    uint16_t range;
    int16_t bearing;
    int16_t orient;
} __attribute__ ((packed)) player_laserbeacon_item_t;

#define PLAYER_MAX_LASERBEACONS 32
/*
 * the laser beacon data packet (all beacons)
 */
typedef struct 
{
    uint16_t count;
    player_laserbeacon_item_t beacon[PLAYER_MAX_LASERBEACONS]; 
} __attribute__ ((packed)) player_laserbeacon_data_t;

/*
 * the laser beacon config packet
 */
typedef struct
{
    uint8_t bit_count;
    uint16_t bit_size;
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
#define PLAYER_BPS_SUBTYPE_SETLASER 2
#define PLAYER_BPS_SUBTYPE_SETBEACON 3



/*************************************************************************/


/*************************************************************************/
/*
 * Broadcast device
 */

// Broadcast command packet
//
typedef struct
{
    uint16_t len;
    uint8_t  msg[4096];
} __attribute ((packed)) player_broadcast_cmd_t;


// Data packet
// Each packet may contain multiple messages.
// Messages are concatenated in the buffer, with each message having the
// following format.
//   message length -- two byte unsigned int
//   message body -- arbitrary length data
// The list is terminated by a message with zero length.
//
typedef struct
{
    uint8_t  buffer[4096];
} __attribute ((packed)) player_broadcast_data_t;


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
} __attribute ((packed)) player_generic_truth_t;

/*************************************************************************/
/*************************************************************************/
/*
 * Occupancy device, exports the world background as an occupancy grid
 */

typedef struct
{
  uint16_t width, height, ppm;
  uint32_t num_pixels;
  
} __attribute ((packed)) player_occupancy_data_t;

typedef struct
{
  uint16_t x, y, color;
} __attribute ((packed)) pixel_t;


#endif

