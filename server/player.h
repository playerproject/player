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
#define PLAYER_SONAR_CODE          ((uint16_t)5)   // fixed range-finder
#define PLAYER_LASER_CODE          ((uint16_t)6)   // scanning range-finder
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
#define PLAYER_IR_CODE             ((uint16_t)22)  // IR array
#define PLAYER_WIFI_CODE	   ((uint16_t)23)  // wifi card status
#define PLAYER_WAVEFORM_CODE	   ((uint16_t)24)  // fetch raw waveforms
#define PLAYER_LOCALIZATION_CODE   ((uint16_t)25)  // localization
// no interface has yet been defined for BPS-like things
//#define PLAYER_BPS_CODE            ((uint16_t)16)

/* the currently assigned device strings */
#define PLAYER_PLAYER_STRING         "player"
#define PLAYER_POWER_STRING          "power"
#define PLAYER_GRIPPER_STRING        "gripper"
#define PLAYER_POSITION_STRING       "position"
#define PLAYER_SONAR_STRING          "sonar"
#define PLAYER_LASER_STRING          "laser"
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
#define PLAYER_IR_STRING             "ir"
#define PLAYER_WIFI_STRING           "wifi"
#define PLAYER_WAVEFORM_STRING       "waveform"
#define PLAYER_LOCALIZATION_STRING   "localization"
// no interface has yet been defined for BPS-like things
//#define PLAYER_BPS_STRING            "bps"

/* The maximum number of devices the server will support. */
#define PLAYER_MAX_DEVICES 64

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

/*************************************************************************
 ** begin section Player
 *************************************************************************/

/** [Synopsis]
   The {\tt player} device represents the server itself, and is used in
   configuring the behavior of the server.  This device is always open.
 */

/** [Constants]
  */
/** The device access modes */
#define PLAYER_READ_MODE 'r'
#define PLAYER_WRITE_MODE 'w'
#define PLAYER_ALL_MODE 'a'
#define PLAYER_CLOSE_MODE 'c'
#define PLAYER_ERROR_MODE 'e'

/** The valid data delivery modes */
#define PLAYER_DATAMODE_PUSH_ALL 0
#define PLAYER_DATAMODE_PULL_ALL 1
#define PLAYER_DATAMODE_PUSH_NEW 2
#define PLAYER_DATAMODE_PULL_NEW 3

/** The request subtypes */
#define PLAYER_PLAYER_DEVLIST_REQ     ((uint16_t)1)
#define PLAYER_PLAYER_DRIVERINFO_REQ  ((uint16_t)2)
#define PLAYER_PLAYER_DEV_REQ         ((uint16_t)3)
#define PLAYER_PLAYER_DATA_REQ        ((uint16_t)4)
#define PLAYER_PLAYER_DATAMODE_REQ    ((uint16_t)5)
#define PLAYER_PLAYER_DATAFREQ_REQ    ((uint16_t)6)
#define PLAYER_PLAYER_AUTH_REQ        ((uint16_t)7)

/** [Data]
    This interface accepts no commands.
*/

/** [Commands]
    This interface accepts no commands.
*/

/** [Configuration: Get device list]
*/

/** A device identifier; devices are differentiated internally in Player by 
    these identifiers, and some messages contain them. */
typedef struct player_device_id
{
  /** The interface provided by the device*/
  uint16_t code;
  /** The index of the device */
  uint16_t index;
  /** The TCP port of the device (only useful with Stage)*/
  uint16_t port;

} __attribute__ ((packed)) player_device_id_t;


/** Get the list of available devices from the server. 
    It's useful for applications such as viewer programs and test suites that
    tailor behave differently depending on which devices are available.
    To request the list, set the subtype to PLAYER_PLAYER_DEVLIST_REQ
    and leave the rest of the fields blank.  Player will return a
    packet with subtype PLAYER_PLAYER_DEVLIST_REQ with the fields
    filled in. */
typedef struct player_device_devlist
{
  /** Subtype; must be PLAYER_PLAYER_DEVLIST_REQ. */
  uint16_t subtype;

  /** The number of devices */
  uint16_t device_count;

  /** The list of available devices. */
  player_device_id_t devices[PLAYER_MAX_DEVICES];
  
} __attribute__ ((packed)) player_device_devlist_t;

/** [Configuration: Get driver name]
*/

/** Get the driver name for a particular device.
    To get a name, set the subtype to PLAYER_PLAYER_DRIVERINFO_REQ
    and set the id field.  Player will return the driver info. */
typedef struct player_device_driverinfo
{
  /** Subtype; must be PLAYER_PLAYER_DRIVERINFO_REQ. */
  uint16_t subtype;

  /** The device identifier. */
  player_device_id_t id;

  /** The driver name (returned) */
  char driver_name[PLAYER_MAX_DEVICE_STRING_LEN];

} __attribute__ ((packed)) player_device_driverinfo_t;

/** [Configuration: Request device access]
*/

/** {\em This is the most important request!}
 Before interacting with a device, the client must request appropriate access.
 The format of this request is: */
typedef struct player_device_req
{
  /** Subtype; must be PLAYER_PLAYER_DEV_REQ */
  uint16_t subtype;
  /** The interface for the device */
  uint16_t code;
  /** The index for the device */
  uint16_t index;
  /** The requested access */
  uint8_t access;

} __attribute__ ((packed)) player_device_req_t;

/** The format of the server's reply is: */
typedef struct player_device_resp
{
  /** Subtype; will be PLAYER_PLAYER_DEV_REQ */
  uint16_t subtype;
  /** The interface for the device */
  uint16_t code;
  /** The index for the device */
  uint16_t index;
  /** The granted access */
  uint8_t access;
  /** The name of the underlying driver */
  uint8_t driver_name[PLAYER_MAX_DEVICE_STRING_LEN];

} __attribute__ ((packed)) player_device_resp_t;

/** The access codes, which are used in both the request and response, are
 given above.   {\bf Read} access means that the
 server will start sending data from the specified device. For instance, if
 read access is obtained for the sonar device Player will start sending sonar
 data to the client. {\bf Write} access means that the client has permission
 to control the actuators of the device. There is no locking mechanism so
 different clients can have concurrent write access to the same actuators.
 {\bf All} access is both of the above and finally {\bf close} means that
 there is no longer any access to the device. Device request messages can
 be sent at any time, providing on the fly reconfiguration for clients
 that need different devices depending on the task at hand.  
 \\
 Of course, not all of the access codes are applicable to all devices;
 for instance it does not make sense to write to the sonars.   However,
 a request for such access will not generate an error; rather, it will be
 granted, but any commands actually sent to that device will be ignored.
 In response to such a device request, the server will send a reply indicating
 the {\em actual} access that was granted for the device.  The granted access
 may be different from the requested access; in particular, if there was
 some error in initializing the device the granted access will be '{\tt e}',
 and the client should not try to read from or write to the device. */

