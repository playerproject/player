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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/*
 * $Id$
 *
 *   the P2OS device.  it's the parent device for all the P2 'sub-devices',
 *   like gripper, position, sonar, etc.  there's a thread here that
 *   actually interacts with P2OS via the serial line.  the other
 *   "devices" communicate with this thread by putting into and getting
 *   data out of shared buffers.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_p2os p2os
 * @brief ActivMedia mobile robots

Many robots made by ActivMedia, such as the Pioneer series and the
AmigoBot, are controlled by a microcontroller that runs a special embedded
operating system called P2OS (aka AROS, PSOS).  The host computer
talks to the P2OS microcontroller over a standard RS232 serial line.
This driver offer access to the various P2OS-mediated devices, logically
splitting up the devices' functionality.

@par Compile-time dependencies

- none

@par Provides

The p2os driver provides the following device interfaces, some of
them named:

- "odometry" @ref interface_position2d
  - This interface returns odometry data, and accepts velocity commands.

- "compass" @ref interface_position2d
  - This interface returns compass data (if equipped).

- "gyro" @ref interface_position2d
  - This interface returns gyroscope data (if equipped).

- @ref interface_power
  - Returns the current battery voltage (12 V when fully charged).

- @ref interface_sonar
  - Returns data from sonar arrays (if equipped)

- @ref interface_aio
  - Returns data from analog I/O ports (if equipped)

- @ref interface_dio
  - Returns data from digital I/O ports (if equipped)

- "gripper" @ref interface_gripper
  - Controls gripper (if equipped)

- "lift" @ref interface_actarray
  - Controls a lift on the gripper (if gripper equipped)
  - The lift is controlled by actuator 0. Position 1.0 is up and 0.0 is down.

- "arm" @ref interface_actarray
  - Controls arm (if equipped)
  - This driver does not support the player_actarray_speed_cmd and
    player_actarray_brakes_config messages.

- @ref interface_limb
  - Inverse kinematics interface to arm
  - This driver does not support the player_limb_setposition_cmd,
    player_limb_vecmove_cmd, player_limb_brakes_req and
    player_limb_speed_req messages.
  - The approach vector is forward along the gripper with the orientation
    vector up from the gripper's centre.
  - The limb takes pose commands in robot coordinates (offset from the robot's
    centre, not the limb's base) and returns pose data in the same coordinate
    space.
  - The kinematics calculator is based on the analytical method by Gan et al. See:
    J.Q. Gan, E. Oyama, E.M. Rosales, and H. Hu, "A complete analytical
    solution to the inverse kinematics of the Pioneer 2 robotic arm,"
    Robotica, vol.23, no.1, pp.123-129, 2005.


- "armgrip" @ref interface_gripper
  - Controls the gripper on the end of the arm (if equipped)
  - Good for using in conjunction with the @ref interface_limb

- @ref interface_bumper
  - Returns data from bumper array (if equipped)

- @ref interface_blobfinder
  - Controls a CMUCam2 connected to the AUX port on the P2OS board
    (if equipped).

- @ref interface_ptz
  - Controls a Canon VCC4 PTZ camera connected to the AUX2 port on 
    the P2OS board (if equipped).

- @ref interface_audio
  - Controls the sound system of the AmigoBot, which can play back
    recorded wav files.

@par Supported configuration requests

- "odometry" @ref interface_position2d :
  - PLAYER_POSITION2D_REQ_SET_ODOM
  - PLAYER_POSITION2D_REQ_MOTOR_POWER
  - PLAYER_POSITION2D_REQ_RESET_ODOM
  - PLAYER_POSITION2D_REQ_GET_GEOM
  - PLAYER_POSITION2D_REQ_VELOCITY_MODE
- @ref interface_sonar :
  - PLAYER_SONAR_REQ_POWER
  - PLAYER_SONAR_REQ_GET_GEOM
- @ref interface_bumper :
  - PLAYER_BUMPER_REQ_GET_GEOM
- @ref interface_blobfinder :
  - PLAYER_BLOBFINDER_REQ_SET_COLOR
  - PLAYER_BLOBFINDER_REQ_SET_IMAGER_PARAMS

@par Configuration file options

- port (string)
  - Default: "/dev/ttyS0"
- use_tcp (boolean)
  - Defaut: 0
  - Set to 1 if a TCP connection should be used instead of serial port (e.g. Amigobot
    with ethernet-serial bridge device attached)
- tcp_remote_host (string)
  - Default: "localhost"
  - Remote hostname or IP address to connect to if using TCP
- tcp_remote_port (integer)
  - Default: 8101
  - Remote port to connect to if using TCP
  - Serial port used to communicate with the robot.
- radio (integer)
  - Default: 0
  - Nonzero if a radio modem is being used; zero for a direct serial link.
    (a radio modem is different from and older than the newer ethernet-serial bridge used
     on newer Pioneers and Amigos)
- bumpstall (integer)
  - Default: -1
  - Determine whether a bumper-equipped robot stalls when its bumpers are
    pressed.  Allowed values are:
      - -1 : Don't change anything; the bumper-stall behavior will
             be determined by the BumpStall value stored in the robot's
             FLASH.
      - 0 : Don't stall.
      - 1 : Stall on front bumper contact.
      - 2 : Stall on rear bumper contact.
      - 3 : Stall on either bumper contact.
- pulse (float)
  - Default: -1
  - Specify a pulse for keeping the robot alive. Pioneer robots have a built-in watchdog in
    the onboard controller. After a timeout period specified in the robot's FLASH, if no commands
    have been received from the player server, the robot will stop. By specifying a positive value
    here, the Player server will send a regular pulse command to the robot to let it know the client
    is still alive. The value should be in seconds, with decimal places allowed (eg 0.5 = half a
    second). Note that if this value is greater than the Pioneer's onboard value, it will still
    time out.
  - Specifying a value of -1 turns off the pulse, meaning that if you do not send regular commands
    from your client program, the robot's onboard controller will time out and stop.
  - WARNING: Overriding the onboard watchdog is dangerous! Specifying -1 and writing your client
    appropriately is definitely the preffered option!
- joystick (integer)
  - Default: 0
  - Use direct joystick control
- direct_wheel_vel_control (integer)
  - Default: 1
  - Send direct wheel velocity commands to P2OS (as opposed to sending
    translational and rotational velocities and letting P2OS smoothly
    achieve them).
- max_xspeed (length)
  - Default: 0.5 m/s
  - Maximum translational velocity
- max_yawspeed (angle)
  - Default: 100 deg/s
  - Maximum rotational velocity
- max_xaccel (length)
  - Default: 0
  - Maximum translational acceleration, in length/sec/sec; nonnegative.
    Zero means use the robot's default value.
- max_xdecel (length)
  - Default: 0
  - Maximum translational deceleration, in length/sec/sec; nonpositive.
    Zero means use the robot's default value.
- max_yawaccel (angle)
  - Default: 0
  - Maximum rotational acceleration, in angle/sec/sec; nonnegative.
    Zero means use the robot's default value.
- rot_kp (integer)
  - Default: -1
  - Rotational PID setting; proportional gain.
    Negative means use the robot's default value.
  - Requires P2OS1.M or above
- rot_kv (integer)
  - Default: -1
  - Rotational PID setting; derivative gain.
    Negative means use the robot's default value.
  - Requires P2OS1.M or above
- rot_ki (integer)
  - Default: -1
  - Rotational PID setting; integral gain.
    Negative means use the robot's default value.
  - Requires P2OS1.M or above
- trans_kp (integer)
  - Default: -1
  - Translational PID setting; proportional gain.
    Negative means use the robot's default value.
  - Requires P2OS1.M or above
- trans_kv (integer)
  - Default: -1
  - Translational PID setting; derivative gain.
    Negative means use the robot's default value.
  - Requires P2OS1.M or above
- trans_ki (integer)
  - Default: -1
  - Translational PID setting; integral gain.
    Negative means use the robot's default value.
  - Requires P2OS1.M or above
- max_yawdecel (angle)
  - Default: 0
  - Maximum rotational deceleration, in angle/sec/sec; nonpositive.
    Zero means use the robot's default value.
- use_vel_band (integer)
  - Default: 0
  - Use velocity bands
- aa_basepos (3 floats)
  - Default: (0.105, 0, 0.3185)
  - Position of the base of the arm from the robot centre in metres.
- aa_baseorient (3 floats)
  - Default: 0, 0, 0
  - Orientation of the base of the arm from the robot centre in radians.
- aa_offsets (6 floats)
  - Default: (0.06875, 0.16, 0, 0.13775, 0.11321, 0)
  - Offsets for the actarray.  Taken from current actuator to next actuator.
    Each offset is a straight line, not measured per axis.
- aa_orients (3x6 floats)
  - Default: all zero
  - Orientation of each actuator when it is at 0. Measured by taking a line from
    this actuator to the next and measuring its angles about the 3 axes of the
    previous actuator's coordinate space.
  - Each set of three values is a single orientation.
- aa_axes (3x6 floats)
  - Default: ((0,0,-1), (0,-1,0), (0,-1,0), (1,0,0), (0,1,0), (0,0,1))
  - The axis of rotation for each joint in the actarray.
  - Each set of three values is a vector along the axis of rotation.
- limb_pos (3 floats)
  - Default: (0.105, 0, 0.3185)
  - Position of the base of the arm from the robot centre in metres.
- limb_links (5 floats)
  - Default: (0.06875, 0.16, 0, 0.13775, 0.11321)
  - Offset from previous joint to this joint in metres.
    e.g. the offset from joint 0 to joint 1 is 0.06875m, and from joint 1 to joint 2 is 0.16m.
  - The default is correct for the standard Pioneer arm at time of writing.
- limb_offsets (5 floats, metres)
  - Default: (0, 0, 0, 0, 0)
  - Angular offset of each joint from desired position to actual position (calibration data).
  - Possibly taken by commanding joints to 0rad with actarray interface, then measuring
    their actual angle.
- gripper_pose (6 floats - 3 in metres, 3 in rads)
  - Default: (0, 0, 0, 0, 0, 0)
  - 3D pose of the standard gripper
- gripper_outersize (3 floats, metres)
  - Default: (0.315, 0.195, 0.035)
  - Size of the outside of the standard gripper
- gripper_innersize (3 floats, metres)
  - Default: (0.205, 0.095, 1.0)
  - Size of the inside of the standard gripper's fingers when fully open,
    i.e. the largest object it can pick up.
- armgrip_outersize (3 floats, metres)
  - Default: (0.09, 0.09, 0.041)
  - Size of the outside of the arm's gripper
- armgrip_innersize (3 floats, metres)
  - Default: (0.054, 0.025, 1.0)
  - Size of the inside of the arm's gripper (largest object it can hold)
- ignore_checksum (boolean)
  - Default: False (no effect)
  - If set to True, the checksum will be ignored


@par Example

@verbatim
driver
(
  name "p2os"
  provides ["odometry::position:0" "compass::position:1" "sonar:0" "power:0"]
)
@endverbatim

@author Brian Gerkey, Kasper Stoy, James McKenna
*/
/** @} */

#include "config.h"

#include "p2os.h"
#include <libplayerinterface/playerxdr.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>  /* for abs() */
#include <netinet/in.h>
#include <termios.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>

Driver*
P2OS_Init(ConfigFile* cf, int section)
{
  return (Driver*)(new P2OS(cf,section));
}

void p2os_Register(DriverTable* table)
{
  table->AddDriver("p2os", P2OS_Init);
}

