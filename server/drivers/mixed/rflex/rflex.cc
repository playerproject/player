/*  Player - One Hell of a Robot Server
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

/* notes:
 * the Main thread continues running when no-one is connected
 * this we retain our odometry data, whether anyone is connected or not
 */

/* Modified by Toby Collett, University of Auckland 2003-02-25
 * Added support for bump sensors for RWI b21r robot, uses DIO
 */

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_rflex rflex

The rflex driver is used to control RWI robots by directly communicating
with RFLEX onboard the robot (i.e., Mobility is bypassed).  To date,
these drivers have been tested on an ATRV-Jr, but they should
work with other RFLEX-controlled robots: you will have to determine some
parameters to set in the config file, however.

As of March 2003 these drivers have been modified to support the
b21r robot, Currently additional support has been added for the @ref
player_interface_power interface and @ref player_interface_bumper
interface. For the pan tilt unit on the b21r please refer to
the @ref player_driver_ptu46 driver.

@par Compile-time dependencies

- none

@par Provides

The rflex driver provides the following device interfaces, some of them named:

- @ref player_interface_position : This interface returns odometry data,
  and accepts velocity commands.
- "sonar" @ref player_interface_sonar : Range data from the sonar array
- "sonar2" @ref player_interface_sonar : Range data from the second sonar array
- @ref player_interface_ir
- @ref player_interface_bumper
- @ref player_interface_power
- @ref player_interface_aio
- @ref player_interface_dio

@par Supported configuration requests

- The @ref player_interface_position interface supports:
  - PLAYER_POSITION_SET_ODOM_REQ
  - PLAYER_POSITION_MOTOR_POWER_REQ
  - PLAYER_POSITION_VELOCITY_MODE_REQ
  - PLAYER_POSITION_RESET_ODOM_REQ
  - PLAYER_POSITION_GET_GEOM_REQ
- The @ref player_interface_ir interface supports:
  - PLAYER_IR_POWER_REQ
  - PLAYER_IR_POSE_REQ
- The "sonar" @ref player_interface_sonar interface supports:
  - PLAYER_SONAR_POWER_REQ
  - PLAYER_SONAR_GET_GEOM_REQ
- The "sonar2" @ref player_interface_sonar interface supports:
  - PLAYER_SONAR_POWER_REQ
  - PLAYER_SONAR_GET_GEOM_REQ
- The @ref player_interface_bumper interface supports:
  - PLAYER_BUMPER_GET_GEOM_REQ

@par Configuration file options

- port (string)
  - Default: "/dev/ttyR0"
  - Serial port used to communicate with the robot.
- mm_length (float)
  - Default: 0.5
  - Length of the robot in millimeters
- mm_width (float)
  - Default: 0.5
  - Width of the robot in millimeters
- odo_distance_conversion (float)
  - Default: 0
  - Odometry conversion. See Note 1.
- odo_angle_conversion (float)
  - Default: 0
  - Odometry conversion. See Note 2.
- default_trans_acceleration (float)
  - Default: 0.1
  - Set translational acceleration, in mm.
- default_rot_acceleration (float)
  - Default: 0.1
  - Set rotational acceleration, in radians.
- rflex_joystick (integer)
  - Default: 0
  - Enables joystick control via the rflex controller
- rflex_joy_pos_ratio (float)
  - Default: 0
  - Joystick to movement conversion ratio
- rflex_joy_ang_ratio (float)
  - Default: 0
  - Joystick to movement conversion ratio
- range_distance_conversion (float)
  - Default: 1
  - Sonar range conversion factor. See Note 7.
- max_num_sonars (integer)
  - Default: 64
  - See Note 4
- num_sonars (integer)
  - Default: 24
  - See Note 4
- sonar_age (integer)
  - Default: 1
  - Prefiltering parameter. See Note 3.
- num_sonar_banks (integer)
  - Default: 8
  - See Note 4
- num_sonars_possible_per_bank (integer)
  - Default: 16
  - See Note 4
- num_sonars_in_bank (integer tuple)
  - Default: [ 8 8 8 ... ]
  - See Note 4
- sonar_echo_delay (integer)
  - Default: 3000
  - Sonar configuration parameters
- sonar_ping_delay (integer)
  - Default: 0
  - Sonar configuration parameters
- sonar_set_delay (integer)
  - Default: 0
  - Sonar configuration parameters
- mmrad_sonar_poses (tuple float)
  - Default: [ 0 0 0 ... ]
  - Sonar positions and directions.  See Note 6.
- sonar_2nd_bank_start (integer)
  - Default: 0
  - Address of the second sonar bank (lower bank on the b21r)
- pose_count (integer)
  - Default: 8
  - Total Number of IR sensors
- rflex_base_bank (integer)
  - Default: 0
  - Base IR Bank
- rflex_bank_count (integer)
  - Default: 0
  - Number of banks in use
- ir_min_range (integer)
  - Default: 100
  - Min range of ir sensors (mm) (Any range below this is returned as 0)
- ir_max_range (integer)
  - Default: 800
  - Max range of ir sensors (mm) (Any range above this is returned as max)
- rflex_banks (float tuple)
  - Default: [ 0 0 0 ... ]
  - Number of IR sensors in each bank
- poses (float tuple)
  - Default: [ 0 0 0 ... ]
  - x,y,theta of IR sensors (mm, mm, deg)
- rflex_ir_calib (float tuple)
  - Default: [ 1 1 ... ]
  - IR Calibration data (see Note 8)
- bumper_count (integer)
  - Default: 0
  - Number of bumper panels
- bumper_def (float tuple)
  - Default: [ 0 0 0 0 0 ... ]
  - x,y,theta,length,radius (mm,mm,deg,mm,mm) for each bumper 
- rflex_bumper_address (integer)
  - Default: 0x40
  - The base address of first bumper in the DIO address range
- rflex_bumper_style (string)
  - Default: "addr"
  - Bumpers can be defined either by addresses or a bit mask
- rflex_power_offset (integer)
  - Default: 0
  - The calibration constant for the power calculation in decivolts

@par Notes
-# Since the units used by the Rflex for odometry appear to be completely
   arbitrary, this coefficient is needed to convert to millimeters: mm =
   (rflex units) / (odo_distance_conversion).  These arbitrary units
   also seem to be different on each robot model. I'm afraid you'll
   have to determine your robot's conversion factor by driving a known
   distance and observing the output of the RFlex.