/** [Configuration: Request data] */

/** When the server is in a {\em pull} data delivery mode (see next request
 for information on data delivery modes), the client can request a single
 round of data by sending a zero-argument request with type code {\tt 0x0003}.
 The response will be a zero-length acknowledgement.*/
typedef struct player_device_data_req
{
  /** Subtype; must be PLAYER_PLAYER_DATA_REQ */
  uint16_t subtype;

} __attribute__ ((packed)) player_device_data_req_t;

/** [Configuration: Change data delivery mode] */

/** 
The Player server supports four data modes, described above.
By default, the server operates in {\tt PLAYER\_DATAMODE\_PUSH\_NEW} mode at 
a frequency of 10Hz.  To switch to a different mode send a request with the 
format given below.  The server's reply will be a zero-length 
acknowledgement. */
typedef struct player_device_datamode_req
{
  /** Subtype; must be PLAYER_PLAYER_DATAMODE_REQ */
  uint16_t subtype;
  /** The requested mode */
  uint8_t mode;

} __attribute__ ((packed)) player_device_datamode_req_t;


/** [Configuration: Change data delivery frequency] */

/** By default, the fixed frequency for the {\em push} data delivery modes is
 10Hz; thus a client which makes no configuration changes will receive
 sensor data approximately every 100ms. The server can send data faster
 or slower; to change the frequency, send a request of the format:*/
typedef struct player_device_datafreq_req
{
  /** Subtype; must be PLAYER_PLAYER_DATAFREQ_REQ */
  uint16_t subtype;
  /** requested frequency in Hz */
  uint16_t frequency;

} __attribute__ ((packed)) player_device_datafreq_req_t;

/**The server's reply will be a
 zero-length acknowledgement. Due to limitations of the Linux scheduler,
 Player's maximum data rate on that platform is approximately 50Hz. */

/** [Configuration: Authentication] */

/** If server authentication has been enabled (by providing {\tt -key <key>}
 on the command-line; see Section~\ref{sect:commandline}), then each
 client must authenticate itself before otherwise interacting with
 the server.  To authenticate, send a request of the format: */
typedef struct player_device_auth_req
{
  /** Subtype; must by PLAYER_PLAYER_AUTH_REQ */
  uint16_t subtype;
  /** The authentication key */
  uint8_t auth_key[PLAYER_KEYLEN];

} __attribute__ ((packed)) player_device_auth_req_t;

 /** If the key matches
 the server's key then the client is authenticated, the server will reply
 with a zero-length acknowledgement, and the client can continue with other
 operations.  If the key does not match, or if the client attempts any
 other server interactions before authenticating, then the connection will
 be closed immediately.  It is only necessary to authenticate each client once.
\\
 Note that this support for authentication is {\bf NOT} a security mechanism.
 The keys are always in plain text, both in memory and when transmitted over
 the network; further, since the key is given on the command-line, there is
 a very good chance that you can find it in plain text in the process table
 (in Linux try {\tt ps -ax | grep player}).  Thus you should not use an
 important password as your key, nor should you rely on Player authentication
 to prevent bad guys from driving your robots (use a firewall instead).
 Rather, authentication was introduced into Player to prevent accidentally
 connecting one's client program to someone else's robot.  This kind of
 accident occurs primarily when Stage is running in a multi-user environment.
 In this case it is very likely that there is a Player server listening
 on port \DEFAULTPORT, and clients will generally connect to that port by
 default, unless a specific option is given.  Check the Stage documentation
 for how to specify a Player authentication key in your {\tt .world} file. */

/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section power
 *************************************************************************/

/** [Synopsis]
  The {\tt power} interface provides access to a robot's power subsystem. */

/** [Constants] */

/** what does this do? */
#define PLAYER_MAIN_POWER_REQ               ((uint8_t)14)

/** [Data] */

/** The {\tt power} device returns data in the format: */
typedef struct player_power_data
{
  /** Battery voltage, in decivolts */
  uint16_t  charge;
} __attribute__ ((packed)) player_power_data_t;

/** [Commands] This interface accepts no commands */

/** [Configuration: Request power] */

/** Packet for requesting power config  - replies with a player_power_data_t
  */
typedef struct player_power_config
{
  /** Packet subtype.  Must be PLAYER_MAIN_POWER_REQ. */
  uint8_t subtype;
} __attribute__ ((packed)) player_power_config_t;
/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section gripper
 *************************************************************************/

/** [Synopsis] The {\tt gripper} interface provides access to a robotic 
 gripper. */

/** [Data] */

/**
The {\tt gripper} interface returns 2 bytes that represent the current state 
of the gripper; the format is given below.
Note that the exact interpretation of this data may vary depending
on the details of your gripper and how it is connected to your robot
(e.g., General I/O vs. User I/O for the Pioneer gripper). */
typedef struct player_gripper_data
{
  /** The current gripper lift and breakbeam state */
  uint8_t state, beams;
} __attribute__ ((packed)) player_gripper_data_t;

/** The following table defines how the data can be interpreted for some
    Pioneer robots and Stage: 
\begin{center}
{\small \begin{tabularx}{\columnwidth}{llX}
\hline
Field & Type & Meaning\\
\hline
{\tt state} & {\bf unsigned byte} & bit 0: Paddles open \newline
                      bit 1: Paddles closed\newline
                      bit 2: Paddles moving\newline
                      bit 3: Paddles error\newline
                      bit 4: Lift is up\newline
                      bit 5: Lift is down\newline
                      bit 6: Lift is moving\newline
                      bit 7: Lift error \\
                      
{\tt beams} & {\bf unsigned byte} & bit 0: Gripper limit reached\newline
                      bit 1: Lift limit reached\newline
                      bit 2: Outer beam obstructed\newline
                      bit 3: Inner beam obstructed\newline
                      bit 4: Left paddle open\newline
                      bit 5: Right paddle open\\

\hline
\end{tabularx}}
\end{center}
*/

