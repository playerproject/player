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
 * Desc: Player communication packet structures and codes
 * CVS:  $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H


/* Include values from the configure script */
#include "playerconfig.h"


/*********************************************************/

/* the message start signifier */
#define PLAYER_STXX ((uint16_t) 0x5878)

/* the player transport protocol types */
#define PLAYER_TRANSPORT_TCP 1
#define PLAYER_TRANSPORT_UDP 2

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
#define PLAYER_NULL_CODE           ((uint16_t)256) // /dev/null analogue
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
#define PLAYER_DESCARTES_CODE      ((uint16_t)18)  // the Descartes platform
#define PLAYER_DIO_CODE            ((uint16_t)20)  // digital I/O
#define PLAYER_AIO_CODE            ((uint16_t)21)  // analog I/O
#define PLAYER_IR_CODE             ((uint16_t)22)  // IR array
#define PLAYER_WIFI_CODE	   ((uint16_t)23)  // wifi card status
#define PLAYER_WAVEFORM_CODE	   ((uint16_t)24)  // fetch raw waveforms
#define PLAYER_LOCALIZE_CODE       ((uint16_t)25)  // localization
#define PLAYER_MCOM_CODE           ((uint16_t)26)  // multicoms
#define PLAYER_SOUND_CODE	   ((uint16_t)27)  // sound file playback
#define PLAYER_AUDIODSP_CODE       ((uint16_t)28)  // audio dsp I/O
#define PLAYER_AUDIOMIXER_CODE     ((uint16_t)29)  // audio I/O
#define PLAYER_POSITION3D_CODE     ((uint16_t)30)  // 3-D position
#define PLAYER_SIMULATION_CODE     ((uint16_t)31)  // simulators
#define PLAYER_SERVICE_ADV_CODE    ((uint16_t)32)  // LAN service advertisement
#define PLAYER_BLINKENLIGHT_CODE   ((uint16_t)33)  // blinking lights 
#define PLAYER_NOMAD_CODE          ((uint16_t)34)  // Nomad robot
#define PLAYER_CAMERA_CODE         ((uint16_t)40)  // camera device (gazebo)
#define PLAYER_MAP_CODE            ((uint16_t)42)  // get a map
#define PLAYER_HUD_CODE            ((uint16_t)43)  // get a HUD interface
#define PLAYER_PLANNER_CODE        ((uint16_t)44)  // 2D motion planner
#define PLAYER_LOG_CODE            ((uint16_t)45)  // log read/write control
#define PLAYER_ENERGY_CODE         ((uint16_t)46)  // energy consumption & charging
#define PLAYER_MOTOR_CODE          ((uint16_t)47)  // motor interface
#define PLAYER_POSITION2D_CODE     ((uint16_t)48)  // 2-D position
#define PLAYER_JOYSTICK_CODE       ((uint16_t)49)  // Joytstick


/* the currently assigned device strings */
#define PLAYER_NULL_STRING           "null"
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
#define PLAYER_DIO_STRING            "dio"
#define PLAYER_AIO_STRING            "aio"
#define PLAYER_IR_STRING             "ir"
#define PLAYER_WIFI_STRING           "wifi"
#define PLAYER_WAVEFORM_STRING       "waveform"
#define PLAYER_LOCALIZE_STRING       "localize"
#define PLAYER_MCOM_STRING           "mcom"
#define PLAYER_SOUND_STRING	         "sound"
#define PLAYER_AUDIODSP_STRING       "audiodsp"
#define PLAYER_AUDIOMIXER_STRING     "audiomixer"
#define PLAYER_MOTOR_STRING          "motor"
#define PLAYER_POSITION2D_STRING     "position2d"
#define PLAYER_POSITION3D_STRING     "position3d"
#define PLAYER_SERVICE_ADV_STRING    "service_adv"
#define PLAYER_SIMULATION_STRING     "simulation"
#define PLAYER_BLINKENLIGHT_STRING   "blinkenlight"
#define PLAYER_NOMAD_STRING          "nomad"
#define PLAYER_ENERGY_STRING         "energy"
#define PLAYER_MAP_STRING            "map"
#define PLAYER_HUD_STRING            "hud"
#define PLAYER_PLANNER_STRING        "planner"
#define PLAYER_LOG_STRING            "log"
#define PLAYER_CAMERA_STRING          "camera"
#define PLAYER_JOYSTICK_STRING        "joystick"

/* The maximum number of devices the server will support. */
#define PLAYER_MAX_DEVICES 256

/* maximum size for request/reply.
 * this is a convenience so that the PlayerQueue can used fixed size elements.
 * need to think about this a little
 */
#define PLAYER_MAX_REQREP_SIZE 4096 /*4KB*/

/* the default player port */
#define PLAYER_PORTNUM 6665

/*
 * info that is spit back as a banner on connection
 */
#define PLAYER_IDENT_STRING "Player v."
#define PLAYER_IDENT_STRLEN 32

#define PLAYER_KEYLEN 32

/* Macro for byte-aligning structures; this is a little
   work-around so the file can be parsed by non-GCC pre-processors
   (such as SWIG). */
#ifndef __PACKED__
#define __PACKED__ __attribute__ ((packed))
#endif


/** Generic message header. */
typedef struct player_msghdr
{
  /** Start character; always equal to "xX" (0x5878) */
  uint16_t stx;     
  /** Message type; must be one of PLAYER_MSGTYPE_* */
  uint16_t type;    
  /** What kind of device; must be one of PLAYER_*_CODE */
  uint16_t device;  
  /** Which device of that kind */
  uint16_t device_index; 
  /** Server's current time (seconds since epoch) */
  uint32_t time_sec;  
  /** Server's current time (microseconds since epoch) */
  uint32_t time_usec; 
  /** Time when the current data/response was generated */
  uint32_t timestamp_sec;  
  /** Time when the current data/response was generated */
  uint32_t timestamp_usec; 
  /** For extension */
  uint32_t reserved;  
  /** Size in bytes of the payload to follow */
  uint32_t size;  
} __PACKED__ player_msghdr_t;

#define PLAYER_MAX_PAYLOAD_SIZE (PLAYER_MAX_MESSAGE_SIZE - sizeof(player_msghdr_t))

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_player player

The @p player device represents the server itself, and is used in
configuring the behavior of the server.  There is only one such device
(with index 0) and it is always open.

This device produces no data and accepts no commands.
@{
*/

/* The device access modes */
#define PLAYER_READ_MODE 114   // 'r'
#define PLAYER_WRITE_MODE 119  // 'w'
#define PLAYER_ALL_MODE 97     // 'a'
#define PLAYER_CLOSE_MODE 99   // 'c'
#define PLAYER_ERROR_MODE 101  // 'e'

/** Data delivery mode: Send data at a fixed rate (default 10Hz;
see PLAYER_PLAYER_DATAFREQ_REQ below to change the rate) from ALL
subscribed devices , regardless of whether the data is new or old. A
PLAYER_MSGTYPE_SYNCH packet follows each set of data. Rarely used. */
#define PLAYER_DATAMODE_PUSH_ALL 0
/** Data delivery mode: Only on request (see PLAYER_PLAYER_DATA_REQ
request below), send data from ALL subscribed devices, regardless of
whether the data is new or old.  A PLAYER_MSGTYPE_SYNCH packet follows
each set of data.  Rarely used. */
#define PLAYER_DATAMODE_PULL_ALL 1
/** Data delivery mode: Send data at a fixed rate (default 10Hz; see
PLAYER_PLAYER_DATAFREQ_REQ below to change the rate) only from those
subscribed devices that have produced new data since the last time data
was pushed to this client.  A PLAYER_MSGTYPE_SYNCH packet follows each
set of data.  This is the default mode. */
#define PLAYER_DATAMODE_PUSH_NEW 2
/** Data delivery mode: Only on request (see PLAYER_PLAYER_DATA_REQ
request below), send data only from those subscribed devices that have
produced new data since the last time data was pushed to this client.
Use this mode if your client runs slowly or at an upredictable rate
(e.g., a GUI). A PLAYER_MSGTYPE_SYNCH packet follows each set of data. */
#define PLAYER_DATAMODE_PULL_NEW 3
/** Data delivery mode: When a subscribed device produces new data, send
it. This is the lowest-latency delivery mode; when a device produces data,
the server (almost) immediately sends it on the client.  So the client may
receive data at an arbitrarily high rate. PLAYER_MSGTYPE_SYNCH packets
are still sent, but at a fixed rate (see PLAYER_PLAYER_DATAFREQ_REQ to
change this rate) that is unrelated to rate at which data are delivered
from devices. */
#define PLAYER_DATAMODE_PUSH_ASYNC 4

/* The request subtypes */
#define PLAYER_PLAYER_DEVLIST_REQ     ((uint16_t)1)
#define PLAYER_PLAYER_DRIVERINFO_REQ  ((uint16_t)2)
#define PLAYER_PLAYER_DEV_REQ         ((uint16_t)3)
#define PLAYER_PLAYER_DATA_REQ        ((uint16_t)4)
#define PLAYER_PLAYER_DATAMODE_REQ    ((uint16_t)5)
#define PLAYER_PLAYER_DATAFREQ_REQ    ((uint16_t)6)
#define PLAYER_PLAYER_AUTH_REQ        ((uint16_t)7)
#define PLAYER_PLAYER_NAMESERVICE_REQ ((uint16_t)8)

/** @brief A device identifier.

 Devices are differentiated internally in Player by 
 these identifiers, and some messages contain them. */
typedef struct player_device_id
{
  /** The interface provided by the device; must be one of PLAYER_*_CODE */
  uint16_t code;
  /** The index of the device */
  uint16_t index;
  /** The TCP port of the device */
  uint16_t port;
} __PACKED__ player_device_id_t;


/** @brief Configuration request: Get the list of available devices.

    It's useful for applications such as viewer programs
    and test suites that tailor behave differently depending on which
    devices are available.  To request the list, set the subtype to
    PLAYER_PLAYER_DEVLIST_REQ and leave the rest of the fields blank.
    Player will return a packet with subtype PLAYER_PLAYER_DEVLIST_REQ
    with the fields filled in. */
typedef struct player_device_devlist
{
  /** Subtype; must be PLAYER_PLAYER_DEVLIST_REQ. */
  uint16_t subtype;

  /** The number of devices */
  uint16_t device_count;

  /** The list of available devices. */
  player_device_id_t devices[PLAYER_MAX_DEVICES];
  
} __PACKED__ player_device_devlist_t;

/** @brief Configuration request: Get the driver name for a particular device.

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

} __PACKED__ player_device_driverinfo_t;

/** @brief Configuration request: Get device access. 

This is the most important
request!  Before interacting with a device, the client must request
appropriate access.

  The access codes, which are used in both the request and response, are
 given above.   @b Read access means that the
 server will start sending data from the specified device. For instance, if
 read access is obtained for the sonar device Player will start sending sonar
 data to the client. @b Write access means that the client has permission
 to control the actuators of the device. There is no locking mechanism so
 different clients can have concurrent write access to the same actuators.
 @b All access is both of the above and finally @b close means that
 there is no longer any access to the device. Device request messages can
 be sent at any time, providing on the fly reconfiguration for clients
 that need different devices depending on the task at hand.  

 Of course, not all of the access codes are applicable to all devices;
 for instance it does not make sense to write to the sonars.   However,
 a request for such access will not generate an error; rather, it will be
 granted, but any commands actually sent to that device will be ignored.
 In response to such a device request, the server will send a reply indicating
 the @e actual access that was granted for the device.  The granted access
 may be different from the requested access; in particular, if there was
 some error in initializing the device the granted access will be @b error,
 and the client should not try to read from or write to the device. */
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

} __PACKED__ player_device_req_t;

/** @brief The format of the server's reply to a PLAYER_PLAYER_DEV_REQ
request. */
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
} __PACKED__ player_device_resp_t;


/** @brief Configuration request: Get data.  

When the server is in a PLAYER_DATAMODE_PULL_* data delivery mode
(see next request for information on data delivery modes), the
client can request a single round of data by sending a zero-argument
request with type code @p 0x0003.  The response will be a zero-length
acknowledgement.  The client @e only needs to make this request when a
PLAYER_DATAMODE_PULL_* mode is in use.  */
typedef struct player_device_data_req
{
  /** Subtype; must be PLAYER_PLAYER_DATA_REQ */
  uint16_t subtype;
} __PACKED__ player_device_data_req_t;

/** @brief Configuration request: Change data delivery mode.

The Player server supports four data modes, described above.
By default, the server operates in @p PLAYER_DATAMODE_PUSH_NEW mode at 
a frequency of 10Hz.  To switch to a different mode send a request with the 
format given below.  The server's reply will be a zero-length 
acknowledgement. */
typedef struct player_device_datamode_req
{
  /** Subtype; must be PLAYER_PLAYER_DATAMODE_REQ */
  uint16_t subtype;
  /** The requested mode */
  uint8_t mode;

} __PACKED__ player_device_datamode_req_t;