-# Conversion coefficient
   for rotation odometry: see odo_distance_conversion. Note that
   heading is re-calculated by the Player driver since the RFlex is not
   very accurate in this respect. See also Note 1.
-# Used for prefiltering:
   the standard Polaroid sensors never return values that are closer
   than the closest obstacle, thus we can buffer locally looking for the
   closest reading in the last "sonar_age" readings. Since the servo
   tick here is quite small, you can still get pretty recent data in
   the client.
-# These values are all used for remapping the sonars from Rflex indexing
   to player indexing. Individual sonars are enumerated 0-15 on each
   board, but at least on my robots each only has between 5 and 8 sonar
   actually attached.  Thus we need to remap all of these indexes to
   get a contiguous array of N sonars for Player.
     - max_num_sonars is the maximum enumeration value+1 of
       all sonar meaning if we have 4 sonar boards this number is 64.
     - num_sonars is the number of physical sonar sensors -
       meaning the number of ranges that will be returned by Player.  -
       num_sonar_banks is the number of sonar boards you have.
     - num_sonars_possible_per_bank is probobly 16 for all
       robots, but I included it here just in case. this is the number of
       sonar that can be attached to each sonar board, meaning the maximum
       enumeration value mapped to each board.  - num_sonars_in_bank
       is the nubmer of physical sonar attached to each board in order -
       you'll notice on each the sonar board a set of dip switches, these
       switches configure the enumeration of the boards (ours are 0-3)
-# The first RFlex device (position, sonar or power) in the config file
   must include this option, and only the first device's value will be used.
-# This is about the ugliest way possible of telling Player where each
   sonar is mounted.  Include in the string groups of three values: "x1
   y1 th1 x2 y2 th2 x3 y3 th3 ...".  x and y are mm and theta is radians,
in Player's robot coordinate system.
-# Used to convert between arbitrary sonar units to millimeters: mm =
   sonar units / range_distance_conversion.
-# Calibration is in the form Range = (Voltage/a)^b and stored in the
   tuple as [a1 b1 a2 b2 ...] etc for each ir sensor.

@par Example 

@verbatim
driver
(
  name "rflex" 
  provides ["position:1" "bumper:0" "sonar:0" "sonar:1" "power:0" "ir:0"]

  rflex_serial_port 		"/dev/ttyR0" 
  mm_length 			500.0
  mm_width 			500.0 
  odo_distance_conversion 	103
  odo_angle_conversion 		35000
  default_trans_acceleration 	500.0
  default_rot_acceleration 	10.0
  rflex_joystick			1
  rflex_joy_pos_ratio		6
  rflex_joy_ang_ratio		-0.01


  bumper_count		14
  bumper_def		[   -216.506351 125.000000 -210.000000 261.799388 250.000000 -0.000000 250.000000 -270.000000 261.799388 250.000000 216.506351 125.000000 -330.000000 261.799388 250.000000 216.506351 -125.000000 -390.000000 261.799388 250.000000 0.000000 -250.000000 -450.000000 261.799388 250.000000 -216.506351 -125.000000 -510.000000 261.799388 250.000000 -240.208678 -99.497692 -157.500000 204.203522 260.000000 -240.208678 99.497692 -202.500000 204.203522 260.000000 -99.497692 240.208678 -247.500000 204.203522 260.000000 99.497692 240.208678 -292.500000 204.203522 260.000000 240.208678 99.497692 -337.500000 204.203522 260.000000 240.208678 -99.497692 -382.500000 204.203522 260.000000 99.497692 -240.208678 -427.500000 204.203522 260.000000 -99.497692 -240.208678 -472.500000 204.203522 260.000000 ]
  rflex_bumper_address	64 # 0x40

  range_distance_conversion 	1.476
  sonar_age 			1
  sonar_echo_delay		30000
  sonar_ping_delay		0
  sonar_set_delay			0
  max_num_sonars 			224
  num_sonars 				48
  num_sonar_banks 		14
  num_sonars_possible_per_bank	16
  num_sonars_in_bank 		[4 4 4 4 4 4 3 3 3 3 3 3 3 3]
  # theta (rads), x, y (mm) in robot coordinates (x is forward)
  mmrad_sonar_poses 	[     3.01069  -247.86122    32.63155     2.74889  -230.96988    95.67086     2.48709  -198.33834   152.19036     2.22529  -152.19036   198.33834     1.96350   -95.67086   230.96988     1.70170   -32.63155   247.86122     1.43990    32.63155   247.86122     1.17810    95.67086   230.96988     0.91630   152.19036   198.33834     0.65450   198.33834   152.19036     0.39270   230.96988    95.67086     0.13090   247.86122    32.63155    -0.13090   247.86122   -32.63155    -0.39270   230.96988   -95.67086    -0.65450   198.33834  -152.19036    -0.91630   152.19036  -198.33834    -1.17810    95.67086  -230.96988    -1.43990    32.63155  -247.86122    -1.70170   -32.63155  -247.86122    -1.96350   -95.67086  -230.96988    -2.22529  -152.19036  -198.33834    -2.48709  -198.33834  -152.19036    -2.74889  -230.96988   -95.67086    -3.01069  -247.86122   -32.63155       4.18879  -130.00000  -225.16660     3.92699  -183.84776  -183.84776     3.66519  -225.16660  -130.00000     3.40339  -251.14071   -67.29295     3.14159  -260.00000     0.00000     2.87979  -251.14071    67.29295     2.61799  -225.16660   130.00000     2.35619  -183.84776   183.84776     2.09440  -130.00000   225.16660     1.83260   -67.29295   251.14071     1.57080     0.00000   260.00000     1.30900    67.29295   251.14071     1.04720   130.00000   225.16660     0.78540   183.84776   183.84776     0.52360   225.16660   130.00000     0.26180   251.14071    67.29295     0.00000   260.00000     0.00000    -0.26180   251.14071   -67.29295    -0.52360   225.16660  -130.00000    -0.78540   183.84776  -183.84776    -1.04720   130.00000  -225.16660    -1.30900    67.29295  -251.14071    -1.57080     0.00000  -260.00000    -1.83260   -67.29295  -251.14071    -2.09440  -130.00000  -225.16660    -2.35619  -183.84776  -183.84776]
  sonar_2nd_bank_start	24
	
  rflex_power_offset		12 # deci volts?

  rflex_base_bank 0
  rflex_bank_count 6
  rflex_banks	[4 4 4 4 4 4]
  pose_count	24
  ir_min_range	100
  ir_max_range	800
  rflex_ir_calib	[ 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 0.0005456 -2.086 ]
  poses		[ -247 32 532 -230 95 517 -198 152 502 -152 198 487 -95 230 472 -32 247 457 32 247 442 95 230 427 152 198 412 198 152 397 230 95 382 247 32 367 247 -32 352 230 -95 337 198 -152 322 152 -198 307 95 -230 292 32 -247 277 -32 -247 262 -95 -230 247 -152 -198 232 -198 -152 217 -230 -95 202 -247 -32 187 ]
)
@endverbatim