P2OS::P2OS(ConfigFile* cf, int section)
        : ThreadedDriver(cf,section,true,PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  // zero ids, so that we'll know later which interfaces were requested
  memset(&this->position_id, 0, sizeof(player_devaddr_t));
  memset(&this->sonar_id, 0, sizeof(player_devaddr_t));
  memset(&this->aio_id, 0, sizeof(player_devaddr_t));
  memset(&this->dio_id, 0, sizeof(player_devaddr_t));
  memset(&this->gripper_id, 0, sizeof(player_devaddr_t));
  memset(&this->bumper_id, 0, sizeof(player_devaddr_t));
  memset(&this->power_id, 0, sizeof(player_devaddr_t));
  memset(&this->compass_id, 0, sizeof(player_devaddr_t));
  memset(&this->gyro_id, 0, sizeof(player_devaddr_t));
  memset(&this->blobfinder_id, 0, sizeof(player_devaddr_t));
  memset(&this->audio_id, 0, sizeof(player_devaddr_t));
  memset(&this->actarray_id, 0, sizeof(player_devaddr_t));
  memset(&this->limb_id, 0, sizeof(player_devaddr_t));
  memset(&this->ptz_id, 0, sizeof(player_devaddr_t));

  this->position_subscriptions = this->sonar_subscriptions = this->actarray_subscriptions = 0;
  this->pulse = -1;

  // intialise members
  sippacket = NULL;

  // Do we create a robot position interface?
  if(cf->ReadDeviceAddr(&(this->position_id), section, "provides",
                        PLAYER_POSITION2D_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->position_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a compass position interface?
  if(cf->ReadDeviceAddr(&(this->compass_id), section, "provides",
                        PLAYER_POSITION2D_CODE, -1, "compass") == 0)
  {
    if(this->AddInterface(this->compass_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a gyro position interface?
  if(cf->ReadDeviceAddr(&(this->gyro_id), section, "provides",
                        PLAYER_POSITION2D_CODE, -1, "gyro") == 0)
  {
    if(this->AddInterface(this->gyro_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }


  // Do we create a sonar interface?
  if(cf->ReadDeviceAddr(&(this->sonar_id), section, "provides",
                      PLAYER_SONAR_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->sonar_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }


  // Do we create an aio interface?
  if(cf->ReadDeviceAddr(&(this->aio_id), section, "provides",
                      PLAYER_AIO_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->aio_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a dio interface?
  if(cf->ReadDeviceAddr(&(this->dio_id), section, "provides",
                      PLAYER_DIO_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->dio_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a gripper interface?
  if(cf->ReadDeviceAddr(&(this->gripper_id), section, "provides",
                      PLAYER_GRIPPER_CODE, -1, "gripper") == 0)
  {
    if(this->AddInterface(this->gripper_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create an actarray interface for the gripper lift?
  if(cf->ReadDeviceAddr(&(this->lift_id), section, "provides",
     PLAYER_ACTARRAY_CODE, -1, "lift") == 0)
  {
    if(this->AddInterface(this->lift_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a bumper interface?
  if(cf->ReadDeviceAddr(&(this->bumper_id), section, "provides",
                      PLAYER_BUMPER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->bumper_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a power interface?
  if(cf->ReadDeviceAddr(&(this->power_id), section, "provides",
                      PLAYER_POWER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->power_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a blobfinder interface?
  if(cf->ReadDeviceAddr(&(this->blobfinder_id), section, "provides",
                      PLAYER_BLOBFINDER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->blobfinder_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create a audio interface?
  if(cf->ReadDeviceAddr(&(this->audio_id), section, "provides",
                      PLAYER_AUDIO_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->audio_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do We create the PTZ Interface
  if(cf->ReadDeviceAddr(&(this->ptz_id), section, "provides",
                      PLAYER_PTZ_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->ptz_id) != 0)
    {
      this->SetError(-1);
      return;
    }
    this->minfov = (int)rint(RTOD(cf->ReadTupleAngle(section, "fov", 0, DTOR(3))));
    this->maxfov = (int)rint(RTOD(cf->ReadTupleAngle(section, "fov", 1, DTOR(30))));
  }

  // Do we create a limb interface?
  if(cf->ReadDeviceAddr(&(this->limb_id), section, "provides", PLAYER_LIMB_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->limb_id) != 0)
    {
      this->SetError(-1);
      return;
    }
    // If we do, we need a kinematics calculator
    kineCalc = new KineCalc;
  }
  else
    kineCalc = NULL;

  // Do we create an arm gripper interface?
  if(cf->ReadDeviceAddr(&(this->armgripper_id), section, "provides", PLAYER_GRIPPER_CODE, -1, "armgrip") == 0)
  {
    if(this->AddInterface(this->armgripper_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Do we create an actarray interface? Note that if we have a limb or arm gripper interface,
  // this implies an actarray interface
  if((cf->ReadDeviceAddr(&(this->actarray_id), section, "provides", PLAYER_ACTARRAY_CODE, -1, "arm") == 0) ||
      this->limb_id.interf || this->armgripper_id.interf)
  {
    if(this->AddInterface(this->actarray_id) != 0)
    {
      this->SetError(-1);
      return;
    }
    // Stop actarray messages in the queue from being overwritten
    this->InQueue->AddReplaceRule (this->actarray_id, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_POS, false);
    this->InQueue->AddReplaceRule (this->actarray_id, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_SPEED, false);
    this->InQueue->AddReplaceRule (this->actarray_id, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_HOME, false);
  }

  // build the table of robot parameters.
  ::initialize_robot_params();

  // Read config file options
  this->ignore_checksum = cf->ReadBool(section, "ignore_checksum", false);
  this->bumpstall = cf->ReadInt(section,"bumpstall",-1);
  this->pulse = cf->ReadFloat(section,"pulse",-1);
  this->rot_kp = cf->ReadInt(section, "rot_kp", -1);
  this->rot_kv = cf->ReadInt(section, "rot_kv", -1);
  this->rot_ki = cf->ReadInt(section, "rot_ki", -1);
  this->trans_kp = cf->ReadInt(section, "trans_kp", -1);
  this->trans_kv = cf->ReadInt(section, "trans_kv", -1);
  this->trans_ki = cf->ReadInt(section, "trans_ki", -1);

  this->psos_serial_port = cf->ReadString(section,"port",DEFAULT_P2OS_PORT);
  //this->psos_use_tcp = cf->ReadBool(section, "use_tcp", false); // TODO after ReadBool added
  this->psos_use_tcp = cf->ReadInt(section, "use_tcp", 0);
  this->psos_tcp_host = cf->ReadString(section, "tcp_remote_host", DEFAULT_P2OS_TCP_REMOTE_HOST);
  this->psos_tcp_port = cf->ReadInt(section, "tcp_remote_port", DEFAULT_P2OS_TCP_REMOTE_PORT);
  this->radio_modemp = cf->ReadInt(section, "radio", 0);
  this->joystickp = cf->ReadInt(section, "joystick", 0);
  this->direct_wheel_vel_control =
          cf->ReadInt(section, "direct_wheel_vel_control", 1);
  this->motor_max_speed = (int)rint(1e3 * cf->ReadLength(section,
                                                         "max_xspeed",
                                                         MOTOR_DEF_MAX_SPEED));
  this->motor_max_turnspeed = (int)rint(RTOD(cf->ReadAngle(section,
                                                         "max_yawspeed",
                                                         MOTOR_DEF_MAX_TURNSPEED)));
  this->motor_max_trans_accel = (short)rint(1e3 *
                                            cf->ReadLength(section,
                                                           "max_xaccel", 0));
  this->motor_max_trans_decel = (short)rint(1e3 *
                                            cf->ReadLength(section,
                                                           "max_xdecel", 0));
  this->motor_max_rot_accel = (short)rint(RTOD(cf->ReadAngle(section,
                                                             "max_yawaccel",
                                                             0)));
  this->motor_max_rot_decel = (short)rint(RTOD(cf->ReadAngle(section,
                                                             "max_yawdecel",
                                                             0)));

  this->use_vel_band = cf->ReadInt(section, "use_vel_band", 0);

  // Gripper configuration
  gripperPose.px = cf->ReadTupleFloat(section, "gripper_pose", 0, 0.0f);
  gripperPose.py = cf->ReadTupleFloat(section, "gripper_pose", 1, 0.0f);
  gripperPose.pz = cf->ReadTupleFloat(section, "gripper_pose", 2, 0.0f);
  gripperPose.proll = cf->ReadTupleFloat(section, "gripper_pose", 3, 0.0f);
  gripperPose.ppitch = cf->ReadTupleFloat(section, "gripper_pose", 4, 0.0f);
  gripperPose.pyaw = cf->ReadTupleFloat(section, "gripper_pose", 5, 0.0f);
  gripperOuterSize.sw = cf->ReadTupleFloat(section, "gripper_outersize", 0, 0.315f);
  gripperOuterSize.sl = cf->ReadTupleFloat(section, "gripper_outersize", 1, 0.195f);
  gripperOuterSize.sh = cf->ReadTupleFloat(section, "gripper_outersize", 2, 0.035f);
  gripperInnerSize.sw = cf->ReadTupleFloat(section, "gripper_innersize", 0, 0.205f);
  gripperInnerSize.sl = cf->ReadTupleFloat(section, "gripper_innersize", 1, 0.095f);
  gripperInnerSize.sh = cf->ReadTupleFloat(section, "gripper_innersize", 2, 0.035f);

  // Arm gripper configuration
  armGripperOuterSize.sw = cf->ReadTupleFloat(section, "armgrip_outersize", 0, 0.09f);
  armGripperOuterSize.sl = cf->ReadTupleFloat(section, "armgrip_outersize", 1, 0.09f);
  armGripperOuterSize.sh = cf->ReadTupleFloat(section, "armgrip_outersize", 2, 0.041f);
  armGripperInnerSize.sw = cf->ReadTupleFloat(section, "armgrip_innersize", 0, 0.054f);
  armGripperInnerSize.sl = cf->ReadTupleFloat(section, "armgrip_innersize", 1, 0.025f);
  armGripperInnerSize.sh = cf->ReadTupleFloat(section, "armgrip_innersize", 2, 1.0f);

  // Actarray configuration
  // Offsets
  aaLengths[0] = cf->ReadTupleFloat(section, "aa_offsets", 1, 0.06875f);
  aaLengths[1] = cf->ReadTupleFloat(section, "aa_offsets", 2, 0.16f);
  aaLengths[2] = cf->ReadTupleFloat(section, "aa_offsets", 3, 0.0925f);
  aaLengths[3] = cf->ReadTupleFloat(section, "aa_offsets", 4, 0.05f);
  aaLengths[4] = cf->ReadTupleFloat(section, "aa_offsets", 5, 0.085f);
  aaLengths[5] = cf->ReadTupleFloat(section, "aa_offsets", 0, 0.0f);
  // Orientations default: all zeros
  for (int ii = 0; ii < 18; ii++)
  {
    aaOrients[ii] = cf->ReadTupleFloat(section, "aa_orients", ii, 0.0f);
  }
  // Joint 0 default: (0, 0, 1)
  aaAxes[0] = cf->ReadTupleFloat(section, "aa_axes", 0, 0.0f);
  aaAxes[1] = cf->ReadTupleFloat(section, "aa_axes", 1, 0.0f);
  aaAxes[2] = cf->ReadTupleFloat(section, "aa_axes", 2, -1.0f);
  // Joint 1 default: (0, 1, 0)
  aaAxes[3] = cf->ReadTupleFloat(section, "aa_axes", 3, 0.0f);
  aaAxes[4] = cf->ReadTupleFloat(section, "aa_axes", 4, -1.0f);
  aaAxes[5] = cf->ReadTupleFloat(section, "aa_axes", 5, 0.0f);
  // Joint 2 default: (0, 1, 0)
  aaAxes[6] = cf->ReadTupleFloat(section, "aa_axes", 6, 0.0f);
  aaAxes[7] = cf->ReadTupleFloat(section, "aa_axes", 7, -1.0f);
  aaAxes[8] = cf->ReadTupleFloat(section, "aa_axes", 8, 0.0f);
  // Joint 3 default: (1, 0, 0)
  aaAxes[9] = cf->ReadTupleFloat(section, "aa_axes", 9, 1.0f);
  aaAxes[10] = cf->ReadTupleFloat(section, "aa_axes", 10, 0.0f);
  aaAxes[11] = cf->ReadTupleFloat(section, "aa_axes", 11, 0.0f);
  // Joint 4 default: (0, 1, 0)
  aaAxes[12] = cf->ReadTupleFloat(section, "aa_axes", 12, 0.0f);
  aaAxes[13] = cf->ReadTupleFloat(section, "aa_axes", 13, 1.0f);
  aaAxes[14] = cf->ReadTupleFloat(section, "aa_axes", 14, 0.0f);
  // Joint 5 default: (0, 0, 1)
  aaAxes[15] = cf->ReadTupleFloat(section, "aa_axes", 15, 0.0f);
  aaAxes[16] = cf->ReadTupleFloat(section, "aa_axes", 16, 0.0f);
  aaAxes[17] = cf->ReadTupleFloat(section, "aa_axes", 17, 1.0f);
  // Joint base position, orientation
  aaBasePos.px = cf->ReadTupleFloat(section, "aa_basepos", 0, 0.105f);
  aaBasePos.py = cf->ReadTupleFloat(section, "aa_basepos", 1, 0.0f);
  aaBasePos.pz = cf->ReadTupleFloat(section, "aa_basepos", 2, 0.3185f);
  aaBaseOrient.proll = cf->ReadTupleFloat(section, "aa_baseorient", 0, 0.0f);
  aaBaseOrient.ppitch = cf->ReadTupleFloat(section, "aa_baseorient", 1, 0.0f);
  aaBaseOrient.pyaw = cf->ReadTupleFloat(section, "aa_baseorient", 2, 0.0f);
  // Limb configuration
  if(kineCalc)
  {
    limb_data.state = PLAYER_LIMB_STATE_IDLE;
    armOffsetX = cf->ReadTupleFloat(section, "limb_pos", 0, 0.105f);
    armOffsetY = cf->ReadTupleFloat(section, "limb_pos", 1, 0.0f);
    armOffsetZ = cf->ReadTupleFloat(section, "limb_pos", 2, 0.3185f);
    double temp1 = cf->ReadTupleFloat(section, "limb_links", 0, 0.06875f);
    double temp2 = cf->ReadTupleFloat(section, "limb_links", 1, 0.16f);
    double temp3 = cf->ReadTupleFloat(section, "limb_links", 2, 0.0f);
    double temp4 = cf->ReadTupleFloat(section, "limb_links", 3, 0.13775f);
    double temp5 = cf->ReadTupleFloat(section, "limb_links", 4, 0.11321f);
    kineCalc->SetLinkLengths (temp1, temp2, temp3, temp4, temp5);
    kineCalc->SetOffset (0, cf->ReadTupleFloat(section, "limb_offsets", 0, 0.0f));
    kineCalc->SetOffset (0, cf->ReadTupleFloat(section, "limb_offsets", 1, 0.0f));
    kineCalc->SetOffset (0, cf->ReadTupleFloat(section, "limb_offsets", 2, 0.0f));
    kineCalc->SetOffset (0, cf->ReadTupleFloat(section, "limb_offsets", 3, 0.0f));
    kineCalc->SetOffset (0, cf->ReadTupleFloat(section, "limb_offsets", 4, 0.0f));
  }

  this->psos_fd = -1;

  sentGripperCmd = false;
  sentArmGripperCmd = true;
  lastGripperCmd = lastLiftCmd = lastArmGripperCmd = lastActArrayCmd = 255;
  memset (&lastLiftPosCmd, 0, sizeof (player_actarray_position_cmd_t));
  memset (&lastActArrayPosCmd, 0, sizeof (player_actarray_position_cmd_t));
}

int P2OS::MainSetup()
{
  int i;
  // this is the order in which we'll try the possible baud rates. we try 9600
  // first because most robots use it, and because otherwise the radio modem
  // connection code might not work (i think that the radio modems operate at
  // 9600).
  int bauds[] = {B9600, B38400, B19200, B115200, B57600};
  int numbauds = sizeof bauds / sizeof(int);
  int currbaud = 0;

  struct termios term;
  unsigned char command;
  P2OSPacket packet, receivedpacket;
  int flags=0;
  bool sent_close = false;
  enum
  {
    NO_SYNC,
    AFTER_FIRST_SYNC,
    AFTER_SECOND_SYNC,
    READY
  } psos_state;

  psos_state = NO_SYNC;

  char name[20], type[20], subtype[20];
  int cnt;


  if(this->psos_use_tcp)
  {

    // TCP socket:

    printf("P2OS connecting to remote host (%s:%d)... ", this->psos_tcp_host, this->psos_tcp_port);
    fflush(stdout);
    if( (this->psos_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror("P2OS::Setup():socket():");
      return(1);
    }
    //printf("created socket %d.\nLooking up hostname...\n", this->psos_fd);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(this->psos_tcp_port);
#if HAVE_GETADDRINFO
    struct addrinfo * addr_ptr = NULL;
    if (getaddrinfo(this->psos_tcp_host, NULL, NULL, &addr_ptr))
    {
      PLAYER_ERROR("Error looking up hostname or address");
      return 1;
    }
    assert(addr_ptr);
    assert(addr_ptr->ai_addr);
    assert((addr_ptr->ai_addr->sa_family) == AF_INET);
    addr.sin_addr.s_addr = (reinterpret_cast<struct sockaddr_in *>(addr_ptr->ai_addr))->sin_addr.s_addr;
    freeaddrinfo(addr_ptr);
    addr_ptr = NULL;
#else
    struct hostent* h = gethostbyname(this->psos_tcp_host);
    if(!h)
    {
      perror("Error looking up hostname or address %s:");
      return(1);
    }
    assert(static_cast<size_t> (h->h_length) <= sizeof(addr.sin_addr));
    //printf("gethostbyname returned address %d length %d.\n", * h->h_addr, h->h_length);
    memcpy(&(addr.sin_addr), h->h_addr, h->h_length);
    //printf("copied address to addr.sin_addr.s_addr=%d\n", addr.sin_addr.s_addr);
#endif
    PLAYER_WARN("Found host address, connecting...");
    if(connect(this->psos_fd, reinterpret_cast<struct sockaddr*> (&addr), sizeof(addr)) < 0)
    {
      perror("Error Connecting to remote host (P2OS::Setup()::connect()):");
      return(1);
    }
    fcntl(this->psos_fd, F_SETFL, O_SYNC | O_NONBLOCK);
    if((flags = fcntl(this->psos_fd, F_GETFL)) < 0)
    {
      perror("P2OS::Setup():fcntl()");
      close(this->psos_fd);
      this->psos_fd = -1;
      return(1);
    }
    assert(flags & O_NONBLOCK);
    PLAYER_WARN("TCP socket connection is OK... ");
    fflush(stdout);
  }
  else
  {

    // Serial port:

    printf("P2OS connection opening serial port %s...",this->psos_serial_port);
    fflush(stdout);

    if((this->psos_fd = open(this->psos_serial_port,
#ifdef __QNXNTO__
                     O_RDWR | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 )
#else
                     O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 )
#endif
    {
      perror("P2OS::Setup():open():");
      return(1);
    }

    if(tcgetattr( this->psos_fd, &term ) < 0 )
    {
      perror("P2OS::Setup():tcgetattr():");
      close(this->psos_fd);
      this->psos_fd = -1;
      return(1);
    }

    cfmakeraw( &term );
    cfsetispeed(&term, bauds[currbaud]);
    cfsetospeed(&term, bauds[currbaud]);

#if defined (__APPLE__)
               /* CLOCAL:      Local connection (no modem control) */
               /* CREAD:       Enable the receiver */
               term.c_cflag |= (CLOCAL | CREAD);

               /* PARENB:      Use NO parity */
               /* CSTOPB:      Use 1 stop bit */
               /* CSIZE:       Next two constants: */
               /* CS8:         Use 8 data bits */
               term.c_cflag &= ~PARENB;
               term.c_cflag &= ~CSTOPB;
               term.c_cflag &= ~CSIZE;
               term.c_cflag |= CS8;

               /* IGNPAR:      Ignore bytes with parity errors */
               /* ICRNL:       Map CR to NL (otherwise a CR input on  the other computer will not terminate input) */
               term.c_iflag |= (IGNPAR | IGNBRK);

               /* No flags at all for output control  */
               term.c_oflag = 0;

               /* IXON:        Disable software flow control  (incoming) */
               /* IXOFF:       Disable software flow control  (outgoing) */
               /* IXANY:       Disable software flow control (any  character can start flow control */
               term.c_iflag &= ~(IXON | IXOFF | IXANY);

               /* NO FLAGS AT ALL FOR LFLAGS */
               term.c_lflag = 0;

               /* Clean the modem line and activate new port  settings */
               tcflush(this->psos_fd, TCIOFLUSH);
               if (tcsetattr(this->psos_fd, TCSANOW, &term) < 0) {
                       perror("P2OS::Setup():tcsetattr()");
                       close(this->psos_fd);
                       this->psos_fd = -1;
                       return(1);
               }
#else
    if(tcsetattr(this->psos_fd, TCSAFLUSH, &term ) < 0)
    {
      perror("P2OS::Setup():tcsetattr():");
      close(this->psos_fd);
      this->psos_fd = -1;
      return(1);
    }

    if(tcflush(this->psos_fd, TCIOFLUSH ) < 0)
    {
      perror("P2OS::Setup():tcflush():");
      close(this->psos_fd);
      this->psos_fd = -1;
      return(1);
    }
#endif
    if((flags = fcntl(this->psos_fd, F_GETFL)) < 0)
    {
      perror("P2OS::Setup():fcntl()");
      close(this->psos_fd);
      this->psos_fd = -1;
      return(1);
    }

    // radio modem initialization code, courtesy of Kim Jinsuck
    //   <jinsuckk@cs.tamu.edu>
    if(this->radio_modemp)
    {
      puts("Initializing radio modem...");
      int ret = write(this->psos_fd, "WMS2\r", 5);
      if (ret < 5)
      {
    	  PLAYER_ERROR1("P2OS: Write failed to complete (%d)",ret);
    	  return 1;
      }

      usleep(50000);
      char modem_buf[50];
      int buf_len = read(this->psos_fd, modem_buf, 5);          // get "WMS2"
      modem_buf[buf_len]='\0';
      printf("wireless modem response = %s\n", modem_buf);

      usleep(10000);
      // get "\n\rConnecting..." --> \n\r is my guess
      buf_len = read(this->psos_fd, modem_buf, 14);
      modem_buf[buf_len]='\0';
      printf("wireless modem response = %s\n", modem_buf);

      // wait until get "Connected to address 2"
      int modem_connect_try = 10;
      while(strstr(modem_buf, "ected to addres") == NULL)
      {
        puts("Initializing radio modem...");
        int ret = write(this->psos_fd, "WMS2\r", 5);
        if (ret < 5)
        {
        	PLAYER_ERROR1("P2OS: Failed to write full packet to modem (%d)", ret);
        	return 1;
        }

        usleep(50000);
        char modem_buf[50];
        int buf_len = read(this->psos_fd, modem_buf, 5);          // get "WMS2"
        modem_buf[buf_len]='\0';
        printf("wireless modem response = %s\n", modem_buf);
        // if "Partner busy!"
        if(modem_buf[2] == 'P')
        {
          printf("Please reset partner modem and try again\n");
          return(1);
        }
        // if "\n\rPartner not found!"
        if(modem_buf[0] == 'P')
        {
          printf("Please check partner modem and try again\n");
          return(1);
        }
        if(modem_connect_try-- == 0)
        {
          usleep(300000);
          buf_len = read(this->psos_fd, modem_buf, 40);
          modem_buf[buf_len]='\0';
          printf("wireless modem response = %s\n", modem_buf);
          // if "Partner busy!"
          if(modem_buf[2] == 'P')
          {
            printf("Please reset partner modem and try again\n");
            return(1);
          }
          // if "\n\rPartner not found!"
          if(modem_buf[0] == 'P')
          {
            printf("Please check partner modem and try again\n");
            return(1);
          }
          if(modem_connect_try-- == 0)
          {
            puts("Failed to connect radio modem, Trying direct connection...");
            break;
          }
        }
      }
    }
    printf("Connected to robot device, handshaking with P2OS...");
    fflush(stdout);
  }// end TCP socket or serial port.

  // Sync:

  int num_sync_attempts = 3;
  while(psos_state != READY)
  {
    switch(psos_state)
    {
      case NO_SYNC:
        command = SYNC0;
        packet.Build(&command, 1);
        packet.Send(this->psos_fd);
        usleep(P2OS_CYCLETIME_USEC);
        break;
      case AFTER_FIRST_SYNC:
        printf("turning off NONBLOCK mode...\n");
        if(fcntl(this->psos_fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
        {
          perror("P2OS::Setup():fcntl()");
          close(this->psos_fd);
          this->psos_fd = -1;
          return(1);
        }
        command = SYNC1;
        packet.Build(&command, 1);
        packet.Send(this->psos_fd);
        break;
      case AFTER_SECOND_SYNC:
        command = SYNC2;
        packet.Build(&command, 1);
        packet.Send(this->psos_fd);
        break;
      default:
        puts("P2OS::Setup():shouldn't be here...");
        break;
    }
    usleep(P2OS_CYCLETIME_USEC);

    if(receivedpacket.Receive(this->psos_fd, this->ignore_checksum))
    {
      if((psos_state == NO_SYNC) && (num_sync_attempts >= 0))
      {
        num_sync_attempts--;
        usleep(P2OS_CYCLETIME_USEC);
        continue;
      }
      else
      {
        // couldn't connect; try different speed.
        if(++currbaud < numbauds)
        {
          cfsetispeed(&term, bauds[currbaud]);
          cfsetospeed(&term, bauds[currbaud]);
          if( tcsetattr(this->psos_fd, TCSAFLUSH, &term ) < 0 )
          {
            perror("P2OS::Setup():tcsetattr():");
            close(this->psos_fd);
            this->psos_fd = -1;
            return(1);
          }

          if(tcflush(this->psos_fd, TCIOFLUSH ) < 0 )
          {
            perror("P2OS::Setup():tcflush():");
            close(this->psos_fd);
            this->psos_fd = -1;
            return(1);
          }
          num_sync_attempts = 3;
          continue;
        }
        else
        {
          // tried all speeds; bail
          break;
        }
      }
    }

    switch(receivedpacket.packet[3])
    {
      case SYNC0:
        psos_state = AFTER_FIRST_SYNC;
        break;
      case SYNC1:
        psos_state = AFTER_SECOND_SYNC;
        break;
      case SYNC2:
        psos_state = READY;
        break;
      default:
        // maybe P2OS is still running from last time.  let's try to CLOSE
        // and reconnect
        if(!sent_close)
        {
          //puts("sending CLOSE");
          command = CLOSE;
          packet.Build( &command, 1);
          packet.Send(this->psos_fd);
          sent_close = true;
          usleep(2*P2OS_CYCLETIME_USEC);
          tcflush(this->psos_fd,TCIFLUSH);
          psos_state = NO_SYNC;
        }
        break;
    }
    usleep(P2OS_CYCLETIME_USEC);
  }

  if(psos_state != READY)
  {
    printf("Couldn't synchronize with P2OS.\n"
           "  Most likely because the robot is not connected %s %s\n",
           this->psos_use_tcp ? "to the ethernet-serial bridge device " : "to the serial port",
           this->psos_use_tcp ? this->psos_tcp_host : this->psos_serial_port);
    close(this->psos_fd);
    this->psos_fd = -1;
    return(1);
  }

  cnt = 4;
  cnt += snprintf(name, sizeof(name), "%s", &receivedpacket.packet[cnt]);
  cnt++;
  cnt += snprintf(type, sizeof(type), "%s", &receivedpacket.packet[cnt]);
  cnt++;
  cnt += snprintf(subtype, sizeof(subtype), "%s", &receivedpacket.packet[cnt]);
  cnt++;


  command = OPEN;
  packet.Build(&command, 1);
  packet.Send(this->psos_fd);
  usleep(P2OS_CYCLETIME_USEC);

  command = PULSE;
  packet.Build(&command, 1);
  packet.Send(this->psos_fd);
  usleep(P2OS_CYCLETIME_USEC);

  printf("Done.\n   Connected to %s, a %s %s\n", name, type, subtype);

  // now, based on robot type, find the right set of parameters
  for(i=0;i<PLAYER_NUM_ROBOT_TYPES;i++)
  {
    if(!strcasecmp(PlayerRobotParams[i].Class,type) &&
       !strcasecmp(PlayerRobotParams[i].Subclass,subtype))
    {
      param_idx = i;
      break;
    }
  }
  if(i == PLAYER_NUM_ROBOT_TYPES)
  {
    fputs("P2OS: Warning: couldn't find parameters for this robot; "
            "using defaults\n",stderr);
    param_idx = 0;
  }
  // first, receive a packet so we know we're connected.
  if(!this->sippacket)
    this->sippacket = new SIP(param_idx);

  this->sippacket->x_offset = 0;
  this->sippacket->y_offset = 0;
  this->sippacket->angle_offset = 0;

  SendReceive((P2OSPacket*)NULL,false);

  // turn off the sonars at first
  this->ToggleSonarPower(0);

  if(this->joystickp)
  {
    // enable joystick control
    P2OSPacket js_packet;
    unsigned char js_command[4];
    js_command[0] = JOYDRIVE;
    js_command[1] = ARGINT;
    js_command[2] = 1;
    js_command[3] = 0;
    js_packet.Build(js_command, 4);
    this->SendReceive(&js_packet,false);
  }

  if(this->blobfinder_id.interf)
    CMUcamReset(false);

  if(this->gyro_id.interf)
  {
    // request that gyro data be sent each cycle
    P2OSPacket gyro_packet;
    unsigned char gyro_command[4];
    gyro_command[0] = GYRO;
    gyro_command[1] = ARGINT;
    gyro_command[2] = 1;
    gyro_command[3] = 0;
    gyro_packet.Build(gyro_command, 4);
    this->SendReceive(&gyro_packet,false);
  }

  if (this->actarray_id.interf)
  {
    // Start a continuous stream of ARMpac packets
    P2OSPacket aaPacket;
    unsigned char aaCmd[4];
    aaCmd[0] = ARM_STATUS;
    aaCmd[1] = ARGINT;
    aaCmd[2] = 2;
    aaCmd[3] = 0;
    aaPacket.Build (aaCmd, 4);
    SendReceive (&aaPacket,false);
    // Ask for an ARMINFOpac packet too
    aaCmd[0] = ARM_INFO;
    aaPacket.Build (aaCmd, 1);
    SendReceive (&aaPacket,false);
  }

  // if requested, set max accel/decel limits
  P2OSPacket accel_packet;
  unsigned char accel_command[4];
  if(this->motor_max_trans_accel > 0)
  {
    accel_command[0] = SETA;
    accel_command[1] = ARGINT;
    accel_command[2] = this->motor_max_trans_accel & 0x00FF;
    accel_command[3] = (this->motor_max_trans_accel & 0xFF00) >> 8;
    accel_packet.Build(accel_command, 4);
    this->SendReceive(&accel_packet,false);
  }

  if(this->motor_max_trans_decel < 0)
  {
    accel_command[0] = SETA;
    accel_command[1] = ARGNINT;
    accel_command[2] = abs(this->motor_max_trans_decel) & 0x00FF;
    accel_command[3] = (abs(this->motor_max_trans_decel) & 0xFF00) >> 8;
    accel_packet.Build(accel_command, 4);
    this->SendReceive(&accel_packet,false);
  }
  if(this->motor_max_rot_accel > 0)
  {
    accel_command[0] = SETRA;
    accel_command[1] = ARGINT;
    accel_command[2] = this->motor_max_rot_accel & 0x00FF;
    accel_command[3] = (this->motor_max_rot_accel & 0xFF00) >> 8;
    accel_packet.Build(accel_command, 4);
    this->SendReceive(&accel_packet,false);
  }
  if(this->motor_max_rot_decel < 0)
  {
    accel_command[0] = SETRA;
    accel_command[1] = ARGNINT;
    accel_command[2] = abs(this->motor_max_rot_decel) & 0x00FF;
    accel_command[3] = (abs(this->motor_max_rot_decel) & 0xFF00) >> 8;
    accel_packet.Build(accel_command, 4);
    this->SendReceive(&accel_packet,false);
  }


  // if requested, change PID settings
  P2OSPacket pid_packet;
  unsigned char pid_command[4];
  if(this->rot_kp >= 0)
  {
    pid_command[0] = ROTKP;
    pid_command[1] = ARGINT;
    pid_command[2] = this->rot_kp & 0x00FF;
    pid_command[3] = (this->rot_kp & 0xFF00) >> 8;
    pid_packet.Build(pid_command, 4);
    this->SendReceive(&pid_packet);
  }
  if(this->rot_kv >= 0)
  {
    pid_command[0] = ROTKV;
    pid_command[1] = ARGINT;
    pid_command[2] = this->rot_kv & 0x00FF;
    pid_command[3] = (this->rot_kv & 0xFF00) >> 8;
    pid_packet.Build(pid_command, 4);
    this->SendReceive(&pid_packet);
  }
  if(this->rot_ki >= 0)
  {
    pid_command[0] = ROTKI;
    pid_command[1] = ARGINT;
    pid_command[2] = this->rot_ki & 0x00FF;
    pid_command[3] = (this->rot_ki & 0xFF00) >> 8;
    pid_packet.Build(pid_command, 4);
    this->SendReceive(&pid_packet);
  }
  if(this->trans_kp >= 0)
  {
    pid_command[0] = TRANSKP;
    pid_command[1] = ARGINT;
    pid_command[2] = this->trans_kp & 0x00FF;
    pid_command[3] = (this->trans_kp & 0xFF00) >> 8;
    pid_packet.Build(pid_command, 4);
    this->SendReceive(&pid_packet);
  }
  if(this->trans_kv >= 0)
  {
    pid_command[0] = TRANSKV;
    pid_command[1] = ARGINT;
    pid_command[2] = this->trans_kv & 0x00FF;
    pid_command[3] = (this->trans_kv & 0xFF00) >> 8;
    pid_packet.Build(pid_command, 4);
    this->SendReceive(&pid_packet);
  }
  if(this->trans_ki >= 0)
  {
    pid_command[0] = TRANSKI;
    pid_command[1] = ARGINT;
    pid_command[2] = this->trans_ki & 0x00FF;
    pid_command[3] = (this->trans_ki & 0xFF00) >> 8;
    pid_packet.Build(pid_command, 4);
    this->SendReceive(&pid_packet);
  }


  // if requested, change bumper-stall behavior
  // 0 = don't stall
  // 1 = stall on front bumper contact
  // 2 = stall on rear bumper contact
  // 3 = stall on either bumper contact
  if(this->bumpstall >= 0)
  {
    if(this->bumpstall > 3)
      PLAYER_ERROR1("ignoring bumpstall value %d; should be 0, 1, 2, or 3",
                    this->bumpstall);
    else
    {
      PLAYER_MSG1(1, "setting bumpstall to %d", this->bumpstall);
      P2OSPacket bumpstall_packet;;
      unsigned char bumpstall_command[4];
      bumpstall_command[0] = BUMP_STALL;
      bumpstall_command[1] = ARGINT;
      bumpstall_command[2] = (unsigned char)this->bumpstall;
      bumpstall_command[3] = 0;
      bumpstall_packet.Build(bumpstall_command, 4);
      this->SendReceive(&bumpstall_packet,false);
    }
  }

  // Set up the PTZ Camera
  if(this->ptz_id.interf){
    SetupPtz();
  }


  // TODO: figure out what the right behavior here is
#if 0
  // zero position command buffer
  player_position_cmd_t zero;
  memset(&zero,0,sizeof(player_position_cmd_t));
  this->PutCommand(this->position_id,(void*)&zero,
                   sizeof(player_position_cmd_t),NULL);
#endif

  return(0);
}






void P2OS::MainQuit()
{
  unsigned char command[20],buffer[20];
  P2OSPacket packet;

  memset(buffer,0,20);

  if(this->psos_fd == -1)
    return;

  // Shut Down the PTZ camera
  if(this->ptz_id.interf){
      usleep(PTZ_SLEEP_TIME_USEC);
      SendAbsPanTilt(0,0);
      usleep(PTZ_SLEEP_TIME_USEC);
      SendAbsZoom(0);
      setPower(0);
      puts("PTZ camera has been shutdown");
  }

  command[0] = STOP;
  packet.Build(command, 1);
  packet.Send(this->psos_fd);
  usleep(P2OS_CYCLETIME_USEC);

  command[0] = CLOSE;
  packet.Build(command, 1);
  packet.Send(this->psos_fd);
  usleep(P2OS_CYCLETIME_USEC);

  close(this->psos_fd);
  this->psos_fd = -1;
  puts("P2OS has been shutdown");
  delete this->sippacket;
  this->sippacket = NULL;
}

P2OS::~P2OS (void)
{
  player_position2d_data_t_cleanup(&p2os_data.position);
  player_sonar_data_t_cleanup (&p2os_data.sonar);
  player_gripper_data_t_cleanup (&p2os_data.gripper);
  player_gripper_data_t_cleanup (&p2os_data.armGripper);
  player_power_data_t_cleanup (&p2os_data.power);
  player_bumper_data_t_cleanup (&p2os_data.bumper);
  player_dio_data_t_cleanup (&p2os_data.dio);
  player_aio_data_t_cleanup (&p2os_data.aio);
  player_blobfinder_data_t_cleanup (&p2os_data.blobfinder);
  player_position2d_data_t_cleanup (&p2os_data.compass);
  player_position2d_data_t_cleanup (&p2os_data.gyro);
  player_actarray_data_t_cleanup (&p2os_data.lift);
  player_actarray_data_t_cleanup (&p2os_data.actArray);

  if (kineCalc)
  {
    delete kineCalc;
    kineCalc = NULL;
  }
}

int
P2OS::Subscribe(player_devaddr_t id)
{
  int setupResult;

  // do the subscription
  if((setupResult = Driver::Subscribe(id)) == 0)
  {
    // also increment the appropriate subscription counter
    if(Device::MatchDeviceAddress(id, this->position_id))
      this->position_subscriptions++;
    else if(Device::MatchDeviceAddress(id, this->sonar_id))
      this->sonar_subscriptions++;
    else if(Device::MatchDeviceAddress(id, this->actarray_id) ||
            Device::MatchDeviceAddress(id, this->limb_id) ||
            Device::MatchDeviceAddress(id, this->armgripper_id))
      // We use the actarray subscriptions count for the limb and arm gripper
      // interfaces too since they're the same physical hardware
      this->actarray_subscriptions++;
  }

  return(setupResult);
}

int
P2OS::Unsubscribe(player_devaddr_t id)
{
  int shutdownResult;

  // do the unsubscription
  if((shutdownResult = Driver::Unsubscribe(id)) == 0)
  {
    // also decrement the appropriate subscription counter
    if(Device::MatchDeviceAddress(id, this->position_id))
    {
      this->position_subscriptions--;
      assert(this->position_subscriptions >= 0);
    }
    else if(Device::MatchDeviceAddress(id, this->sonar_id))
    {
      this->sonar_subscriptions--;
      assert(this->sonar_subscriptions >= 0);
    }
    else if(Device::MatchDeviceAddress(id, this->actarray_id) ||
            Device::MatchDeviceAddress(id, this->limb_id) ||
            Device::MatchDeviceAddress(id, this->armgripper_id))
    {
      // We use the actarray subscriptions count for the limb
      // interface too since they're the same physical hardware
      this->actarray_subscriptions--;
      assert(this->actarray_subscriptions >= 0);
    }
  }

  return(shutdownResult);
}

void
P2OS::StandardSIPPutData(double timestampStandardSIP)
{
  // put odometry data
  this->Publish(this->position_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_POSITION2D_DATA_STATE,
                (void*)&(this->p2os_data.position),
                sizeof(player_position2d_data_t),
                &timestampStandardSIP);

  // put sonar data
  this->Publish(this->sonar_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_SONAR_DATA_RANGES,
                (void*)&(this->p2os_data.sonar),
                sizeof(player_sonar_data_t),
                &timestampStandardSIP);
  delete this->p2os_data.sonar.ranges;

  // put aio data
  this->Publish(this->aio_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_AIO_DATA_STATE,
                (void*)&(this->p2os_data.aio),
                sizeof(player_aio_data_t),
                &timestampStandardSIP);

  // put dio data
  this->Publish(this->dio_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_DIO_DATA_VALUES,
                (void*)&(this->p2os_data.dio),
                sizeof(player_dio_data_t),
                &timestampStandardSIP);

  // put gripper data
  this->Publish(this->gripper_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_GRIPPER_DATA_STATE,
                (void*)&(this->p2os_data.gripper),
                sizeof(player_gripper_data_t),
                &timestampStandardSIP);

  // put lift data
  this->Publish(this->lift_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_ACTARRAY_DATA_STATE,
                (void*)&(this->p2os_data.lift),
                sizeof(player_actarray_data_t),
                &timestampStandardSIP);

  // put bumper data
  this->Publish(this->bumper_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_BUMPER_DATA_STATE,
                (void*)&(this->p2os_data.bumper),
                sizeof(player_bumper_data_t),
                &timestampStandardSIP);

  // put power data
  this->Publish(this->power_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_POWER_DATA_STATE,
                (void*)&(this->p2os_data.power),
                sizeof(player_power_data_t),
                &timestampStandardSIP);

  // put compass data
  this->Publish(this->compass_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_POSITION2D_DATA_STATE,
                (void*)&(this->p2os_data.compass),
                sizeof(player_position2d_data_t),
                &timestampStandardSIP);

  // put PTZ data
  this->Publish(this->ptz_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_PTZ_DATA_STATE,
                (void*)&(this->ptz_data));
}

void
P2OS::GyroPutData(double timestampGyro)
{
  // put gyro data
  this->Publish(this->gyro_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_POSITION2D_DATA_STATE,
                (void*)&(this->p2os_data.gyro),
                sizeof(player_position2d_data_t),
                &timestampGyro);
}

void
P2OS::BlobfinderPutData(double timestampSERAUX)
{
  // put blobfinder data
  this->Publish(this->blobfinder_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_BLOBFINDER_DATA_BLOBS,
                (void*)&(this->p2os_data.blobfinder),
                sizeof(player_blobfinder_data_t),
                &timestampSERAUX);
}

void
P2OS::ActarrayPutData(double timestampArm)
{
  // put actarray data
  this->Publish(this->actarray_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_ACTARRAY_DATA_STATE,
                (void*)&(this->p2os_data.actArray),
                sizeof(player_actarray_data_t),
                &timestampArm);
  delete[] this->p2os_data.actArray.actuators;

  // put limb data
  this->Publish(this->limb_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_LIMB_DATA_STATE,
                (void*)&(this->limb_data),
                sizeof(player_limb_data_t),
                &timestampArm);

  // put arm gripper data
  this->Publish(this->armgripper_id,
                PLAYER_MSGTYPE_DATA,
                PLAYER_GRIPPER_DATA_STATE,
                (void*)&(this->p2os_data.armGripper),
                sizeof(player_gripper_data_t),
                &timestampArm);
}

void
P2OS::Main()
{
  int last_sonar_subscrcount=0;
  int last_position_subscrcount=0;
  int last_actarray_subscrcount=0;
  double currentTime;
  struct timeval timeVal;

  for(;;)
  {
    pthread_testcancel();

    // we want to turn on the sonars if someone just subscribed, and turn
    // them off if the last subscriber just unsubscribed.
    if(!last_sonar_subscrcount && this->sonar_subscriptions)
      this->ToggleSonarPower(1);
    else if(last_sonar_subscrcount && !(this->sonar_subscriptions))
      this->ToggleSonarPower(0);
    last_sonar_subscrcount = this->sonar_subscriptions;

    // Same for the actarray - this will also turn it on and off with limb subscriptions
    if(!last_actarray_subscrcount && this->actarray_subscriptions)
      this->ToggleActArrayPower(1, false);
    else if(last_actarray_subscrcount && !(this->actarray_subscriptions))
      this->ToggleActArrayPower(0, false);
    last_actarray_subscrcount = this->actarray_subscriptions;

    // we want to reset the odometry and enable the motors if the first
    // client just subscribed to the position device, and we want to stop
    // and disable the motors if the last client unsubscribed.
    if(!last_position_subscrcount && this->position_subscriptions)
    {
      this->ToggleMotorPower(0);
      this->ResetRawPositions();
    }
    else if(last_position_subscrcount && !(this->position_subscriptions))
    {
      // enable motor power
      this->ToggleMotorPower(1);
    }
    last_position_subscrcount = this->position_subscriptions;

    // The Amigo board seems to drop commands once in a while.  This is
    // a hack to restart the serial reads if that happens.
    if(this->blobfinder_id.interf)
    {
      struct timeval now_tv;
      GlobalTime->GetTime(&now_tv);
      if (now_tv.tv_sec > lastblob_tv.tv_sec)
      {
        P2OSPacket cam_packet;
        unsigned char cam_command[4];

        cam_command[0] = GETAUX2;
        cam_command[1] = ARGINT;
        cam_command[2] = 0;
        cam_command[3] = 0;
        cam_packet.Build(cam_command, 4);
        SendReceive(&cam_packet);

        cam_command[0] = GETAUX2;
        cam_command[1] = ARGINT;
        cam_command[2] = CMUCAM_MESSAGE_LEN * 2 -1;
        cam_command[3] = 0;
        cam_packet.Build(cam_command, 4);
        SendReceive(&cam_packet);
        GlobalTime->GetTime(&lastblob_tv);  // Reset last blob packet time
      }
    }

    if(this->ptz_id.interf)
    {
        int pan,tilt;
        int zoom;
        //fprintf(stderr, "PTZ MAIN LOOP\n");
        //fprintf(stderr, "Getting Pan/Tilt Data\n");
        if(GetAbsPanTilt(&pan,&tilt) < 0)
        {
            fputs("canonvcc4:Main():GetAbsPanTilt() errored. bailing.\n",
                    stderr);
            pthread_exit(NULL);
        }

        usleep(30000);
        //fprintf(stderr, "Getting Zoom Data\n");
        zoom = 0;
        if(GetAbsZoom(&zoom) < 0)
        {
            fputs("canonvcc4:Main():GetAbsZoom() errored. bailing.\n", stderr);
            pthread_exit(NULL);
        }
        // Do the necessary coordinate conversions.  Camera's natural pan
        // coordinates increase clockwise; we want them the other way, so
        // we negate pan here.  Zoom values are converted from arbitrary
        // units to a field of view (in degrees).
        pan = -pan;

        //printf("before zoom = %i\n", zoom);
        //zoom = this->maxfov + (zoom * (this->maxfov - this->minfov)) / maxzoom;
        //printf("after zoom = %i\n", zoom);

        ptz_data.pan = DTOR((unsigned short)pan);
        ptz_data.tilt = DTOR((unsigned short)tilt);

        //double m = (double)(this->minfov - this->maxfov) / (double)this->maxzoom;
        ptz_data.zoom = DTOR(this->maxfov+(zoom * (double)(this->minfov - this->maxfov) / (double)this->maxzoom));
    }

    // handle pending messages
    if(!this->InQueue->Empty())
    {
      ProcessMessages();
    }

    // Check if need to send a pulse to the robot
    if (this->pulse != -1)
    {
      gettimeofday (&timeVal, NULL);
      currentTime = static_cast<double> (timeVal.tv_sec) + (static_cast<double> (timeVal.tv_usec) / 1e6);
      if ((currentTime - lastPulseTime) > this->pulse)
      {
        SendPulse ();
        // Update the time of last pulse/command
        lastPulseTime = currentTime;
      }
    }
    // Hack fix to get around the fact that if no commands are sent to the robot via SendReceive,
    // the driver will never read SIP packets and so never send data back to clients.
    // We need a better way of doing regular checks of the serial port - peek in sendreceive, maybe?
    // Because if there is no data waiting this will sit around waiting until one comes
    SendReceive (NULL, true);
  }
}

/* send the packet, then receive and parse an SIP */
int
P2OS::SendReceive(P2OSPacket* pkt, bool publish_data)
{
  P2OSPacket packet;

  // zero the combined data buffer.  it will be filled with the latest data
  // by corresponding SIP::Fill*()
  memset(&(this->p2os_data),0,sizeof(player_p2os_data_t));
  if((this->psos_fd >= 0) && this->sippacket)
  {
    if(pkt)
      pkt->Send(this->psos_fd);

    /* receive a packet */
    pthread_testcancel();
    if(packet.Receive(this->psos_fd, this->ignore_checksum))
    {
      puts("RunPsosThread(): Receive errored");
      pthread_exit(NULL);
    }

    if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB &&
       (packet.packet[3] == 0x30 || packet.packet[3] == 0x31 ||
       packet.packet[3] == 0x32 || packet.packet[3] == 0x33 ||
       packet.packet[3] == 0x34))
    {

      /* It is a server packet, so process it */
      this->sippacket->ParseStandard( &packet.packet[3] );
      this->sippacket->FillStandard(&(this->p2os_data));

      if(publish_data)
        this->StandardSIPPutData(packet.timestamp);
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB &&
            packet.packet[3] == SERAUX)
    {
       // This is an AUX serial packet
        if(ptz_id.interf)
        {
            /* It is an extended SIP (ptz) packet, so process it */
            /* Be sure to pass data size too (packet[2])! */
            //fprintf(stderr,"Got a PTZ packet\n");
            int len;
            //printf("Got ptz packet\n");

            len = packet.packet[2] - 3;
            //packet.PrintHex();
            //fprintf(stderr, "Got PTZ Packet of length %i\n",len);

            if ( cb.gotPacket() ){
                fprintf(stderr, "ptz_error: got a message, but we already have the complete packet.\n");
            }
            else {
                for ( int i = 4; i < 4+len ; i++ )
                {
                    cb.putOnBuf(packet.packet[i]);
                }
            }

        }
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB &&
            packet.packet[3] == SERAUX2)
    {
      // This is an AUX2 serial packet

      if(blobfinder_id.interf)
      {
        /* It is an extended SIP (blobfinder) packet, so process it */
        /* Be sure to pass data size too (packet[2])! */
        this->sippacket->ParseSERAUX( &packet.packet[2] );
        this->sippacket->FillSERAUX(&(this->p2os_data));

        if(publish_data)
          this->BlobfinderPutData(packet.timestamp);

        P2OSPacket cam_packet;
        unsigned char cam_command[4];

        /* We cant get the entire contents of the buffer,
        ** and we cant just have P2OS send us the buffer on a regular basis.
        ** My solution is to flush the buffer and then request exactly
        ** CMUCAM_MESSAGE_LEN * 2 -1 bytes of data.  This ensures that
        ** we will get exactly one full message, and it will be "current"
        ** within the last 2 messages.  Downside is that we end up pitching
        ** every other CMUCAM message.  Tradeoffs... */
        // Flush
        cam_command[0] = GETAUX2;
        cam_command[1] = ARGINT;
        cam_command[2] = 0;
        cam_command[3] = 0;
        cam_packet.Build(cam_command, 4);
        this->SendReceive(&cam_packet,publish_data);

        // Reqest next packet
        cam_command[0] = GETAUX2;
        cam_command[1] = ARGINT;
        // Guarantee exactly 1 full message
        cam_command[2] = CMUCAM_MESSAGE_LEN * 2 -1;
        cam_command[3] = 0;
        cam_packet.Build(cam_command, 4);
        this->SendReceive(&cam_packet,publish_data);
        GlobalTime->GetTime(&lastblob_tv);  // Reset last blob packet time
      }
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB &&
            (packet.packet[3] == 0x50 || packet.packet[3] == 0x80 ||
//            packet.packet[3] == 0xB0 || packet.packet[3] == 0xC0 ||
            packet.packet[3] == 0xC0 ||
            packet.packet[3] == 0xD0 || packet.packet[3] == 0xE0))
    {
      /* It is a vision packet from the old Cognachrome system*/

      /* we don't understand these yet, so ignore */
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB &&
            packet.packet[3] == GYROPAC)
    {
      if(this->gyro_id.interf)
      {
        /* It's a set of gyro measurements */
        this->sippacket->ParseGyro(&packet.packet[2]);
        this->sippacket->FillGyro(&(this->p2os_data));

        if(publish_data)
          this->GyroPutData(packet.timestamp);

        /* Now, the manual says that we get one gyro packet each cycle,
         * right before the standard SIP.  So, we'll call SendReceive()
         * again (with no packet to send) to get the standard SIP.  There's
         * a definite danger of infinite recursion here if the manual
         * is wrong.
         */
        this->SendReceive(NULL,publish_data);
      }
    }
    else if(packet.packet[0] == 0xFA && packet.packet[1] == 0xFB &&
            (packet.packet[3] == 0x20))
    {
      //printf("got a CONFIGpac:%d\n",packet.size);
    }
    else if (packet.packet[0] == 0xFA && packet.packet[1] == 0xFB && packet.packet[3] == ARMPAC)
    {
      if (actarray_id.interf)
      {
        // ARMpac - current arm status
        double joints[6];
        sippacket->ParseArm (&packet.packet[2]);
        for (int ii = 0; ii < 6; ii++)
        {
          sippacket->armJointPosRads[ii] = TicksToRadians (ii, sippacket->armJointPos[ii]);
          joints[ii] = sippacket->armJointPosRads[ii];
        }
        sippacket->FillArm(&p2os_data);
        if(kineCalc)
        {
          kineCalc->CalculateFK (joints);
          limb_data.position.px = kineCalc->GetP ().x + armOffsetX;
          limb_data.position.py = kineCalc->GetP ().y + armOffsetY;
          limb_data.position.pz = kineCalc->GetP ().z + armOffsetZ;
          limb_data.approach.px = kineCalc->GetA ().x;
          limb_data.approach.py = kineCalc->GetA ().y;
          limb_data.approach.pz = kineCalc->GetA ().z;
          limb_data.orientation.px = kineCalc->GetO ().x;
          limb_data.orientation.py = kineCalc->GetO ().y;
          limb_data.orientation.pz = kineCalc->GetO ().z;
          if (limb_data.state != PLAYER_LIMB_STATE_OOR && limb_data.state != PLAYER_LIMB_STATE_COLL)
          {
            if (sippacket->armJointMoving[0] || sippacket->armJointMoving[1] || sippacket->armJointMoving[2] ||
                sippacket->armJointMoving[3] || sippacket->armJointMoving[4])
            {
              limb_data.state = PLAYER_LIMB_STATE_MOVING;
            }
            else
              limb_data.state = PLAYER_LIMB_STATE_IDLE;
          }
        }
        if(publish_data)
        this->ActarrayPutData(packet.timestamp);
      }

      // Go for another SIP - there had better be one or things will probably go boom
      SendReceive(NULL,publish_data);
    }
    else if (packet.packet[0] == 0xFA && packet.packet[1] == 0xFB && packet.packet[3] == ARMINFOPAC)
    {
      // ARMINFOpac - arm configuration stuff
      if (actarray_id.interf)
      {
        sippacket->ParseArmInfo (&packet.packet[2]);
        // Update the KineCalc with the new info for joints - one would assume this doesn't change, though...
        if (kineCalc)
        {
          for (int ii = 0; ii < 5; ii++)
            kineCalc->SetJointRange (ii, TicksToRadians (ii, sippacket->armJoints[ii].min), TicksToRadians (ii, sippacket->armJoints[ii].max));
          // Go for another SIP - there had better be one or things will probably go boom
        }

        SendReceive(NULL,publish_data);
      }
    }
    else
    {
      packet.PrintHex();
    }
  }

  return(0);
}

void
P2OS::ResetRawPositions()
{
  P2OSPacket pkt;
  unsigned char p2oscommand[4];

  if(this->sippacket)
  {
    this->sippacket->rawxpos = 0;
    this->sippacket->rawypos = 0;
    this->sippacket->xpos = 0;
    this->sippacket->ypos = 0;
    p2oscommand[0] = SETO;
    p2oscommand[1] = ARGINT;
    pkt.Build(p2oscommand, 2);
    this->SendReceive(&pkt,false);
  }
}


/****************************************************************
** Reset the CMUcam.  This includes flushing the buffer and
** setting interface output mode to raw.  It also restarts
** tracking output (current mode)
****************************************************************/
void P2OS::CMUcamReset(bool doLock)
{
  CMUcamStopTracking(doLock); // Stop the current tracking.

  P2OSPacket cam_packet;
  unsigned char cam_command[10];

  printf("Resetting the CMUcam...\n");
  cam_command[0] = TTY3;
  cam_command[1] = ARGSTR;
  strncpy((char*)&cam_command[3], "RS\r",4);
  cam_command[2] = strlen((char *)&cam_command[3]);
  cam_packet.Build(cam_command, (int)cam_command[2]+3);
  this->SendReceive(&cam_packet,doLock);

  // Set for raw output + no ACK/NACK
  printf("Setting raw mode...\n");
  cam_command[0] = TTY3;
  cam_command[1] = ARGSTR;
  strncpy((char*)&cam_command[3], "RM 3\r",6);
  cam_command[2] = strlen((char *)&cam_command[3]);
  cam_packet.Build(cam_command, (int)cam_command[2]+3);
  this->SendReceive(&cam_packet,doLock);
  usleep(100000);

  printf("Flushing serial buffer...\n");
  cam_command[0] = GETAUX2;
  cam_command[1] = ARGINT;
  cam_command[2] = 0;
  cam_command[3] = 0;
  cam_packet.Build(cam_command, 4);
  this->SendReceive(&cam_packet,doLock);

  sleep(1);
  // (Re)start tracking
  this->CMUcamStartTracking(false);
}


/****************************************************************
** Start CMUcam blob tracking.  This method can be called 3 ways:
**   1) with a set of 6 color arguments (RGB min and max)
**   2) with auto tracking (-1 argument)
**   3) with current values (0 or no arguments)
****************************************************************/
void P2OS::CMUcamTrack(int rmin, int rmax,
                       int gmin, int gmax,
                       int bmin, int bmax)
{
  this->CMUcamStopTracking(); // Stop the current tracking.

  P2OSPacket cam_packet;
  unsigned char cam_command[50];

  if (!rmin && !rmax && !gmin && !gmax && !bmin && !bmax)
  {
    CMUcamStartTracking();
  }
  else if (rmin<0 || rmax<0 || gmin<0 || gmax<0 || bmin<0 || bmax<0)
  {
    printf("Activating CMUcam color tracking (AUTO-mode)...\n");
    cam_command[0] = TTY3;
    cam_command[1] = ARGSTR;
    strncpy((char*)&cam_command[3], "TW\r",4);
    cam_command[2] = strlen((char *)&cam_command[3]);
    cam_packet.Build(cam_command, (int)cam_command[2]+3);
    this->SendReceive(&cam_packet);
  }
  else
  {
    printf("Activating CMUcam color tracking (MANUAL-mode)...\n");
    //printf("      RED: %d %d    GREEN: %d %d    BLUE: %d %d\n",
    //                   rmin, rmax, gmin, gmax, bmin, bmax);
    cam_command[0] = TTY3;
    cam_command[1] = ARGSTR;
    snprintf((char*)&cam_command[3], sizeof(cam_command) - 3, "TC %d %d %d %d %d %d\r",
             rmin, rmax, gmin, gmax, bmin, bmax);
    cam_command[2] = strlen((char *)&cam_command[3]);
    cam_packet.Build(cam_command, (int)cam_command[2]+3);
    this->SendReceive(&cam_packet);
  }

  cam_command[0] = GETAUX2;
  cam_command[1] = ARGINT;
  cam_command[2] = CMUCAM_MESSAGE_LEN * 2 -1; // Guarantee 1 full message
  cam_command[3] = 0;
  cam_packet.Build(cam_command, 4);
  this->SendReceive(&cam_packet);
}

/****************************************************************
** Start Tracking - with last config
****************************************************************/
void P2OS::CMUcamStartTracking(bool doLock)
{
   P2OSPacket cam_packet;
   unsigned char cam_command[50];

    // Then start it up with current values.
    cam_command[0] = TTY3;
    cam_command[1] = ARGSTR;
    strncpy((char*)&cam_command[3], "TC\r", 4);
    cam_command[2] = strlen((char *)&cam_command[3]);
    cam_packet.Build(cam_command, (int)cam_command[2]+3);
    this->SendReceive(&cam_packet,false);
}


/****************************************************************
** Stop Tracking - This should be done before any new command
** are issued to the CMUcam.
****************************************************************/
void P2OS::CMUcamStopTracking(bool doLock)
{
  P2OSPacket cam_packet;
  unsigned char cam_command[50];

  // First we must STOP tracking.  Just send a return.
  cam_command[0] = TTY3;
  cam_command[1] = ARGSTR;
  strncpy((char*)&cam_command[3], "\r", 2);
  cam_command[2] = strlen((char *)&cam_command[3]);
  cam_packet.Build(cam_command, (int)cam_command[2]+3);
  this->SendReceive(&cam_packet,doLock);
}

/* toggle sonars on/off, according to val */
void
P2OS::ToggleSonarPower(unsigned char val)
{
  unsigned char command[4];
  P2OSPacket packet;

  command[0] = SONAR;
  command[1] = ARGINT;
  command[2] = val;
  command[3] = 0;
  packet.Build(command, 4);
  SendReceive(&packet,false);
}

/* toggle motors on/off, according to val */
void
P2OS::ToggleMotorPower(unsigned char val)
{
  unsigned char command[4];
  P2OSPacket packet;

  command[0] = ENABLE;
  command[1] = ARGINT;
  command[2] = val;
  command[3] = 0;
  packet.Build(command, 4);
  SendReceive(&packet,false);
}

/////////////////////////////////////////////////////
//  Actarray stuff
/////////////////////////////////////////////////////

// Ticks to degrees from the ARIA software
inline double P2OS::TicksToDegrees (int joint, unsigned char ticks)
{
  if ((joint < 0) || (joint >= sippacket->armNumJoints))
    return 0;

  double result;
  int pos = ticks - sippacket->armJoints[joint].centre;
  result = 90.0 / static_cast<double> (sippacket->armJoints[joint].ticksPer90);
  result = result * pos;
  if ((joint >= 0) && (joint <= 2))
    result = -result;

  return result;
}

// Degrees to ticks from the ARIA software
inline unsigned char P2OS::DegreesToTicks (int joint, double degrees)
{
  double val;

  if ((joint < 0) || (joint >= sippacket->armNumJoints))
    return 0;

  val = static_cast<double> (sippacket->armJoints[joint].ticksPer90) * degrees / 90.0;
  val = round (val);
  if ((joint >= 0) && (joint <= 2))
    val = -val;
  val += sippacket->armJoints[joint].centre;

  if (val < sippacket->armJoints[joint].min)
    return sippacket->armJoints[joint].min;
  else if (val > sippacket->armJoints[joint].max)
    return sippacket->armJoints[joint].max;
  else
    return static_cast<int> (round (val));
}

inline double P2OS::TicksToRadians (int joint, unsigned char ticks)
{
  double result = DTOR (TicksToDegrees (joint, ticks));
  return result;
}

inline unsigned char P2OS::RadiansToTicks (int joint, double rads)
{
  unsigned char result = static_cast<unsigned char> (DegreesToTicks (joint, RTOD (rads)));
  return result;
}

inline double P2OS::RadsPerSectoSecsPerTick (int joint, double speed)
{
  double degs = RTOD (speed);
  double ticksPerDeg = static_cast<double> (sippacket->armJoints[joint].ticksPer90) / 90.0f;
  double ticksPerSec = degs * ticksPerDeg;
  double secsPerTick = 1000.0f / ticksPerSec;

  if (secsPerTick > 127)
    return 127;
  else if (secsPerTick < 1)
    return 1;
  return secsPerTick;
}

inline double P2OS::SecsPerTicktoRadsPerSec (int joint, double msecs)
{
  double ticksPerSec = 1.0 / (static_cast<double> (msecs) / 1000.0);
  double ticksPerDeg = static_cast<double> (sippacket->armJoints[joint].ticksPer90) / 90.0f;
  double degs = ticksPerSec / ticksPerDeg;
  double rads = DTOR (degs);

  return rads;
}

void P2OS::ToggleActArrayPower (unsigned char value, bool lock)
{
  unsigned char command[4];
  P2OSPacket packet;

  command[0] = ARM_POWER;
  command[1] = ARGINT;
  command[2] = value;
  command[3] = 0;
  packet.Build (command, 4);
  SendReceive (&packet, lock);
}

void P2OS::SetActArrayJointSpeed (int joint, double speed)
{
  unsigned char command[4];
  P2OSPacket packet;

  command[0] = ARM_SPEED;
  command[1] = ARGINT;
  command[2] = static_cast<int> (round (speed));
  command[3] = joint;
  packet.Build (command, 4);
  SendReceive (&packet);
}

/////////////////////////////////////////////////////
//  End actarray stuff
/////////////////////////////////////////////////////


int
P2OS::ProcessMessage(QueuePointer & resp_queue,
                     player_msghdr * hdr,
                     void * data)
{
  // Check for capabilities requests first
  HANDLE_CAPABILITY_REQUEST (position_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
  HANDLE_CAPABILITY_REQUEST (actarray_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
  HANDLE_CAPABILITY_REQUEST (lift_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
  HANDLE_CAPABILITY_REQUEST (limb_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
  HANDLE_CAPABILITY_REQUEST (gripper_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
  HANDLE_CAPABILITY_REQUEST (armgripper_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
  // Position2d caps
  HANDLE_CAPABILITY_REQUEST (position_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL);
  // Act array caps
  HANDLE_CAPABILITY_REQUEST (actarray_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_POS);
  HANDLE_CAPABILITY_REQUEST (actarray_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_MULTI_POS);
  HANDLE_CAPABILITY_REQUEST (actarray_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_HOME);
  HANDLE_CAPABILITY_REQUEST (actarray_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_ACTARRAY_REQ_POWER);
  HANDLE_CAPABILITY_REQUEST (actarray_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_ACTARRAY_REQ_GET_GEOM);
  HANDLE_CAPABILITY_REQUEST (actarray_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_ACTARRAY_REQ_SPEED);
  // Lift caps
  HANDLE_CAPABILITY_REQUEST (lift_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_POS);
  HANDLE_CAPABILITY_REQUEST (lift_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_HOME);
  HANDLE_CAPABILITY_REQUEST (lift_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_ACTARRAY_REQ_GET_GEOM);
  // Limb caps
  HANDLE_CAPABILITY_REQUEST (limb_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_CMD_HOME);
  HANDLE_CAPABILITY_REQUEST (limb_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_CMD_STOP);
  HANDLE_CAPABILITY_REQUEST (limb_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_CMD_SETPOSE);
  HANDLE_CAPABILITY_REQUEST (limb_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_LIMB_REQ_POWER);
  HANDLE_CAPABILITY_REQUEST (limb_id, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_LIMB_REQ_GEOM);
  // Gripper caps
  HANDLE_CAPABILITY_REQUEST (gripper_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_OPEN);
  HANDLE_CAPABILITY_REQUEST (gripper_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_CLOSE);
  HANDLE_CAPABILITY_REQUEST (gripper_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_STOP);
  // Arm gripper caps
  HANDLE_CAPABILITY_REQUEST (armgripper_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_OPEN);
  HANDLE_CAPABILITY_REQUEST (armgripper_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_CLOSE);
  HANDLE_CAPABILITY_REQUEST (armgripper_id, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_STOP);

  // Process other messages
  if(hdr->type == PLAYER_MSGTYPE_REQ)
    return(this->HandleConfig(resp_queue,hdr,data));
  else if(hdr->type == PLAYER_MSGTYPE_CMD)
    return(this->HandleCommand(hdr,data));
  else
    return(-1);
}

int
P2OS::HandleConfig(QueuePointer & resp_queue,
                   player_msghdr * hdr,
                   void * data)
{
  int joint = 0;
  double newSpeed = 0.0f;

  // check for position config requests
  if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                           PLAYER_POSITION2D_REQ_SET_ODOM,
                           this->position_id))
  {
    if(hdr->size != sizeof(player_position2d_set_odom_req_t))
    {
      PLAYER_WARN("Arg to odometry set requests wrong size; ignoring");
      return(-1);
    }
    player_position2d_set_odom_req_t* set_odom_req =
            (player_position2d_set_odom_req_t*)data;

    this->sippacket->x_offset = ((int)rint(set_odom_req->pose.px*1e3)) -
            this->sippacket->xpos;
    this->sippacket->y_offset = ((int)rint(set_odom_req->pose.py*1e3)) -
            this->sippacket->ypos;
    this->sippacket->angle_offset = ((int)rint(RTOD(set_odom_req->pose.pa))) -
            this->sippacket->angle;

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_SET_ODOM);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_POSITION2D_REQ_MOTOR_POWER,
                                this->position_id))
  {
    /* motor state change request
     *   1 = enable motors
     *   0 = disable motors (default)
     */
    if(hdr->size != sizeof(player_position2d_power_config_t))
    {
      PLAYER_WARN("Arg to motor state change request wrong size; ignoring");
      return(-1);
    }
    player_position2d_power_config_t* power_config =
            (player_position2d_power_config_t*)data;
    this->ToggleMotorPower(power_config->state);

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_MOTOR_POWER);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_POSITION2D_REQ_RESET_ODOM,
                                this->position_id))
  {
    /* reset position to 0,0,0: no args */
    if(hdr->size != 0)
    {
      PLAYER_WARN("Arg to reset position request is wrong size; ignoring");
      return(-1);
    }
    ResetRawPositions();

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_RESET_ODOM);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_POSITION2D_REQ_GET_GEOM,
                                this->position_id))
  {
    /* Return the robot geometry. */
    if(hdr->size != 0)
    {
      PLAYER_WARN("Arg get robot geom is wrong size; ignoring");
      return(-1);
    }
    player_position2d_geom_t geom;
    // TODO: Figure out this rotation offset somehow; it's not
    //       given in the Saphira parameters.  For now, -0.1 is
    //       about right for a Pioneer 2DX.
    geom.pose.px = -0.1;
    geom.pose.py = 0.0;
    geom.pose.pyaw = 0.0;
    // get dimensions from the parameter table
    geom.size.sl = PlayerRobotParams[param_idx].RobotLength / 1e3;
    geom.size.sw = PlayerRobotParams[param_idx].RobotWidth / 1e3;

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_POSITION2D_REQ_GET_GEOM,
                  (void*)&geom, sizeof(geom), NULL);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_POSITION2D_REQ_VELOCITY_MODE,
                                this->position_id))
  {
    /* velocity control mode:
     *   0 = direct wheel velocity control (default)
     *   1 = separate translational and rotational control
     */
    if(hdr->size != sizeof(player_position2d_velocity_mode_config_t))
    {
      PLAYER_WARN("Arg to velocity control mode change request is wrong "
                  "size; ignoring");
      return(-1);
    }
    player_position2d_velocity_mode_config_t* velmode_config =
            (player_position2d_velocity_mode_config_t*)data;

    if(velmode_config->value)
      direct_wheel_vel_control = false;
    else
      direct_wheel_vel_control = true;

    this->Publish(this->position_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_VELOCITY_MODE);
    return(0);
  }
  // check for sonar config requests
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_SONAR_REQ_POWER,
                                this->sonar_id))
  {
    /*
     * 1 = enable sonars
     * 0 = disable sonar
     */
    if(hdr->size != sizeof(player_sonar_power_config_t))
    {
      PLAYER_WARN("Arg to sonar state change request wrong size; ignoring");
      return(-1);
    }
    player_sonar_power_config_t* sonar_config =
            (player_sonar_power_config_t*)data;
    this->ToggleSonarPower(sonar_config->state);

    this->Publish(this->sonar_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_SONAR_REQ_POWER);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_SONAR_REQ_GET_GEOM,
                                this->sonar_id))
  {
    /* Return the sonar geometry. */
    if(hdr->size != 0)
    {
      PLAYER_WARN("Arg get sonar geom is wrong size; ignoring");
      return(-1);
    }
    player_sonar_geom_t geom;
    geom.poses_count = PlayerRobotParams[param_idx].SonarNum;
    geom.poses = new player_pose3d_t[geom.poses_count];
    for(int i = 0; i < PlayerRobotParams[param_idx].SonarNum; i++)
    {
      sonar_pose_t pose = PlayerRobotParams[param_idx].sonar_pose[i];
      geom.poses[i].px = pose.x / 1e3;
      geom.poses[i].py = pose.y / 1e3;
      geom.poses[i].pyaw = DTOR(pose.th);
    }

    this->Publish(this->sonar_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_SONAR_REQ_GET_GEOM,
                  (void*)&geom);
    delete [] geom.poses;
    return(0);
  }
  // check for blobfinder requests
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                             PLAYER_BLOBFINDER_REQ_SET_COLOR,
                             this->blobfinder_id))
  {
    // Set the tracking color (RGB max/min values)

    if(hdr->size != sizeof(player_blobfinder_color_config_t))
    {
      puts("Arg to blobfinder color request wrong size; ignoring");
      return(-1);
    }
    player_blobfinder_color_config_t* color_config =
            (player_blobfinder_color_config_t*)data;

    CMUcamTrack(color_config->rmin,
                color_config->rmax,
                color_config->gmin,
                color_config->gmax,
                color_config->bmin,
                color_config->bmax);

    this->Publish(this->blobfinder_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_BLOBFINDER_REQ_SET_COLOR);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_BLOBFINDER_REQ_SET_IMAGER_PARAMS,
                                this->blobfinder_id))
  {
    // Set the imager control params
    if(hdr->size != sizeof(player_blobfinder_imager_config_t))
    {
      puts("Arg to blobfinder imager request wrong size; ignoring");
      return(-1);
    }
    player_blobfinder_imager_config_t* imager_config =
            (player_blobfinder_imager_config_t*)data;

    P2OSPacket cam_packet;
    unsigned char cam_command[50];
    int np;

    np=3;

    CMUcamStopTracking(); // Stop the current tracking.

    cam_command[0] = TTY3;
    cam_command[1] = ARGSTR;
    np += snprintf((char*)&cam_command[np], sizeof(cam_command) - np, "CR ");

    if (imager_config->brightness >= 0)
      np += snprintf((char*)&cam_command[np],sizeof(cam_command) - np, " 6 %d",
                    imager_config->brightness);

    if (imager_config->contrast >= 0)
      np += snprintf((char*)&cam_command[np],sizeof(cam_command) - np, " 5 %d",
                    imager_config->contrast);

    if (imager_config->autogain >= 0)
    {
      if (imager_config->autogain == 0)
        np += snprintf((char*)&cam_command[np],sizeof(cam_command) - np, " 19 32");
      else
        np += snprintf((char*)&cam_command[np], sizeof(cam_command) - np," 19 33");
    }

    if (imager_config->colormode >= 0)
    {
      if (imager_config->colormode == 3)
        np += snprintf((char*)&cam_command[np],sizeof(cam_command) - np, " 18 36");
      else if (imager_config->colormode == 2)
        np += snprintf((char*)&cam_command[np],sizeof(cam_command) - np, " 18 32");
      else if (imager_config->colormode == 1)
        np += snprintf((char*)&cam_command[np],sizeof(cam_command) - np, " 18 44");
      else
        np += snprintf((char*)&cam_command[np],sizeof(cam_command) - np, " 18 40");
    }

    if (np > 6)
    {
      snprintf((char*)&cam_command[np],sizeof(cam_command) - np, "\r");
      cam_command[2] = strlen((char *)&cam_command[3]);
      cam_packet.Build(cam_command, (int)cam_command[2]+3);
      SendReceive(&cam_packet);

      printf("Blobfinder imager parameters updated.\n");
      printf("       %s\n", &cam_command[3]);
    } else
      printf("Blobfinder imager parameters NOT updated.\n");

    CMUcamTrack();  // Restart tracking

    this->Publish(this->blobfinder_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_BLOBFINDER_REQ_SET_IMAGER_PARAMS);
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_ACTARRAY_REQ_POWER,this->actarray_id))
  {
    ToggleActArrayPower (((player_actarray_power_config_t*) data)->value);
    this->Publish(this->actarray_id, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_ACTARRAY_REQ_POWER);
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_ACTARRAY_REQ_GET_GEOM,this->actarray_id))
  {
    // First ask for an ARMINFOpac (because we need to get any updates to speed settings)
    P2OSPacket aaPacket;
    unsigned char aaCmd = ARM_INFO;
    aaPacket.Build (&aaCmd, 1);
    SendReceive (&aaPacket);

    player_actarray_geom_t aaGeom;
    player_actarray_actuatorgeom_t *actuators;

    aaGeom.actuators_count = sippacket->armNumJoints;
    actuators = new player_actarray_actuatorgeom_t[sippacket->armNumJoints];
    if (actuators == NULL)
    {
      PLAYER_ERROR ("Failed to allocate memory for actuator data");
      return -1;
    }
    aaGeom.actuators = actuators;

    for (int ii = 0; ii < sippacket->armNumJoints; ii++)
    {
      aaGeom.actuators[ii].type = PLAYER_ACTARRAY_TYPE_ROTARY;
      aaGeom.actuators[ii].length = aaLengths[ii];
      aaGeom.actuators[ii].orientation.proll = aaOrients[ii * 3];
      aaGeom.actuators[ii].orientation.ppitch = aaOrients[ii * 3 + 1];
      aaGeom.actuators[ii].orientation.pyaw = aaOrients[ii * 3 + 2];
      aaGeom.actuators[ii].axis.px = aaAxes[ii * 3];
      aaGeom.actuators[ii].axis.py = aaAxes[ii * 3 + 1];
      aaGeom.actuators[ii].axis.pz = aaAxes[ii * 3 + 2];
      aaGeom.actuators[ii].min = static_cast<float> (TicksToRadians (ii, sippacket->armJoints[ii].min));
      aaGeom.actuators[ii].centre = static_cast<float> (TicksToRadians (ii, sippacket->armJoints[ii].centre));
      aaGeom.actuators[ii].max = static_cast<float> (TicksToRadians (ii, sippacket->armJoints[ii].max));
      aaGeom.actuators[ii].home = static_cast<float> (TicksToRadians (ii, sippacket->armJoints[ii].home));
      aaGeom.actuators[ii].config_speed = static_cast<float> (SecsPerTicktoRadsPerSec (ii, sippacket->armJoints[ii].speed));
      aaGeom.actuators[ii].hasbrakes = 0;
    }

    aaGeom.base_pos.px = aaBasePos.px;
    aaGeom.base_pos.py = aaBasePos.py;
    aaGeom.base_pos.pz = aaBasePos.pz;
    aaGeom.base_orientation.proll = aaBaseOrient.proll;
    aaGeom.base_orientation.ppitch = aaBaseOrient.ppitch;
    aaGeom.base_orientation.pyaw = aaBaseOrient.pyaw;

    this->Publish(this->actarray_id, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_ACTARRAY_REQ_GET_GEOM, &aaGeom, sizeof (aaGeom), NULL);
    delete[] actuators;
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_ACTARRAY_REQ_SPEED,this->actarray_id))
  {
    joint = ((player_actarray_speed_config_t*) data)->joint + 1;
    newSpeed = RadsPerSectoSecsPerTick (joint, ((player_actarray_speed_config_t*) data)->speed);
    SetActArrayJointSpeed (joint, newSpeed);

    this->Publish(this->actarray_id, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_ACTARRAY_REQ_SPEED);
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_LIMB_REQ_POWER,this->limb_id))
  {
    ToggleActArrayPower (((player_actarray_power_config_t*) data)->value);
    this->Publish(this->actarray_id, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LIMB_REQ_POWER);
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_LIMB_REQ_BRAKES,this->limb_id))
  {
    // We don't have any brakes
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_LIMB_REQ_GEOM,this->limb_id))
  {
    player_limb_geom_req_t limbGeom;

    limbGeom.basePos.px = armOffsetX;
    limbGeom.basePos.py = armOffsetY;
    limbGeom.basePos.pz = armOffsetZ;

    this->Publish(this->limb_id, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LIMB_REQ_GEOM, &limbGeom, sizeof (limbGeom), NULL);
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_LIMB_REQ_SPEED,this->limb_id))
  {
    // FIXME - need to figure out what sort of speed support we should provide through the IK interface
    // Would need some form of motion control
    // For now, just set all joint speeds - take the value as being rad/s instead of m/s
    float speed = ((player_limb_speed_req_t*) data)->speed;
    for (int ii = 1; ii < 6; ii++)
    {
      newSpeed = RadsPerSectoSecsPerTick (ii, speed);
      SetActArrayJointSpeed (ii, newSpeed);
    }

    this->Publish(this->limb_id, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_LIMB_REQ_SPEED);
    return 0;
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_BUMPER_REQ_GET_GEOM, this->bumper_id))
  {
    /* Return the bumper geometry. */
    if(hdr->size != 0)
    {
      PLAYER_WARN("Arg get bumper geom is wrong size; ignoring");
      return(-1);
    }
    player_bumper_geom_t geom;
    geom.bumper_def_count = PlayerRobotParams[param_idx].NumFrontBumpers + PlayerRobotParams[param_idx].NumRearBumpers;
    geom.bumper_def = new player_bumper_define_t[geom.bumper_def_count];
    for(unsigned int ii = 0; ii < geom.bumper_def_count; ii++)
    {
      bumper_def_t def = PlayerRobotParams[param_idx].bumper_geom[ii];
      geom.bumper_def[ii].pose.px = def.x;
      geom.bumper_def[ii].pose.py = def.y;
      geom.bumper_def[ii].pose.pyaw = DTOR(def.th);
      geom.bumper_def[ii].length = def.length;
      geom.bumper_def[ii].radius = def.radius;
    }

    this->Publish(this->bumper_id, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_BUMPER_REQ_GET_GEOM,
                  (void*)&geom);
    delete [] geom.bumper_def;
    return(0);
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_ACTARRAY_REQ_GET_GEOM,this->lift_id))
  {
    player_actarray_geom_t aaGeom;
    player_actarray_actuatorgeom_t actuator;
    aaGeom.actuators = &actuator;

    aaGeom.actuators_count = 1;
    memset (aaGeom.actuators, 0, sizeof (player_actarray_actuator_t));

    aaGeom.actuators[0].type = PLAYER_ACTARRAY_TYPE_LINEAR;
    aaGeom.actuators[0].min = 0.0f;
    aaGeom.actuators[0].centre = 0.5f;
    aaGeom.actuators[0].max = 1.0f;
    aaGeom.actuators[0].home = 1.0f;
    aaGeom.actuators[0].config_speed = 0.02f; // 2cm/s, according to the manual
    aaGeom.actuators[0].hasbrakes = 0;

    this->Publish(this->lift_id, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_ACTARRAY_REQ_GET_GEOM, &aaGeom, sizeof (aaGeom), NULL);
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_GRIPPER_REQ_GET_GEOM,this->gripper_id))
  {
    player_gripper_geom_t geom;
    memset (&geom, 0, sizeof (player_gripper_geom_t));

    geom.pose = gripperPose;
    geom.outer_size = gripperOuterSize;
    geom.inner_size = gripperInnerSize;
    geom.num_beams = 2;
    geom.capacity = 0;

    this->Publish(this->gripper_id, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_GRIPPER_REQ_GET_GEOM, &geom, sizeof (geom), NULL);
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,PLAYER_GRIPPER_REQ_GET_GEOM,this->armgripper_id))
  {
    player_gripper_geom_t geom;
    memset (&geom, 0, sizeof (player_gripper_geom_t));

    memset (&(geom.pose), 0, sizeof (player_pose3d_t));  // Hard to know since it's on the end of the arm
    geom.outer_size = armGripperOuterSize;
    geom.inner_size = armGripperInnerSize;
    geom.num_beams = 0;
    geom.capacity = 0;

    this->Publish(this->armgripper_id, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_GRIPPER_REQ_GET_GEOM, &geom, sizeof (geom), NULL);
    return 0;
  }
  // PTZ Stuff now.
  else if (Message::MatchMessage(hdr,
              PLAYER_MSGTYPE_REQ,
              PLAYER_PTZ_REQ_GENERIC, device_addr))
  {
      assert(hdr->size == sizeof(player_ptz_req_generic_t));

      player_ptz_req_generic_t *cfg = (player_ptz_req_generic_t *)data;

      // check whether command or inquiry...
      if (cfg->config[0] == 0x01)
      {
          if (SendCommand((uint8_t *)cfg->config, cfg->config_count) < 0)
              Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);
          else
              Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, hdr->subtype);
          return 0;
      }
      else
      {
          // this is an inquiry, so we have to send data back
          cfg->config_count = SendRequest((uint8_t*)cfg->config,
                                          cfg->config_count,
                                          (uint8_t*)cfg->config);
          Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, hdr->subtype);
      }
      return 0;
  }
  else
  {
    PLAYER_WARN("unknown config request to p2os driver");
    return(-1);
  }

  return 0;
}

void P2OS::SendPulse (void)
{
  unsigned char command;
  P2OSPacket packet;

  command = PULSE;
  packet.Build(&command, 1);
  SendReceive(&packet);
}

///////////////////////////////////////////////////////////////////////////////
//  Command handling
///////////////////////////////////////////////////////////////////////////////

void
P2OS::HandlePositionCommand(player_position2d_cmd_vel_t position_cmd)
{
  int speedDemand, turnRateDemand;
  double leftvel, rightvel;
  double rotational_term;
  unsigned short absspeedDemand, absturnRateDemand;
  unsigned char motorcommand[4];
  P2OSPacket motorpacket;

  speedDemand = (int)rint(position_cmd.vel.px * 1e3);
  turnRateDemand = (int)rint(RTOD(position_cmd.vel.pa));

  if(this->direct_wheel_vel_control)
  {
    // convert xspeed and yawspeed into wheelspeeds
    rotational_term = (M_PI/180.0) * turnRateDemand /
            PlayerRobotParams[param_idx].DiffConvFactor;
    leftvel = (speedDemand - rotational_term);
    rightvel = (speedDemand + rotational_term);

    // Apply wheel speed bounds
    if(fabs(leftvel) > this->motor_max_speed)
    {
      if(leftvel > 0)
      {
        rightvel *= this->motor_max_speed/leftvel;
        leftvel = this->motor_max_speed;
        puts("Left wheel velocity threshholded!");
      }
      else
      {
        rightvel *= -this->motor_max_speed/leftvel;
        leftvel = -this->motor_max_speed;
      }
    }
    if(fabs(rightvel) > this->motor_max_speed)
    {
      if(rightvel > 0)
      {
        leftvel *= this->motor_max_speed/rightvel;
        rightvel = this->motor_max_speed;
        puts("Right wheel velocity threshholded!");
      }
      else
      {
        leftvel *= -this->motor_max_speed/rightvel;
        rightvel = -this->motor_max_speed;
      }
    }

    // Apply control band bounds
    if(this->use_vel_band)
    {
      // This band prevents the wheels from turning in opposite
      // directions
      if (leftvel * rightvel < 0)
      {
        if (leftvel + rightvel >= 0)
        {
          if (leftvel < 0)
            leftvel = 0;
          if (rightvel < 0)
            rightvel = 0;
        }
        else
        {
          if (leftvel > 0)
            leftvel = 0;
          if (rightvel > 0)
            rightvel = 0;
        }
      }
    }

    // Apply byte range bounds
    if (leftvel / PlayerRobotParams[param_idx].Vel2Divisor > 126)
      leftvel = 126 * PlayerRobotParams[param_idx].Vel2Divisor;
    if (leftvel / PlayerRobotParams[param_idx].Vel2Divisor < -126)
      leftvel = -126 * PlayerRobotParams[param_idx].Vel2Divisor;
    if (rightvel / PlayerRobotParams[param_idx].Vel2Divisor > 126)
      rightvel = 126 * PlayerRobotParams[param_idx].Vel2Divisor;
    if (rightvel / PlayerRobotParams[param_idx].Vel2Divisor < -126)
      rightvel = -126 * PlayerRobotParams[param_idx].Vel2Divisor;

    // send the speed command
    motorcommand[0] = VEL2;
    motorcommand[1] = ARGINT;
    motorcommand[2] = (char)(rightvel /
                             PlayerRobotParams[param_idx].Vel2Divisor);
    motorcommand[3] = (char)(leftvel /
                             PlayerRobotParams[param_idx].Vel2Divisor);

    motorpacket.Build(motorcommand, 4);
    this->SendReceive(&motorpacket);
  }
  else
  {
    // do separate trans and rot vels

    motorcommand[0] = VEL;
    if(speedDemand >= 0)
      motorcommand[1] = ARGINT;
    else
      motorcommand[1] = ARGNINT;

    absspeedDemand = (unsigned short)abs(speedDemand);
    if(absspeedDemand < this->motor_max_speed)
    {
      motorcommand[2] = absspeedDemand & 0x00FF;
      motorcommand[3] = (absspeedDemand & 0xFF00) >> 8;
    }
    else
    {
      puts("Speed demand threshholded!");
      motorcommand[2] = this->motor_max_speed & 0x00FF;
      motorcommand[3] = (this->motor_max_speed & 0xFF00) >> 8;
    }
    motorpacket.Build(motorcommand, 4);
    this->SendReceive(&motorpacket);

    motorcommand[0] = RVEL;
    if(turnRateDemand >= 0)
      motorcommand[1] = ARGINT;
    else
      motorcommand[1] = ARGNINT;

    absturnRateDemand = (unsigned short)abs(turnRateDemand);
    if(absturnRateDemand < this->motor_max_turnspeed)
    {
      motorcommand[2] = absturnRateDemand & 0x00FF;
      motorcommand[3] = (absturnRateDemand & 0xFF00) >> 8;
    }
    else
    {
      puts("Turn rate demand threshholded!");
      motorcommand[2] = this->motor_max_turnspeed & 0x00FF;
      motorcommand[3] = (this->motor_max_turnspeed & 0xFF00) >> 8;
    }

    motorpacket.Build(motorcommand, 4);
    this->SendReceive(&motorpacket);

  }
}

void
P2OS::HandleAudioCommand(player_audio_sample_item_t audio_cmd)
{
  unsigned char soundcommand[4];
  P2OSPacket soundpacket;
  unsigned short soundindex;

  soundindex = audio_cmd.index;

  if(!this->sent_audio_cmd || (soundindex != this->last_audio_cmd.index))
  {
    soundcommand[0] = SOUND;
    soundcommand[1] = ARGINT;
    soundcommand[2] = soundindex & 0x00FF;
    soundcommand[3] = (soundindex & 0xFF00) >> 8;
    soundpacket.Build(soundcommand,4);
    SendReceive(&soundpacket);
    fflush(stdout);

    this->last_audio_cmd.index = soundindex;
  }
}

///////////////////////////////////////////////////////////////////////////////
//  Arm actuator array commands

void P2OS::HandleActArrayPosCmd (player_actarray_position_cmd_t cmd)
{
  unsigned char command[4];
  P2OSPacket packet;

  if (!(lastActArrayCmd == PLAYER_ACTARRAY_CMD_POS) || ((lastActArrayCmd == PLAYER_ACTARRAY_CMD_POS) &&
       (cmd.joint != lastActArrayPosCmd.joint || cmd.position != lastActArrayPosCmd.position)))
  {
    command[0] = ARM_POS;
    command[1] = ARGINT;
    command[2] = RadiansToTicks (cmd.joint, cmd.position);
    command[3] = static_cast<unsigned char> (cmd.joint) + 1;
    packet.Build(command, 4);
    SendReceive(&packet);
    sippacket->armJointTargetPos[static_cast<unsigned char> (cmd.joint)] = command[2];
  }
}

void P2OS::HandleActArrayHomeCmd (player_actarray_home_cmd_t cmd)
{
  unsigned char command[4];
  P2OSPacket packet;

  if ((lastActArrayCmd == PLAYER_ACTARRAY_CMD_POS) || (!(lastActArrayCmd == PLAYER_ACTARRAY_CMD_POS) &&
       (cmd.joint != lastActArrayHomeCmd.joint)))
  {
    command[0] = ARM_HOME;
    command[1] = ARGINT;
    command[2] = (cmd.joint == -1) ? 7 : (static_cast<unsigned char> (cmd.joint) + 1);
    command[3] = 0;
    packet.Build(command, 4);
    SendReceive(&packet);
  }
}

int P2OS::HandleActArrayCommand (player_msghdr * hdr, void * data)
{
  if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_POS, this->actarray_id))
  {
    player_actarray_position_cmd_t cmd;
    cmd = *(player_actarray_position_cmd_t*) data;
    this->HandleActArrayPosCmd (cmd);
    lastActArrayCmd = PLAYER_ACTARRAY_CMD_POS;
    return 0;
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_HOME, this->actarray_id))
  {
    player_actarray_home_cmd_t cmd;
    cmd = *(player_actarray_home_cmd_t*) data;
    this->HandleActArrayHomeCmd (cmd);
    lastActArrayCmd = PLAYER_ACTARRAY_CMD_HOME;
    return 0;
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_CMD_MULTI_POS, this->actarray_id))
  {
    player_actarray_multi_position_cmd_t cmd = *(player_actarray_multi_position_cmd_t*) data;
    player_actarray_position_cmd_t singleCmd;
    for (unsigned int ii = 0; ii < cmd.positions_count && ii < 6; ii++)
    {
      singleCmd.joint = ii;
      singleCmd.position = cmd.positions[ii];
      this->HandleActArrayPosCmd (singleCmd);
    }
    lastActArrayCmd = PLAYER_ACTARRAY_CMD_MULTI_POS;
  }

  return -1;
}

///////////////////////////////////////////////////////////////////////////////
//  Limb commands

void P2OS::HandleLimbHomeCmd (void)
{
  unsigned char command[4];
  P2OSPacket packet;

  command[0] = ARM_HOME;
  command[1] = ARGINT;
  command[2] = 7;
  command[3] = 0;
  packet.Build(command, 4);
  SendReceive(&packet);
}

void P2OS::HandleLimbStopCmd (void)
{
  unsigned char command[4];
  P2OSPacket packet;

  command[0] = ARM_STOP;
  command[1] = ARGINT;

  for (int ii = 1; ii < 5; ii++)
  {
    command[2] = ii;
    command[3] = 0;
    packet.Build (command, 4);
    SendReceive (&packet);
  }
}

void P2OS::HandleLimbSetPoseCmd (player_limb_setpose_cmd_t cmd)
{
  unsigned char command[4];
  P2OSPacket packet;
  EndEffector pose;

//   printf ("Moving limb to pose (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n", cmd.position.px, cmd.position.py, cmd.position.pz, cmd.approach.px, cmd.approach.py, cmd.approach.pz, cmd.orientation.px, cmd.orientation.py, cmd.orientation.pz);

  pose.p.x = cmd.position.px - armOffsetX;
  pose.p.y = cmd.position.py - armOffsetY;
  pose.p.z = cmd.position.pz - armOffsetZ;
  pose.a.x = cmd.approach.px; pose.a.y = cmd.approach.py; pose.a.z = cmd.approach.pz;
  pose.o.x = cmd.orientation.px; pose.o.y = cmd.orientation.py; pose.o.z = cmd.orientation.pz;
  pose.a = kineCalc->Normalise (pose.a);
  pose.o = kineCalc->Normalise (pose.o);
  pose.n = kineCalc->CalculateN (pose);

//   printf ("Pose = %f, %f, %f\t", pose.p.x, pose.p.y, pose.p.z);
//   printf ("Approach = %f, %f, %f\n", pose.a.x, pose.a.y, pose.a.z);
//   printf ("Orientation = %f, %f, %f\t", pose.o.x, pose.o.y, pose.o.z);
//   printf ("Normal = %f, %f, %f\n", pose.n.x, pose.n.y, pose.n.z);

  if (!kineCalc->CalculateIK (pose))
  {
    limb_data.state = PLAYER_LIMB_STATE_OOR;
    return;
  }

  command[0] = ARM_POS;
  command[1] = ARGINT;

  for (int ii = 0; ii < 5; ii++)
  {
    command[2] = RadiansToTicks (ii, kineCalc->GetTheta (ii));
    command[3] = ii + 1;
    packet.Build (command, 4);
    SendReceive (&packet);
//     printf ("Sent joint %d to %f (%d)\n", ii, kineCalc->GetTheta (ii), command[2]);
  }

  limb_data.state = PLAYER_LIMB_STATE_MOVING;
}

// NOTE: Not functional
void P2OS::HandleLimbSetPositionCmd (player_limb_setposition_cmd_t cmd)
{
  EndEffector pose;
  unsigned char command[4];
  P2OSPacket packet;

  pose.p.x = cmd.position.px - armOffsetX;
  pose.p.y = -(cmd.position.py - armOffsetY);
  pose.p.z = cmd.position.pz - armOffsetZ;

  // Use the pose info from the last reported arm position (cause the IK calculator doesn't
  // calculate without full pose data)
  pose.o = kineCalc->GetO ();
  pose.a = kineCalc->GetA ();
  pose.n = kineCalc->GetN ();

  if (!kineCalc->CalculateIK (pose))
  {
    limb_data.state = PLAYER_LIMB_STATE_OOR;
    return;
  }

  command[0] = ARM_POS;
  command[1] = ARGINT;

  for (int ii = 0; ii < 5; ii++)
  {
    command[2] = RadiansToTicks (ii, kineCalc->GetTheta (ii));
    command[3] = ii + 1;
    packet.Build (command, 4);
    SendReceive (&packet);
  }

  limb_data.state = PLAYER_LIMB_STATE_MOVING;
}

// NOTE: Not functional
void P2OS::HandleLimbVecMoveCmd (player_limb_vecmove_cmd_t cmd)
{
  EndEffector pose;
  unsigned char command[4];
  P2OSPacket packet;

  // To do a vector move, calculate a new position that is offset from the current
  // by the length of the desired move in the direction of the desired vector.
  // Since we lack constant motion control, but are moving over a small range, this
  // should hopefully give an accurate representation of a vector move.
  // UPDATE: Turns out it doesn't work. Hopefully I'll have time to rewrite
  // this driver in the future so that it can support proper constant motion
  // control without being an unmaintainable mess.
  // As such, this vector move isn't actually a vector move as it is intended in
  // the interface. I'll leave it in because it could be useful as an "offset"
  // command, but this should be noted in the docs for the driver.

  pose.p = kineCalc->GetP ();
  pose.o = kineCalc->GetO ();
  pose.a = kineCalc->GetA ();
  pose.n = kineCalc->GetN ();

  KineVector offset;
  offset.x = cmd.direction.px;   offset.y = -cmd.direction.py;   offset.z = cmd.direction.pz;
  offset = kineCalc->Normalise (offset);
  offset.x *= cmd.length;
  offset.y *= cmd.length;
  offset.z *= cmd.length;

  pose.p.x += offset.x;
  pose.p.y += offset.y;
  pose.p.z += offset.z;

  if (!kineCalc->CalculateIK (pose))
  {
    limb_data.state = PLAYER_LIMB_STATE_OOR;
    return;
  }

  command[0] = ARM_POS;
  command[1] = ARGINT;

  for (int ii = 0; ii < 5; ii++)
  {
    command[2] = RadiansToTicks (ii, kineCalc->GetTheta (ii));
    command[3] = ii + 1;
    packet.Build (command, 4);
    SendReceive (&packet);
  }

  limb_data.state = PLAYER_LIMB_STATE_MOVING;
}

int P2OS::HandleLimbCommand (player_msghdr *hdr, void *data)
{
  if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_CMD,PLAYER_LIMB_CMD_HOME,this->limb_id))
  {
    this->HandleLimbHomeCmd ();
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_CMD,PLAYER_LIMB_CMD_STOP,this->limb_id))
  {
    this->HandleLimbStopCmd ();
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_CMD,PLAYER_LIMB_CMD_SETPOSE,this->limb_id))
  {
    player_limb_setpose_cmd_t cmd;
    cmd = *(player_limb_setpose_cmd_t*) data;
    this->HandleLimbSetPoseCmd (cmd);
    return 0;
  }
  return -1;
}