/** [Commands]  */
/**
The {\tt gripper} interface accepts 2-byte commands, the format of
which is given below. 
These two bytes are sent directly to the gripper; refer to Table 3-3 page
10 in the Pioneer 2 Gripper Manual\cite{gripman} for a list of commands. The
first byte is the command. The second is the argument for the LIFTcarry and
GRIPpress commands, but for all others it is ignored. */
typedef struct player_gripper_cmd
{
  /** the command and optional argument */
  uint8_t cmd, arg; 
} __attribute__ ((packed)) player_gripper_cmd_t;
/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section position
 *************************************************************************/

/** [Synopsis] The {\tt position} interface is used to control a planar 
 mobile robot base. */

/** [Constants] */
/** the various configuration subtypes */
#define PLAYER_POSITION_GET_GEOM_REQ          ((uint8_t)1)
#define PLAYER_POSITION_MOTOR_POWER_REQ       ((uint8_t)2)
#define PLAYER_POSITION_VELOCITY_MODE_REQ     ((uint8_t)3)
#define PLAYER_POSITION_RESET_ODOM_REQ        ((uint8_t)4)
#define PLAYER_POSITION_POSITION_MODE_REQ     ((uint8_t)5)
#define PLAYER_POSITION_SPEED_PID_REQ         ((uint8_t)6)
#define PLAYER_POSITION_POSITION_PID_REQ      ((uint8_t)7)
#define PLAYER_POSITION_SPEED_PROF_REQ        ((uint8_t)8)
#define PLAYER_POSITION_SET_ODOM_REQ          ((uint8_t)9)

/** [Data] */
/**
The {\tt position} interface returns data regarding the odometric pose and
velocity of the robot, as well as motor stall information; the format is: */
typedef struct player_position_data
{
  /** X and Y position, in mm */
  int32_t xpos, ypos;
  /** Yaw, in degrees */
  int32_t yaw; 
  /** X and Y translational velocities, in mm/sec */
  int32_t xspeed, yspeed; 
  /** Angular velocity, in degrees/sec */
  int32_t yawspeed;
  /** Are the motors stalled? */
  uint8_t stall;
} __attribute__ ((packed)) player_position_data_t;

/** [Commands] */
/**
The {\tt position} interface accepts new positions and/or velocities
for the robot's motors (drivers may support position control, speed control,
or both); the format is */
typedef struct player_position_cmd
{
  /** X and Y position, in mm */
  int32_t xpos, ypos;
  /** Yaw, in degrees */
  int32_t yaw; 
  /** X and Y translational velocities, in mm/sec */
  int32_t xspeed, yspeed; 
  /** Angular velocity, in degrees/sec */
  int32_t yawspeed;
} __attribute__ ((packed)) player_position_cmd_t;

/** [Configuration: Query geometry] */

/** To request robot geometry, set the subtype to PLAYER_POSITION_GET_GEOM_REQ
    and leave the other fields empty.  The server will reply with the 
    pose and size fields filled in. */
typedef struct player_position_geom
{
  /** Packet subtype.  Must be PLAYER_POSITION_GET_GEOM_REQ. */
  uint8_t subtype;

  /** Pose of the robot base, in the robot cs (mm, mm, degrees). */
  uint16_t pose[3];

  /** Dimensions of the base (mm, mm). */
  uint16_t size[2];
  
} __attribute__ ((packed)) player_position_geom_t;

/** [Configuration: Motor power] */
/**
On some robots, the motor power can be turned on and off from software.
To do so, send a request with the format given below, and with the
appropriate {\tt state} (zero for motors off and non-zero for motors on).
The server will reply with a zero-length acknowledgement.

Be VERY careful with this command!  You are very likely to start the robot 
running across the room at high speed with the battery charger still attached.
*/
typedef struct player_position_power_config
{ 
  /** subtype; must be PLAYER_POSITION_MOTOR_POWER_REQ */
  uint8_t request;
  /** 0 for off, 1 for on */
  uint8_t value; 
} __attribute__ ((packed)) player_position_power_config_t;

/** [Configuration: Change velocity control] */
/**
Some robots offer different velocity control modes.
It can be changed by sending a request with the format given below,
including the appropriate mode.  No matter which mode is used, the external
client interface to the {\tt position} device remains the same.   The server 
will reply with a zero-length acknowledgement*/
typedef struct player_position_velocitymode_config
{
  /** subtype; must be PLAYER_POSITION_VELOCITY_MODE_REQ */
  uint8_t request; 
  /** driver-specific */
  uint8_t value; 
} __attribute__ ((packed)) player_position_velocitymode_config_t;

/** The {\tt p2os_position} driver offers two modes of velocity control: 
 separate translational and rotational control and direct wheel control.  When
in the separate mode, the robot's microcontroller internally computes left
and right wheel velocities based on the currently commanded translational
and rotational velocities and then attenuates these values to match a nice
predefined acceleration profile.  When in the direct mode, the microcontroller
simply passes on the current left and right wheel velocities.  Essentially,
the separate mode offers smoother but slower (lower acceleration) control,
and the direct mode offers faster but jerkier (higher acceleration) control.
Player's default is to use the direct mode.  Set {\tt mode} to zero for
direct control and non-zero for separate control.

For the {\tt reb_position} driver, 0 is direct velocity control, 1 is for 
      velocity-based heading PD controller.
*/

/** [Configuration: Reset odometry] */
/** To reset the robot's odometry to $(x,y,\theta) = (0,0,0)$, use the
 following request.  The server will reply with a zero-length 
 acknowledgement. */
typedef struct player_position_resetodom_config
{
  /** subtype; must be PLAYER_POSITION_RESET_ODOM_REQ */
  uint8_t request; 
} __attribute__ ((packed)) player_position_resetodom_config_t;

/** [Configuration: Change position control] */
/** */
typedef struct player_position_position_mode_req
{
  /** subtype;  must be PLAYER_POSITION_POSITION_MODE_REQ */
  uint8_t subtype; 
  /** 0 for velocity mode, 1 for position mode */
  uint8_t state; 
} __attribute__ ((packed)) player_position_position_mode_req_t;

/** [Configuration: Set odometry] */
/** To set the robot's odometry to a particular state, use this request: */
typedef struct player_position_set_odom_req
{
  /** subtype; must be PLAYER_SET_ODOM_REQ */
  uint8_t subtype; 
  /** X and Y (in mm?) */
  int32_t x, y;
  /** Heading (in degrees) */
  uint16_t theta;
}__attribute__ ((packed)) player_position_set_odom_req_t;

/** [Configuration: Set velocity PID parameters] */
/** */
typedef struct player_position_speed_pid_req
{
  /** subtype; must be PLAYER_POSITION_SPEED_PID_REQ */
  uint8_t subtype;
  /** PID parameters */
  int32_t kp, ki, kd;
} __attribute__ ((packed)) player_position_speed_pid_req_t;

