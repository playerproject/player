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
 * Desc: This is just a wrapper for the messages.h include, because I figure
 *       we should have an include file with a meaningful name.
 * CVS:  $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <playercommon.h>
#include <stdint.h>

/*********************************************************/

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

/* the currently assigned interface codes */
#define PLAYER_PLAYER_CODE         ((uint16_t)1)   // the server itself
#define PLAYER_POWER_CODE          ((uint16_t)2)   // power subsystem
#define PLAYER_GRIPPER_CODE        ((uint16_t)3)   // gripper
#define PLAYER_POSITION_CODE       ((uint16_t)4)   // device that moves about
#define PLAYER_FRF_CODE            ((uint16_t)5)   // fixed range-finder
#define PLAYER_SRF_CODE            ((uint16_t)6)   // scanning range-finder
#define PLAYER_BLOBFINDER_CODE     ((uint16_t)7)   // visual blobfinder
#define PLAYER_PTZ_CODE            ((uint16_t)8)   // pan-tilt-zoom unit
#define PLAYER_AUDIO_CODE          ((uint16_t)9)   // audio I/O
#define PLAYER_FIDUCIAL_CODE       ((uint16_t)10)  // fiducial detector
#define PLAYER_COMMS_CODE          ((uint16_t)11)  // inter-Player radio I/O
#define PLAYER_SPEECH_CODE         ((uint16_t)12)  // speech I/O
#define PLAYER_GPS_CODE            ((uint16_t)13)  // GPS unit
#define PLAYER_BUMPER_CODE         ((uint16_t)14)  // bumper array
#define PLAYER_TRUTH_CODE          ((uint16_t)15)  // ground-truth (via Stage)
#define PLAYER_IDARTURRET_CODE     ((uint16_t)16)  // ranging + comms
#define PLAYER_IDAR_CODE           ((uint16_t)17)  // ranging + comms
// Descartes should be subsumed by position
#define PLAYER_DESCARTES_CODE      ((uint16_t)18)  // the Descartes platform
// Mote should be subsumed by comms?
#define PLAYER_MOTE_CODE           ((uint16_t)19)  // the USC Mote
#define PLAYER_DIO_CODE            ((uint16_t)20)  // digital I/O
#define PLAYER_AIO_CODE            ((uint16_t)21)  // analog I/O

// no interface has yet been defined for BPS-like things
//#define PLAYER_BPS_CODE            ((uint16_t)16)

/* the currently assigned device strings */
#define PLAYER_PLAYER_STRING         "player"
#define PLAYER_POWER_STRING          "power"
#define PLAYER_GRIPPER_STRING        "gripper"
#define PLAYER_POSITION_STRING       "position"
#define PLAYER_FRF_STRING            "frf"
#define PLAYER_SRF_STRING            "srf"
#define PLAYER_BLOBFINDER_STRING     "blobfinder"
#define PLAYER_PTZ_STRING            "ptz"
#define PLAYER_AUDIO_STRING          "audio"
#define PLAYER_FIDUCIAL_STRING       "fiducial"
#define PLAYER_COMMS_STRING          "comms"
#define PLAYER_SPEECH_STRING         "speech"
#define PLAYER_GPS_STRING            "gps"
#define PLAYER_BUMPER_STRING         "bumper"
#define PLAYER_TRUTH_STRING          "truth"
#define PLAYER_IDARTURRET_STRING     "idarturret"
#define PLAYER_IDAR_STRING           "idar"
#define PLAYER_DESCARTES_STRING      "descartes"
#define PLAYER_MOTE_STRING           "mote"
#define PLAYER_DIO_STRING            "dio"
#define PLAYER_AIO_STRING            "aio"
//#define PLAYER_BPS_STRING            "bps"

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

/* a device identifier; devices are differentiated internally in Player by 
 * these ids
 */
typedef struct
{
  uint16_t code;
  uint16_t index;
  uint16_t port;
} __attribute__ ((packed)) player_device_id_t;

/*************************************************************************/
/*
 * The "Player" device
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
 * Power interface
 */
typedef struct
{
  uint16_t  charge;
} __attribute__ ((packed)) player_power_data_t;

// TODO: what does this do?
#define PLAYER_MAIN_POWER_REQ               ((uint8_t)14)
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
 * Position interface
 */

/* Position device command buffer */
typedef struct
{
  int32_t xpos, ypos, zpos;
  int32_t yaw, pitch, roll;
  int32_t xspeed, yspeed, zspeed;
  int32_t yawspeed, pitchspeed, rollspeed;
} __attribute__ ((packed)) player_position_cmd_t;