///////////////////////////////////////////////////////////////////////////////
//  Lift commands

int P2OS::HandleLiftCommand (player_msghdr *hdr, void *data)
{
  unsigned char command[4];
  P2OSPacket packet;

  if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_CMD,PLAYER_ACTARRAY_CMD_POS,this->lift_id))
  {
    player_actarray_position_cmd_t cmd;
    cmd = *(player_actarray_position_cmd_t*) data;

    // If not the first joint, return error
    if (cmd.joint > 0)
      return -1;

    if (lastLiftCmd == PLAYER_ACTARRAY_CMD_POS && lastLiftPosCmd.position == cmd.position)
      return 0;

    command[0] = GRIPPER;
    command[1] = ARGINT;

    // If the position is 1 or 0, then it's easy: just use LIFTup or LIFTdown
    if (cmd.position <= 0.0)
    {
     command[2] = LIFTdown;
     command[3] = 0;
      packet.Build (command, 4);
      SendReceive (&packet);
    }
    else if (cmd.position >= 1.0)
    {
      command[3] = LIFTup;
     command[3] = 0;
      packet.Build (command, 4);
      SendReceive (&packet);
    }
    else
    {
      // Lift position is a range from 0 to 1. 0 corresponds to down, 1 corresponds to up.
      // Setting positions in between is done using the carry time.
      // According to the manual, the lift can move 7cm at a rate of 2cm/s (in ideal conditions).
      // So to figure out the lift time for a given position, we consider an AA position of 1 to
      // correspond to a lift position of 7cm and 0 to 0cm. Given a speed of 2cm/s, this means
      // the lift takes 3.5s to move over its full range. So an AA position is converted to a
      // time by 3.5 * cmd.pos. For example, 0.5 is 3.5 * 0.5 = 1.75s travel time.
      // We then use the LIFTcarry command to set this travel time. LIFTcarry is specified as
      // an integer, with each step equal to 20ms of travel time. So the LIFTcarry value
      // becomes travel time / 0.02.
      // It is important to remember that the LIFTcarry is (if my reading of the manul is correct)
      // an offset command, not absolute position command. So we have to track the last commanded

      // position of the lift and work from that to get the correct travel time (and direction).

      double offset = cmd.position - lastLiftPosCmd.position;
      double travelTime = offset * 3.5f;
      short liftCarryVal = static_cast<short> (travelTime / 0.02f);

      // Send the LIFTcarry command
     command[2] = LIFTcarry;
     command[3] = 0;
      packet.Build (command, 4);
      SendReceive (&packet);

      // Followed by the carry time
      command[0] = GRIPPERVAL;
      command[2] = liftCarryVal & 0x00FF;
      command[3] = (liftCarryVal & 0xFF00) >> 8;
      packet.Build (command, 4);
      SendReceive (&packet);
    }

    lastLiftCmd = PLAYER_ACTARRAY_CMD_POS;
    lastLiftPosCmd = cmd;
    sippacket->lastLiftPos = cmd.position;
    return 0;
  }
  else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_CMD,PLAYER_ACTARRAY_CMD_HOME,this->lift_id))
  {
    if (lastLiftCmd == PLAYER_ACTARRAY_CMD_HOME)
      return 0;

    // For home, just send the lift to up position
    command[0] = GRIPPER;
    command[1] = ARGINT;
   command[2] = LIFTup;
   command[3] = 0;
    packet.Build (command, 4);
    SendReceive (&packet);
    lastLiftCmd = PLAYER_ACTARRAY_CMD_HOME;
    lastLiftPosCmd.position = 1.0f;
    return 0;
  }
  return -1;
}