/** [Configuration: Set position PID parameters] */
/** */
typedef struct player_position_position_pid_req
{
  /** subtype; must be PLAYER_POSITION_POSITION_PID_REQ */
  uint8_t subtype; 
  /** PID parameters */
  int32_t kp, ki, kd;
} __attribute__ ((packed)) player_position_position_pid_req_t;

/** [Configuration: Set speed profile parameters] */
/** */
typedef struct player_position_speed_prof_req
{
  /** subtype; must be PLAYER_POSITION_SPEED_PROF_REQ */
  uint8_t subtype; 
  /** max speed */
  int16_t speed;
  /** max acceleration */
  int16_t acc;
} __attribute__ ((packed)) player_position_speed_prof_req_t;

/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section sonar
 *************************************************************************/
/** [Synopsis]
The {\tt sonar} interface provides access to a collection of fixed range 
sensors, such as a sonar array. */

/** [Constants] */
/** maximum number of sonar samples in a data packet */
#define PLAYER_SONAR_MAX_SAMPLES 32
/** request types */
#define PLAYER_SONAR_GET_GEOM_REQ   ((uint8_t)1)
#define PLAYER_SONAR_POWER_REQ      ((uint8_t)2)

/** [Data] */
/**
The {\tt sonar} interface returns up to 32 range readings from a robot's 
sonars.  The format is: */
typedef struct player_sonar_data
{
  /** The number of valid range readings. */
  uint16_t range_count;
  
  /** The range readings */
  uint16_t ranges[PLAYER_SONAR_MAX_SAMPLES];
  
} __attribute__ ((packed)) player_sonar_data_t;

/** [Commands] This interface accepts no commands. */

/** [Configuration: Query geometry] */
/** To query the geometry of the sonar transducers, use the request given
 below, but only fill in the subtype.  The server will reply with the other 
 fields filled in. */
typedef struct player_sonar_geom
{
  /** Subtype.  Must be PLAYER_SONAR_GET_GEOM_REQ. */
  uint8_t subtype;

  /** The number of valid poses. */
  uint16_t pose_count;

  /** Pose of each sonar, in robot cs (mm, mm, degrees). */
  int16_t poses[PLAYER_SONAR_MAX_SAMPLES][3];
  
} __attribute__ ((packed)) player_sonar_geom_t;

/** [Configuration: Sonar power] */
/** On some robots, the sonars can be turned on and off from software.  To do
    so, issue a request of the form given below.  The server will reply with
    a zero-length acknowledgement. */
typedef struct player_sonar_power_config
{
  /** Packet subtype.  Must be PLAYER_P2OS_SONAR_POWER_REQ. */
  uint8_t subtype;

  /** Turn power off (0) or on (>0) */
  uint8_t value;

} __attribute__ ((packed)) player_sonar_power_config_t;

/*************************************************************************
 ** end section
 *************************************************************************/


/*************************************************************************
 ** begin section laser
 *************************************************************************/

/** [Synopsis]

    The laser interface provides access to a single-origin scanning range
    sensor, such as a SICK laser range-finder.
*/

/** [Constants]
 */

/** The maximum number of laser range values */
#define PLAYER_LASER_MAX_SAMPLES  401

/** Laser request subtypes. */
#define PLAYER_LASER_GET_GEOM   0x01
#define PLAYER_LASER_SET_CONFIG 0x02
#define PLAYER_LASER_GET_CONFIG 0x03
#define PLAYER_LASER_POWER_CONFIG 0x04

/** [Data]

    Devices supporting the {\tt laser} interface can be configured to
    scan at different angles and resolutions.  As such, the data
    returned by the {\tt laser} interface can take different forms.
    To make interpretation of the data simple, the {\tt laser} data
    packet contains some extra fields before the actual range data.
    These fields tell the client the starting and ending angles of the
    scan, the angular resolution of the scan, and the number of range
    readings included.  Scans proceed counterclockwise about the
    laser, and $0^{\circ}$ is forward.  The laser can return a maximum
    of 401 readings; this limits the valid combinations of scan width
    and angular resolution.
*/

/** The laser data packet.  */
typedef struct player_laser_data
{
  /** Start and end angles for the laser scan (in units of 0.01
      degrees).  */
  int16_t min_angle, max_angle;

  /** Angular resolution (in units of 0.01 degrees).  */
  uint16_t resolution;

  /** Number of range/intensity readings.  */
  uint16_t range_count;

  /** Range readings (mm). Note that some drivers can produce negative
      values.  */
  int16_t ranges[PLAYER_LASER_MAX_SAMPLES];

  /** Intensity readings. */
  uint8_t intensity[PLAYER_LASER_MAX_SAMPLES];
     
} __attribute__ ((packed)) player_laser_data_t;


/** [Command]
    This device accepts no commands.
*/


/** [Configuration: get geometry]
    
    The laser geometry (position and size) can be queried using
    the PLAYER_LASER_GET_GEOM request.  The request and
    reply packets have the same format.
*/

/** Request/reply packet for getting laser geometry. */
typedef struct player_laser_geom
{
  /** The packet subtype.  Must be PLAYER_LASER_GET_GEOM. */
  uint8_t subtype;

  /** Laser pose, in robot cs (mm, mm, degrees). */
  int16_t pose[3];

  /** Laser dimensions (mm, mm). */
  int16_t size[2];

} __attribute__ ((packed)) player_laser_geom_t;


/** [Configuration: get/set scan properties]

    The scan configuration can be queried using the
    PLAYER_LASER_GET_CONFIG request and modified using
    the PLAYER_LASER_SET_CONFIG request.

    The sicklms200 driver, for example, is usually configured to scan
    a swath of $180^{\circ}$ with a resolution of $0.5^{\circ}$, to
    generate a total of 361 readings.  At this aperture, the laser
    generates a new scan every 200ms or so, for a data rate of 5Hz.
    This rate can be raised by reducing the aperture to encompass less
    than the full $180^{\circ}$, or by lowering the resolution to
    $1^{\circ}$.
    
    Read the documentation for your driver to determine what
    configuration values are permissible.
 */