/* Position device data buffer */
typedef struct 
{
  int32_t xpos, ypos, zpos;
  int32_t yaw, pitch, roll;
  int32_t xspeed, yspeed, zspeed;
  int32_t yawspeed, pitchspeed, rollspeed;
  uint8_t stall;
} __attribute__ ((packed)) player_position_data_t;

/* the various configuration commands */
#define PLAYER_POSITION_GET_GEOM_REQ          ((uint8_t)1)

/* Packet for getting the position geometry. */
typedef struct
{
  /* Packet subtype.  Must be PLAYER_POSITION_GET_GEOM_REQ. */
  uint8_t subtype;

  /* Pose of the robot base, in the robot cs (mm, mm, degrees). */
  uint16_t pose[3];

  /* Dimensions of the base (mm, mm). */
  uint16_t size[2];
} __attribute__ ((packed)) player_position_geom_t;
/*************************************************************************/


/*************************************************************************/
/*
 * Fixed range-finder (FRF) interface
 */
#define PLAYER_FRF_MAX_SAMPLES 32

/* the frf data packet */
typedef struct
{
  /* The number of valid range readings. */
  uint16_t range_count;
  
  /* for the Pioneer, start at the front left sonar and number clockwise */
  uint16_t ranges[PLAYER_FRF_MAX_SAMPLES];
} __attribute__ ((packed)) player_frf_data_t;

#define PLAYER_FRF_GET_GEOM_REQ   ((uint8_t)1)

/* Packet for getting the FRF geometry. */
typedef struct
{
  /* Packet subtype.  Must be PLAYER_FRF_GET_GEOM_REQ. */
  uint8_t subtype;

  /* The number of valid poses. */
  uint16_t pose_count;

  /* Pose of each sonar, in robot cs (mm, mm, degrees). */
  int16_t poses[PLAYER_FRF_MAX_SAMPLES][3];
  
} __attribute__ ((packed)) player_frf_geom_t;
/*************************************************************************/

/*************************************************************************/
/*
 * Scanning range-finder (SRF) interface
 */

#define PLAYER_SRF_MAX_SAMPLES  401

/* The srf data packet. */
typedef struct
{
  /* Start and end angles for the srf scan (in units of 0.01 degrees). */
  int16_t min_angle;
  int16_t max_angle;

  /* Angular resolution (in units of 0.01 degrees).  */
  uint16_t resolution;

  /* Range readings.  <range_count> specifies the number of valid
   * readings.  Reflectivity data is stored in the top three bits of
   * each range reading.  */
  uint16_t range_count;
  uint16_t ranges[PLAYER_SRF_MAX_SAMPLES];
} __attribute__ ((packed)) player_srf_data_t;


/* Laser request subtypes. */
#define PLAYER_SRF_GET_GEOM   0x01
#define PLAYER_SRF_SET_CONFIG 0x02
#define PLAYER_SRF_GET_CONFIG 0x03

/* SRF geometry packet. */
typedef struct
{
  /* The packet subtype.  Must be PLAYER_SRF_GET_GEOM. */
  uint8_t subtype;

  /* SRF pose, in robot cs (mm, mm, radians). */
  int16_t pose[3];

  /* SRF dimensions (mm, mm). */
  int16_t size[2];
  
} __attribute__ ((packed)) player_srf_geom_t;


/* SRF configuration packet. */
typedef struct
{
  /* The packet subtype.  Set this to PLAYER_SRF_SET_CONFIG to set
   * the laser configuration; or set to PLAYER_SRF_GET_CONFIG to get
   * the laser configuration.  */
  uint8_t subtype;

  /* Start and end angles for the srf scan (in units of 0.01
   * degrees).  Valid range is -9000 to +9000.  */
  int16_t min_angle;
  int16_t max_angle;

  /* Scan resolution (in units of 0.01 degrees).  Valid resolutions
   * are 25, 50, 100.  */
  uint16_t resolution;

  /* Enable reflection intensity data. */
  uint8_t  intensity;
} __attribute__ ((packed)) player_srf_config_t;
/*************************************************************************/

/*************************************************************************/
/*
 * Blobfinder interface
 */

// TODO: move data format to a flat list of blobs

#define PLAYER_BLOBFINDER_MAX_CHANNELS 32
#define PLAYER_BLOBFINDER_MAX_BLOBS_PER_CHANNEL 10

typedef struct
{
  uint16_t index, num;
} __attribute__ ((packed)) player_blobfinder_header_elt_t;