/** @brief Configuration request: Change data delivery frequency.  

By default, the fixed frequency for the PUSH data delivery modes is
10Hz; thus a client which makes no configuration changes will receive
sensor data approximately every 100ms. The server can send data faster
or slower; to change the frequency, send a request with this format.
The server's reply will be a zero-length acknowledgement. */
typedef struct player_device_datafreq_req
{
  /** Subtype; must be PLAYER_PLAYER_DATAFREQ_REQ */
  uint16_t subtype;
  /** requested frequency in Hz */
  uint16_t frequency;
} __PACKED__ player_device_datafreq_req_t;


/** @brief Configuration request: Authentication.  

If server authentication has been enabled (by providing '-key &lt;key&gt;'
on the command-line; see @ref commandline); then each client must
authenticate itself before otherwise interacting with the server.
To authenticate, send a request with this format.

If the key matches the server's key then the client is authenticated,
the server will reply with a zero-length acknowledgement, and the client
can continue with other operations.  If the key does not match, or if
the client attempts any other server interactions before authenticating,
then the connection will be closed immediately.  It is only necessary
to authenticate each client once.

Note that this support for authentication is @b NOT a security mechanism.
The keys are always in plain text, both in memory and when transmitted
over the network; further, since the key is given on the command-line,
there is a very good chance that you can find it in plain text in the
process table (in Linux try 'ps -ax | grep player').  Thus you should
not use an important password as your key, nor should you rely on
Player authentication to prevent bad guys from driving your robots (use
a firewall instead).  Rather, authentication was introduced into Player
to prevent accidentally connecting one's client program to someone else's
robot.  This kind of accident occurs primarily when Stage is running in
a multi-user environment.  In this case it is very likely that there
is a Player server listening on port 6665, and clients will generally
connect to that port by default, unless a specific option is given.

This mechanism was never really used, and may be removed. */
typedef struct player_device_auth_req
{
  /** Subtype; must by PLAYER_PLAYER_AUTH_REQ */
  uint16_t subtype;
  /** The authentication key */
  uint8_t auth_key[PLAYER_KEYLEN];

} __PACKED__ player_device_auth_req_t;


/** Documentation about nameservice goes here. */
typedef struct player_device_nameservice_req
{
  /** Subtype; must by PLAYER_PLAYER_NAMESERVICE_REQ */
  uint16_t subtype;
  /** The robot name */
  uint8_t name[PLAYER_MAX_DEVICE_STRING_LEN];
  /** The corresponding port */
  uint16_t port;
} __PACKED__ player_device_nameservice_req_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_power power