///////////////////////////////////////////////////////////////////////////////
//  Gripper commands

void P2OS::OpenGripper (void)
{
  unsigned char cmd[4];
  P2OSPacket packet;

 /*
  if (sentGripperCmd && lastGripperCmd == PLAYER_GRIPPER_CMD_OPEN)
    return;
 */

  cmd[0] = GRIPPER;
  cmd[1] = ARGINT;
 cmd[2] = GRIPopen;  // low bits of unsigned 16bit int
 cmd[3] = 0;         // high bits of unsigned 16bit int
  packet.Build (cmd, 4);
  SendReceive (&packet);

  sentGripperCmd = true;
  lastGripperCmd = PLAYER_GRIPPER_CMD_OPEN;
}

void P2OS::CloseGripper (void)
{
  unsigned char cmd[4];
  P2OSPacket packet;

 /*
  if (sentGripperCmd && lastGripperCmd == PLAYER_GRIPPER_CMD_CLOSE)
    return;
 */

  cmd[0] = GRIPPER;
  cmd[1] = ARGINT;
 cmd[2] = GRIPclose; // low bits of unsigned 16 bit int
 cmd[3] = 0;         // high bits of unsigned 16bit int
  packet.Build (cmd, 4);
  SendReceive (&packet);

  sentGripperCmd = true;
  lastGripperCmd = PLAYER_GRIPPER_CMD_CLOSE;
}