#define PLAYER_BLOBFINDER_HEADER_SIZE \
  (2*sizeof(uint16_t) + sizeof(player_blobfinder_header_elt_t)*PLAYER_BLOBFINDER_MAX_CHANNELS)

typedef struct
{
  /* A descriptive color for the blob (useful for gui's).
   * The color is stored as packed 32-bit RGB, i.e., 0x00RRGGBB. */
  uint32_t color;

  /* The blob area (pixels). */
  uint32_t area;

  /* The blob centroid (image coords). */
  uint16_t x, y;

  /* Bounding box for the blob (image coords). */
  uint16_t left, right, top, bottom;

  /* Range in mm to the blob center */
  uint16_t range;
} __attribute__ ((packed)) player_blobfinder_blob_elt_t;

#define PLAYER_BLOBFINDER_BLOB_SIZE sizeof(player_blobfinder_blob_elt_t)

typedef struct
{
  /* The image dimensions. */
  uint16_t width, height;

  /* The blobs (indexed by channel). */
  player_blobfinder_header_elt_t header[PLAYER_BLOBFINDER_MAX_CHANNELS];
  player_blobfinder_blob_elt_t 
          blobs[PLAYER_BLOBFINDER_MAX_BLOBS_PER_CHANNEL*
                PLAYER_BLOBFINDER_MAX_CHANNELS];
} __attribute__ ((packed)) player_blobfinder_data_t;
/*************************************************************************/

/*************************************************************************/
/*
 * PTZ interface
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

/**************************************************************************
 * The Audio interface
 */
//TODO: fill out the data and command formats

#define AUDIO_DATA_BUFFER_SIZE 20
#define AUDIO_COMMAND_BUFFER_SIZE 3*sizeof(short)
/*************************************************************************/

/*************************************************************************/
/*
 * Fiducial interface
 */

#define PLAYER_FIDUCIAL_MAX_SAMPLES 32

/* The fiducial data packet (one fiducial). */
typedef struct
{
  /* The fiducial id.  Fiducials that cannot be identified get id 0. */
  uint8_t id;

  /* Fiducial range (in mm) relative to the detector. */
  uint16_t range;

  /* Fiducial bearing and orientation (in degrees) relative to detector. */
  int16_t bearing;
  int16_t orient;
  
} __attribute__ ((packed)) player_fiducial_item_t;


/* The fiducial data packet (all fiducials). */
typedef struct 
{
  /* List of detected fiducials */
  uint16_t count;
  player_fiducial_item_t fiducials[PLAYER_FIDUCIAL_MAX_SAMPLES];
  
} __attribute__ ((packed)) player_fiducial_data_t;


/* Request packet subtypes */
#define PLAYER_FIDUCIAL_GET_GEOM   0x01

/* Fiducial geometry packet. */
typedef struct
{
  /* Packet subtype.  Must be PLAYER_FIDUCIAL_GET_GEOM. */
  uint8_t subtype;

  /* Pose of the detector, in the robot cs (mm, mm, degrees). */
  uint16_t pose[3];
} __attribute__ ((packed)) player_fiducial_geom_t;
/*************************************************************************/

/*************************************************************************/
/*
 * Comms interface
 */

/* Request packet sub-types */
#define PLAYER_COMMS_SUBTYPE_SEND 0x01
#define PLAYER_COMMS_SUBTYPE_RECV 0x02

/* Comms request/reply packet. */
typedef struct
{
  /* Packet subtype.  Set to PLAYER_COMMS_SUBTYPE_SEND to send a
   * broadcast messages.  Set to PLAYER_COMMS_SUBTYPE_RECV to read
   * the next message in the incoming message queue. */
  uint8_t subtype;

  /* The message to send, or the message that was received*/
  uint8_t data[PLAYER_MAX_REQREP_SIZE - 1];
} __attribute__ ((packed)) player_comms_msg_t;
/*************************************************************************/

/*************************************************************************/
/*
 * Speech interface
 */

/* queue size */
#define PLAYER_SPEECH_MAX_STRING_LEN 256
#define PLAYER_SPEECH_MAX_QUEUE_LEN 4

/*
 * Speech command packet: ASCII string to say
 */
typedef struct
{
  uint8_t string[PLAYER_SPEECH_MAX_STRING_LEN];
} __attribute__ ((packed)) player_speech_cmd_t;
/*************************************************************************/

/*************************************************************************/
/*
 * GPS interface
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
/*************************************************************************/

/*************************************************************************/
/*
 * Bumper interface
 */

#define PLAYER_BUMPER_MAX_SAMPLES 32