The @p power interface provides access to a robot's power subsystem. 
This interface accepts no commands
@{
*/

/** @brief Data

The @p power interface returns data in this format. */
typedef struct player_power_data
{
  /** Battery voltage, in decivolts */
  uint16_t  charge;
} __PACKED__ player_power_data_t;


/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_gripper gripper

The @p gripper interface provides access to a robotic gripper.  This
interface is VERY Pioneer-specific, and should really be generalized.
 @{
 */

/** @brief Data

The @p gripper interface returns 2 bytes that represent the current
state of the gripper; the format is given below.  Note that the exact
interpretation of this data may vary depending on the details of your
gripper and how it is connected to your robot (e.g., General I/O vs. User
I/O for the Pioneer gripper).

The following list defines how the data can be interpreted for some
Pioneer robots and Stage: 

- state (unsigned byte) 
  - bit 0: Paddles open
  - bit 1: Paddles closed
  - bit 2: Paddles moving
  - bit 3: Paddles error
  - bit 4: Lift is up
  - bit 5: Lift is down
  - bit 6: Lift is moving
  - bit 7: Lift error
- beams (unsigned byte)
  - bit 0: Gripper limit reached
  - bit 1: Lift limit reached
  - bit 2: Outer beam obstructed
  - bit 3: Inner beam obstructed
  - bit 4: Left paddle open
  - bit 5: Right paddle open
*/
typedef struct player_gripper_data
{
  /** The current gripper lift and breakbeam state */
  uint8_t state, beams;
} __PACKED__ player_gripper_data_t;


/** @brief Command

The @p gripper interface accepts 2-byte commands, the format of which
is given below.  These two bytes are sent directly to the gripper;
refer to Table 3-3 page 10 in the Pioneer 2 Gripper Manual for a list of
commands. The first byte is the command. The second is the argument for
the LIFTcarry and GRIPpress commands, but for all others it is ignored. */
typedef struct player_gripper_cmd
{
  /** the command and optional argument */
  uint8_t cmd, arg; 
} __PACKED__ player_gripper_cmd_t;
/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_motor motor

The @p motor interface is used to control a single motor.
@{
*/

/* The various configuration request types. */
//#define PLAYER_MOTOR_GET_GEOM_REQ             ((uint8_t)1)
#define PLAYER_MOTOR_POWER_REQ                ((uint8_t)2)
#define PLAYER_MOTOR_VELOCITY_MODE_REQ        ((uint8_t)3)
#define PLAYER_MOTOR_RESET_ODOM_REQ           ((uint8_t)4)
#define PLAYER_MOTOR_POSITION_MODE_REQ        ((uint8_t)5)
#define PLAYER_MOTOR_SPEED_PID_REQ            ((uint8_t)6)
#define PLAYER_MOTOR_POSITION_PID_REQ         ((uint8_t)7)
#define PLAYER_MOTOR_SPEED_PROF_REQ           ((uint8_t)8)
#define PLAYER_MOTOR_SET_ODOM_REQ             ((uint8_t)9)
#define PLAYER_MOTOR_SET_GEAR_REDUCITION_REQ  ((uint8_t)10)
#define PLAYER_MOTOR_SET_TICS_REQ             ((uint8_t)11)

/** @brief Data

  The @p motor interface returns data regarding the position and
velocity of the motor, as well as stall information. */
typedef struct player_motor_data
{
  /** Theta in mrad (1 milliradian = ~.06 degrees) */
  int32_t theta;
  /** Angular velocity in mrad/sec */
  int32_t thetaspeed;
  /** Are the motors stalled? */
  uint8_t stall;
} __PACKED__ player_motor_data_t;

/** @brief Command

The @p motor interface accepts new positions and/or velocities
for the motors (drivers may support position control, speed control,
or both). */
typedef struct player_motor_cmd
{
  /** Theta in mrad */
  int32_t theta;
  /** Angular velocities, in mrad/sec */
  int32_t thetaspeed;
  /** Motor state (zero is either off or locked, depending on the driver). */
  uint8_t state;
  /** Command type; 0 = velocity, 1 = position. */
  uint8_t type;
} __PACKED__ player_motor_cmd_t;

/** @brief Configuration request: Change position control. */
typedef struct player_motor_position_mode_req
{
  /** subtype;  must be PLAYER_MOTOR_POSITION_MODE_REQ */
  uint8_t subtype;
  /** 0 for velocity mode, 1 for position mode */
  uint8_t state;
} __PACKED__ player_motor_position_mode_req_t;


/** @brief Configuration request: Change velocity control mode.

Some motors offer different velocity control modes.  It can be changed by
sending a request with the format given below, including the appropriate
mode.  No matter which mode is used, the external client interface to
the @p motor device remains the same.   The server will reply with a
zero-length acknowledgement.*/
typedef struct player_motor_velocitymode_config
{
  /** subtype; must be PLAYER_MOTOR_VELOCITY_MODE_REQ */
  uint8_t request;
  /** driver-specific */
  uint8_t value;
} __PACKED__ player_motor_velocitymode_config_t;

/** @brief Configuration request: Reset odometry.

To reset the motors's odometry to @f$\theta = 0@f$, use the following
request.  The server will reply with a zero-length acknowledgement. */
typedef struct player_motor_resetodom_config
{
  /** subtype; must be PLAYER_MOTOR_RESET_ODOM_REQ */
  uint8_t request;
} __PACKED__ player_motor_resetodom_config_t;

/** @brief Configuration request: Set odometry.

To set the motor's odometry to a particular state, use this request. */
typedef struct player_motor_set_odom_req
{
  /** subtype; must be PLAYER_MOTOR_SET_ODOM_REQ */
  uint8_t subtype;
  /** Theta in mrad */
  int32_t theta;
}__PACKED__ player_motor_set_odom_req_t;

/** @brief Configuration request: Set velocity PID parameters. */
typedef struct player_motor_speed_pid_req
{
  /** subtype; must be PLAYER_MOTOR_SPEED_PID_REQ */
  uint8_t subtype;
  /** PID parameters */
  int32_t kp, ki, kd;
} __PACKED__ player_motor_speed_pid_req_t;

/** @brief Configuration request: Set motor PID parameters. */
typedef struct player_motor_position_pid_req
{
  /** subtype; must be PLAYER_MOTOR_POSITION_PID_REQ */
  uint8_t subtype;
  /** PID parameters */
  int32_t kp, ki, kd;
} __PACKED__ player_motor_position_pid_req_t;

/** @brief Configuration request: Set speed profile parameters.

This is usefull in position control mode when you want to ramp your
acceleration and deacceleration.  */
typedef struct player_motor_speed_prof_req
{
  /** subtype; must be PLAYER_MOTOR_SPEED_PROF_REQ */
  uint8_t subtype;
  /** max speed */
  int32_t speed;
  /** max acceleration */
  int32_t acc;
} __PACKED__ player_motor_speed_prof_req_t;

/** @brief Configuration request: Motor power.

On some robots, the motor power can be turned on and off from software.
To do so, send a request with the format given below, and with the
appropriate @p state (zero for motors off and non-zero for motors on).
The server will reply with a zero-length acknowledgement.

Be VERY careful with this command!  You are very likely to start the
robot running across the room at high speed with the battery charger
still attached.  */
typedef struct player_motor_power_config
{
  /** subtype; must be PLAYER_MOTOR_MOTOR_POWER_REQ */
  uint8_t request;
  /** 0 for off, 1 for on */
  uint8_t value;
} __PACKED__ player_motor_power_config_t;

/** @} */
/** @} */


/***************************************************************************/
/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_position position

The @p position interface is used to control mobile robot bases in 2D.
@{
*/

/* The various configuration request types. */
#define PLAYER_POSITION_GET_GEOM_REQ          ((uint8_t)1)
#define PLAYER_POSITION_MOTOR_POWER_REQ       ((uint8_t)2)
#define PLAYER_POSITION_VELOCITY_MODE_REQ     ((uint8_t)3)
#define PLAYER_POSITION_RESET_ODOM_REQ        ((uint8_t)4)
#define PLAYER_POSITION_POSITION_MODE_REQ     ((uint8_t)5)
#define PLAYER_POSITION_SPEED_PID_REQ         ((uint8_t)6)
#define PLAYER_POSITION_POSITION_PID_REQ      ((uint8_t)7)
#define PLAYER_POSITION_SPEED_PROF_REQ        ((uint8_t)8)
#define PLAYER_POSITION_SET_ODOM_REQ          ((uint8_t)9)

/* These are possible Segway RMP config commands; see the status command
in the RMP manual */
#define PLAYER_POSITION_RMP_VELOCITY_SCALE      ((uint8_t)51)
#define PLAYER_POSITION_RMP_ACCEL_SCALE         ((uint8_t)52)
#define PLAYER_POSITION_RMP_TURN_SCALE          ((uint8_t)53)
#define PLAYER_POSITION_RMP_GAIN_SCHEDULE       ((uint8_t)54)
#define PLAYER_POSITION_RMP_CURRENT_LIMIT       ((uint8_t)55)
#define PLAYER_POSITION_RMP_RST_INTEGRATORS     ((uint8_t)56)
#define PLAYER_POSITION_RMP_SHUTDOWN            ((uint8_t)57)

/* These are used for resetting the Segway RMP's integrators. */
#define PLAYER_POSITION_RMP_RST_INT_RIGHT       0x01
#define PLAYER_POSITION_RMP_RST_INT_LEFT        0x02
#define PLAYER_POSITION_RMP_RST_INT_YAW         0x04
#define PLAYER_POSITION_RMP_RST_INT_FOREAFT     0x08


/** @brief Data

The @p position interface returns data regarding the odometric pose and
velocity of the robot, as well as motor stall information. */
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
} __PACKED__ player_position_data_t;

/** @brief Command

The @p position interface accepts new positions and/or velocities
for the robot's motors (drivers may support position control, speed control,
or both). */
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
  /** Motor state (zero is either off or locked, depending on the driver). */
  uint8_t state;
  /** Command type; 0 = velocity, 1 = position. */
  uint8_t type;
} __PACKED__ player_position_cmd_t;

/** @brief Configuration request: Query geometry.  

To request robot geometry,
set the subtype to PLAYER_POSITION_GET_GEOM_REQ and leave the other fields
empty.  The server will reply with the pose and size fields filled in. */
typedef struct player_position_geom
{
  /** Packet subtype.  Must be PLAYER_POSITION_GET_GEOM_REQ. */
  uint8_t subtype;
  /** Pose of the robot base, in the robot cs (mm, mm, degrees). */
  uint16_t pose[3];
  /** Dimensions of the base (mm, mm). */
  uint16_t size[2];
} __PACKED__ player_position_geom_t;


/** @brief Configuratoin request: Motor power.  

  On some robots, the motor power
can be turned on and off from software.  To do so, send a request with
the format given below, and with the appropriate @p state (zero for
motors off and non-zero for motors on).  The server will reply with a
zero-length acknowledgement.

Be VERY careful with this command!  You are very likely to start the
robot running across the room at high speed with the battery charger
still attached.
*/
typedef struct player_position_power_config
{ 
  /** subtype; must be PLAYER_POSITION_MOTOR_POWER_REQ */
  uint8_t request;
  /** 0 for off, 1 for on */
  uint8_t value; 
} __PACKED__ player_position_power_config_t;

/** @brief Configuration request: Change velocity control. 

  Some robots offer
different velocity control modes.  It can be changed by sending a
request with the format given below, including the appropriate mode.
No matter which mode is used, the external client interface to the
@p position device remains the same.  The server will reply with a
zero-length acknowledgement.

The @ref player_driver_p2os driver offers two modes of velocity control:
separate translational and rotational control and direct wheel control.
When in the separate mode, the robot's microcontroller internally
computes left and right wheel velocities based on the currently commanded
translational and rotational velocities and then attenuates these values
to match a nice predefined acceleration profile.  When in the direct
mode, the microcontroller simply passes on the current left and right
wheel velocities.  Essentially, the separate mode offers smoother but
slower (lower acceleration) control, and the direct mode offers faster
but jerkier (higher acceleration) control.  Player's default is to use
the direct mode.  Set @a mode to zero for direct control and non-zero
for separate control.

For the @ref player_driver_reb driver, 0 is direct velocity control,
1 is for velocity-based heading PD controller.
*/
typedef struct player_position_velocitymode_config
{
  /** subtype; must be PLAYER_POSITION_VELOCITY_MODE_REQ */
  uint8_t request; 
  /** driver-specific */
  uint8_t value; 
} __PACKED__ player_position_velocitymode_config_t;

/** @brief Configuration request: Reset odometry.  

  To reset the robot's odometry
to @f$(x,y,\theta) = (0,0,0)@f$, use the following request.  The server will
reply with a zero-length acknowledgement. */
typedef struct player_position_resetodom_config
{
  /** subtype; must be PLAYER_POSITION_RESET_ODOM_REQ */
  uint8_t request; 
} __PACKED__ player_position_resetodom_config_t;

/** @brief Configuration request: Change control mode. */
typedef struct player_position_position_mode_req
{
  /** subtype;  must be PLAYER_POSITION_POSITION_MODE_REQ */
  uint8_t subtype; 
  /** 0 for velocity mode, 1 for position mode */
  uint8_t state; 
} __PACKED__ player_position_position_mode_req_t;

/** @brief Configuration request: Set odometry.  

To set the robot's odometry
to a particular state, use this request: */
typedef struct player_position_set_odom_req
{
  /** subtype; must be PLAYER_POSITION_SET_ODOM_REQ */
  uint8_t subtype; 
  /** X and Y (in mm) */
  int32_t x, y;
  /** Heading (in degrees) */
  int32_t theta;
}__PACKED__ player_position_set_odom_req_t;

/** @brief Configuration request: Set velocity PID parameters. */
typedef struct player_position_speed_pid_req
{
  /** subtype; must be PLAYER_POSITION_SPEED_PID_REQ */
  uint8_t subtype;
  /** PID parameters */
  int32_t kp, ki, kd;
} __PACKED__ player_position_speed_pid_req_t;

/** @brief Configuration request: Set position PID parameters. */
typedef struct player_position_position_pid_req
{
  /** subtype; must be PLAYER_POSITION_POSITION_PID_REQ */
  uint8_t subtype; 
  /** PID parameters */
  int32_t kp, ki, kd;
} __PACKED__ player_position_position_pid_req_t;

/** @brief Configuration request: Set speed profile parameters. */
typedef struct player_position_speed_prof_req
{
  /** subtype; must be PLAYER_POSITION_SPEED_PROF_REQ */
  uint8_t subtype; 
  /** max speed */
  int16_t speed;
  /** max acceleration */
  int16_t acc;
} __PACKED__ player_position_speed_prof_req_t;

/** @brief Configuration request: Segway RMP-specific configuration. */
typedef struct player_rmp_config 
{
  /** subtype: must be of PLAYER_RMP_* */
  uint8_t subtype;
  /** Holds various values depending on the type of config.
      See the "Status" command in the Segway manual. */
  uint16_t value;
} __PACKED__ player_rmp_config_t;


/** @} */
/** @} */


/***************************************************************************/
/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_position2d position2d

The @p position2d interface is used to control a planar mobile robot
base.  This interface is the same as the @p position interface, but uses
different units (e.g., mrad instead of deg).  The @p position2d interface
will probably replace the @p position interface (eventually).
@{
*/

/* The various configuration request types. */
#define PLAYER_POSITION2D_GET_GEOM_REQ          ((uint8_t)1)
#define PLAYER_POSITION2D_MOTOR_POWER_REQ       ((uint8_t)2)
#define PLAYER_POSITION2D_VELOCITY_MODE_REQ     ((uint8_t)3)
#define PLAYER_POSITION2D_RESET_ODOM_REQ        ((uint8_t)4)
#define PLAYER_POSITION2D_POSITION_MODE_REQ     ((uint8_t)5)
#define PLAYER_POSITION2D_SPEED_PID_REQ         ((uint8_t)6)
#define PLAYER_POSITION2D_POSITION_PID_REQ      ((uint8_t)7)
#define PLAYER_POSITION2D_SPEED_PROF_REQ        ((uint8_t)8)
#define PLAYER_POSITION2D_SET_ODOM_REQ          ((uint8_t)9)

/* These are possible Segway RMP config commands; see the status command in
the RMP manual */
#define PLAYER_POSITION2D_RMP_VELOCITY_SCALE      ((uint8_t)51)
#define PLAYER_POSITION2D_RMP_ACCEL_SCALE         ((uint8_t)52)
#define PLAYER_POSITION2D_RMP_TURN_SCALE          ((uint8_t)53)
#define PLAYER_POSITION2D_RMP_GAIN_SCHEDULE       ((uint8_t)54)
#define PLAYER_POSITION2D_RMP_CURRENT_LIMIT       ((uint8_t)55)
#define PLAYER_POSITION2D_RMP_RST_INTEGRATORS     ((uint8_t)56)
#define PLAYER_POSITION2D_RMP_SHUTDOWN            ((uint8_t)57)

/* These are used for resetting the Segway RMP's integrators. */
#define PLAYER_POSITION2D_RMP_RST_INT_RIGHT       0x01
#define PLAYER_POSITION2D_RMP_RST_INT_LEFT        0x02
#define PLAYER_POSITION2D_RMP_RST_INT_YAW         0x04
#define PLAYER_POSITION2D_RMP_RST_INT_FOREAFT     0x08

/** @brief Data

The @p position2d interface returns data regarding the odometric pose
and velocity of the robot, as well as motor stall information */
typedef struct player_position2d_data
{
  /** X and Y position, in mm */
  int32_t xpos, ypos;
  /** Yaw, in mrad */
  int32_t yaw;
  /** X and Y translational velocities, in mrad/sec */
  int32_t xspeed, yspeed;
  /** Angular velocity, in mrad/sec */
  int32_t yawspeed;
  /** Are the motors stalled? */
  uint8_t stall;
} __PACKED__ player_position2d_data_t;

/** @brief Command

The @p position2d interface accepts new positions and/or velocities for
the robot's motors (drivers may support position control, speed control,
or both). */
typedef struct player_position2d_cmd
{
  /** X and Y position2d, in mm */
  int32_t xpos, ypos;
  /** Yaw, in mrad */
  int32_t yaw;
  /** X and Y translational velocities, in mrad/sec */
  int32_t xspeed, yspeed;
  /** Angular velocity, in mrad/sec */
  int32_t yawspeed;
  /** Motor state (zero is either off or locked, depending on the driver). */
  uint8_t state;
  /** Command type; 0 = velocity, 1 = position. */
  uint8_t type;
} __PACKED__ player_position2d_cmd_t;

/** @brief Configuration request: Query geometry.

To request robot geometry, set the subtype to
PLAYER_POSITION2D_GET_GEOM_REQ and leave the other fields empty.
The server will reply with the pose and size fields filled in. */
typedef struct player_position2d_geom
{
  /** Packet subtype.  Must be PLAYER_POSITION2D_GET_GEOM_REQ. */
  uint8_t subtype;
  /** Pose of the robot base, in the robot cs (mm, mm, mrad). */
  uint16_t pose[3];
  /** Dimensions of the base (mm, mm). */
  uint16_t size[2];
} __PACKED__ player_position2d_geom_t;

/** @brief Configuration request : Motor power.

On some robots, the motor power can be turned on and off from software.
To do so, send a request with the format given below, and with the
appropriate @p state (zero for motors off and non-zero for motors on).
The server will reply with a zero-length acknowledgement.

Be VERY careful with this command!  You are very likely to start the
robot running across the room at high speed with the battery charger
still attached.  */
typedef struct player_position2d_power_config
{
  /** subtype; must be PLAYER_POSITION2D_MOTOR_POWER_REQ */
  uint8_t request;
  /** 0 for off, 1 for on */
  uint8_t value;
} __PACKED__ player_position2d_power_config_t;

/** @brief Configuration request: Change velocity control.

Some robots offer different velocity control modes.  It can be changed by
sending a request with the format given below, including the appropriate
mode.  No matter which mode is used, the external client interface to
the @p position2d device remains the same.  The server will reply with
a zero-length acknowledgement.

The @ref player_driver_p2os driver offers two modes of velocity control:
separate translational and rotational control and direct wheel control.
When in the separate mode, the robot's microcontroller internally
computes left and right wheel velocities based on the currently commanded
translational and rotational velocities and then attenuates these values
to match a nice predefined acceleration profile.  When in the direct
mode, the microcontroller simply passes on the current left and right
wheel velocities.  Essentially, the separate mode offers smoother but
slower (lower acceleration) control, and the direct mode offers faster
but jerkier (higher acceleration) control.  Player's default is to use
the direct mode.  Set @a mode to zero for direct control and non-zero
for separate control.

For the @ref player_driver_reb driver, 0 is direct velocity control,
1 is for velocity-based heading PD controller.*/ typedef struct
player_position2d_velocitymode_config
{
  /** subtype; must be PLAYER_POSITION2D_VELOCITY_MODE_REQ */
  uint8_t request;
  /** driver-specific */
  uint8_t value;
} __PACKED__ player_position2d_velocitymode_config_t;


/** @brief Configuration request: Reset odometry.

To reset the robot's odometry to @f$(x,y,\theta) = (0,0,0)@f$, use
the following request.  The server will reply with a zero-length
acknowledgement. */
typedef struct player_position2d_resetodom_config
{
  /** subtype; must be PLAYER_POSITION2D_RESET_ODOM_REQ */
  uint8_t request;
} __PACKED__ player_position2d_resetodom_config_t;

/** @brief Configuration request: Change position control. */
typedef struct player_position2d_position_mode_req
{
  /** subtype;  must be PLAYER_POSITION2D_POSITION_MODE_REQ */
  uint8_t subtype;
  /** 0 for velocity mode, 1 for position mode */
  uint8_t state;
} __PACKED__ player_position2d_position_mode_req_t;

/** @brief Configuration request: Set odometry.

To set the robot's odometry to a particular state, use this request. */
typedef struct player_position2d_set_odom_req
{
  /** subtype; must be PLAYER_POSITION2D_SET_ODOM_REQ */
  uint8_t subtype;
  /** X and Y (in mm) */
  int32_t x, y;
  /** Heading (in mrad) */
  int32_t theta;
}__PACKED__ player_position2d_set_odom_req_t;

/** @brief Configuration request: Set velocity PID parameters. */
typedef struct player_position2d_speed_pid_req
{
  /** subtype; must be PLAYER_POSITION2D_SPEED_PID_REQ */
  uint8_t subtype;
  /** PID parameters */
  int32_t kp, ki, kd;
} __PACKED__ player_position2d_speed_pid_req_t;

/** @brief Configuration request: Set position PID parameters. */
typedef struct player_position2d_position_pid_req
{
  /** subtype; must be PLAYER_POSITION2D_POSITION_PID_REQ */
  uint8_t subtype;
  /** PID parameters */
  int32_t kp, ki, kd;
} __PACKED__ player_position2d_position_pid_req_t;

/** @brief Configuration request: Set speed profile parameters. */
typedef struct player_position2d_speed_prof_req
{
  /** subtype; must be PLAYER_POSITION2D_SPEED_PROF_REQ */
  uint8_t subtype;
  /** max speed mrad/s*/
  int32_t speed;
  /** max acceleration mrad/s/s*/
  int32_t acc;
} __PACKED__ player_position2d_speed_prof_req_t;

/** @} */
/** @} */


/***************************************************************************/
/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_position3d position3d

The position3d interface is used to control mobile robot bases in 3D
(i.e., pitch and roll are important).
@{
*/

/* Supported config requests */
#define PLAYER_POSITION3D_GET_GEOM_REQ          ((uint8_t)1)
#define PLAYER_POSITION3D_MOTOR_POWER_REQ       ((uint8_t)2)
#define PLAYER_POSITION3D_VELOCITY_MODE_REQ     ((uint8_t)3)
#define PLAYER_POSITION3D_RESET_ODOM_REQ        ((uint8_t)4)
#define PLAYER_POSITION3D_POSITION_MODE_REQ     ((uint8_t)5)
#define PLAYER_POSITION3D_SPEED_PID_REQ         ((uint8_t)6)
#define PLAYER_POSITION3D_POSITION_PID_REQ      ((uint8_t)7)
#define PLAYER_POSITION3D_SPEED_PROF_REQ        ((uint8_t)8)
#define PLAYER_POSITION3D_SET_ODOM_REQ          ((uint8_t)9)

/** @brief Data

This interface returns data regarding the odometric pose and velocity
of the robot, as well as motor stall information.  */
typedef struct player_position3d_data
{
  /** X, Y, and Z position, in mm */
  int32_t xpos, ypos, zpos;
  /** Roll, pitch, and yaw, in mrad */
  int32_t roll, pitch, yaw;
  /** X, Y, and Z translational velocities, in mm/s */
  int32_t xspeed, yspeed, zspeed;
  /** Angular velocities, in mrad/s */
  int32_t rollspeed, pitchspeed, yawspeed;
  /** Are the motors stalled? */
  uint8_t stall;
} __PACKED__ player_position3d_data_t;


/** @brief Command

It accepts new positions and/or velocities for the robot's motors
(drivers may support position control, speed control, or both).  */
typedef struct player_position3d_cmd
{
  /** X, Y, and Z position, in mm */
  int32_t xpos, ypos, zpos;
  /** Roll, pitch, and yaw, in mrad */
  uint32_t roll, pitch, yaw;
  /** X, Y, and Z translational velocities, in mm/s */
  int32_t xspeed, yspeed, zspeed;
  /** Angular velocities, in mrad/s */
  int32_t rollspeed, pitchspeed, yawspeed;
  /** Motor state (zero is either off or locked, depending on the driver). */
  uint8_t state;
  /** Command type; 0 = velocity, 1 = position. */
  uint8_t type;
} __PACKED__ player_position3d_cmd_t;


/** @brief Configuration request: Query geometry.

To request robot geometry, set the subtype to PLAYER_POSITION_GET_GEOM_REQ
and leave the other fields empty.  The server will reply with the pose
and size fields filled in.  */
typedef struct player_position3d_geom
{
  /** Packet subtype.  Must be PLAYER_POSITION_GET_GEOM_REQ. */
  uint8_t subtype;

  /** Pose of the robot base, in the robot cs (mm, mm, mm, mrad, mrad, mrad).*/
  uint16_t pose[6];

  /** Dimensions of the base (mm, mm, mm). */
  uint16_t size[3];

} __PACKED__ player_position3d_geom_t;


/** @brief Configuration request: Motor power.

On some robots, the motor power can be turned on and off from software.
To do so, send a request with the format given below, and with the
appropriate @p state (zero for motors off and non-zero for motors on).
The server will reply with a zero-length acknowledgement.

Be VERY careful with this command!  You are very likely to start the
robot running across the room at high speed with the battery charger
still attached.  */
typedef struct player_position3d_power_config
{
  /** subtype; must be PLAYER_POSITION3D_MOTOR_POWER_REQ */
  uint8_t request;
  /** 0 for off, 1 for on */
  uint8_t value;
} __PACKED__ player_position3d_power_config_t;


/** @brief Configuration request: Change position control. */
typedef struct player_position3d_position_mode_req
{
  /** subtype;  must be PLAYER_POSITION_POSITION_MODE_REQ */
  uint8_t subtype;
  /** 0 for velocity mode, 1 for position mode */
  uint8_t state;
} __PACKED__ player_position3d_position_mode_req_t;


/** @brief Configuration request: Change velocity control.  

Some robots offer different velocity control modes.  It can be changed by
sending a request with the format given below, including the appropriate
mode.  No matter which mode is used, the external client interface to
the @p position3d device remains the same.   The server will reply with
a zero-length acknowledgement */
typedef struct player_position3d_velocitymode_config
{
  /** subtype; must be PLAYER_POSITION3D_VELOCITY_MODE_REQ */
  uint8_t request;
  /** driver-specific */
  uint8_t value;
} __PACKED__ player_position3d_velocitymode_config_t;


/** @brief Configuration request: Set odometry.

To set the robot's odometry to a particular state, use this request.  */
typedef struct player_position3d_set_odom_req
{
  /** subtype; must be PLAYER_POSITION3D_SET_ODOM_REQ */
  uint8_t subtype;
  /** X, Y, and Z (in mm) */
  int32_t x, y, z;
  /** Heading (in mrad) */
  int32_t roll, pitch, yaw;
}__PACKED__ player_position3d_set_odom_req_t;


/** @brief Configuration request: Reset odometry.

To reset the robot's odometry to @f$(x,y,\theta) = (0,0,0)@f$, use this
request.  The server will reply with a zero-length acknowledgement.  */
typedef struct player_position3d_resetodom_config
{
  /** subtype; must be PLAYER_POSITION3D_RESET_ODOM_REQ */
  uint8_t request;
} __PACKED__ player_position3d_resetodom_config_t;


/** @brief Configuration request: Set velocity PID parameters. */
typedef struct player_position3d_speed_pid_req
{
  /** subtype; must be PLAYER_POSITION3D_SPEED_PID_REQ */
  uint8_t subtype;
  /** PID parameters */
  int32_t kp, ki, kd;
} __PACKED__ player_position3d_speed_pid_req_t;


/** @brief Configuration request: Set position PID parameters. */
typedef struct player_position3d_position_pid_req
{
  /** subtype; must be PLAYER_POSITION3D_POSITION_PID_REQ */
  uint8_t subtype;
  /** PID parameters */
  int32_t kp, ki, kd;
} __PACKED__ player_position3d_position_pid_req_t;

/** @brief Configuration request: Set odometry. */
typedef struct player_position3d_speed_prof_req
{
  /** subtype; must be PLAYER_POSITION3D_SPEED_PROF_REQ */
  uint8_t subtype;
  /** max speed (in mrad/s)*/
  int32_t speed;
  /** max acceleration (in mrad/s/s)*/
  int32_t acc;
} __PACKED__ player_position3d_speed_prof_req_t;

/** @} */
/** @} */

/***************************************************************************/
/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_sonar sonar

The @p sonar interface provides access to a collection of fixed range
sensors, such as a sonar array.  This interface accepts no commands.
@{
*/

/** maximum number of sonar samples in a data packet */
#define PLAYER_SONAR_MAX_SAMPLES 64
/* request types */
#define PLAYER_SONAR_GET_GEOM_REQ   ((uint8_t)1)
#define PLAYER_SONAR_POWER_REQ      ((uint8_t)2)

/** @brief Data

The @p sonar interface returns up to PLAYER_SONAR_MAX_SAMPLES range
readings from a robot's sonars. */
typedef struct player_sonar_data
{
  /** The number of valid range readings. */
  uint16_t range_count;
  /** The range readings */
  uint16_t ranges[PLAYER_SONAR_MAX_SAMPLES];
} __PACKED__ player_sonar_data_t;


/** @brief Configuration request: Query geometry.

To query the geometry of the sonar transducers, use this request, but
only fill in the subtype.  The server will reply with the other fields
filled in. */
typedef struct player_sonar_geom
{
  /** Subtype.  Must be PLAYER_SONAR_GET_GEOM_REQ. */
  uint8_t subtype;
  /** The number of valid poses. */
  uint16_t pose_count;
  /** Pose of each sonar, in robot cs (mm, mm, degrees). */
  int16_t poses[PLAYER_SONAR_MAX_SAMPLES][3];
} __PACKED__ player_sonar_geom_t;

/** @brief Configuration request: Sonar power.

On some robots, the sonars can be turned on and off from software.
To do so, issue a request of this form.  The server will reply with a
zero-length acknowledgement. */
typedef struct player_sonar_power_config
{
  /** Packet subtype.  Must be PLAYER_SONAR_POWER_REQ. */
  uint8_t subtype;
  /** Turn power off (0) or on (>0) */
  uint8_t value;
} __PACKED__ player_sonar_power_config_t;

/** @} */
/** @} */


/*************************************************************************/
/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_laser laser

The laser interface provides access to a single-origin scanning range
sensor, such as a SICK laser range-finder (e.g., @ref player_driver_sicklms200).

Devices supporting the laser interface can be configured to scan at
different angles and resolutions.  As such, the data returned by the
laser interface can take different forms.  To make interpretation of the
data simple, the laser data packet contains some extra fields before the
actual range data.  These fields tell the client the starting and ending
angles of the scan, the angular resolution of the scan, and the number of
range readings included.  Scans proceed counterclockwise about the laser
(0 degrees is forward).  The laser can return a maximum of 401 readings;
this limits the valid combinations of scan width and angular resolution.

This interface accepts no commands.

@{
*/

/** The maximum number of laser range values */
#define PLAYER_LASER_MAX_SAMPLES  401

/* Laser request subtypes. */
#define PLAYER_LASER_GET_GEOM   0x01
#define PLAYER_LASER_SET_CONFIG 0x02
#define PLAYER_LASER_GET_CONFIG 0x03
#define PLAYER_LASER_POWER_CONFIG 0x04

/** @brief Data

The laser data packet.  */
typedef struct player_laser_data
{
  /** Start and end angles for the laser scan (in units of 0.01
      degrees).  */
  int16_t min_angle, max_angle;
  /** Angular resolution (in units of 0.01 degrees).  */
  uint16_t resolution;
  /** range resolution.  ranges should be multipled by this. */
  uint16_t range_res;
  /** Number of range/intensity readings.  */
  uint16_t range_count;
  /** Range readings (mm). */
  uint16_t ranges[PLAYER_LASER_MAX_SAMPLES];
  /** Intensity readings. */
  uint8_t intensity[PLAYER_LASER_MAX_SAMPLES];
} __PACKED__ player_laser_data_t;

/** @brief Configuration request:  Get geometry.

The laser geometry (position and size) can be queried using the
PLAYER_LASER_GET_GEOM request.  The request and reply packets have the
same format. */
typedef struct player_laser_geom
{
  /** The packet subtype.  Must be PLAYER_LASER_GET_GEOM. */
  uint8_t subtype;
  /** Laser pose, in robot cs (mm, mm, degrees). */
  int16_t pose[3];
  /** Laser dimensions (mm, mm). */
  int16_t size[2];
} __PACKED__ player_laser_geom_t;

/** @brief Configuration request: Get/set scan properties.

The scan configuration (resolution, aperture, etc) can be queried
using the PLAYER_LASER_GET_CONFIG request and modified using the
PLAYER_LASER_SET_CONFIG request.  Read the documentation for your driver
to determine what configuration values are permissible. */
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

  /** Range Resolution.  Valid: 1, 10, 100 (For mm, cm, dm). */
  uint16_t range_res;

  /** Enable reflection intensity data. */
  uint8_t  intensity;
  
} __PACKED__ player_laser_config_t;


/** @brief Configuration request: Turn power on/off.

Use this request to turn laser power on or off (assuming your hardware
supports it). */
typedef struct player_laser_power_config
{
  /** Must be PLAYER_LASER_POWER_CONFIG. */
  uint8_t subtype;
  /** 0 to turn laser off, 1 to turn laser on */
  uint8_t value;
} __PACKED__ player_laser_power_config_t;

/** @} */
/** @} */

/***************************************************************************/
/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_blobfinder blobfinder

The blobfinder interface provides access to devices that detect blobs
in images.

@{
*/

/** The maximum number of blobs in total. */
#define PLAYER_BLOBFINDER_MAX_BLOBS 256

/* Config request codes */
#define PLAYER_BLOBFINDER_SET_COLOR_REQ             ((uint8_t)1)
#define PLAYER_BLOBFINDER_SET_IMAGER_PARAMS_REQ     ((uint8_t)2)


/** @brief Structure describing a single blob. */
typedef struct player_blobfinder_blob
{
  /** Blob id. */
  int16_t id;
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
} __PACKED__ player_blobfinder_blob_t;

/** @brief Data

The list of detected blobs, returned as data by @p blobfinder devices. */
typedef struct player_blobfinder_data
{
  /** The image dimensions. */
  uint16_t width, height;
  /** The list of blobs. */
  uint16_t blob_count;
  player_blobfinder_blob_t blobs[PLAYER_BLOBFINDER_MAX_BLOBS];
} __PACKED__ player_blobfinder_data_t;


/** @brief Configuration request: Set tracking color.

For some sensors (ie CMUcam), simple blob tracking tracks only one
color.  To set the tracking color, send a request with the format below,
including the RGB color ranges (max and min).  Values of -1 will cause
the track color to be automatically set to the current window color.
This is useful for setting the track color by holding the tracking object
in front of the lens.
*/
typedef struct player_blobfinder_color_config
{ 
  /** Must be PLAYER_BLOBFINDER_SET_COLOR_REQ. */
  uint8_t subtype;
  /** RGB minimum and max values (0-255) **/
  int16_t rmin, rmax;
  int16_t gmin, gmax;
  int16_t bmin, bmax;
} __PACKED__ player_blobfinder_color_config_t;


/** @brief Configuration request: Set imager params.

Imaging sensors that do blob tracking generally have some sorts of
image quality parameters that you can tweak.  The following ones
are implemented here:
   - brightness  (0-255)
   - contrast    (0-255)
   - auto gain   (0=off, 1=on)
   - color mode  (0=RGB/AutoWhiteBalance Off,  1=RGB/AutoWhiteBalance On,
                2=YCrCB/AWB Off, 3=YCrCb/AWB On)
To set the params, send a request with the format below.  Any
values set to -1 will be left unchanged.
*/
typedef struct player_blobfinder_imager_config
{ 
  /** Must be PLAYER_BLOBFINDER_SET_IMAGER_PARAMS_REQ. */
  uint8_t subtype;
  /** Contrast & Brightness: (0-255)  -1=no change. */
  int16_t brightness;
  int16_t contrast;
  /** Color Mode
      ( 0=RGB/AutoWhiteBalance Off,  1=RGB/AutoWhiteBalance On,
      2=YCrCB/AWB Off, 3=YCrCb/AWB On)  -1=no change.
  */
  int8_t  colormode;
  /** AutoGain:   0=off, 1=on.  -1=no change. */
  int8_t  autogain;
} __PACKED__ player_blobfinder_imager_config_t;

/** @} */
/** @} */


/*************************************************************************/
/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_ptz ptz

The ptz interface is used to control a pan-tilt-zoom unit, such as a camera.

@{
*/

/** Code for generic configuration request */
#define PLAYER_PTZ_GENERIC_CONFIG_REQ	((uint8_t)1)
/** Code for control mode configuration request */
#define PLAYER_PTZ_CONTROL_MODE_REQ ((uint8_t)2)
/** Code for autoservo configuration request */
#define PLAYER_PTZ_AUTOSERVO ((uint8_t)3)

/** Maximum command length for use with PLAYER_PTZ_GENERIC_CONFIG_REQ, 
    based on the Sony EVID30 camera right now. */
#define PLAYER_PTZ_MAX_CONFIG_LEN	32

/** Control mode, for use with PLAYER_PTZ_CONTROL_MODE_REQ */
#define PLAYER_PTZ_VELOCITY_CONTROL 0
/** Control mode, for use with PLAYER_PTZ_CONTROL_MODE_REQ */
#define PLAYER_PTZ_POSITION_CONTROL 1

/** @brief Data

The ptz interface returns data reflecting the current state of the
Pan-Tilt-Zoom unit. */
typedef struct player_ptz_data
{
  /** Pan (degrees) */
  int16_t pan;
  /** Tilt (degrees) */
  int16_t tilt;
  /** Field of view (degrees). */
  int16_t zoom;
  /** Current pan/tilt velocities (deg/sec) */
  int16_t panspeed, tiltspeed;
} __PACKED__ player_ptz_data_t;

/** @brief Command

The ptz interface accepts commands that set change the state of the unit.
Note that the commands are absolute, not relative. */
typedef struct player_ptz_cmd
{
  /** Desired pan angle (degrees) */
  int16_t pan;
  /** Desired tilt angle (degrees) */
  int16_t tilt;
  /** Desired field of view (degrees). */
  int16_t zoom;
  /** Desired pan/tilt velocities (deg/sec) */
  int16_t panspeed, tiltspeed;
} __PACKED__ player_ptz_cmd_t;

/** @brief Configuration request: Generic request

This ioctl allows the client to send a unit-specific command to the unit.
Whether data is returned depends on the command that was sent.  The server
may fill in "config" with a reply if applicable. */
typedef struct player_ptz_generic_config
{
  /** Must be set to PLAYER_PTZ_GENERIC_CONFIG_REQ */
  uint8_t	subtype;
  /** Length of data in config buffer */
  uint16_t	length;
  /** Buffer for command/reply */
  uint8_t	config[PLAYER_PTZ_MAX_CONFIG_LEN];
} __PACKED__ player_ptz_generic_config_t;

/** @brief Configuration request: Control mode.

This ioctl allows the client to switch between position and velocity
control, for those drivers that support it.  Note that this request
changes how the driver interprets forthcoming commands from all
clients. */
typedef struct player_ptz_controlmode_config
{
  /** Must be set to PLAYER_PTZ_CONTROL_MODE_REQ */
  uint8_t	subtype;
  /** Mode to use: must be either PLAYER_PTZ_VELOCITY_CONTROL or
      PLAYER_PTZ_POSITION_CONTROL. */
  uint8_t mode;
} __PACKED__ player_ptz_velocitymode_config_t;

/** @} */
/** @} */


/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_camera camera

The camera interface is used to see what the camera sees.  It is
intended primarily for server-side (i.e., driver-to-driver) data
transfers, rather than server-to-client transfers.  Image data can be
in may formats (see below), but is always packed (i.e., pixel rows are
byte-aligned).

This interface has no commands or configuration requests.
 
@{
*/

/* Image dimensions. */
#define PLAYER_CAMERA_IMAGE_WIDTH 640
#define PLAYER_CAMERA_IMAGE_HEIGHT 480
#define PLAYER_CAMERA_IMAGE_SIZE (PLAYER_CAMERA_IMAGE_WIDTH * PLAYER_CAMERA_IMAGE_HEIGHT * 4)

/** Image format : 8-bit monochrome. */
#define PLAYER_CAMERA_FORMAT_MONO8  1
/** Image format : 16-bit monochrome (network byte order). */
#define PLAYER_CAMERA_FORMAT_MONO16 2
/** Image format : 16-bit color (5 bits R, 6 bits G, 5 bits B). */
#define PLAYER_CAMERA_FORMAT_RGB565 4
/** Image format : 24-bit color (8 bits R, 8 bits G, 8 bits B). */
#define PLAYER_CAMERA_FORMAT_RGB888 5

/* Compression methods. */
#define PLAYER_CAMERA_COMPRESS_RAW 0
#define PLAYER_CAMERA_COMPRESS_JPEG 1

/** @brief Data */
typedef struct player_camera_data
{
  /** Image dimensions (pixels). */
  uint16_t width, height;
  /** Image bits-per-pixel (8, 16, 24, 32). */
  uint8_t bpp;
  /** Image format (must be compatible with depth). */
  uint8_t format;
  /** Some images (such as disparity maps) use scaled pixel values;
      for these images, fdiv specifies the scale divisor (i.e., divide
      the integer pixel value by fdiv to recover the real pixel value). */
  uint16_t fdiv;
  /** Image compression; PLAYER_CAMERA_COMPRESS_RAW indicates no
      compression. */
  uint8_t compression;
  /** Size of image data as stored in image buffer (bytes) */
  uint32_t image_size;
  /** Compressed image data. */
  uint8_t image[PLAYER_CAMERA_IMAGE_SIZE];
} __PACKED__ player_camera_data_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_audio audio

The @p audio interface is used to control sound hardware, if equipped.

@{
*/

#define AUDIO_DATA_BUFFER_SIZE 20
#define AUDIO_COMMAND_BUFFER_SIZE 3*sizeof(short)


/** @brief Data

The @p audio interface reads the audio stream from @p /dev/audio (which
is assumed to be associated with a sound card connected to a microphone)
and performs some analysis on it.  Five frequency/amplitude pairs are then
returned as data. */
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
} __PACKED__ player_audio_data_t;

/** @brief Command

The @p audio interface accepts commands to produce fixed-frequency tones
through @p /dev/dsp (which is assumed to be associated with a sound card
to which a speaker is attached). */
typedef struct player_audio_cmd
{
  /** Frequency to play (Hz?) */
  uint16_t frequency;
  /** Amplitude to play (dB?) */
  uint16_t amplitude;
  /** Duration to play (sec?) */
  uint16_t duration;
} __PACKED__ player_audio_cmd_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_audiodsp audiodsp

The @p audiodsp interface is used to control sound hardware, if equipped.

@{
*/

#define PLAYER_AUDIODSP_SET_CONFIG 0x01
#define PLAYER_AUDIODSP_GET_CONFIG 0x02
#define PLAYER_AUDIODSP_PLAY_TONE  0x03
#define PLAYER_AUDIODSP_PLAY_CHIRP 0x04
#define PLAYER_AUDIODSP_REPLAY     0x05


/** @brief Data

The @p audiodsp interface reads the audio stream from @p /dev/dsp (which
is assumed to be associated with a sound card connected to a microphone)
and performs some analysis on it.  Five frequency/amplitude pairs are
then returned as data. */
typedef struct player_audiodsp_data
{
  /** Hz */
  uint16_t freq[5];
  /** Db ? */
  uint16_t amp[5];

} __PACKED__ player_audiodsp_data_t;

/** @brief Command 

The @p audiodsp interface accepts commands to produce fixed-frequency
tones or binary phase shift keyed(BPSK) chirps through @p /dev/dsp
(which is assumed to be associated with a sound card to which a speaker
is attached). */
typedef struct player_audiodsp_cmd
{
  /** The packet subtype. Set to PLAYER_AUDIODSP_PLAY_TONE to play a single
   frequency; bitString and bitStringLen do not need to be set. Set to
   PLAYER_AUDIODSP_PLAY_CHIRP to play a BPSKeyed chirp; bitString should 
   contain the binary string to encode, and bitStringLen set to the length of 
   the bitString. Set to PLAYER_AUDIODSP_REPLAY to replay the last sound. **/
  uint8_t subtype;
  /** Frequency to play (Hz) */
  uint16_t frequency;
  /** Amplitude to play (dB?) */
  uint16_t amplitude;
  /** Duration to play (msec) */
  uint32_t duration;
  /** BitString to encode in sine wave */
  unsigned char bitString[PLAYER_MAX_DEVICE_STRING_LEN];
  /** Length of the bit string */
  uint16_t bitStringLen;
} __PACKED__ player_audiodsp_cmd_t;

/** @brief Configuration request : Get/set audio properties.

The audiodsp configuration can be queried using the
PLAYER_AUDIODSP_GET_CONFIG request and modified using the
PLAYER_AUDIODSP_SET_CONFIG request.

The sample format is defined in sys/soundcard.h, and defines the byte
size and endian format for each sample.

The sample rate defines the Hertz at which to sample.

Mono or stereo sampling is defined in the channels parameter where
1==mono and 2==stereo.
 */

/** @brief Configuration request: Get/set audio configuration.

Request/reply packet for getting and setting the audio configuration. */
typedef struct player_audiodsp_config
{
  /** The packet subtype. Set this to PLAYER_AUDIODSP_SET_CONFIG to set
      the audiodsp configuration; or set to PLAYER_AUDIODSP_GET_CONFIG to get
      the audiodsp configuration. */
  uint8_t subtype;
  /** Format with which to sample */
  int16_t sampleFormat;
  /** Sample rate in Hertz */
  uint16_t sampleRate;
  /** Number of channels to use. 1=mono, 2=stereo */
  uint8_t channels;
} __PACKED__ player_audiodsp_config_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_audiomixer audiomixer

The @p audiomixer interface is used to control sound levels.
@{
*/

#define PLAYER_AUDIOMIXER_SET_MASTER 0x01
#define PLAYER_AUDIOMIXER_SET_PCM    0x02
#define PLAYER_AUDIOMIXER_SET_LINE   0x03
#define PLAYER_AUDIOMIXER_SET_MIC    0x04
#define PLAYER_AUDIOMIXER_SET_IGAIN  0x05
#define PLAYER_AUDIOMIXER_SET_OGAIN  0x06


/** @brief Command

The @p audiomixer interface accepts commands to set the left
and right volume levels of various channels. The channel may be
PLAYER_AUDIOMIXER_MASTER for the master volume, PLAYER_AUDIOMIXER_PCM
for the PCM volume, PLAYER_AUDIOMIXER_LINE for the line in volume,
PLAYER_AUDIOMIXER_MIC for the microphone volume, PLAYER_AUDIOMIXER_IGAIN
for the input gain, and PLAYER_AUDIOMIXER_OGAIN for the output gain.
*/
typedef struct player_audiomixer_cmd
{
  /** documentation for these fields goes here
   */
  uint8_t subtype;
  uint16_t left;
  uint16_t right;

} __PACKED__ player_audiomixer_cmd_t;

/** @brief Configuration request: Get levels

The @p audiomixer interface provides accepts a configuration request
which returns the current state of the mixer levels.
*/
typedef struct player_audiomixer_config
{
  uint8_t subtype;
  uint16_t masterLeft, masterRight;
  uint16_t pcmLeft, pcmRight;
  uint16_t lineLeft, lineRight;
  uint16_t micLeft, micRight;
  uint16_t iGain, oGain;
} __PACKED__ player_audiomixer_config_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_waveform waveform

The @p waveform interface is used to receive arbitrary digital samples,
say from a digital audio device.
@{
*/

/*4K - half the packet max*/
#define PLAYER_WAVEFORM_DATA_MAX 4096

/** @brief Data

The @p waveform interface reads a digitized waveform from the target
device.*/
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
} __PACKED__ player_waveform_data_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_blinkenlight blinkenlight

The @p blinkenlight interface is used to switch on and off a flashing
indicator light, and to set it's flash period.

This interface accepts no configuration requests 
@{
*/

/** @brief Data

The @p blinkenlight data provides the current state of the indicator
light.*/
typedef struct player_blinkenlight_data
{
  /** zero: disabled, non-zero: enabled */
  uint8_t enable;
  /** flash period (one whole on-off cycle) in milliseconds. */
  uint16_t period_ms;
} __PACKED__ player_blinkenlight_data_t;

/** @brief Command

The @p blinkenlight command sets the current state of the indicator
light. It uses the same packet as the data.*/
typedef player_blinkenlight_data_t player_blinkenlight_cmd_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_fiducial fiducial

The fiducial interface provides access to devices that detect coded
fiducials (markers) placed in the environment.  It can also be used
for devices the detect natural landmarks.

@{
*/

/** The maximum number of fiducials that can be detected at one time. */
#define PLAYER_FIDUCIAL_MAX_SAMPLES 32

/** The maximum size of a data packet exchanged with a fiducial at one time.*/
#define PLAYER_FIDUCIAL_MAX_MSG_LEN 32

/* Request packet subtypes */
#define PLAYER_FIDUCIAL_GET_GEOM     0x01
#define PLAYER_FIDUCIAL_GET_FOV      0x02
#define PLAYER_FIDUCIAL_SET_FOV      0x03
#define PLAYER_FIDUCIAL_SEND_MSG     0x04
#define PLAYER_FIDUCIAL_RECV_MSG     0x05
#define PLAYER_FIDUCIAL_EXCHANGE_MSG 0x06
#define PLAYER_FIDUCIAL_GET_ID       0x07
#define PLAYER_FIDUCIAL_SET_ID       0x08


/** @brief Info on a single detected fiducial 

The fiducial data packet contains a list of these. */
typedef struct player_fiducial_item
{
  /** The fiducial id.  Fiducials that cannot be identified get id
      -1. */
  int16_t id;
  /** Fiducial position relative to the detector (x, y, z) in mm. */
  int32_t pos[3];
  /** Fiducial orientation relative to the detector (r, p, y) in millirad. */
  int32_t rot[3];
  /** Uncertainty in the measured pose (x, y, z) in mm. */
  int32_t upos[3];
  /** Uncertainty in fiducial orientation relative to the detector
      (r, p, y) in millirad. */
  int32_t urot[3];
} __PACKED__ player_fiducial_item_t;


/** @brief Data

The fiducial data packet (all fiducials). */
typedef struct player_fiducial_data
{
  /** The number of detected fiducials */
  uint16_t count;
  /** List of detected fiducials */
  player_fiducial_item_t fiducials[PLAYER_FIDUCIAL_MAX_SAMPLES];
  
} __PACKED__ player_fiducial_data_t;



/** @brief Configuration request: Get geometry.

The geometry (pose and size) of the fiducial device can be queried using
the PLAYER_FIDUCIAL_GET_GEOM request.  The request and reply packets
have the same format.
*/
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
} __PACKED__ player_fiducial_geom_t;

/** @brief Configuration request: Get/set sensor field of view.

The field of view of the fiducial device can be set using
the PLAYER_FIDUCIAL_SET_FOV request, and queried using the
PLAYER_FIDUCIAL_GET_FOV request. The device replies to a SET request
with the actual FOV achieved. In both cases the request and reply packets
have the same format.
*/
typedef struct player_fiducial_fov
{
  /** Packet subtype.  PLAYER_FIDUCIAL_GET_FOV or PLAYER_FIDUCIAL_SET_FOV. */
  uint8_t subtype;
  /** The minimum range of the sensor in mm */
  uint16_t min_range;
  /** The maximum range of the sensor in mm */
  uint16_t max_range;  
  /** The receptive angle of the sensor in degrees. */
  uint16_t view_angle;
} __PACKED__ player_fiducial_fov_t;


/** @brief Configuration request: Get/set fiducial value.

Some fiducial finder devices display their own fiducial. They can use the
PLAYER_FIDUCIAL_GET_ID config to report the identifier displayed by the
fiducial. Make the request using the player_fiducial_id_t structure. The
device replies with the same structure with the id field set.

Some devices can dynamically change the identifier they display. They
can use the PLAYER_FIDUCIAL_SET_ID config to allow a client to set the
currently displayed value. Make the request with the player_fiducial_id_t
structure. The device replies with the same structure with the id field
set to the value it actually used. You should check this value, as the
device may not be able to display the value you requested.

Currently supported by the stg_fiducial driver.
*/
typedef struct player_fiducial_id
{
  /** Packet subtype. Must be PLAYER_FIDUCIAL_GET_ID or PLAYER_FIDUCIAL_SET_ID. */
  uint8_t subtype;
  /** The value displayed */
  uint32_t id;
} __PACKED__ player_fiducial_id_t;



/** @brief Configuration request: Fiducial messaging.

META-NOTE: FIDUCIAL MESSAGING IS NOT WORKING IN STAGE CVS HEAD OR
STAGE-1.5

NOTE: These configs are currently supported only by the Stage fiducial
driver (stg_fidicial), but are intended to be a general interface for
addressable, peer-to-peer messaging.

The fiducial sensor can attempt to send a message to a target using the
PLAYER_FIDUCIAL_SEND_MSG request. If target_id is -1, the message is
broadcast to all targets. The device replies with an ACK if the message
was sent OK, but receipt by the target is not guaranteed. The intensity
field sets a transmit power in device-dependent units. If the consume flag
is set, the message is transmitted just once. If it is unset, the message
may transmitted repeatedly (at device-dependent intervals, if at all).

Send a PLAYER_FIDUCIAL_RECV_MSG request to obtain the last message
recieved from the indicated target. If the consume flag is set,
the message is deleted from the device's buffer, if unset, the same
message can be retreived multiple times until a new message arrives. The
power field indicates the intensity of the recieved messag, again in
device-dependent units.

Similarly, the PLAYER_FIDUCIAL_EXCHANGE_MSG request sends a message,
then returns the most recently received message. Depending on the device
and the situation, this could be a reflection of the sent message, a
reply from the target of the sent message, or a message received from
an unrelated sender.
*/
typedef struct player_fiducial_msg
{
  /** the fiducial ID of the intended target. */
  int32_t target_id;
  /** the raw data of the message */
  uint8_t bytes[PLAYER_FIDUCIAL_MAX_MSG_LEN];
  /** the length of the message in bytes.*/
  uint8_t len; 
  /** the power to transmit, or the intensity of a received message.
      0-255 in device-dependent units.*/
  uint8_t intensity; 
} __PACKED__ player_fiducial_msg_t;


/** @brief Configuration request: Fiducial receive message request.

The server replies with a player_fiducial_msg_t */
typedef struct player_fiducial_msg_rx_req
{
  /**  Must be PLAYER_FIDUCIAL_RECV_MSG.*/
  uint8_t subtype;  
  /** If non-zero, empty the buffer when getting the message. If
      zero, leave the message in the buffer */
  uint8_t consume;
}  __PACKED__ player_fiducial_msg_rx_req_t;

/** @brief Configuration request: Fiducial send message request.

The server replies with ACK/NACK only.*/
typedef struct player_fiducial_msg_tx_req
{
  /**  Must be PLAYER_FIDUCIAL_SEND_MSG. */
  uint8_t subtype;  
  /** If non-zero, send the message just once. If zero, the device may
      send the message repeatedly. */
  uint8_t consume;
  /** The message to send. */
  player_fiducial_msg_t msg;
}  __PACKED__ player_fiducial_msg_tx_req_t;

/** @brief Configuration request: Fiducial exchange message request.

The device sends the message, then replies with the last message
received, which may be (but is not guaranteed to be) be a reply to the
sent message. NOTE: this is not yet supported by Stage-1.4.*/
typedef struct player_fiducial_msg_txrx_req
{ 
  /** Must be PLAYER_FIDUCIAL_EXCHANGE_MSG. */
  uint8_t subtype;
  /**  The message to send */
  player_fiducial_msg_t msg;
  /** If non-zero, send the message just once. If zero, the device may
      send the message repeatedly. */
  uint8_t consume_send;
  /** If non-zero, empty the buffer when getting the message. If
      zero, leave the message in the buffer */
  uint8_t consume_reply;
}  __PACKED__ player_fiducial_msg_txrx_req_t; 
  
/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_speech speech

The @p speech interface provides access to a speech synthesis system.

The @p speech interface returns no data.
@{
*/

/** Maximum string length */
#define PLAYER_SPEECH_MAX_STRING_LEN 256


/** @brief Command

The @p speech interface accepts a command that is a string to
be given to the speech synthesizer.*/
typedef struct player_speech_cmd
{
  /** The string to say */
  uint8_t string[PLAYER_SPEECH_MAX_STRING_LEN];
} __PACKED__ player_speech_cmd_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_gps gps

The @p gps interface provides access to an absolute position system,
such as GPS.

This interface accepts no commands.
@{
*/

/** @brief Data

The @p gps interface gives current global position and heading information.
*/
typedef struct player_gps_data
{
  /** GPS (UTC) time, in seconds and microseconds since the epoch. */
  uint32_t time_sec, time_usec; 
  /** Latitude, in 1/60 of an arc-second (i.e., 1/216000 of a degree).  
      Positive is north of equator, negative is south of equator. */
  int32_t latitude;
  /** Longitude, in 1/60 of an arc-second (i.e., 1/216000 of a degree).  
      Positive is east of prime meridian, negative is west 
      of prime meridian. */
  int32_t longitude;
  /** Altitude, in millimeters.  Positive is above reference (e.g.,
      sea-level), and negative is below. */
  int32_t altitude;
  /** UTM WGS84 coordinates, easting and northing (cm). */
  int32_t utm_e, utm_n;
  /** Quality of fix 0 = invalid, 1 = GPS fix, 2 = DGPS fix */
  uint8_t quality;
  /** Number of satellites in view. */
  uint8_t num_sats;
  /** Horizontal dilution of position (HDOP), times 10 */
  uint16_t hdop;
  /** Horizonal error (mm) */
  uint32_t err_horz;
  /** Vertical error (mm) */
  uint32_t err_vert;
} __PACKED__ player_gps_data_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_bumper bumper

The @p bumper interface returns data from a bumper array.  This interface
accepts no commands.
@{
*/

/** Maximum number of bumper samples */
#define PLAYER_BUMPER_MAX_SAMPLES 32
/** The request subtypes */
#define PLAYER_BUMPER_GET_GEOM_REQ          ((uint8_t)1)

/** @brief Data

The @p bumper interface gives current bumper state*/
typedef struct player_bumper_data
{
  /** the number of valid bumper readings */
  uint8_t bumper_count;
  /** array of bumper values */
  uint8_t bumpers[PLAYER_BUMPER_MAX_SAMPLES];
} __PACKED__ player_bumper_data_t;

/** @brief The geometry of a single bumper */
typedef struct player_bumper_define
{
  /** the local pose of a single bumper in mm */
  int16_t x_offset, y_offset, th_offset;   
  /** length of the sensor in mm  */
  uint16_t length; 
  /** radius of curvature in mm - zero for straight lines */
  uint16_t radius; 
} __PACKED__ player_bumper_define_t;

/** @brief Configuration request: Query geometry

To query the geometry of a bumper array, give the following request,
filling in only the subtype.  The server will repond with the other
fields filled in. */
typedef struct player_bumper_geom
{
  /** Packet subtype.  Must be PLAYER_BUMPER_GET_GEOM_REQ. */
  uint8_t subtype;
  /** The number of valid bumper definitions. */
  uint16_t bumper_count;
  /** geometry of each bumper */
  player_bumper_define_t bumper_def[PLAYER_BUMPER_MAX_SAMPLES];
} __PACKED__ player_bumper_geom_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_truth truth

The @p truth interface provides access to the absolute state of entities.
Note that, unless your robot has superpowers, @p truth devices are
only avilable in simulation.
@{
*/

/* Request packet subtypes. */
#define PLAYER_TRUTH_GET_POSE 0x00
#define PLAYER_TRUTH_SET_POSE 0x01
#define PLAYER_TRUTH_SET_POSE_ON_ROOT 0x02
#define PLAYER_TRUTH_GET_FIDUCIAL_ID 0x03
#define PLAYER_TRUTH_SET_FIDUCIAL_ID 0x04

/** @brief Data

The @p truth interface returns data concerning the current state of an
entity. */
typedef struct player_truth_data
{
  /** Object pose in world cs (mm, mm, degrees). */
  int32_t px, py, pa; 
} __PACKED__ player_truth_data_t;

/** @brief Configuration request: Get/set pose

To get the pose of an object, use the following request, filling in only
the subtype with PLAYER_TRUTH_GET_POSE (this functionality is subsumed by
the data packet and should probably be removed).  The server will respond
with the other fields filled in.  To set the pose, set the subtype to
PLAYER_TRUTH_SET_POS and fill in the rest of the fields with the new
pose. */
typedef struct player_truth_pose
{
  /** Packet subtype.  Must be either PLAYER_TRUTH_GET_POSE or
    PLAYER_TRUTH_SET_POSE or PLAYER_TRUTH_SET_POSE_ON_ROOT. The last
    option places the object on the background and sets its
    pose. Great for repositioning pucks that have been picked up.*/
  uint8_t subtype;
  /** Object pose in world cs (mm, mm, degrees). */
  int32_t px, py, pa; 
} __PACKED__ player_truth_pose_t;

/** @brief Configuration request: Get/set fiducial ID number.

To get the fiducial ID of an object, use the following request, filling
in only the subtype with PLAYER_TRUTH_GET_FIDUCIAL_ID. The server will
respond with the ID field filled in.  To set the fiducial ID, set the
subtype to PLAYER_TRUTH_SET_FIDUCIAL_ID and fill in the ID field with
the desired value. */
typedef struct player_truth_fiducial_id
{
  /** Packet subtype.  Must be either PLAYER_TRUTH_GET_FIDUCIAL_ID or
    PLAYER_TRUTH_SET_FIDUCIAL_ID */
  uint8_t subtype;
  /** the fiducial ID */
  int16_t id;
} __PACKED__ player_truth_fiducial_id_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_simulation simulation

Player devices may either be real hardware or virtual devices
generated by a simulator such as Stage or Gazebo.  This interface
provides direct access to a simulator.

This interface doesn't do much yet. It is in place to later support things
like pausing and restarting the simulation clock, saving and loading,
etc. It is documented because it is used by the stg_simulation driver;
required by all stageclient drivers (stg_*).

Note: the Stage and Gazebo developers should confer on the best design
for this interface. Suggestions welcome on playerstage-developers.

@{
*/

/** @brief Data

Just a placeholder for now; data will be added in future.
*/
typedef struct player_simulation_data
{
  /** A single byte of as-yet-unspecified data. Useful for experiments. */
  uint8_t data;
} __PACKED__ player_simulation_data_t;

/** @brief Command

Just a placeholder for now; data will be added in future.
*/
typedef struct player_simulation_cmd
{
  /** A single byte of as-yet-unspecified command. Useful for experiments. */
  uint8_t cmd;
} __PACKED__ player_simulation_cmd_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_dio dio

The @p dio interface provides access to a digital I/O device.
@{
*/

/** @brief Data

The @p dio interface returns data regarding the current state of the
digital inputs. */
typedef struct player_dio_data
{
  /** number of samples */
  uint8_t count; 
  /** bitfield of samples */
  uint32_t digin;
} __PACKED__ player_dio_data_t;


/** @brief Command

The @p dio interface accepts 4-byte commands which consist of the ouput
bitfield */
typedef struct player_dio_cmd
{
  /** the command */ 
  uint8_t count;
  /* optional argument */
  uint32_t digout;
} __PACKED__ player_dio_cmd_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_aio aio

The @p aio interface provides access to an analog I/O device.

@todo
- Add command structure.
@{
*/

/** The maximum number of analog I/O samples */
#define PLAYER_AIO_MAX_SAMPLES 8

/** @brief Data

The @p aio interface returns data regarding the current state of the
analog inputs. */
typedef struct player_aio_data
{
  /** number of valid samples */
  uint8_t count;
  /** the samples */
  int32_t anin[PLAYER_AIO_MAX_SAMPLES];
} __PACKED__ player_aio_data_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_joystick joystick

The joystick interface provides access to the state of a joystick.
It allows another driver or a (possibly off-board) client to read and
use the state of a joystick.

This interface accepts no commands or configuration requests.
@{
*/

/** @brief Data

The joystick data packet, which contains the current state of the
joystick */
typedef struct player_joystick_data
{
  /** Current joystick position (unscaled) */
  int16_t xpos, ypos;
  /** Scaling factors */
  int16_t xscale, yscale;
  /** Button states (bitmask) */
  uint16_t buttons;
} __PACKED__ player_joystick_data_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_wifi wifi

The @p wifi interface provides access to the state of a wireless network
interface.

This interface accepts no commands.
@{
*/

/** The maximum number of remote hosts to report on */
#define PLAYER_WIFI_MAX_LINKS 32

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

/* config requests */
#define PLAYER_WIFI_MAC_REQ		((uint8_t)1)
#define PLAYER_WIFI_IWSPY_ADD_REQ		((uint8_t)10)
#define PLAYER_WIFI_IWSPY_DEL_REQ		((uint8_t)11)
#define PLAYER_WIFI_IWSPY_PING_REQ		((uint8_t)12)

/** @brief Link information for one host.

The @p wifi interface returns data regarding the signal characteristics 
of remote hosts as perceived through a wireless network interface; this
is the format of the data for each host. */
typedef struct player_wifi_link
{
  /** MAC address. */
  char mac[32];
  /** IP address. */
  char ip[32];
  /** ESSID. */
  char essid[32];
  /** Mode (master, adhoc, etc). */
  uint8_t mode;
  /** Frequency (MHz). */
  uint16_t freq;
  /** Encryted?. */
  uint8_t encrypt;
  /** Link quality, level and noise information these could be uint8_t
  instead, <linux/wireless.h> will only return that much.  maybe some
  other architecture needs larger?? */
  uint16_t qual, level, noise;
} __PACKED__ player_wifi_link_t;


/** @brief Data

The complete data packet format. */
typedef struct player_wifi_data
{
  /** A list of links */
  player_wifi_link_t links[PLAYER_WIFI_MAX_LINKS];
  /** length of said list */
  uint16_t link_count;
  /** mysterious throughput calculated by driver */
  uint32_t throughput;
  /** current bitrate of device */
  int32_t bitrate;
  /** operating mode of device */
  uint8_t mode;
  /** Indicates type of link quality info we have */
  uint8_t qual_type;
  /** Maximum values for quality, level and noise. */
  uint16_t maxqual, maxlevel, maxnoise;
  /** MAC address of current access point/cell */
  char ap[32];
} __PACKED__ player_wifi_data_t;

/** @brief Configuration request */
typedef struct player_wifi_mac_req 
{
  uint8_t		subtype;
} __PACKED__ player_wifi_mac_req_t;

/** @brief Configuration request */
typedef struct player_wifi_iwspy_addr_req
{
  uint8_t		subtype;
  char			address[32];
} __PACKED__ player_wifi_iwspy_addr_req_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_ir ir

The @p ir interface provides access to an array of infrared (IR) range
sensors.

This interface accepts no commands.
@{
*/

/** Maximum number of samples */
#define PLAYER_IR_MAX_SAMPLES 32
/* config requests */
#define PLAYER_IR_POSE_REQ   ((uint8_t)1)
#define PLAYER_IR_POWER_REQ  ((uint8_t)2)

/** @brief Data

The @p ir interface returns range readings from the IR array. */
typedef struct player_ir_data
{
  /** number of samples */
  uint16_t range_count;
  /** voltages (units?) */
  uint16_t voltages[PLAYER_IR_MAX_SAMPLES];
  /** ranges (mm) */
  uint16_t ranges[PLAYER_IR_MAX_SAMPLES];
} __PACKED__ player_ir_data_t;

/** @brief A set of IR poses. */
typedef struct player_ir_pose
{
  /** the number of ir samples returned by this robot */
  uint16_t pose_count;
  /** the pose of each IR detector on this robot (mm, mm, degrees) */
  int16_t poses[PLAYER_IR_MAX_SAMPLES][3];
} __PACKED__ player_ir_pose_t;


/** @brief Configuration request: Query pose.

To query the pose of the IRs, use this request, filling in only
the subtype.  The server will respond with the other fields filled in. */
typedef struct player_ir_pose_req
{
  /** subtype; must be PLAYER_IR_POSE_REQ */
  uint8_t subtype; 
  /** the poses */
  player_ir_pose_t poses;
} __PACKED__ player_ir_pose_req_t;

/** @brief Configuration request: IR power.

To turn IR power on and off, use this request.  The server will reply
with a zero-length acknowledgement */
typedef struct player_ir_power_req
{
  /** must be PLAYER_IR_POWER_REQ */
  uint8_t subtype; 
  /** 0 for power off, 1 for power on */
  uint8_t state; 
} __PACKED__ player_ir_power_req_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_localize localize

The @p localize interface provides pose information for the robot.
Generally speaking, localization drivers will estimate the pose of the
robot by comparing observed sensor readings against a pre-defined map
of the environment.  See, for the example, the @ref player_driver_amcl
driver, which implements a probabilistic Monte-Carlo localization
algorithm.

This interface accepts no commands.
@{
*/

/** The maximum number of pose hypotheses. */
#define PLAYER_LOCALIZE_MAX_HYPOTHS            10

/* Request/reply packet subtypes */
#define PLAYER_LOCALIZE_SET_POSE_REQ           ((uint8_t)1)
#define PLAYER_LOCALIZE_GET_CONFIG_REQ         ((uint8_t)2)
#define PLAYER_LOCALIZE_SET_CONFIG_REQ         ((uint8_t)3)

/** @brief Hypothesis format.

Since the robot pose may be ambiguous (i.e., the robot may at any
of a number of widely spaced locations), the @p localize interface is
capable of returning more that one hypothesis. */
typedef struct player_localize_hypoth
{
  /** The mean value of the pose estimate (mm, mm, arc-seconds). */
  int32_t mean[3];
  /** The covariance matrix pose estimate (mm$^2$, arc-seconds$^2$). */
  int64_t cov[3][3];
  /** The weight coefficient for linear combination (alpha * 1e6). */
  uint32_t alpha;
} __PACKED__ player_localize_hypoth_t;

/** @brief Data

The @p localize interface returns a data packet containing an an array
of hypotheses. */
typedef struct player_localize_data
{
  /** The number of pending (unprocessed observations) */
  uint16_t pending_count;
  /** The time stamp of the last observation processed. */
  uint32_t pending_time_sec, pending_time_usec;
  /** The number of pose hypotheses. */
  uint32_t hypoth_count;
  /** The array of the hypotheses. */
  player_localize_hypoth_t hypoths[PLAYER_LOCALIZE_MAX_HYPOTHS];
} __PACKED__ player_localize_data_t;


/** @brief Configuration request: Set the robot pose estimate.

Set the current robot pose hypothesis.  The server will reply with a
zero length response packet. */
typedef struct player_localize_set_pose
{
  /** Request subtype; must be PLAYER_LOCALIZE_SET_POSE_REQ. */
  uint8_t subtype;
  /** The mean value of the pose estimate (mm, mm, arc-seconds). */
  int32_t mean[3];
  /** The covariance matrix pose estimate (mm$^2$, arc-seconds$^2$). */
  int64_t cov[3][3];

} __PACKED__ player_localize_set_pose_t;


/** @brief Configuration request: Get/Set configuration.

To retrieve the configuration, set the subtype to
PLAYER_LOCALIZE_GET_CONFIG_REQ and leave the other fields empty. The
server will reply with the following configuaration fields filled
in.  To change the current configuration, set the subtype to
PLAYER_LOCALIZE_SET_CONFIG_REQ and fill the configuration fields. */
typedef struct player_localize_config
{ 
  /** Request subtype; must be either PLAYER_LOCALIZE_GET_CONFIG_REQ
      or PLAYER_LOCALIZE_SET_CONFIG_REQ */
  uint8_t subtype;
  /** Maximum number of particles (for drivers using particle
   * filters). */
  uint32_t num_particles;
} __PACKED__ player_localize_config_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_map map

The @p map interface provides acces to an occupancy grid map.
This interface returns no data and accepts no commands. The map is
delivered in tiles, via a sequence of configuration requests.
@{
*/

/** The max number of cells we can send in one tile */
#define PLAYER_MAP_MAX_CELLS_PER_TILE (PLAYER_MAX_REQREP_SIZE - 17)
/* Configuration subtypes */
#define PLAYER_MAP_GET_INFO_REQ       ((uint8_t)1)
#define PLAYER_MAP_GET_DATA_REQ       ((uint8_t)2)

/** @brief Configuration request: Get map information.

Retrieve the size and scale information of a current map. This request
is used to get the size information before you request the actual map
data. Set the subtype to PLAYER_MAP_GET_INFO_REQ; the server will reply
with the size information filled in. */
typedef struct player_map_info
{
  /** Request subtype; must be PLAYER_MAP_GET_INFO_REQ */
  uint8_t subtype; 
  /** The scale of the map (pixels per kilometer). */
  uint32_t scale; 
  /** The size of the map (pixels). */
  uint32_t width, height;
} __PACKED__ player_map_info_t;

/** @brief Configuration request: Get map data.

Retrieve the map data. Beacause of the limited size of a request-reply
messages, the map data is tranfered in tiles.  In the request packet,
set the column and row index of a specific tile; the server will reply
with the requested map data filled in. */
typedef struct player_map_data
{
  /** Request subtype; must be PLAYER_MAP_GET_DATA_REQ. */
  uint8_t subtype; 
  /** The tile origin (pixels). */
  uint32_t col, row;
  /** The size of the tile (pixels). */
  uint32_t width, height;
  /** Cell occupancy value (empty = -1, unknown = 0, occupied = +1). */
  int8_t data[PLAYER_MAX_REQREP_SIZE - 17];
} __PACKED__ player_map_data_t;


/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_mcom mcom

The @p mcom interface is designed for exchanging information between
clients.  A client sends a message of a given "type" and "channel". This
device stores adds the message to that channel's stack.  A second client
can then request data of a given "type" and "channel".  Push, Pop, Read,
and Clear operations are defined, but their semantics can vary, based
on the stack discipline of the underlying driver.  For example, the @ref
player_driver_lifomcom driver enforces a last-in-first-out stack.

This interface returns no data and accepts no commands.
@{
*/

/** size of the data field in messages */
#define MCOM_DATA_LEN           128     
#define MCOM_COMMAND_BUFFER_SIZE    (sizeof(player_mcom_config_t))
#define MCOM_DATA_BUFFER_SIZE   0       
/** number of buffers to keep per channel */
#define MCOM_N_BUFS             10      
/** size of channel name */
#define MCOM_CHANNEL_LEN        8       
/** returns this if empty */
#define MCOM_EMPTY_STRING	"(EMPTY)"
/* request ids */
#define PLAYER_MCOM_PUSH_REQ    0
#define PLAYER_MCOM_POP_REQ     1
#define PLAYER_MCOM_READ_REQ    2
#define PLAYER_MCOM_CLEAR_REQ   3
#define PLAYER_MCOM_SET_CAPACITY_REQ	4


/** @brief A piece of data. */
typedef struct player_mcom_data
{
  /** a flag */
  char full;  
  /** the data */
  char data[MCOM_DATA_LEN];
} __PACKED__ player_mcom_data_t;


/** @brief Configuration request: Config requests sent to server. */
typedef struct player_mcom_config
{
  /** Which request.  Should be one of the defined request ids. */
  uint8_t command;
  /** The "type" of the data. */
  uint16_t type;
  /** The name of the channel. */
  char channel[MCOM_CHANNEL_LEN];
  /** The data. */
  player_mcom_data_t data;
} __PACKED__ player_mcom_config_t;

/** @brief Configuration reply from server. */
typedef struct player_mcom_return
{
  /** The "type" of the data */
  uint16_t type;
  /** The name of the channel. */
  char channel[MCOM_CHANNEL_LEN];
  /** The data. */
  player_mcom_data_t data;
} __PACKED__ player_mcom_return_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_nomad nomad

The nomad interface affords control of Nomadics Nomad robots and
relatives.

Warning: some of the command and data variables are specified in native
Nomad units; they don't follow Player's conventions. This will be changed
once someone figiures out exactly what the Nomad units are.

@{
*/

#define PLAYER_NOMAD_SONAR_COUNT 16
#define PLAYER_NOMAD_BUMPER_COUNT 16
#define PLAYER_NOMAD_IR_COUNT 16

/** @brief Data

The nomad data packet provides a pose estimate, sonar and infrared range
readings, and bumper contact readings.*/
typedef struct player_nomad_data
{
  /** X and Y position, in mm */
  int32_t x, y;
  /** heading, in degrees */
  int32_t a; 
  /** translation velocity (in native Nomad units) */
  int32_t vel_trans; 
  /** steering velocity (in native Nomad units) */
  int32_t vel_steer; 
  /** turret rotation velocity (in native Nomad units) */
  int32_t vel_turret; 
  /** sonar range sensors: range in mm */
  uint16_t sonar[PLAYER_NOMAD_SONAR_COUNT];
  /** infrared range sensors: range in mm */
  uint16_t ir[PLAYER_NOMAD_IR_COUNT];
  /** bump sensors: zero - no contact, non-zero - contact */
  uint8_t bumper[PLAYER_NOMAD_BUMPER_COUNT];
} __PACKED__ player_nomad_data_t;
  
/**  @brief Command


The Nomad command packet lets you set independent velocties for
translation, steering and turret rotation. These are specified in native
Nomad units. 

@todo
This should change in future to match normal Player style (mm/sec),
once someone figures out exactly what the Nomad units are. */
typedef struct player_nomad_cmd 
{
  int32_t vel_trans, vel_steer, vel_turret; 
} __PACKED__ player_nomad_cmd_t;

    
/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_sound sound

The @p sound interface allows playback of a pre-recorded sound (e.g.,
on an Amigobot).

This interface provides no data.
@{
*/

/** @brief Command

The @p sound interface accepts an index of a pre-recorded sound file
to play.  */
typedef struct player_sound_cmd 
{
  /** Index of sound to be played. */
  uint16_t index;
} __PACKED__ player_sound_cmd_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_hud hud

The @p hud interface provides an drawing interface for simulators for the
purpose of creating a Heads Up Display. This is currently only supported
by Gazebo.

This interface provides no data and accepts no commands.
@{
*/

/* Drawable subtypes. */
#define PLAYER_HUD_BOX 0x00
#define PLAYER_HUD_LINE 0x01
#define PLAYER_HUD_TEXT 0x02
#define PLAYER_HUD_CIRCLE 0x03

/** @brief Configuration request: Draw element

Request the HUD to draw a new element to the screen. */
typedef struct player_hud_config
{
  /** Packet subtype. */
  uint8_t subtype;
  /** Id of this drawing element */
  uint32_t id;
  /** Two points */
  int16_t pt1[2], pt2[2];
  /** Single values */
  int16_t value1;
  /** A text string to render */
  char text[PLAYER_MAX_DEVICE_STRING_LEN];
  /** Color to render at */
  int16_t color[3];
  /** Remove this element? */
  int8_t remove;
  /** Draw this element filled in? */
  int8_t filled;
} __PACKED__ player_hud_config_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_energy energy

The @p energy interface provides data about energy storage, consumption
and charging.  This interface accepts no commands.
@{
*/

/** @brief Data

The @p energy interface reports he amount of energy stored, current rate
of energy consumption or aquisition, and whether or not the device is
connected to a charger. */
typedef struct player_energy_data
{
  /** energy stored, in milliJoules. */
  int32_t mjoules;
  /** estimated current energy consumption (negative values) or
      aquisition (positive values), in milliWatts (milliJoules/sec). */
  int32_t mwatts; 
  /** charge exchange status: if 1, the device is currently receiving
      charge from another energy device. If -1 the device is currently
      providing charge to another energy device. If 0, the device is
      not exchanging charge with an another device. */
  int8_t charging;
  
} __PACKED__ player_energy_data_t;

/** @brief Configuration request */
typedef struct player_energy_command
{
  /** boolean controlling recharging. If FALSE, recharging is
      disabled. Defaults to TRUE */
  uint8_t enable_input;
  /** boolean controlling whether others can recharge from this
      device. If FALSE, charging others is disabled. Defaults to TRUE.*/  
  uint8_t enable_output; 
} __PACKED__ player_energy_chargepolicy_config_t;

/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_planner planner

The @p planner interface provides control of a 2-D motion planner.
@{
*/

#define PLAYER_PLANNER_GET_WAYPOINTS_REQ     ((uint8_t)10)

/** maximum number of waypoints in a single plan */
#define PLAYER_PLANNER_MAX_WAYPOINTS 128

/** @brief Data

The @p planner interface reports the current execution state of the
planner. */
typedef struct player_planner_data
{
  /** Did the planner find a valid path? */
  uint8_t valid;
  /** Have we arrived at the goal? */
  uint8_t done;
  /** Current location (mm,mm,deg) */
  int32_t px,py,pa;
  /** Goal location (mm,mm,deg) */
  int32_t gx,gy,ga;
  /** Current waypoint location (mm,mm,deg) */
  int32_t wx,wy,wa;
  /** Current waypoint index (handy if you already have the list
      of waypoints). May be negative if there's no plan, or if 
      the plan is done */
  int16_t curr_waypoint;
  /** Number of waypoints in the plan */
  uint16_t waypoint_count;
} __PACKED__ player_planner_data_t;

/** @brief Command

The @p planner interface accepts a new goal. */
typedef struct player_planner_cmd
{
  /** Goal location (mm,mm,deg) */
  int32_t gx,gy,ga;
} __PACKED__ player_planner_cmd_t;

/** @brief A waypoint */
typedef struct player_planner_waypoint
{
  int32_t x,y,a;
} __PACKED__ player_planner_waypoint_t;

/** @brief Configuration request: Get waypoints */
typedef struct player_planner_waypoints_req
{
  /** subtype: must be of PLAYER_PLANNER_GET_WAYPOINTS_REQ */
  uint8_t subtype;
  /** Number of waypoints to follow */
  uint16_t count;
  player_planner_waypoint_t waypoints[PLAYER_PLANNER_MAX_WAYPOINTS];
} __PACKED__ player_planner_waypoints_req_t;


/** @} */
/** @} */

/** @addtogroup interfaces */
/** @{ */
/** @defgroup player_interface_log log

The @p log interface provides start/stop control of data logging/playback.

The @p log interface produces no data and accepts no commands.
@{
*/

/* The subtypes for config reqeusts */
#define PLAYER_LOG_SET_WRITE_STATE_REQ 1
#define PLAYER_LOG_SET_READ_STATE_REQ 2
#define PLAYER_LOG_GET_STATE_REQ 3
#define PLAYER_LOG_SET_READ_REWIND_REQ 4

/* Types of log devices */
#define PLAYER_LOG_TYPE_READ 1
#define PLAYER_LOG_TYPE_WRITE 2

/** @brief Configuration request: Set logging state

Start/stop data logging */
typedef struct player_log_set_write_state
{
  /** Request type: must be PLAYER_LOG_SET_WRITE_STATE_REQ */
  uint8_t subtype;
  /** State: 0=disabled, 1=enabled */
  uint8_t state;
} __PACKED__ player_log_set_write_state_t;

/** @brief Configuration request: Set playback state

Start/stop data playback */
typedef struct player_log_set_read_state
{
  /** Request type: must be PLAYER_LOG_SET_READ_STATE_REQ */
  uint8_t subtype;
  /** State: 0=disabled, 1=enabled */
  uint8_t state;
} __PACKED__ player_log_set_read_state_t;

/** @brief Configuration request: Rewind playback

Rewind log playback to beginning of logfile; does not affect playback
state (i.e., whether it is started or stopped */
typedef struct player_log_set_read_rewind
{
  /** Request type: must be PLAYER_LOG_SET_READ_REWIND_REQ */
  uint8_t subtype;
} __PACKED__ player_log_set_read_rewind_t;

/** @brief Configuration request: Get state.

Find out whether logging/playback is enabled or disabled */
typedef struct player_log_get_state
{
  /** Request type: must be PLAYER_LOG_GET_STATE_REQ */
  uint8_t subtype;
  /** The type of log device, either PLAYER_LOG_TYPE_READ or
      PLAYER_LOG_TYPE_WRITE */
  uint8_t type;
  /** Logging/playback state: 0=disabled, 1=enabled */
  uint8_t state;
} __PACKED__ player_log_get_state_t;

/** @} */
/** @} */

#endif /* PLAYER_H */