void P2OS::StopGripper (void)
{
  unsigned char cmd[4];
  P2OSPacket packet;

  if (sentGripperCmd && lastGripperCmd == PLAYER_GRIPPER_CMD_STOP)
    return;

  cmd[0] = GRIPPER;
  cmd[1] = ARGINT;
 cmd[2] = GRIPstop;
 cmd[3] = 0;
  packet.Build (cmd, 4);
  SendReceive (&packet);

  sentGripperCmd = true;
  lastGripperCmd = PLAYER_GRIPPER_CMD_STOP;
}

int P2OS::HandleGripperCommand (player_msghdr * hdr, void * data)
{
  if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_OPEN, this->gripper_id))
  {
    OpenGripper ();
    return 0;
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_CLOSE, this->gripper_id))
  {
    CloseGripper ();
    return 0;
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_STOP, this->gripper_id))
  {
    StopGripper ();
    return 0;
  }
  return -1;
}

///////////////////////////////////////////////////////////////////////////////
//  Arm gripper commands

void P2OS::OpenArmGripper (void)
{
  unsigned char command[4];
  P2OSPacket packet;

  if (sentArmGripperCmd && lastArmGripperCmd == PLAYER_GRIPPER_CMD_OPEN)
    return;

  command[0] = ARM_POS;
  command[1] = ARGINT;
  command[2] = sippacket->armJoints[5].max;
  command[3] = 6;
  packet.Build(command, 4);
  SendReceive(&packet);

  sippacket->armJointTargetPos[5] = command[2];
  sentArmGripperCmd = true;
  lastArmGripperCmd = PLAYER_GRIPPER_CMD_OPEN;
}