/** Request/reply packet for getting and setting the laser configuration. */
typedef struct player_laser_config
{
  /** The packet subtype.  Set this to PLAYER_LASER_SET_CONFIG to set
      the laser configuration; or set to PLAYER_LASER_GET_CONFIG to get
      the laser configuration.  */
  uint8_t subtype;

  /** Start and end angles for the laser scan (in units of 0.01
      degrees).  Valid range is -9000 to +9000.  */
  int16_t min_angle, max_angle;

  /** Scan resolution (in units of 0.01 degrees).  Valid resolutions
      are 25, 50, 100.  */
  uint16_t resolution;

  /** Enable reflection intensity data. */
  uint8_t  intensity;
  
} __attribute__ ((packed)) player_laser_config_t;


/** Turn the laser power on or off. */
typedef struct player_laser_power_config
{
  /** Must be PLAYER_LASER_POWER_CONFIG. */
  uint8_t subtype;

  /** 0 to turn laser off, 1 to turn laser on */
  uint8_t value;
  
} __attribute__ ((packed)) player_laser_power_config_t;


/*************************************************************************
 ** end section
 *************************************************************************/


/*************************************************************************
 ** begin section blobfinder
 *************************************************************************/

/** [Synopsis]
    The blobfinder interface provides access to devices that detect
    colored blobs.
*/

/** [Constants]
 */

/** The maximum number of unique color classes. */
#define PLAYER_BLOBFINDER_MAX_CHANNELS 32

/** The maximum number of blobs for each color class. */
#define PLAYER_BLOBFINDER_MAX_BLOBS_PER_CHANNEL 10

/** The maximum number of blobs in total. */
#define PLAYER_BLOBFINDER_MAX_BLOBS PLAYER_BLOBFINDER_MAX_CHANNELS * PLAYER_BLOBFINDER_MAX_BLOBS_PER_CHANNEL


/** [Data]

    The format of the {\tt blobfinder} data packet is very similar to
    the ACTS v1.2/2.0 format, but a bit simpler.  The packet length is
    variable, with each packet containing both a list of blobs and a
    header that provides an index into that list.  For each channel,
    the header entry tells you which blob to start with and how many
    blobs there are.
*/

/** Blob index entry. */
typedef struct player_blobfinder_header_elt
{
  /** Offset of the first blob for this channel. */
  uint16_t index;

  /** Number of blobs for this channel. */
  uint16_t num;
  
} __attribute__ ((packed)) player_blobfinder_header_elt_t;


/** Structure describing a single blob. */
typedef struct player_blobfinder_blob_elt
{
  /** A descriptive color for the blob (useful for gui's).  The color
      is stored as packed 32-bit RGB, i.e., 0x00RRGGBB. */
  uint32_t color;

  /** The blob area (pixels). */
  uint32_t area;

  /** The blob centroid (image coords). */
  uint16_t x, y;

  /** Bounding box for the blob (image coords). */
  uint16_t left, right, top, bottom;

  /** Range (mm) to the blob center */
  uint16_t range;
  
} __attribute__ ((packed)) player_blobfinder_blob_elt_t;


/** The list of detected blobs. */
typedef struct player_blobfinder_data
{
  /** The image dimensions. */
  uint16_t width, height;

  /** An index into the list of blobs (blobs are indexed by channel). */
  player_blobfinder_header_elt_t header[PLAYER_BLOBFINDER_MAX_CHANNELS];

  /** The list of blobs. */
  player_blobfinder_blob_elt_t blobs[PLAYER_BLOBFINDER_MAX_BLOBS];
  
} __attribute__ ((packed)) player_blobfinder_data_t;


#define PLAYER_BLOBFINDER_HEADER_SIZE \
  (2*sizeof(uint16_t) + sizeof(player_blobfinder_header_elt_t)*PLAYER_BLOBFINDER_MAX_CHANNELS)

#define PLAYER_BLOBFINDER_BLOB_SIZE sizeof(player_blobfinder_blob_elt_t)


/** [Command]
    This device accepts no commands.
*/

/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section ptz
 *************************************************************************/

/** [Synopsis]
The {\tt ptz} interface is used to control a pan-tilt-zoom camera. */

/** [Data] */
/** The {\tt ptz} interface returns data reflecting the current state of the
Pan-Tilt-Zoom camera unit; the format is: */
typedef struct player_ptz_data
{
  /** Pan (degrees) */
  int16_t pan;

  /** Tilt (degrees) */
  int16_t tilt;

  /** Field of view (degrees). */
  int16_t zoom;

} __attribute__ ((packed)) player_ptz_data_t;

/** [Command] */
/**
The {\tt ptz} interface accepts commands that set change the state of
the camera; the format is given below. Note that
the commands are {\em absolute}, not relative. */
typedef struct player_ptz_cmd
{
  /** Pan (degrees) */
  int16_t pan;

  /** Tilt (degrees) */
  int16_t tilt;

  /** Field of view (degrees). */
  int16_t zoom;
  
} __attribute__ ((packed)) player_ptz_cmd_t;

/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section audio
 *************************************************************************/
#define AUDIO_DATA_BUFFER_SIZE 20
#define AUDIO_COMMAND_BUFFER_SIZE 3*sizeof(short)

/** [Synopsis]
The {\tt audio} interface is used to control sound hardware, if equipped. */

/** [Data] */
/**
The {\tt audio} interface reads the audio stream from {\tt /dev/audio} (which
is assumed to be associated with a sound card connected to a microphone)
and performs some analysis on it.  Five frequency/amplitude pairs are then
returned as data; the format is: */
typedef struct player_audio_data
{
  /** Hz, db ? */
  uint16_t frequency0, amplitude0;
  /** Hz, db ? */
  uint16_t frequency1, amplitude1;
  /** Hz, db ? */
  uint16_t frequency2, amplitude2;
  /** Hz, db ? */
  uint16_t frequency3, amplitude3;
  /** Hz, db ? */
  uint16_t frequency4, amplitude4;
} __attribute__ ((packed)) player_audio_data_t;

/** [Command] */
/** The {\tt audio} interface accepts commands to produce fixed-frequency
tones through {\tt /dev/dsp} (which is assumed to be associated with
a sound card to which a speaker is attached); the format is:*/
typedef struct player_audio_cmd
{
  /** Frequency to play (Hz?) */
  uint16_t frequency;
  /** Amplitude to play (dB?) */
  uint16_t amplitude;
  /** Duration to play (sec?) */
  uint16_t duration;
} __attribute__ ((packed)) player_audio_cmd_t;
/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section waveform
 *************************************************************************/

#define PLAYER_WAVEFORM_DATA_MAX 4096 /*4K - half the packet max*/

/** [Synopsis] The {\tt waveform} interface is used to receive
    arbitrary digital samples, say from a digital audio device. */

/** [Data] */
/**
   The {\tt waveform} interface reads a digitized waveform from the
   target device.*/