@par Authors

Matthew Brewer, Toby Collett
*/
/** @} */



#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>  /* for abs() */
#include <netinet/in.h>
#include <termios.h>

#include "rflex.h"
#include "rflex_configs.h"

//rflex communications stuff
#include "rflex_commands.h"
#include "rflex-io.h"

#include <driver.h>
#include <error.h>
#include <drivertable.h>
#include <playertime.h>
extern PlayerTime* GlobalTime;

extern int               RFLEX::joy_control;

//NOTE - this is accessed as an extern variable by the other RFLEX objects
rflex_config_t rflex_configs;

/* initialize the driver.
 *
 * returns: pointer to new REBIR object
 */
Driver*
RFLEX_Init(ConfigFile *cf, int section)
{
  return (Driver *) new RFLEX( cf, section);
}

/* register the Khepera IR driver in the drivertable
 *
 * returns: 
 */
void
RFLEX_Register(DriverTable *table) 
{
  table->AddDriver("rflex", RFLEX_Init);
}

RFLEX::RFLEX(ConfigFile* cf, int section)
        : Driver(cf,section)
{
  // zero ids, so that we'll know later which interfaces were requested
  memset(&this->position_id, 0, sizeof(player_device_id_t));
  memset(&this->sonar_id, 0, sizeof(player_device_id_t));
  memset(&this->sonar_id_2, 0, sizeof(player_device_id_t));
  memset(&this->ir_id, 0, sizeof(player_device_id_t));
  memset(&this->bumper_id, 0, sizeof(player_device_id_t));
  memset(&this->power_id, 0, sizeof(player_device_id_t));
  memset(&this->aio_id, 0, sizeof(player_device_id_t));
  memset(&this->dio_id, 0, sizeof(player_device_id_t));

  this->position_subscriptions = 0;
  this->sonar_subscriptions = 0;
  this->ir_subscriptions = 0;
  this->bumper_subscriptions = 0;

  // Do we create a robot position interface?
  if(cf->ReadDeviceId(&(this->position_id), section, "provides", 
                      PLAYER_POSITION_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->position_id, PLAYER_ALL_MODE,
                          sizeof(player_position_data_t),
                          sizeof(player_position_cmd_t), 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a sonar interface?
  if(cf->ReadDeviceId(&(this->sonar_id), section, "provides", 
                      PLAYER_SONAR_CODE, -1, "sonar") == 0)
  {
    if(this->AddInterface(this->sonar_id, PLAYER_READ_MODE,
                          sizeof(player_sonar_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a second sonar interface?
  if(cf->ReadDeviceId(&(this->sonar_id_2), section, "provides", 
                      PLAYER_SONAR_CODE, -1, "sonar2") == 0)
  {
    if(this->AddInterface(this->sonar_id_2, PLAYER_READ_MODE,
                          sizeof(player_sonar_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }


  // Do we create an ir interface?
  if(cf->ReadDeviceId(&(this->ir_id), section, "provides", 
                      PLAYER_IR_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->ir_id, PLAYER_READ_MODE,
                          sizeof(player_ir_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a bumper interface?
  if(cf->ReadDeviceId(&(this->bumper_id), section, "provides", 
                      PLAYER_BUMPER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->bumper_id, PLAYER_READ_MODE,
                          sizeof(player_bumper_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a power interface?
  if(cf->ReadDeviceId(&(this->power_id), section, "provides", 
                      PLAYER_POWER_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->power_id, PLAYER_READ_MODE,
                          sizeof(player_power_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create an aio interface?
  if(cf->ReadDeviceId(&(this->aio_id), section, "provides", 
                      PLAYER_AIO_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->aio_id, PLAYER_READ_MODE,
                          sizeof(player_aio_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a dio interface?
  if(cf->ReadDeviceId(&(this->dio_id), section, "provides", 
                      PLAYER_DIO_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->dio_id, PLAYER_READ_MODE,
                          sizeof(player_dio_data_t), 0, 0, 0) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  //just sets stuff to zero
  set_config_defaults();

  // joystick override
  joy_control = 0;

  //get serial port: everyone needs it (and we dont' want them fighting)
  strncpy(rflex_configs.serial_port,
          cf->ReadString(section, "rflex_serial_port", 
                         rflex_configs.serial_port),
          sizeof(rflex_configs.serial_port));

  ////////////////////////////////////////////////////////////////////////
  // Position-related options

  //length
  rflex_configs.mm_length=
    cf->ReadFloat(section, "mm_length",0.5);
  //width
  rflex_configs.mm_width=
    cf->ReadFloat(section, "mm_width",0.5);
  //distance conversion
  rflex_configs.odo_distance_conversion=
    cf->ReadFloat(section, "odo_distance_conversion", 0.0);
  //angle conversion
  rflex_configs.odo_angle_conversion=
    cf->ReadFloat(section, "odo_angle_conversion", 0.0);
  //default trans accel
  rflex_configs.mmPsec2_trans_acceleration=
    cf->ReadFloat(section, "default_trans_acceleration",0.1);
  //default rot accel
  rflex_configs.radPsec2_rot_acceleration=
    cf->ReadFloat(section, "default_rot_acceleration",0.1);

  // use rflex joystick for position
  rflex_configs.use_joystick |= cf->ReadInt(section, "rflex_joystick",0);
  rflex_configs.joy_pos_ratio = cf->ReadFloat(section, "rflex_joy_pos_ratio",0);
  rflex_configs.joy_ang_ratio = cf->ReadFloat(section, "rflex_joy_ang_ratio",0);

  ////////////////////////////////////////////////////////////////////////
  // Sonar-related options
  int x;

  rflex_configs.range_distance_conversion=
          cf->ReadFloat(section, "range_distance_conversion",1);
  rflex_configs.max_num_sonars=
          cf->ReadInt(section, "max_num_sonars",64);
  rflex_configs.num_sonars=
          cf->ReadInt(section, "num_sonars",24);
  rflex_configs.sonar_age=
          cf->ReadInt(section, "sonar_age",1);
  rflex_configs.num_sonar_banks=
          cf->ReadInt(section, "num_sonar_banks",8);
  rflex_configs.num_sonars_possible_per_bank=
          cf->ReadInt(section, "num_sonars_possible_per_bank",16);
  rflex_configs.num_sonars_in_bank=(int *) malloc(rflex_configs.num_sonar_banks*sizeof(double));
  for(x=0;x<rflex_configs.num_sonar_banks;x++)
    rflex_configs.num_sonars_in_bank[x]=
            (int) cf->ReadTupleFloat(section, "num_sonars_in_bank",x,8);
  rflex_configs.sonar_echo_delay=
          cf->ReadInt(section, "sonar_echo_delay",3000);
  rflex_configs.sonar_ping_delay=
          cf->ReadInt(section, "sonar_ping_delay",0);
  rflex_configs.sonar_set_delay=
          cf->ReadInt(section, "sonar_set_delay", 0);
  rflex_configs.mmrad_sonar_poses=(sonar_pose_t *) malloc(rflex_configs.num_sonars*sizeof(sonar_pose_t));
  for(x=0;x<rflex_configs.num_sonars;x++)
  {
    rflex_configs.mmrad_sonar_poses[x].x=
            cf->ReadTupleFloat(section, "mmrad_sonar_poses",3*x+1,0.0);
    rflex_configs.mmrad_sonar_poses[x].y=
            cf->ReadTupleFloat(section, "mmrad_sonar_poses",3*x+2,0.0);
    rflex_configs.mmrad_sonar_poses[x].t=
            cf->ReadTupleFloat(section, "mmrad_sonar_poses",3*x,0.0);
  }
  rflex_configs.sonar_2nd_bank_start=cf->ReadInt(section, "sonar_2nd_bank_start", 0);
  rflex_configs.sonar_1st_bank_end=rflex_configs.sonar_2nd_bank_start>0?rflex_configs.sonar_2nd_bank_start:rflex_configs.num_sonars;
  ////////////////////////////////////////////////////////////////////////
  // IR-related options

  int pose_count=cf->ReadInt(section, "pose_count",8);
  rflex_configs.ir_base_bank=cf->ReadInt(section, "rflex_base_bank",0);
  rflex_configs.ir_bank_count=cf->ReadInt(section, "rflex_bank_count",0);
  rflex_configs.ir_min_range=cf->ReadInt(section,"ir_min_range",100);
  rflex_configs.ir_max_range=cf->ReadInt(section,"ir_max_range",800); 
  rflex_configs.ir_count=new int[rflex_configs.ir_bank_count];
  rflex_configs.ir_a=new double[pose_count];
  rflex_configs.ir_b=new double[pose_count];
  rflex_configs.ir_poses.pose_count=pose_count;
  int RunningTotal = 0;
  for(int i=0; i < rflex_configs.ir_bank_count; ++i)
    RunningTotal += (rflex_configs.ir_count[i]=(int) cf->ReadTupleFloat(section, "rflex_banks",i,0));

  // posecount is actually unnecasary, but for consistancy will juse use it for error checking :)
  if (RunningTotal != rflex_configs.ir_poses.pose_count)
  {
    fprintf(stderr,"Error in config file, pose_count not equal to total poses in bank description\n");  
    rflex_configs.ir_poses.pose_count = RunningTotal;
  }		

  //  rflex_configs.ir_poses.poses=new int16_t[rflex_configs.ir_poses.pose_count];
  for(x=0;x<rflex_configs.ir_poses.pose_count;x++)
  {
    rflex_configs.ir_poses.poses[x][0]=(int) cf->ReadTupleFloat(section, "poses",x*3,0);
    rflex_configs.ir_poses.poses[x][1]=(int) cf->ReadTupleFloat(section, "poses",x*3+1,0);
    rflex_configs.ir_poses.poses[x][2]=(int) cf->ReadTupleFloat(section, "poses",x*3+2,0);

    // Calibration parameters for ir in form range=(a*voltage)^b
    rflex_configs.ir_a[x] = cf->ReadTupleFloat(section, "rflex_ir_calib",x*2,1);
    rflex_configs.ir_b[x] = cf->ReadTupleFloat(section, "rflex_ir_calib",x*2+1,1);	
  }

  ////////////////////////////////////////////////////////////////////////
  // Bumper-related options
  rflex_configs.bumper_count = cf->ReadInt(section, "bumper_count",0);
  rflex_configs.bumper_def = new player_bumper_define_t[rflex_configs.bumper_count];
  for(x=0;x<rflex_configs.bumper_count;++x)
  {
    rflex_configs.bumper_def[x].x_offset = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x,0)); //mm
    rflex_configs.bumper_def[x].y_offset = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+1,0)); //mm
    rflex_configs.bumper_def[x].th_offset = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+2,0)); //deg
    rflex_configs.bumper_def[x].length = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+3,0)); //mm
    rflex_configs.bumper_def[x].radius = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+4,0));	//mm  	
  }
  rflex_configs.bumper_address = cf->ReadInt(section, "rflex_bumper_address",DEFAULT_RFLEX_BUMPER_ADDRESS);


  const char *bumperStyleStr = cf->ReadString(section, "rflex_bumper_style",DEFAULT_RFLEX_BUMPER_STYLE);

  if(strcmp(bumperStyleStr,RFLEX_BUMPER_STYLE_BIT) == 0)
  {
    rflex_configs.bumper_style = BUMPER_BIT;
  }
  else if(strcmp(bumperStyleStr,RFLEX_BUMPER_STYLE_ADDR) == 0)
  {
    rflex_configs.bumper_style = BUMPER_ADDR;
  }
  else
  {
    //Invalid value
    rflex_configs.bumper_style = BUMPER_ADDR;
  }

  ////////////////////////////////////////////////////////////////////////
  // Power-related options
  rflex_configs.power_offset = cf->ReadInt(section, "rflex_power_offset",DEFAULT_RFLEX_POWER_OFFSET);

  rflex_fd = -1;
}

int RFLEX::Setup(){
  /* now spawn reading thread */
  StartThread();
  return(0);
}

int RFLEX::Shutdown()
{
  if(rflex_fd == -1)
  {
    return 0;
  }
  StopThread();

  //make sure it doesn't go anywhere
  rflex_stop_robot(rflex_fd,(int) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration));
  //kill that infernal clicking
  rflex_sonars_off(rflex_fd);

  return 0;
}

int 
RFLEX::Subscribe(player_device_id_t id)
{
  int setupResult;

  // do the subscription
  if((setupResult = Driver::Subscribe(id)) == 0)
  {
    // also increment the appropriate subscription counter
    switch(id.code)
    {
      case PLAYER_POSITION_CODE:
        this->position_subscriptions++;
        break;
      case PLAYER_SONAR_CODE:
        this->sonar_subscriptions++;
        break;
      case PLAYER_BUMPER_CODE:
        this->bumper_subscriptions++;
        break;
      case PLAYER_IR_CODE:
        this->ir_subscriptions++;
        break;
    }
  }

  return(setupResult);
}

int 
RFLEX::Unsubscribe(player_device_id_t id)
{
  int shutdownResult;

  // do the unsubscription
  if((shutdownResult = Driver::Unsubscribe(id)) == 0)
  {
    // also decrement the appropriate subscription counter
    switch(id.code)
    {
      case PLAYER_POSITION_CODE:
        assert(--this->position_subscriptions >= 0);
        break;
      case PLAYER_SONAR_CODE:
        assert(--this->sonar_subscriptions >= 0);
        break;
      case PLAYER_BUMPER_CODE:
        assert(--this->bumper_subscriptions >= 0);
        break;
      case PLAYER_IR_CODE:
        assert(--this->ir_subscriptions >= 0);
        break;
    }
  }

  return(shutdownResult);
}

void 
RFLEX::Main()
{
  printf("Rflex Thread Started\n");

  //sets up connection, and sets defaults
  //configures sonar, motor acceleration, etc.
  if(initialize_robot()<0){
    fprintf(stderr,"ERROR, no connection to RFLEX established\n");
    exit(1);
  }
  reset_odometry();


  player_position_cmd_t command;
  unsigned char config[RFLEX_CONFIG_BUFFER_SIZE];

  static double mmPsec_speedDemand=0.0, radPsec_turnRateDemand=0.0;
  bool newmotorspeed, newmotorturn;

  int config_size;
  int i;
  int last_sonar_subscrcount = 0;
  int last_position_subscrcount = 0;
  int last_ir_subscrcount = 0;

  while(1)
  {
    // we want to turn on the sonars if someone just subscribed, and turn
    // them off if the last subscriber just unsubscribed.
    if(!last_sonar_subscrcount && this->sonar_subscriptions)
        rflex_sonars_on(rflex_fd);
    else if(last_sonar_subscrcount && !(this->sonar_subscriptions))
      rflex_sonars_off(rflex_fd);
    last_sonar_subscrcount = this->sonar_subscriptions;

    // we want to turn on the ir if someone just subscribed, and turn
    // it off if the last subscriber just unsubscribed.
    if(!last_ir_subscrcount && this->ir_subscriptions)
      rflex_ir_on(rflex_fd);
    else if(last_ir_subscrcount && !(this->ir_subscriptions))
      rflex_ir_off(rflex_fd);
    last_ir_subscrcount = this->ir_subscriptions;

    // we want to reset the odometry and enable the motors if the first 
    // client just subscribed to the position device, and we want to stop 
    // and disable the motors if the last client unsubscribed.

    //first user logged in
    if(!last_position_subscrcount && this->position_subscriptions)
    {
      //set drive defaults
      rflex_motion_set_defaults(rflex_fd);

      //make sure robot doesn't go anywhere
      rflex_stop_robot(rflex_fd,(int) (MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration)));

      //clear the buffers
      player_position_cmd_t position_cmd;
      memset(&position_cmd,0,sizeof(player_position_cmd_t));
      PutCommand(this->position_id,(unsigned char*)(&position_cmd), 
                 sizeof(position_cmd), NULL);
    }
    //last user logged out
    else if(last_position_subscrcount && !(this->position_subscriptions))
    {
      //make sure robot doesn't go anywhere
      rflex_stop_robot(rflex_fd,(int) (MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration)));
      // disable motor power
      rflex_brake_on(rflex_fd);
    }
    last_position_subscrcount = this->position_subscriptions;
    
    void* client;

    // check if there is a new sonar config
    if((config_size = GetConfig(this->sonar_id, &client, 
                                (void*)config, sizeof(config), NULL)))
    {
      switch(config[0])
      {
        case PLAYER_SONAR_POWER_REQ:
          /*
           * 1 = enable sonars
           * 0 = disable sonar
           */
          if(config_size != sizeof(player_sonar_power_config_t))
          {
            puts("Arg to sonar state change request wrong size; ignoring");
            if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_sonar_power_config_t sonar_config;
          sonar_config = *((player_sonar_power_config_t*)config);

          if(sonar_config.value==0)
            rflex_sonars_off(rflex_fd);
          else
            rflex_sonars_on(rflex_fd);

          if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        case PLAYER_SONAR_GET_GEOM_REQ:
          /* Return the sonar geometry. */

          if(config_size != 1)
          {
            puts("Arg get sonar geom is wrong size; ignoring");
            if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_sonar_geom_t geom;
          geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;
          geom.pose_count = htons((short) rflex_configs.sonar_1st_bank_end);
          for (i = 0; i < rflex_configs.sonar_1st_bank_end; i++)
          {
            geom.poses[i][0] = htons((short) rflex_configs.mmrad_sonar_poses[i].x);
            geom.poses[i][1] = htons((short) rflex_configs.mmrad_sonar_poses[i].y);
            geom.poses[i][2] = htons((short) RAD2DEG_CONV(rflex_configs.mmrad_sonar_poses[i].t));
          }

          if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_ACK, 
                      &geom, sizeof(geom), NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        default:
          puts("Sonar got unknown config request");
          if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }

    // check if there is a new sonar 2 config
    if((config_size = GetConfig(this->sonar_id_2, &client, 
                                (void*)config, sizeof(config), NULL)))
    {
      switch(config[0])
      {
        case PLAYER_SONAR_POWER_REQ:
          /*
           * 1 = enable sonars
           * 0 = disable sonar
           */
          if(config_size != sizeof(player_sonar_power_config_t))
          {
            puts("Arg to sonar state change request wrong size; ignoring");
            if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_sonar_power_config_t sonar_config;
          sonar_config = *((player_sonar_power_config_t*)config);

          if(sonar_config.value==0)
            rflex_sonars_off(rflex_fd);
          else
            rflex_sonars_on(rflex_fd);

          if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        case PLAYER_SONAR_GET_GEOM_REQ:
          /* Return the sonar geometry. */

          if(config_size != 1)
          {
            puts("Arg get sonar geom is wrong size; ignoring");
            if(PutReply(this->sonar_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_sonar_geom_t geom;
          geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;
          geom.pose_count = htons((short) rflex_configs.num_sonars - rflex_configs.sonar_2nd_bank_start);
          for (i = 0; i < rflex_configs.num_sonars - rflex_configs.sonar_2nd_bank_start; i++)
          {
            geom.poses[i][0] = htons((short) rflex_configs.mmrad_sonar_poses[i+rflex_configs.sonar_2nd_bank_start].x);
            geom.poses[i][1] = htons((short) rflex_configs.mmrad_sonar_poses[i+rflex_configs.sonar_2nd_bank_start].y);
            geom.poses[i][2] = htons((short) RAD2DEG_CONV(rflex_configs.mmrad_sonar_poses[i+rflex_configs.sonar_2nd_bank_start].t));
          }

          if(PutReply(this->sonar_id_2, client, PLAYER_MSGTYPE_RESP_ACK, 
                      &geom, sizeof(geom), NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        default:
          puts("Sonar got unknown config request");
          if(PutReply(this->sonar_id_2, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }


    // check if there is a new bumper config
    if((config_size = GetConfig(this->bumper_id, &client, 
                                (void*)config, sizeof(config), NULL)))
    {
      switch(config[0])
      {
        case PLAYER_BUMPER_GET_GEOM_REQ:
          /* Return the bumper geometry. */
          if(config_size != 1)
          {
            puts("Arg get bumper geom is wrong size; ignoring");
            if(PutReply(this->bumper_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          // Assemble geometry structure for sending
          player_bumper_geom_t geom;
          geom.subtype = PLAYER_BUMPER_GET_GEOM_REQ;
          geom.bumper_count = htons((short) rflex_configs.bumper_count);
          for (i = 0; i < rflex_configs.bumper_count; i++)
          {
            geom.bumper_def[i].x_offset = htons((short) rflex_configs.bumper_def[i].x_offset); //mm
            geom.bumper_def[i].y_offset = htons((short) rflex_configs.bumper_def[i].y_offset); //mm
            geom.bumper_def[i].th_offset = htons((short) rflex_configs.bumper_def[i].th_offset); //deg
            geom.bumper_def[i].length = htons((short) rflex_configs.bumper_def[i].length); //mm
            geom.bumper_def[i].radius = htons((short) rflex_configs.bumper_def[i].radius); //mm
          }

          // Send
          if(PutReply(this->bumper_id, client, PLAYER_MSGTYPE_RESP_ACK, 
                      &geom, sizeof(geom), NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

          // Arent any request other than geometry
        default:
          puts("Bumper got unknown config request");
          if(PutReply(this->bumper_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }

    // check if there is a new ir config
    if((config_size = GetConfig(this->ir_id, &client, 
                                (void*)config, sizeof(config), NULL)))
    {
      switch(config[0])
      {
        case PLAYER_IR_POSE_REQ:
          /* Return the ir geometry. */
          if(config_size != 1)
          {
            puts("Arg get bumper geom is wrong size; ignoring");
            if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          // Assemble geometry structure for sending
          //printf("sending geometry to client, posecount = %d\n", rflex_configs.ir_poses.pose_count);
          player_ir_pose_req geom;
          geom.subtype = PLAYER_IR_POSE_REQ;
          geom.poses.pose_count = htons((short) rflex_configs.ir_poses.pose_count);
          for (i = 0; i < rflex_configs.ir_poses.pose_count; i++){
            geom.poses.poses[i][0] = htons((short) rflex_configs.ir_poses.poses[i][0]); //mm
            geom.poses.poses[i][1] = htons((short) rflex_configs.ir_poses.poses[i][1]); //mm
            geom.poses.poses[i][2] = htons((short) rflex_configs.ir_poses.poses[i][2]); //deg
          }

          // Send
          if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_ACK, 
                      &geom, sizeof(geom), NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
        case PLAYER_IR_POWER_REQ:
          /* Return the ir geometry. */
          if(config_size != 1)
          {
            puts("Arg get ir geom is wrong size; ignoring");
            if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          if (config[1] == 0)
            rflex_ir_off(rflex_fd);
          else
            rflex_ir_on(rflex_fd);

          // Send
          if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

          // Arent any request other than geometry and power
        default:
          puts("Ir got unknown config request");
          if(PutReply(this->ir_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }

    // check if there is a new position config
    if((config_size = GetConfig(this->position_id, &client, 
                                (void*)config, sizeof(config), NULL)))
    {
      switch(config[0])
      {
        case PLAYER_POSITION_SET_ODOM_REQ:
          if(config_size != sizeof(player_position_set_odom_req_t))
          {
            puts("Arg to odometry set requests wrong size; ignoring");
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_position_set_odom_req_t set_odom_req;
          set_odom_req = *((player_position_set_odom_req_t*)config);
          //in mm
          set_odometry((long) ntohl(set_odom_req.x),(long) ntohl(set_odom_req.y),(short) ntohs(set_odom_req.theta));

          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        case PLAYER_POSITION_MOTOR_POWER_REQ:
          /* motor state change request 
           *   1 = enable motors
           *   0 = disable motors (default)
           */
          if(config_size != sizeof(player_position_power_config_t))
          {
            puts("Arg to motor state change request wrong size; ignoring");
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_position_power_config_t power_config;
          power_config = *((player_position_power_config_t*)config);

          if(power_config.value==0)
            rflex_brake_on(rflex_fd);
          else
            rflex_brake_off(rflex_fd);

          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        case PLAYER_POSITION_VELOCITY_MODE_REQ:
          /* velocity control mode:
           *   0 = direct wheel velocity control (default)
           *   1 = separate translational and rotational control
           */
          fprintf(stderr,"WARNING!!: only velocity mode supported\n");
          if(config_size != sizeof(player_position_velocitymode_config_t))
          {
            puts("Arg to velocity control mode change request is wrong size; ignoring");
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          player_position_velocitymode_config_t velmode_config;
          velmode_config = *((player_position_velocitymode_config_t*)config);

          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        case PLAYER_POSITION_RESET_ODOM_REQ:
          /* reset position to 0,0,0: no args */
          if(config_size != sizeof(player_position_resetodom_config_t))
          {
            puts("Arg to reset position request is wrong size; ignoring");
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }
          reset_odometry();

          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_ACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;

        case PLAYER_POSITION_GET_GEOM_REQ:
          /* Return the robot geometry. */
          if(config_size != 1)
          {
            puts("Arg get robot geom is wrong size; ignoring");
            if(PutReply(this->position_id, client, 
                        PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
            break;
          }

          // TODO : get values from somewhere.
          player_position_geom_t geom;
          geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
          //mm
          geom.pose[0] = htons((short) (0));
          geom.pose[1] = htons((short) (0));
          //radians
          geom.pose[2] = htons((short) (0));
          //mm
          geom.size[0] = htons((short) (rflex_configs.mm_length));
          geom.size[1] = htons((short) (rflex_configs.mm_width));

          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_ACK, &geom, sizeof(geom), NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
        default:
          puts("Position got unknown config request");
          if(PutReply(this->position_id, client, 
                      PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
      }
    }


    if(this->position_subscriptions || rflex_configs.use_joystick)
    {
      /* read the clients' commands from the common buffer */
      GetCommand(this->position_id,(unsigned char*)&command, 
                 sizeof(command), NULL);

      newmotorspeed = false;
      newmotorturn = false;
      //the long casts are necicary (ntohl returns unsigned - we need signed)
      if(mmPsec_speedDemand != (long) ntohl(command.xspeed))
      {
        newmotorspeed = true;
        mmPsec_speedDemand = (long) ntohl(command.xspeed);
      }
      if(radPsec_turnRateDemand != DEG2RAD_CONV((long) ntohl(command.yawspeed)))
      {
        newmotorturn = true;
        radPsec_turnRateDemand = DEG2RAD_CONV((long) ntohl(command.yawspeed));
      }
      /* NEXT, write commands */
      // rflex has a built in failsafe mode where if no move command is recieved in a 
      // certain interval the robot stops.
      // this is a good thing given teh size of the robot...
      // if network goes down or some such and the user looses control then the robot stops
      // if the robot is running in an autonomous mdoe it is easy enough to simply 
      // resend the command repeatedly

      // allow rflex joystick to overide the player command
      if (joy_control > 0)
        --joy_control;
      // only set new command if type is valid and their is a new command
      else if (command.type == 0)
      {
        rflex_set_velocity(rflex_fd,(long) MM2ARB_ODO_CONV(mmPsec_speedDemand),(long) RAD2ARB_ODO_CONV(radPsec_turnRateDemand),(long) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration));    
        command.type = 255;
        PutCommand(this->position_id,(unsigned char *)&command, 
                   sizeof(command), NULL);
      }
    }
    else
      rflex_stop_robot(rflex_fd,(long) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration));

    /* Get data from robot */
    player_rflex_data_t rflex_data;
    memset(&rflex_data,0,sizeof(player_rflex_data_t));
    update_everything(&rflex_data);
    pthread_testcancel();

    PutData(this->position_id,
            (void*)&rflex_data.position,
            sizeof(player_position_data_t),
            NULL);
    PutData(this->sonar_id,
            (void*)&rflex_data.sonar,
            sizeof(player_sonar_data_t),
            NULL);
    PutData(this->sonar_id_2,
            (void*)&rflex_data.sonar2,
            sizeof(player_sonar_data_t),
            NULL);
    PutData(this->ir_id,
            (void*)&rflex_data.ir,
            sizeof(player_ir_data_t),
            NULL);
    PutData(this->bumper_id,
            (void*)&rflex_data.bumper,
            sizeof(player_bumper_data_t),
            NULL);
    PutData(this->power_id,
            (void*)&rflex_data.power,
            sizeof(player_power_data_t),
            NULL);
    PutData(this->aio_id,
            (void*)&rflex_data.aio,
            sizeof(player_aio_data_t),
            NULL);
    PutData(this->dio_id,
            (void*)&rflex_data.dio,
            sizeof(player_dio_data_t),
            NULL);

    pthread_testcancel();
  }
  pthread_exit(NULL);
}

int RFLEX::initialize_robot(){
#ifdef _REENTRANT
  if (thread_is_running)
    {
      fprintf(stderr,"Tried to connect to the robot while the command loop "
                  "thread is running.\n");
      fprintf(stderr,"This is a bug in the code, and must be fixed.\n");
      return -1;
    }
#endif

  if (rflex_open_connection(rflex_configs.serial_port, &rflex_fd) < 0)
    return -1;
  
  rflex_initialize(rflex_fd, (int) MM2ARB_ODO_CONV(rflex_configs.mmPsec2_trans_acceleration),(int) RAD2ARB_ODO_CONV(rflex_configs.radPsec2_rot_acceleration), 0, 0);

  return 0;
}

void RFLEX::reset_odometry() {
  mm_odo_x=0;
  mm_odo_y=0;
  rad_odo_theta= 0.0;
}

void RFLEX::set_odometry(long mm_x, long mm_y, short deg_theta) {
  mm_odo_x=mm_x;
  mm_odo_y=mm_y;
  rad_odo_theta=DEG2RAD_CONV(deg_theta);
}

void RFLEX::update_everything(player_rflex_data_t* d)
{

  int arb_ranges[PLAYER_SONAR_MAX_SAMPLES];
  char abumper_ranges[PLAYER_BUMPER_MAX_SAMPLES];
  unsigned char air_ranges[PLAYER_IR_MAX_SAMPLES];

  // changed these variable-size array declarations to the 
  // bigger-than-necessary ones above, because older versions of gcc don't
  // support variable-size arrays.
  // int arb_ranges[rflex_configs.num_sonars];
  // char abumper_ranges[rflex_configs.bumper_count];
  // unsigned char air_ranges[rflex_configs.ir_poses.pose_count];

  static int initialized = 0;

  double mm_new_range_position; double rad_new_bearing_position;
  double mmPsec_t_vel;
  double radPsec_r_vel;

  int arb_t_vel, arb_r_vel;
  static int arb_last_range_position;
  static int arb_last_bearing_position;
  int arb_new_range_position;
  int arb_new_bearing_position;
  double mm_displacement;
  short a_num_sonars, a_num_bumpers, a_num_ir;

  int batt,brake;

  int i;

  //update status
  rflex_update_status(rflex_fd, &arb_new_range_position,
                      &arb_new_bearing_position, &arb_t_vel,
                      &arb_r_vel);
  mmPsec_t_vel=ARB2MM_ODO_CONV(arb_t_vel);
  radPsec_r_vel=ARB2RAD_ODO_CONV(arb_r_vel);
  mm_new_range_position=ARB2MM_ODO_CONV(arb_new_range_position);
  rad_new_bearing_position=ARB2RAD_ODO_CONV(arb_new_bearing_position);
  
  if (!initialized) {
    initialized = 1;
  } else {
    rad_odo_theta += ARB2RAD_ODO_CONV(arb_new_bearing_position - arb_last_bearing_position);
    rad_odo_theta = normalize_theta(rad_odo_theta);
    mm_displacement = ARB2MM_ODO_CONV(arb_new_range_position - arb_last_range_position);

    //integrate latest motion into odometry
    mm_odo_x += mm_displacement * cos(rad_odo_theta);
    mm_odo_y += mm_displacement * sin(rad_odo_theta);
    d->position.xpos = htonl((long) mm_odo_x);
    d->position.ypos = htonl((long) mm_odo_y);
    while(rad_odo_theta<0)
      rad_odo_theta+=2*M_PI;
    d->position.yaw = htonl((long) RAD2DEG_CONV(rad_odo_theta) %360);

    d->position.xspeed = htonl((long) mmPsec_t_vel);
    d->position.yawspeed = htonl((long) RAD2DEG_CONV(radPsec_r_vel));
    //TODO - get better stall information (battery draw?)
  }
  d->position.stall = false;

  arb_last_range_position = arb_new_range_position;
  arb_last_bearing_position = arb_new_bearing_position;

   //note - sonar mappings are strange - look in rflex_commands.c
  if(this->sonar_subscriptions)
  {
    // TODO - currently bad sonar data is sent back to clients 
    // (not enough data buffered, so sonar sent in wrong order - missing intermittent sonar values - fix this
    a_num_sonars=rflex_configs.num_sonars;

    pthread_testcancel();
    rflex_update_sonar(rflex_fd, a_num_sonars,
		       arb_ranges);
    pthread_testcancel();
    d->sonar.range_count=htons(rflex_configs.sonar_1st_bank_end);
    for (i = 0; i < rflex_configs.sonar_1st_bank_end; i++){
      d->sonar.ranges[i] = htons((uint16_t) ARB2MM_RANGE_CONV(arb_ranges[i]));
    }
    d->sonar2.range_count=htons(rflex_configs.num_sonars - rflex_configs.sonar_2nd_bank_start);
    for (i = 0; i < rflex_configs.num_sonars - rflex_configs.sonar_2nd_bank_start; i++){
      d->sonar2.ranges[i] = htons((uint16_t) ARB2MM_RANGE_CONV(arb_ranges[rflex_configs.sonar_2nd_bank_start+i]));
    }
  }

  // if someone is subscribed to bumpers copy internal data to device
  if(this->bumper_subscriptions)
  {
    a_num_bumpers=rflex_configs.bumper_count;

    pthread_testcancel();
    // first make sure our internal state is up to date
    rflex_update_bumpers(rflex_fd, a_num_bumpers, abumper_ranges);
    pthread_testcancel();

    d->bumper.bumper_count=(a_num_bumpers);
    memcpy(d->bumper.bumpers,abumper_ranges,a_num_bumpers);
  }

  // if someone is subscribed to irs copy internal data to device
  if(this->ir_subscriptions)
  {
    a_num_ir=rflex_configs.ir_poses.pose_count;

    pthread_testcancel();
    // first make sure our internal state is up to date
    rflex_update_ir(rflex_fd, a_num_ir, air_ranges);
    pthread_testcancel();

    d->ir.range_count = htons(a_num_ir);
    for (int i = 0; i < a_num_ir; ++i)
    {
      d->ir.voltages[i] = htons(air_ranges[i]);
      // using power law mapping of form range = (a*voltage)^b
      int range = (int) (pow(rflex_configs.ir_a[i] *((double) air_ranges[i]),rflex_configs.ir_b[i]));
      // check for min and max ranges, < min = 0 > max = max
      range = range < rflex_configs.ir_min_range ? 0 : range;
      range = range > rflex_configs.ir_max_range ? rflex_configs.ir_max_range : range;
      d->ir.ranges[i] = htons(range);		
    }
  }

  //this would get the battery,time, and brake state (if we cared)
  //update system (battery,time, and brake also request joystick data)
  rflex_update_system(rflex_fd,&batt,&brake);
  d->power.charge = htons(static_cast<uint16_t> (batt/10) + rflex_configs.power_offset);
}

//default is for ones that don't need any configuration
void RFLEX::GetOptions(ConfigFile *cf,int section,rflex_config_t *configs){
  //do nothing at all
}

//this is so things don't crash if we don't load a device
//(and thus it's settings)
void RFLEX::set_config_defaults(){
  strcpy(rflex_configs.serial_port,"/dev/ttyR0");
  rflex_configs.mm_length=0.0;
  rflex_configs.mm_width=0.0;
  rflex_configs.odo_distance_conversion=0.0;
  rflex_configs.odo_angle_conversion=0.0;
  rflex_configs.range_distance_conversion=0.0;
  rflex_configs.mmPsec2_trans_acceleration=500.0;
  rflex_configs.radPsec2_rot_acceleration=500.0;
  rflex_configs.use_joystick=false;
  rflex_configs.joy_pos_ratio=0;
  rflex_configs.joy_ang_ratio=0;
  
  
  rflex_configs.max_num_sonars=0;
  rflex_configs.num_sonars=0;
  rflex_configs.sonar_age=0;
  rflex_configs.num_sonar_banks=0;
  rflex_configs.num_sonars_possible_per_bank=0;
  rflex_configs.num_sonars_in_bank=NULL;
  rflex_configs.mmrad_sonar_poses=NULL;
  
  rflex_configs.bumper_count = 0;
  rflex_configs.bumper_address = 0;
  rflex_configs.bumper_def = NULL;
  
  rflex_configs.ir_poses.pose_count = 0;
  rflex_configs.ir_base_bank = 0;
  rflex_configs.ir_bank_count = 0;
  rflex_configs.ir_count = NULL;
  rflex_configs.ir_a = NULL;
  rflex_configs.ir_b = NULL;
}