void P2OS::CloseArmGripper (void)
{
  unsigned char command[4];
  P2OSPacket packet;

  if (sentArmGripperCmd && lastArmGripperCmd == PLAYER_GRIPPER_CMD_CLOSE)
    return;

  command[0] = ARM_POS;
  command[1] = ARGINT;
  command[2] = sippacket->armJoints[5].min;
  command[3] = 6;
  packet.Build(command, 4);
  SendReceive(&packet);

  sippacket->armJointTargetPos[5] = command[2];
  sentArmGripperCmd = true;
  lastArmGripperCmd = PLAYER_GRIPPER_CMD_CLOSE;
}

void P2OS::StopArmGripper (void)
{
  unsigned char command[4];
  P2OSPacket packet;

  if (sentArmGripperCmd && lastArmGripperCmd == PLAYER_GRIPPER_CMD_STOP)
    return;

  command[0] = ARM_STOP;
  command[1] = ARGINT;
  command[2] = 6;
  command[3] = 0;
  packet.Build(command, 4);
  SendReceive(&packet);

  sentArmGripperCmd = true;
  lastArmGripperCmd = PLAYER_GRIPPER_CMD_STOP;
}

int P2OS::HandleArmGripperCommand (player_msghdr *hdr, void *data)
{
  if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_OPEN, this->armgripper_id))
  {
    OpenArmGripper ();
    return 0;
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_CLOSE, this->armgripper_id))
  {
    CloseArmGripper ();
    return 0;
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_STOP, this->armgripper_id))
  {
    StopArmGripper ();
    return 0;
  }
  return -1;
}