typedef struct player_waveform_data
{
  /** Bit rate - bits per second */
  uint32_t rate;
  /** Depth - bits per sample */
  uint16_t depth;
  /** Samples - the number of bytes of raw data */
  uint32_t samples;
  /** data - an array of raw data */
  uint8_t data[ PLAYER_WAVEFORM_DATA_MAX ];
} __attribute__ ((packed)) player_waveform_data_t;

/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section fiducial
 *************************************************************************/

/** [Synopsis]

    The fiducial interface provides access to devices that detect
    coded fiducials (markers) placed in the environment.
 */

/** [Constants] */

/** The maximum number of fiducials that can be detected at one time. */
#define PLAYER_FIDUCIAL_MAX_SAMPLES 32

/** Request packet subtypes */
#define PLAYER_FIDUCIAL_GET_GEOM   0x01

/** [Data]

    The fiducial data packet contains a list of the detected
    fiducials.  Each fiducial is described by the player_fiducial_item
    structure listed below.
*/

/** The fiducial data packet (one fiducial). */
typedef struct player_fiducial_item
{
  /** The fiducial id.  Fiducials that cannot be identified get id
      -1. */
  int16_t id;

  /** Fiducial pose relative to the detector (range, bearing, orient)
      in units (mm, degrees, degrees). */
  int16_t pose[3];

  /** Uncertainty in the measured pose (range, bearing, orient) in
      units of (mm, degrees, degrees). */
  int16_t upose[3];
  
} __attribute__ ((packed)) player_fiducial_item_t;


/** The fiducial data packet (all fiducials). */
typedef struct player_fiducial_data
{
  /** The number of detected fiducials */
  uint16_t count;

  /** List of detected fiducials */
  player_fiducial_item_t fiducials[PLAYER_FIDUCIAL_MAX_SAMPLES];
  
} __attribute__ ((packed)) player_fiducial_data_t;


/** [Command]
    This device accepts no commands.
*/


/** [Configuration: get geometry]

    The geometry (pose and size) of the fiducial device can be queried
    using the PLAYER_FIDUCIAL_GET_GEOM request.  The request and
    reply packets have the same format.
*/

/** Fiducial geometry packet. */
typedef struct player_fiducial_geom
{
  /** Packet subtype.  Must be PLAYER_FIDUCIAL_GET_GEOM. */
  uint8_t subtype;

  /** Pose of the detector in the robot cs (x, y, orient) in units if
      (mm, mm, degrees). */
  uint16_t pose[3];

  /** Size of the detector in units of (mm, mm) */
  uint16_t size[2];  
  
  /** Dimensions of the fiducials in units of (mm, mm). */
  uint16_t fiducial_size[2];
  
} __attribute__ ((packed)) player_fiducial_geom_t;


/*************************************************************************
 ** end section
 *************************************************************************/


/*************************************************************************
 ** begin section comms
 *************************************************************************/

/** [Synopsis]
    The comms interface allows clients to communicate with each other
    through the Player server.
 */


/** [Data]
    This interface returns unstructured, variable length binary
    data.  The packet size must be less than or equal to
    PLAYER_MAX_MESSAGE_SIZE.  Note that more than one data packet may
    be returned in any given cycle (i.e. in the interval between two
    SYNCH packets).
*/

/* Comms data packets.  I've made it a packet so we can add routing
   instructions at some later date. */
typedef struct
{
  /* The message. */
  uint8_t msg;
  
} __attribute__ ((packed)) player_comms_data_t;

/** [Command]
    This interface accepts unstructred, variable length binary data.
    The packet size must be less than or equal to
    PLAYER_MAX_MESSAGE_SIZE.
 */

/* Comms command packets.  I've made it a packet so we can add routing
   instructions at some later date. */
typedef struct
{
  /* The message. */
  uint8_t msg;
  
} __attribute__ ((packed)) player_comms_cmd_t;


/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section speech
 *************************************************************************/

/** [Synopsis]
The {\tt speech} interface provides access to a speech synthesis system. */

/** [Constants] */
/** incoming command queue parameters */
#define PLAYER_SPEECH_MAX_STRING_LEN 256
#define PLAYER_SPEECH_MAX_QUEUE_LEN 4

/** [Data] The {\tt speech} interface returns no data. */

/** [Command] */
/**
The {\tt speech} interface accepts a command that is a string to be given
to the speech synthesizer.  The command packet is simply 256 bytes that
are interpreted as ASCII, and so the maximum length of each string is 256
characters.*/
typedef struct player_speech_cmd
{
  /** The string to say */
  uint8_t string[PLAYER_SPEECH_MAX_STRING_LEN];
} __attribute__ ((packed)) player_speech_cmd_t;
/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section gps
 *************************************************************************/

/** [Synopsis]
The {\tt gps} interface provides access to an absolute position system, such
as GPS. */

/** [Data] */
/**
The {\tt gps} interface gives current global position and heading information; 
the format is: */
typedef struct player_gps_data
{
  /** $(X,Y,\theta)$ in (mm,mm,degrees) */
  int32_t xpos,ypos,heading;
} __attribute__ ((packed)) player_gps_data_t;

/** [Commands]  This interface accepts no commands. */
/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section bumper
 *************************************************************************/

/** [Constants] */
/** Maximum number of bumper samples */
#define PLAYER_BUMPER_MAX_SAMPLES 32
/** The request subtypes */
#define PLAYER_BUMPER_GET_GEOM_REQ          ((uint8_t)1)

/** [Data] */
/**
The {\tt gps} interface gives current global position and heading information; 
the format is: */
typedef struct player_bumper_data
{
  /** the number of valid bumper readings */
  uint8_t bumper_count;

  /** array of bumper values */
  uint8_t bumpers[PLAYER_BUMPER_MAX_SAMPLES];
} __attribute__ ((packed)) player_bumper_data_t;

/** [Commands] This interface accepts no commands. */

/** [Configuration: Query geometry] */
/** To query the geometry of a bumper array, give the following request,
    filling in only the subtype.  The server will repond with the other fields
    filled in. */

/** The geometry of a single bumper */
typedef struct player_bumper_define
{
  /** the local pose of a single bumper in mm */
  int16_t x_offset, y_offset, th_offset;   
  /** length of the sensor in mm  */
  uint16_t length; 
  /** radius of curvature in mm - zero for straight lines */
  uint16_t radius; 
} __attribute__ ((packed)) player_bumper_define_t;