typedef struct
{
  /* the number of valid bumper readings */
  uint8_t bumper_count;

  /* array of bumper values */
  uint8_t bumpers[PLAYER_BUMPER_MAX_SAMPLES];
} __attribute__ ((packed)) player_bumper_data_t;

// TODO: what do these do?
#define PLAYER_BUMPER_POWER_REQ             ((uint8_t)13)
#define PLAYER_BUMPER_GET_GEOM_REQ          ((uint8_t)14)
/*************************************************************************/

/*************************************************************************/
/*
 * Truth device, used for getting and setting data about entities in Stage.
 */

/* Data packet with current state of truth object. */
typedef struct
{
  /* Object pose in world cs (mm, mm, degrees). */
  int32_t px, py, pa; 

} __attribute__ ((packed)) player_truth_data_t;

/* Request packet subtypes. */
#define PLAYER_TRUTH_GET_POSE 0x00
#define PLAYER_TRUTH_SET_POSE 0x01

/* Config packet for setting state of truth object. */
typedef struct
{
  /* Packet subtype.  Use PLAYER_TRUTH_GET_POSE to get the pose.  Use
  * PLAYER_TRUTH_SET_POSE to set the pose. */
  uint8_t subtype;
  
  /* Object pose in world cs (mm, mm, degrees). */
  int32_t px, py, pa; 

} __attribute__ ((packed)) player_truth_pose_t;
/*************************************************************************/

/*************************************************************************/
/*
 * IDAR device - HRL's infrared data and ranging turret
 */

#define IDARBUFLEN 16   // idar message max in bytes
#define RAYS_PER_SENSOR 5 // resolution

#define IDAR_TRANSMIT 0
#define IDAR_RECEIVE 1
#define IDAR_RECEIVE_NOFLUSH 2
#define IDAR_TRANSMIT_RECEIVE 3

typedef struct
{
  unsigned char mesg[IDARBUFLEN];
  uint8_t len; //0-IDARBUFLEN
  uint8_t intensity; //0-255
} __attribute__ ((packed)) idartx_t;

typedef struct
{
  unsigned char mesg[IDARBUFLEN];
  uint8_t len; //0-255
  uint8_t intensity; //0-255
  uint8_t reflection; // true/false
  uint32_t timestamp_sec;
  uint32_t timestamp_usec;
  uint16_t range; // mm
} __attribute__ ((packed)) idarrx_t; 

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

/*************************************************************************/
/*
 * IDARTurret device - a collection of IDARs with a combined interface
 */

#define PLAYER_IDARTURRET_IDAR_COUNT 8

// define structures to get and receive messages from a collection of
// IDARs in one go.

typedef struct
{
  idarrx_t rx[ PLAYER_IDARTURRET_IDAR_COUNT ];
} __attribute__ ((packed)) player_idarturret_reply_t;

typedef  struct
{
  uint8_t instruction;
  idartx_t tx[ PLAYER_IDARTURRET_IDAR_COUNT ];
} __attribute__ ((packed)) player_idarturret_config_t;
/*************************************************************************/

/*************************************************************************/
/*
 * Descartes Device - a small holonomic robot with bumpers
 */

/* command buffer */
typedef struct
{
  int16_t speed, heading, distance; // UNITS: mm/sec, degrees, mm
} __attribute__ ((packed)) player_descartes_config_t;

/* data buffer */
typedef struct 
{
  int32_t xpos,ypos; // mm, mm
  int16_t theta; //  degrees
  uint8_t bumpers[2]; // booleans
} __attribute__ ((packed)) player_descartes_data_t;

// no commands or replies for descartes
/*************************************************************************/

/*************************************************************************/
/*
 * Mote radio device
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
/*************************************************************************/

/*************************************************************************/
/*
 * DIO interface
 */

// TODO: add command

/* DIO data packet */
typedef struct
{
  uint8_t count; // number of samples
  uint32_t digin; // bitfield
} __attribute__ ((packed)) player_dio_data_t;
/*************************************************************************/

/*************************************************************************/
/*
 * AIO interface
 */

#define PLAYER_AIO_MAX_SAMPLES 8

// TODO: add command

/* AIO data packet */
typedef struct
{
  uint8_t count;
  int32_t anin[PLAYER_AIO_MAX_SAMPLES];
} __attribute__ ((packed)) player_aio_data_t;
/*************************************************************************/

/*************************************************************************/
/*
 * BPS Device
 *
 *   A global positioning device using laser beacons (hence *B*PS)
 *   Dont forget to specify the beacon positions (using the setbeacon request).
 */