///////////////////////////////////////////////////////////////////////////////

int
P2OS::HandleCommand(player_msghdr * hdr, void* data)
{
  int retVal = -1;
  struct timeval timeVal;

  if(Message::MatchMessage(hdr,
                           PLAYER_MSGTYPE_CMD,
                           PLAYER_POSITION2D_CMD_VEL,
                           this->position_id))
  {
    // get and send the latest motor command
    player_position2d_cmd_vel_t position_cmd;
    position_cmd = *(player_position2d_cmd_vel_t*)data;
    this->HandlePositionCommand(position_cmd);
    retVal = 0;
  }
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_CMD_SAMPLE_PLAY, this->audio_id))
  {
    // get and send the latest audio command, if it's new
    player_audio_sample_item_t audio_cmd;
    audio_cmd = *(player_audio_sample_item_t*)data;
    this->HandleAudioCommand(audio_cmd);
    retVal = 0;
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, -1, actarray_id))
  {
    retVal = HandleActArrayCommand (hdr, data);
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, -1, limb_id))
  {
    retVal = HandleLimbCommand (hdr, data);
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, -1, lift_id))
  {
    retVal = HandleLiftCommand (hdr, data);
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, -1, gripper_id))
  {
    retVal = HandleGripperCommand (hdr, data);
  }
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, -1, gripper_id))
  {
    retVal = HandleGripperCommand(hdr, data);
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, -1, armgripper_id))
  {
    retVal = HandleArmGripperCommand (hdr, data);
  }

  // Update the time of last pulse/command on successful handling of commands
  if (retVal == 0 && pulse != -1)
  {
    gettimeofday (&timeVal, NULL);
    lastPulseTime = static_cast<double> (timeVal.tv_sec) + (static_cast<double> (timeVal.tv_usec) / 1e6);
  }
  return retVal;
}


// I'm going to put my PTZ stuff down here for now.
/************************************************************************/

int
P2OS::SetupPtz()
{
    int err;
    int error_code;
    int pan, tilt;
    fprintf(stderr, "Setting up the Canon PTZ Camera.\n");

    //err = setPower(0);

    do {
    //fprintf(stderr, "\nPowering off/on the camera!!!!!!!!!!!!!!\n");
    fprintf(stderr, "\nPowering On the Camera.\n");
    err = setPower(1);
    while (error_code == CAM_ERROR_BUSY)
    {
        fprintf(stdout, "power on busy: %x\n", error_code);
        err = setPower(1);
    }
    if ((err != 0) &&
            (error_code != CAM_ERROR_NONE) && (error_code != CAM_ERROR_MODE))
    {
        printf("Could not set power on: %x\n", error_code);
        setPower(0);
        //close(ptz_fd);
        return -1;
    }

    /* Set Host Mode Control */
    fprintf(stderr, "\nSeting Host Control mode.\n");
    err = setControlMode();
    while (error_code == CAM_ERROR_BUSY)
    {
        printf("control mode busy: %x\n", error_code);
        err = setControlMode();
    }
    if (err)
    {
        printf("Could not set control mode\n");
        setPower(0);
        //close(ptz_fd);
        return -1;
    }


    /* Send Init Command */
    fprintf(stderr, "\nSendInit()\n");
    err = sendInit();
    while (error_code == CAM_ERROR_BUSY)
    {
        fprintf(stdout, "sendInit busy: %x\n", error_code);
        err = sendInit();
    }
    if ((err != 0) && (error_code != CAM_ERROR_NONE) && (error_code != CAM_ERROR_MODE))
    {
        printf("Could not sendInit off: %x\n", error_code);
        setPower(0);
        return -1;
    }
    }while (error_code == CAM_ERROR_MODE);

    /* Turn on Notify Command */
    /*
    fprintf(stderr, "Setting Notify Command on\n");
    err = setNotifyCommand();
    while (error_code == CAM_ERROR_BUSY)
    {
        printf("notify command busy: %x\n", error_code);
        err = setNotifyCommand();
    }
    if (err)
    {
        printf("Could not set notify command\n");
        setPower(0);
        //close(ptz_fd);
        return -1;
    }
    */

    /* Now Turn Power on. */
    /*
    fprintf(stderr, "Powering On the Camera.\n");
    err = setPower(1);
    while (error_code == CAM_ERROR_BUSY)
    {
        fprintf(stdout, "power on busy: %x\n", error_code);
        err = setPower(1);
    }
    if ((err != 0) &&
            (error_code != CAM_ERROR_NONE) && (error_code != CAM_ERROR_MODE))
    {
        printf("Could not set power on: %x\n", error_code);
        setPower(0);
        return -1;
    }
    */
    fprintf(stderr, "\nSetting the default tilt range.\n");
    err = setDefaultTiltRange();
    while (error_code == CAM_ERROR_BUSY)
    {
        printf("control mode busy: %x\n", error_code);
        err = setDefaultTiltRange();
    }
    if (err)
    {
        printf("Could not set default tilt range\n");
        setPower(0);
        //close(ptz_fd);
        return -1;
    }

    /* try to get current state, just to make sure we actually have a camera */
    fprintf(stderr, "\nGetting the Abs Pan Tilt\n");
    err = GetAbsPanTilt(&pan,&tilt);
    if (err)
    {
        printf("Couldn't connect to PTZ device most likely because the camera\n"
                "is not connected or is connected not to AUX1: %x\n",
                error_code);
        setPower(0);
        //close(ptz_fd);
        //ptz_fd = -1;
        return(-1);
    }
    fprintf(stderr, "getAbsPantilt: %d %d\n", pan, tilt);

    // Get the Zoom Range. 0 to what
    fprintf(stderr, "Getting Max Zoom Range.\n");
    err = GetMaxZoom(&maxzoom);
    if (err)
    {
        fprintf(stderr, "Couldn't get max zoom range.\n");
        setPower(0);
        //close(ptz_fd);
        //ptz_fd = -1;
        return(-1);
    }
    fprintf(stderr, "maxzoom value = %i \n", maxzoom);
    fprintf(stderr, "Done Initializing the PTZ Camera.\n");
    return 0;
}


int
P2OS::SendCommand(unsigned char *str, int len)
{
  int err = 0;
  P2OSPacket ptz_packet;
  P2OSPacket request_pkt;
  unsigned char request[4];

  // Zero out the Receive Buffer
  request[0] = GETAUX;
  request[1] = ARGINT;
  request[2] = 0;
  request[3] = 0;
  request_pkt.Build(request,4);
  SendReceive(&request_pkt,false);

  if(len > MAX_PTZ_COMMAND_LENGTH)
  {
    fprintf(stderr,
	    "CANNONvcc4::SendCommand(): message is too large (%d bytes)\n",
	    len);
    return(-1);
  }

  //err = write(ptz_fd, str, len);
  // Since I'm hardcoding this to AUX1, basically we gotta stick the AUX1DATA
  // header on this and then give it to the p2os send command.
  unsigned char mybuf[MAX_PTZ_COMMAND_LENGTH];
  mybuf[0] = TTY2;
  mybuf[1] = ARGSTR;
  mybuf[2] = len;
  // Copy the command
  memcpy(&mybuf[3], str, len);
  //for (int i = 0; i < len+3; i ++)
  //  fprintf(stderr, "0x%x ", mybuf[i]);
  //fprintf(stderr,"\n");
  ptz_packet.Build(mybuf, len+3);

  /*
  printf("ptz_command packet = ");
  ptz_packet.PrintHex();
  */

  // Send the packet
  this->SendReceive(&ptz_packet, false);

  if (err != 0)
  {
    perror("canonvcc4::Send():write():");
    return(-1);
  }
  return(0);
}
/************************************************************************/
//int
//canonvcc4::SendRequest(unsigned char* str, int len)

int P2OS::SendRequest(unsigned char* str, int len, unsigned char* reply, uint8_t camera)
{
    return this->SendCommand(str,len);
}

/************************************************************************/
void
P2OS::PrintPacket(char* str, unsigned char* cmd, int len)
{
  for(int i=0;i<len;i++)
    printf(" %.2x", cmd[i]);
  puts("");
}
/************************************************************************/