/** The geometry for all bumpers. */
typedef struct player_bumper_geom
{
  /** Packet subtype.  Must be PLAYER_BUMPER_GET_GEOM_REQ. */
  uint8_t subtype;

  /** The number of valid bumper definitions. */
  uint16_t bumper_count;

  /** geometry of each bumper */
  player_bumper_define_t bumper_def[PLAYER_BUMPER_MAX_SAMPLES];
  
} __attribute__ ((packed)) player_bumper_geom_t;

/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section truth
 *************************************************************************/
/** [Synopsis]
The {\tt truth} interface provides access to the absolute state of entities. 
*/

/** [Constants] */
/** Request packet subtypes. */
#define PLAYER_TRUTH_GET_POSE 0x00
#define PLAYER_TRUTH_SET_POSE 0x01

/** [Data] */
/** The {\tt truth} interface returns data concerning the current state of
an entity; the format is: */
typedef struct player_truth_data
{
  /** Object pose in world cs (mm, mm, degrees). */
  int32_t px, py, pa; 

} __attribute__ ((packed)) player_truth_data_t;

/** [Commands] This interface accepts no commands. */

/** [Configuration: Get/set pose] */
/** To get the pose of an object, use the following request, filling in only
 the subtype with PLAYER_TRUTH_GET_POSE.  The server will respond with the other fields filled in.  To set the pose, set the subtype to PLAYER_TRUTH_SET_POS
 and fill in the rest of the fields with the new pose. */
typedef struct player_truth_pose
{
  /** Packet subtype.  Must be either PLAYER_TRUTH_GET_POSE or
    PLAYER_TRUTH_SET_POSE */
  uint8_t subtype;
  
  /** Object pose in world cs (mm, mm, degrees). */
  int32_t px, py, pa; 

} __attribute__ ((packed)) player_truth_pose_t;
/*************************************************************************
 ** end section
 *************************************************************************/

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
  uint8_t mesg[IDARBUFLEN];
  uint8_t len; //0-IDARBUFLEN
  uint8_t intensity; //0-255
} __attribute__ ((packed)) idartx_t;