/* BPS data packet */
typedef struct
{
  /* (px, py, pa): current global pose (mm, mm, degrees).  */
  int32_t px, py, pa;

  /* (ux, uy, ua): uncertainty (mm, mm, degrees). */
  int32_t ux, uy, ua;

  /* (err): residual error in estimate (x 1e6) */
  int32_t err;

} __attribute__ ((packed)) player_bps_data_t;


/* Request packet subtypes */
#define PLAYER_BPS_SET_CONFIG 1
#define PLAYER_BPS_GET_CONFIG 2
#define PLAYER_BPS_SET_BEACON 3
#define PLAYER_BPS_GET_BEACON 4


/* BPS configuration packet.  This structure is currently empty. */
typedef struct
{
  /* subtype : set PLAYER_BPS_SET_CONFIG to set the configuration or
   * PLAYER_BPS_GET_CONFIG to get the configuration. */
  uint8_t subtype;

} __attribute__ ((packed)) player_bps_config_t;


/* BPS beacon packet. */
typedef struct
{
  /* Packet subtype : set to PLAYER_BPS_SET_BEACON to set the pose of
   * a beacon.  Set to PLAYER_BPS_GET_BEACON to get the pose of a
   * beacon, and set id to the id of the beacon to get.. */
  uint8_t subtype;

  /* Beacon id : must be non-zero. */
  uint8_t id;

  /* Beacon pose (mm, mm, degrees) in the world cs. */
  int32_t px, py, pa;

  /* Uncertainty in the beacon pose (mm, mm, degrees). */
  int32_t ux, uy, ua;
  
} __attribute__ ((packed)) player_bps_beacon_t;
/*************************************************************************/

/*************************************************************************/
/*
 * Joystick Device
 */

typedef struct
{
  uint8_t  xpos, ypos;
  uint8_t  button0, button1;
} __attribute__ ((packed)) player_joystick_data_t;
/*************************************************************************/

/*************************************************************************/
/*
 * RWI drivers
 *
 * All RWI devices use the same struct for sending config commands.
 * The request numbers are found near the devices to which they
 * pertain.
 */

typedef struct
{
  uint8_t   request;
  uint8_t   value;
} __attribute__ ((packed)) player_rwi_config_t;
/*************************************************************************/

/*************************************************************************/
/*
 * The p2os_sonar driver
 */

/* the various configuration commands 
 * NOTE: these must not be the same as any other P2OS device! */
#define PLAYER_P2OS_SONAR_POWER_REQ      ((uint8_t)2)

/* Packet for configuring the Pioneer sonar. */
typedef struct
{
  /* Packet subtype.  Must be PLAYER_P2OS_SONAR_POWER_REQ. */
  uint8_t subtype;

  /* Turn sonars on or off. */
  uint8_t arg;
} __attribute__ ((packed)) player_p2os_sonar_config_t;
/*************************************************************************/

/*************************************************************************/
/*
 * The p2os_position driver
 */
#define PLAYER_P2OS_POSITION_MOTOR_POWER_REQ       ((uint8_t)2)
#define PLAYER_P2OS_POSITION_VELOCITY_CONTROL_REQ  ((uint8_t)3)
#define PLAYER_P2OS_POSITION_RESET_ODOM_REQ        ((uint8_t)4)

// 
typedef struct
{
  uint8_t request; // one of the above request types
  uint8_t value;  // value for the request (usually 0 or 1)
} __attribute__ ((packed)) player_p2os_position_config_t;
/*************************************************************************/

/*************************************************************************/
/*
 * The acts driver
 */

#define ACTS_NUM_CHANNELS 32
#define ACTS_HEADER_SIZE_1_0 2*ACTS_NUM_CHANNELS  
#define ACTS_HEADER_SIZE_1_2 4*ACTS_NUM_CHANNELS  
#define ACTS_BLOB_SIZE_1_0 10
#define ACTS_BLOB_SIZE_1_2 16
#define ACTS_MAX_BLOBS_PER_CHANNEL 10
/*************************************************************************/

/*************************************************************************/
/*
 * The laserbarcode driver
 */

#define PLAYER_LASERBARCODE_SET_CONFIG 0x02
#define PLAYER_LASERBARCODE_GET_CONFIG 0x03

/* laserbarcode configuration packet. */
typedef struct
{
  /* Packet subtype.  Set to PLAYER_LASERBARCODE_SET_CONFIG to set the
   *  device configuration.  Set to PLAYER_LASERBARCODE_GET_CONFIG to
   *  get the device configuration. */
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
} __attribute__ ((packed)) player_laserbarcode_config_t;
/*************************************************************************/


#endif /* PLAYER_H */