int
P2OS::SendAbsPanTilt(int pan, int tilt)
{
  unsigned char command[MAX_PTZ_COMMAND_LENGTH];
  int convpan, convtilt;
  unsigned char buf[5];
  int ppan, ttilt;

  ppan = pan; ttilt = tilt;
  if (abs(pan) > PTZ_PAN_MAX)
    {
      if(pan < -PTZ_PAN_MAX)
	ppan = (int)-PTZ_PAN_MAX;
      else
	if(pan > PTZ_PAN_MAX)
	  ppan = (int)PTZ_PAN_MAX;
      //puts("Camera pan angle thresholded");
    }
  if (tilt > PTZ_TILT_MAX)
    ttilt = (int)PTZ_TILT_MAX;
  else
    if(tilt < PTZ_TILT_MIN)
      ttilt = (int)PTZ_TILT_MIN;
  //puts("Camera pan angle thresholded");

  //puts("Camera tilt angle thresholded");

  convpan = (int)floor(ppan/.1125) + 0x8000;
  convtilt = (int)floor(ttilt/.1125) + 0x8000;
//   fprintf(stdout, "ppan: %d ttilt: %d conpan: %d contilt: %d\n",
// 	  ppan,ttilt,convpan,convtilt);
  command[0] = 0xFF;
  command[1] = 0x30;
  command[2] = 0x30;
  command[3] = 0x00;
  command[4] = 0x62;
  // pan position

  sprintf((char *)buf, "%X", convpan);

  command[5] = buf[0];
  command[6] = buf[1];
  command[7] = buf[2];
  command[8] = buf[3];
  // tilt position
  sprintf((char *)buf, "%X", convtilt);
  command[9]  = buf[0];
  command[10] = buf[1];
  command[11] = buf[2];
  command[12] = buf[3];
  command[13] = (unsigned char) 0xEF;
  SendCommand(command, 14);
  //  PrintPacket( "sendabspantilt: ", command, 14);
  return(ReceiveCommandAnswer(6));
}
/************************************************************************/
int
P2OS::setDefaultTiltRange()
{
  unsigned char command[MAX_PTZ_COMMAND_LENGTH];
  unsigned char buf[8];
  int maxtilt, mintilt;

  command[0] = 0xFF;
  command[1] = 0x30;
  command[2] = 0x30;
  command[3] = 0x00;
  command[4] = 0x64;
  command[5] = 0x31;

  mintilt = (int)(floor(PTZ_TILT_MIN/.1125) + 0x8000);
  sprintf((char *)buf, "%X", mintilt);

  command[6] = buf[0];
  command[7] = buf[1];
  command[8] = buf[2];
  command[9] = buf[3];
  // tilt position
  maxtilt = (int)(floor(PTZ_TILT_MAX/.1125) + 0x8000);
  sprintf((char *)buf, "%X", maxtilt);

  command[10] = buf[0];
  command[11] = buf[1];
  command[12] = buf[2];
  command[13] = buf[3];
  command[14] = (unsigned char) 0xEF;

  SendCommand(command, 15);
  //  PrintPacket( "setDefaultRange: ", command, 15);
  return(ReceiveCommandAnswer(6));

}

/************************************************************************/
int
P2OS::GetAbsPanTilt(int* pan, int* tilt)
{
  unsigned char command[MAX_PTZ_COMMAND_LENGTH];
  unsigned char reply[MAX_PTZ_REQUEST_LENGTH];
  int reply_len;
  unsigned char buf[4];
  char byte;
  unsigned int u_val;
  int val;
  int i;

  command[0] = 0xFF;
  command[1] = 0x30;
  command[2] = 0x30;
  command[3] = 0x00;
  command[4] = 0x63;
  command[5] = 0xEF;

  if (SendRequest(command, 6, reply))
    return(-1);
  //  PrintPacket("getabspantilt: ", command, 6);
  //reply_len = ReceiveRequestAnswer(reply,6,14);
  reply_len = ReceiveRequestAnswer(reply,14,0);

  if ( reply_len != 14 ) {
    fprintf(stderr, "Reply Len = %i\n", reply_len);
    return -1;
  }

  // remove the ascii encoding, and put into 4-byte array
  for (i = 0; i < 4; i++)
  {
      byte = reply[i+5];
      if (byte < 0x40)
          byte = byte - 0x30;
      else
          byte = byte - 'A' + 10;
      buf[i] = byte;
  }

  // convert the 4-bytes into a number
  u_val = buf[0] * 0x1000 + buf[1] * 0x100 + buf[2] * 0x10 + buf[3];

  // convert the number to a value that's meaningful, based on camera specs
  val = (int)(((int)u_val - (int)0x8000) * 0.1125);

  // now set myPan to the response received for where the camera thinks it is
  *pan = val;

  // repeat the steps for the tilt value
  for (i = 0; i < 4; i++)
    {
      byte = reply[i+9];
      if (byte < 0x40)
	byte = byte - 0x30;
      else
	byte = byte - 'A' + 10;
      buf[i] = byte;
    }
  u_val = buf[0] * 0x1000 + buf[1] * 0x100 + buf[2] * 0x10 + buf[3];
  val =(int)(((int)u_val  - (int)0x8000) * 0.1125);
  *tilt = val;

  return(0);
}

/************************************************************************/
int
P2OS::GetAbsZoom(int* zoom)
{
  unsigned char command[MAX_PTZ_COMMAND_LENGTH];
  unsigned char reply[MAX_PTZ_REQUEST_LENGTH];
  int reply_len;
  char byte;
  unsigned char buf[4];
  unsigned int u_zoom;
  int i;

  command[0] = 0xFF;
  command[1] = 0x30;
  command[2] = 0x30;
  command[3] = 0x00;
  command[4] = 0xB4;
  command[5] = 0x30;
  command[6] = 0xEF;

  if (SendRequest(command, 7, reply))
    return(-1);
  //  PrintPacket( "getabszoom: ", command, 6);

  reply_len = ReceiveRequestAnswer(reply,10,0);

   if (reply_len == 6)
     return -1;

  // remove the ascii encoding, and put into 2 bytes
  for (i = 0; i < 4; i++)
  {
    byte = reply[i + 5];
    if (byte < 0x40)
      byte = byte - 0x30;
    else
      byte = byte - 'A' + 10;
    buf[i] = byte;
  }

  // convert the 2 bytes into a number
  u_zoom = 0;
  for (i = 0; i < 4; i++)
    u_zoom += buf[i] * (unsigned int) pow(16.0, (double)(3 - i));
  *zoom = u_zoom;
  return(0);
}

/************************************************************************/
int
P2OS::SendAbsZoom(int zoom)
{
  unsigned char command[MAX_PTZ_COMMAND_LENGTH];
  unsigned char buf[5];
  int i;

  if(zoom < 0)
    zoom = 0;
  //puts("Camera zoom thresholded");

  else
    if(zoom > maxzoom){
      zoom = maxzoom;
      //printf("putting zoom at MAX_ZOOM\n");
    }

  command[0] = 0xFF;
  command[1] = 0x30;
  command[2] = 0x30;
  command[3] = 0x00;
  command[4] = 0xB3;

  sprintf((char *)buf, "%4X", zoom);

  for (i=0;i<3;i++)
    if (buf[i] == ' ')
      buf[i] = '0';

  // zoom position
  command[5] = buf[0];
  command[6] = buf[1];
  command[7] = buf[2];
  command[8] = buf[3];
  command[9] = 0xEF;

  if (SendCommand(command, 10))
    return -1;
  //PrintPacket( "setabszoom: ", command, 10);
  return (ReceiveCommandAnswer(6));
}

void P2OS::get_ptz_packet(int s1, int s2)
{
    //printf("get_ptz_packet()\n");
    static const int TIMEOUT = 100;
    int packetCount = 0;
    unsigned char request[4];
    P2OSPacket request_pkt;
    bool secondSent = false;

    request[0] = GETAUX;
    request[1] = ARGINT;
    request[2] = s1;
    request[3] = 0;

    // Reset our receiving buffer.
    cb.reset();

    //sleep(1);
    //Request the request-size back
    request_pkt.Build(request,4);
    SendReceive(&request_pkt,false);

    while ( !cb.gotPacket() )
    {
        if ( packetCount++ > TIMEOUT ) {
            // Give Up We're not getting it.
            fprintf(stderr, "Waiting for packet timed out.\n");
            return;
        }
        if ( cb.size() == s1 && !secondSent)
        {
            if ( s2 > s1 )
            {
                // We got the first packet size, but we don't have a full packet.
                int newsize = s2 - s1;
                fprintf(stderr, "Requesting Second Packet of size %i\n", newsize);
                request[2] = newsize;
                request_pkt.Build(request,4);
                secondSent = true;
                //cb.printBuf();
                SendReceive(&request_pkt,false);
            }
            else
            {
                // We got the first packet but don't have a full packet, this is an error.
                fprintf(stderr, "Error: Got reply from AUX1 But don't have a full packet.\n");
                break;
            }
        }

        // Keep reading data until we get a response from the camera.
        //fprintf(stderr, "foo\n");
        SendReceive(NULL,false);
    }
    //fprintf(stderr, "Got ptz_packet: ");
    //cb.printBuf();
}

/************************************************************************/
int
P2OS::ReceiveCommandAnswer(int asize)
{
    int num;
    unsigned char reply[MAX_PTZ_REQUEST_LENGTH];
    int len = 0;
    unsigned char byte;
    int t;
    int error_code;
    // puts("Receivecommandanswer begin\n");
    memset(reply, 0, COMMAND_RESPONSE_BYTES);

    get_ptz_packet(asize);
    //fprintf(stderr, "Recieved Packet: ");
    //cb.printBuf();

    for (num = 0; num <= COMMAND_RESPONSE_BYTES + 1; num++)
    {
        // if we don't get any bytes, or if we've just exceeded the limit
        // then return null
        //err = read_ptz(&byte, 1);
        t = cb.getFromBuf();
        if ( t < 0 ) { // Buf Error!
            printf("circbuf error!\n");
        }
        else {
            byte = (unsigned char)t;
        }
        if (byte == (unsigned char)0xFE)
        {
            reply[0] = byte;
            len ++;
            break;
        }
    }
    if (len == 0)
        return -1;

    // we got the header character so keep reading bytes for MAX_RESPONSE_BYTES more
    for(num = 1; num <= MAX_PTZ_REQUEST_LENGTH; num++)
    {
        t = cb.getFromBuf();
        if (t < 0)
        {
            // there are no more bytes, so check the last byte for the footer
            if (reply[len - 1] !=  (unsigned char)0xEF)
            {
                fputs("canonvcc4::receiveCommandAnswer: Discarding bad packet.",
                        stderr);
                return -1;
            }
            else
                break;
        }
        else
        {
            // add the byte to the array
            reply[len] = (unsigned char)t;
            len ++;
        }
    }

    // Check the response
    if (len != 6)
    {
        fputs("canonvcc4::receiveCommandAnswer:Incorrect number of bytes in response packet.",
                stderr);
        return -1;
    }

    // check the header and footer
    if (reply[0] != (unsigned char)0xFE || reply[5] != (unsigned char)0xEF)
    {
        fputs("canonvcc4::receiveCommandAnswer: Bad header or footer character in response packet.", stderr);
        return -1;
    }
    // so far so good.  Set myError to the error byte
    error_code = reply[3];
    //PrintPacket("receivecommandasnwer: ", reply, 6);
    if (error_code == CAM_ERROR_NONE)
        return 0;
    else {
        switch(error_code){
        case CAM_ERROR_BUSY:
            fputs("Error: CAM_ERROR_BUSY\n", stderr);
            break;
        case CAM_ERROR_PARAM:
            fputs("Error: CAM_ERROR_PARAM\n", stderr);
            break;
        case CAM_ERROR_MODE:
            fputs("Error: CAM_ERROR_MODE\n", stderr);
            break;
        default:
            fputs("Error: Unknown error response from camera.\n",stderr);
            break;
        }
    }

    return -1;
}

/************************************************************************/
/* These commands often have variable packet lengths, if there is an error,
 * there is a smaller packet size. If we request the larger packet size first,
 * then we will never get a response back. Because of this, we have to first
 * request the smaller size, check if its a full packet, if it's not, request
 * the rest of the packet. Also according to the source code for ARIA, we can
 * not do more than 2 requests for a single packet, therefor, we can't just
 * request 1 byte over and over again.
 *
 * So here, s1 is the size of the smaller packet.
 * And s2 is the size of the larger packet.
 */
int
P2OS::ReceiveRequestAnswer(unsigned char *data, int s1, int s2)
{
    int num;
    unsigned char reply[MAX_PTZ_REQUEST_LENGTH];
    int len = 0;
    unsigned char byte;
    int t;
    int error_code;

    memset(reply, 0, MAX_PTZ_REQUEST_LENGTH);
    get_ptz_packet(s1, s2);
    //cb.printBuf();

    for (num = 0; num <= COMMAND_RESPONSE_BYTES + 1; num++)
    {
        // if we don't get any bytes, or if we've just exceeded the limit
        // then return null
        t = cb.getFromBuf();
        if ( t < 0 ) { // Buf Error!
            printf("circbuf error!\n");
        }
        else {
            byte = (unsigned char)t;
        }
        if (byte == (unsigned char)0xFE)
        {
            reply[0] = byte;
            len ++;
            break;
        }
    }
    if (len == 0)
        return -1;
    // we got the header character so keep reading bytes for MAX_RESPONSE_BYTES more
    for(num = 1; num <= MAX_PTZ_REQUEST_LENGTH; num++)
    {
        t = cb.getFromBuf();
        if (t < 0)
        {
            // there are no more bytes, so check the last byte for the footer
            if (reply[len - 1] !=  (unsigned char)0xEF)
            {
                fputs("canonvcc4::receiveRequest: Discarding bad packet.",
                        stderr);
                return -1;
            }
            else
                break;
        }
        else
        {
            // add the byte to the array
            reply[len] = (unsigned char)t;
            len ++;
        }
    }
    // Check the response length: pt: 14; zoom: 10
    if (len != 6 && len != 8 && len != 10 && len != 14)
    {
        fputs("Arvcc4::packetHandler: Incorrect number of bytes in response packet.", stderr);
        return -1;
    }

    if (reply[0] !=  (unsigned char)0xFE ||
            reply[len - 1] != (unsigned char)0xEF)
    {
        fputs("canonvcc4::receiveRequestArvcc4: Bad header or footer character in response packet.", stderr);
        return -1;
    }

    // so far so good.  Set myError to the error byte
    error_code = reply[3];
    //  PrintPacket("receiverequestasnwer: ", reply, len);
    if (error_code == CAM_ERROR_NONE)
    {
        memcpy(data, reply, len);
        return len;
    }
    return -1;
}

/************************************************************************/
int
P2OS::setControlMode()
{
  unsigned char command[MAX_PTZ_COMMAND_LENGTH];

  command[0] = 0xFF;
  command[1] = 0x30;
  command[2] = 0x30;
  command[3] = 0x00;
  command[4] = 0x90;
  command[5] = 0x30;
  command[6] = 0xEF;

  if (SendCommand(command, 7))
    return -1;
  //  usleep(5000000);
  return (ReceiveCommandAnswer(6));
}
/************************************************************************/
int
P2OS::setNotifyCommand()
{
  unsigned char command[MAX_PTZ_COMMAND_LENGTH];

  command[0] = 0xFF;
  command[1] = 0x30;
  command[2] = 0x30;
  command[3] = 0x00;
  command[4] = 0x94;
  command[5] = 0x31;
  command[6] = 0xEF;

  if (SendCommand(command, 7))
    return -1;
  //  usleep(5000000);
  return (ReceiveCommandAnswer(6));
}
/************************************************************************/
int
P2OS::setPower(int on)
{
  unsigned char command[MAX_PTZ_COMMAND_LENGTH];

  command[0] = 0xFF;
  command[1] = 0x30;
  command[2] = 0x30;
  command[3] = 0x00;
  command[4] = 0xA0;
  if (on)
    command[5] = 0x31;
  else
    command[5] = 0x30;
  command[6] = 0xEF;

  if (SendCommand(command, 7))
    return -1;
  //  usleep(5000000);
  return (ReceiveCommandAnswer(6));
}
/************************************************************************/
int
P2OS::setOnScreenOff()
{
  unsigned char command[MAX_PTZ_COMMAND_LENGTH];

  command[0] = 0xFF;
  command[1] = 0x30;
  command[2] = 0x30;
  command[3] = 0x00;
  command[4] = 0x91;
  command[5] = 0x30;
  command[6] = 0x30;
  command[7] = 0xEF;

  if (SendCommand(command, 8))
    return -1;
  //  usleep(5000000);
  return (ReceiveCommandAnswer(6));
}

int P2OS::sendInit()
{
    unsigned char command[MAX_PTZ_COMMAND_LENGTH];

    command[0] = 0xFF;
    command[1] = 0x30;
    command[2] = 0x30;
    command[3] = 0x00;
    command[4] = 0x58;
    command[5] = 0x30;
    command[6] = 0xEF;

    if (SendCommand(command, 7))
        return -1;
    //  usleep(5000000);
    return (ReceiveCommandAnswer(6));
}

int P2OS::GetMaxZoom(int * maxzoom){
    unsigned char command[MAX_PTZ_COMMAND_LENGTH];
    unsigned char reply[MAX_PTZ_REQUEST_LENGTH];
    int reply_len;
    char byte;
    unsigned char buf[4];
    unsigned int u_zoom;
    int i;

    command[0] = 0xFF;
    command[1] = 0x30;
    command[2] = 0x30;
    command[3] = 0x00;
    command[4] = 0xB4;
    command[5] = 0x33;
    command[6] = 0xEF;

    if (SendCommand(command, 7))
        return -1;
    //  usleep(5000000);

    reply_len = ReceiveRequestAnswer(reply,10,0);

    if ( reply_len == 6 ){
        return -1;
    }

    // remove the ascii encoding, and put into 2 bytes
    for (i = 0; i < 4; i++)
    {
        byte = reply[i + 5];
        if (byte < 0x40)
            byte = byte - 0x30;
        else
            byte = byte - 'A' + 10;
        buf[i] = byte;
    }

    // convert the 2 bytes into a number
    u_zoom = 0;
    for (i = 0; i < 4; i++)
        u_zoom += buf[i] * (unsigned int) pow(16.0, (double)(3 - i));
    *maxzoom = u_zoom;

    return 0;
}

/* Circular Buffer To deal with getting data back from AUX */
circbuf::circbuf(int size)
{
    this->buf = new unsigned char[size];
    this->mysize  = size;
    this->start = this->end = 0;
}

void circbuf::printBuf(){
    int i = start;
    printf("circbuf: ");
    while ( i != end ){
        printf("0x%x ", buf[i]);
        i = (i+1)%mysize;
    }
    printf("\n");
}


void circbuf::putOnBuf(unsigned char c)
{
    buf[end] = c;
    end = (end+1)%mysize;
    if ( end == start )
    {
        // We're overwriting old data.
        start = (start + 1)%mysize;
    }

    // Check to see if we have the whole packet now. (ends with 0xEF)
    if ( c == 0xEF )
    {
        gotPack = true;
    }
}

bool circbuf::haveData()
{
    return !(this->start == this->end);
}

int circbuf::getFromBuf()
{
    if ( start != end ){
        unsigned char ret = buf[start];
        start = (start+1)%mysize;
        return (int)ret;
    }
    else
    {
        return -1;
    }
}

int circbuf::size()
{
    if ( end > start )
    {
        return end-start;
    }
    else if ( start > end )
    {
        return mysize - start - end - 1;
    }
    else
    {
        return 0;
    }
}

bool circbuf::gotPacket()
{
    return gotPack;
}

void circbuf::reset()
{
    memset(buf, 0, mysize);
    gotPack = false;
    start = end = 0;
}