typedef struct
{
  uint8_t mesg[IDARBUFLEN];
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

/*************************************************************************
 ** begin section dio
 *************************************************************************/

/** [Synopsis]
The {\tt dio} interface provides access to a digital I/O device. */


/** [Data] */
/** The {\tt dio} interface returns data regarding the current state of the
digital inputs; the format is: */
typedef struct player_dio_data
{
  /** number of samples */
  uint8_t count; 
  /** bitfield of samples */
  uint32_t digin;
} __attribute__ ((packed)) player_dio_data_t;


/** [Commands] This interface accepts no commands. */

// TODO: add command

/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section aio
 *************************************************************************/

/** [Synopsis]
The {\tt aio} interface provides access to an analog I/O device. */

/** [Constants] */
/** The maximum number of analog I/O samples */
#define PLAYER_AIO_MAX_SAMPLES 8

/** [Data] */
/**
The {\tt aio} interface returns data regarding the current state of the
analog inputs; the format is: */
typedef struct player_aio_data
{
  /** number of valid samples */
  uint8_t count;
  /** the samples */
  int32_t anin[PLAYER_AIO_MAX_SAMPLES];
} __attribute__ ((packed)) player_aio_data_t;

/** [Commands] This interface accepts no commands. */

// TODO: add command

/*************************************************************************
 ** end section
 *************************************************************************/

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
 *
 * TODO: this struct should be renamed in an interface-specific way and moved
 *       up into the section(s) for which is pertains.  also, request type
 *       codes should be claimed for each one (requests are now part of the
 *       device interface)
 */

typedef struct
{
  uint8_t   request;
  uint8_t   value;
} __attribute__ ((packed)) player_rwi_config_t;
/*************************************************************************/

/*************************************************************************
 ** begin section wifi
 *************************************************************************/
/** [Synopsis]
The {\tt wifi} interface provides access to the state of a wireless network
interface. */

/** [Constants] */
/** The maximum number of remote hosts to report on */
#define PLAYER_WIFI_MAX_LINKS 16

/** link quality is in dBm */
#define PLAYER_WIFI_QUAL_DBM		1
/** link quality is relative */
#define PLAYER_WIFI_QUAL_REL		2
/** link quality is unknown */
#define PLAYER_WIFI_QUAL_UNKNOWN	3

/** unknown operating mode */
#define PLAYER_WIFI_MODE_UNKNOWN	0
/** driver decides the mode */
#define PLAYER_WIFI_MODE_AUTO		1
/** ad hoc mode */
#define PLAYER_WIFI_MODE_ADHOC		2
/** infrastructure mode (multi cell network, roaming) */
#define PLAYER_WIFI_MODE_INFRA		3
/** access point, master mode */
#define PLAYER_WIFI_MODE_MASTER		4
/** repeater mode */
#define PLAYER_WIFI_MODE_REPEAT		5
/** secondary/backup repeater */
#define PLAYER_WIFI_MODE_SECOND		6

/** config requests */
#define PLAYER_WIFI_MAC_REQ		((uint8_t)1)

typedef struct player_wifi_mac_req 
{
  uint8_t		subtype;
} __attribute__ ((packed)) player_wifi_mac_req_t;

/** [Data] */
/** The {\tt wifi} interface returns data regarding the signal characteristics 
    of remote hosts as perceived through a wireless network interface; the 
    format of the data for each host is: */
typedef struct player_wifi_link
{
  /** IP address of destination. */
  char ip[32];

  /** Indicates type of link quality info we have */
  uint8_t qual_type;
  /** Link quality, level and noise information */
  // these could be uint8_t instead, <linux/wireless.h> will only
  // return that much.  maybe some other architecture needs larger??
  uint16_t qual, level, noise;
  uint16_t maxqual, maxlevel, maxnoise;
} __attribute__ ((packed)) player_wifi_link_t;


/** The complete data packet format is: */
typedef struct player_wifi_data
{
  /** A list of links */
  int link_count;
  player_wifi_link_t links[PLAYER_WIFI_MAX_LINKS];

  /** mysterious throughput calculated by driver */
  uint32_t throughput;

  /** current bitrate of device */
  int32_t bitrate;

  /** operating mode of device */
  uint8_t mode;

  /** MAC address of current access point/cell */
  char ap[32];
} __attribute__ ((packed)) player_wifi_data_t;

/** [Commands] This interface accepts no commands. */

/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section ir
 *************************************************************************/

/** [Synopsis]
The {\tt ir} interface provides access to an array of infrared (IR) range
sensors. */

/** [Constants] */
/** Maximum number of samples */
#define PLAYER_IR_MAX_SAMPLES 8
/** config requests */
#define PLAYER_IR_POSE_REQ   ((uint8_t)1)
#define PLAYER_IR_POWER_REQ  ((uint8_t)2)

/** [Data] */
/** The {\tt ir} interface returns range readings from the IR array; the format 
is: */
typedef struct player_ir_data
{
  /** number of samples */
  uint16_t range_count;
  /** voltages (units?) */
  uint16_t voltages[PLAYER_IR_MAX_SAMPLES];
  /** ranges (mm?) */
  uint16_t ranges[PLAYER_IR_MAX_SAMPLES];
} __attribute__ ((packed)) player_ir_data_t;

/** [Commands] This interface accepts no commands. */

/** [Configuration: Query pose] */
/** To query the pose of the IRs, use the following request, filling in only
    the subtype.  The server will respond with the other fields filled in. */

/** gets the pose of the IR sensors on a robot */
typedef struct player_ir_pose
{
  /** the number of ir samples returned by this robot */
  uint16_t pose_count;
  /** the pose of each IR detector on this robot (mm, mm, degrees) */
  int16_t poses[PLAYER_IR_MAX_SAMPLES][3];
} __attribute__ ((packed)) player_ir_pose_t;

/**  ioctl struct for getting IR pose of a robot */
typedef struct player_ir_pose_req
{
  /** subtype; must be PLAYER_IR_POSE_REQ */
  uint8_t subtype; 
  /** the poses */
  player_ir_pose_t poses[PLAYER_IR_MAX_SAMPLES];
} __attribute__ ((packed)) player_ir_pose_req_t;


/** [Configuration: IR power] */
/** To turn IR power on and off, use this request.  The server will reply with
 a zero-length acknowledgement */
typedef struct player_ir_power_req
{
  /** must be PLAYER_IR_POWER_REQ */
  uint8_t subtype; 
  /** 0 for power off, 1 for power on */
  uint8_t state; 
} __attribute__ ((packed)) player_ir_power_req_t;

/*************************************************************************
 ** end section
 *************************************************************************/

/*************************************************************************
 ** begin section localization
 *************************************************************************/

/** [Synopsis] The {\tt localization} interface provides a pose information
using probablistic localization algorithms. */

/** [Constants] */
/** the maximum number of pose hypothesis */
#define PLAYER_LOCALIZATION_MAX_HYPOTHESIS         10
/** the various configuration subtypes */
#define PLAYER_LOCALIZATION_RESET_REQ              ((uint8_t)1)
#define PLAYER_LOCALIZATION_GET_CONFIG_REQ         ((uint8_t)2)
#define PLAYER_LOCALIZATION_SET_CONFIG_REQ         ((uint8_t)3)
#define PLAYER_LOCALIZATION_GET_MAP_HDR_REQ        ((uint8_t)4)
#define PLAYER_LOCALIZATION_GET_MAP_DATA_REQ       ((uint8_t)5)

/** [Data] */
/**
The {\tt localization} interface returns multiple hypothesis, of which format is: */
typedef struct player_localization_hypothesis
{
    /** the central pose (mean) of a hypothesis */
    int32_t mean[3];
    /** the covariance matrix of a hypothesis */
    int32_t cov[3][3];
    /** coefficient for linear combination : int(alpha * billion) */
    uint32_t alpha;
} __attribute__ ((packed)) player_localization_hypothesis_t;

/**
The {\tt localization} interface returns data regarding the pose of the robot;
the format is: */
typedef struct player_localization_data
{
    /** the number of pose hypothesis */
    uint32_t num_hypothesis;
    /** the array of the hypothesis */
    player_localization_hypothesis_t hypothesis[PLAYER_LOCALIZATION_MAX_HYPOTHESIS];
} __attribute__ ((packed)) player_localization_data_t;

/** [Commands] This interface accepts no commands. */

/** [Configuration: Reset particles] */
/**
Reset localization algorithm; there is no parameter necessary. */
typedef struct player_localization_reset
{
    /** subtype; Must be PLAYER_LOCALIZATION_RESET_REQ. */
    uint8_t subtype;
} __attribute__ ((packed)) player_localization_reset_t;

/** [Configuration: Get/set configuration] */
/**
To retrieve the configuration, set the subtype to PLAYER_LOCALIZATION_GET_CONFIG_REQ
and leave the other fields empty. The server will reply with the following
configuaration fields filled in.
 
To change the current configuration, set the subtype to PLAYER_LOCALIZATION_SET_CONFIG_REQ
and fill the configuration fields. */
typedef struct player_localization_config
{ 
  /** subtype; must be either PLAYER_LOCALIZATION_GET_CONFIG_REQ or
      PLAYER_LOCALIZATION_SET_CONFIG_REQ */
  uint8_t subtype;
  /** configuration for particle-based localization algorithms */
  uint32_t num_particles;
} __attribute__ ((packed)) player_localization_config_t;

/** [Configuration: Get a map information] */
/**
Retrieve the size and scale information of a current map. This request is used
to get the size information with a given scale before you request the actual
map data. Set the subtype to PLAYER_LOCALIZATION_GET_MAP_HDR_REQ and set the scale factor,
then the server will reply with the size information filled in. */
typedef struct player_localization_map_header
{
  /** subtype; must be PLAYER_LOCALIZATION_GET_MAP_HDR_REQ */
  uint8_t subtype; 
  /** the scale of the map */
  uint8_t scale; 
  /** the size of the map */
  uint32_t width, height;
  /** the number of pixels per kilo-meter */
  uint32_t ppkm;
} __attribute__ ((packed)) player_localization_map_header_t;

/** [Configuration: Get map data] */
/**
Retrieve the (scaled) map data. Since the maximum size of a request-replay message,
map data should be partitioned and tranfered block by block. Set the subtype to
PLAYER_LOCALIZATION_GET_MAP_DATA_REQ, set the scale factor, and set the row index of a block.
Then, the server will replay with the requested block data filled in. */
typedef struct player_localization_map_data
{
  /** subtype; must be PLAYER_LOCALIZATION_MAP_DATA_REQ */
  uint8_t subtype; 
  /** the scale of the map */
  uint8_t scale;
  /** the index row */
  uint16_t row;
  /** map data */
  uint8_t data[PLAYER_MAX_REQREP_SIZE - 4];
} __attribute__ ((packed)) player_localization_map_data_t;

/*************************************************************************
 ** end section
 *************************************************************************/


#endif /* PLAYER_H */
