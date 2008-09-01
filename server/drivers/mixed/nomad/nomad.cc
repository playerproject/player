/*     nomad.cc - Nomad 200 plugin driver for Player 2.0
 *     Copyright (C) 2008  Víctor Costa da Silva Campos - kozttah@gmail.com
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <string>
#include <libplayercore/playercore.h>
#include <cmath>
#include <iostream>
#include "Nclient.h"

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_nomad nomad
 * @brief NOMAD 200 driver

The nomad driver controls the Nomadics NOMAD200 robot, and should be easily
modified to support close relatives.

In order for you to use the driver, you must first open the Nserver
program in a computer. If you'd like to use the driver to control a real nomad,
make sure that the Nserver can connect to the robot (check the NOMAD 200 user's
manual for information on how to this). If you want to use the real robot and
you are running Nserver on Nomad, you can just run Nserver with the setup files
that came with it. Before running player, you should ld the module i200m
(modprobe i200m), run i200minit and then run robotd.

@par Provides

The p2os driver provides the following device interfaces, some of them named:

- @ref interface_position2d
- @ref interface_bumber
- @ref interface_sonar
- @ref interface_ir
- "turret" @ref interface_position1d
- "compass" @ref interface_position1d

@par Configuration file options

- NOMAD_TIMEOUT_S (integer)
  - Default: 5
  - time (s) between commands to the server that shuts down the nomad - must be
    bigger than cycle time
- CYCLE_TIME_US (integer)
  - Default: 100000
  - time (us) between reading cycles
- NOMAD_MAX_VEL_TRANS (integer)
  - Default: 200
  - maximum translational speed allowed (tenths of inch/s)
- NOMAD_MAX_ACC_TRANS (integer)
  - Default: 300
  - maximum translational acceleration allowed (tenths of inch/s²)
- NOMAD_MAX_VEL_STEER (integer)
  - Default: 450
  - maximum steering and turret speed allowed (tenths of degree/s)
- NOMAD_MAX_ACC_STEER (integer)
  - Default: 300
  - maximum steering and turret acceleration allowed (tenths of degree/s²)
- REAL_ROBOT (integer)
  - Default: 3
  - specifies whether to use real robot (0), same state that's configured in
    Nserver (3) or simulated robot(other)
- LOCKED (integer)
  - Default: 1
  - specifies whether the turret movement is locked (1) with the base movement
    or not (0)
  - LOCKED must be set to one in order to instantiate the turret control
- ZERO (integer)
  - Default: 1
  - specifies whether to zero (1) the robot on startup or not (0)
  - if you do set ZERO to 1, be sure that there's enough room around the robot
    when starting player
- HOST (string)
  - Default: "localhost"
  - the name of the computer running Nserver
- PORT (integer)
  - Default: 7019
  - the port of HOST in which Nserver is listening

@par Example

@verbatim
driver
(
  name "nomad_driver"
  provides ["position2d:0" "bumper:0" "sonar:0" "ir:0" "turret:::position1d:0" "compass:::position1d:1"]

  # Options
  NOMAD_TIMEOUT_S 5
  CYCLE_TIME_US 100000
  NOMAD_MAX_VEL_TRANS 200
  NOMAD_MAX_ACC_TRANS 300
  NOMAD_MAX_VEL_STEER 450
  NOMAD_MAX_ACC_STEER 300
  REAL_ROBOT 3
  LOCKED 1
  ZERO 1
  HOST "localhost"
  PORT 7019
)
@endverbatim

@author Víctor Campos
 */
/** @} */

//TODO add gripper support

using std::cout;
using std::endl;

//constants used in the code
const double nomadRadius = 0.225; //nomad radius in meters - measured
const double PI = 3.14159265; // aproximate value of pi

//a function that converts meters to tenths of inches
float m2tenths_inch(float num) {
  return num* 393.7;
};

//a function that converts tenths of inches in meters
float tenths_inch2m(float num) {
  return num*0.00254;
};

//a function that converts tenths of degrees in radians
float tenths_deg2rad(float num) {
  return num*0.00174532925;
};

//a function that converts radians in tenths of degrees
float rad2tenths_deg(float num) {
  return num*(1800/PI);
};



class NomadDriver : public Driver
{
public:
  //constructor
  NomadDriver(ConfigFile* cf, int section);

  //mandatory functions
  virtual int Setup(); //called when a client connects to a playe server using this driver
  virtual int Shutdown(); //called upon driver shutdown

  //invoked on each incoming message
  virtual int ProcessMessage(QueuePointer& resp_queue,player_msghdr *hdr,void * data);

  // Holders for desired velocities
  float VelTrans; //translational speed
  float VelSteer; //steering rate (base motor alignment rate)
  float VelTurret; //turret steering rate

  // Holder for position (odometry) data sent from below
  player_position2d_data_t posdata;

  // Holder for position (odometry) data sent from above
  player_position2d_set_odom_req odomCommand;

private:
  virtual void Main(); //main function
  void zeroOnStartup(); //zero the robot on startup
  player_devaddr_t position_addr; // holder for the position2d address in the device table
  player_devaddr_t bumper_addr; //holder for the bumper address in the device table
  player_devaddr_t sonar_addr; //holder for the sonar address in the device table
  player_devaddr_t ir_addr; //holder for the ir address in the device table
  player_devaddr_t turret_addr; //holder for the position1d that controls the turret
  player_devaddr_t compass_addr; //holder for the position1d that returns compass data

  static const unsigned int sonar_count = 16; //number of sonar sensors in the robot
  static const unsigned int ir_count = 16; //number of infrared sensors in the robot

  //Variables from config file
  int NOMAD_TIMEOUT_S; //timeout between commands before the nomad shuts down
  int NOMAD_MAX_VEL_TRANS; //maximum speed allowed for translation (in tenths of inch/s)
  int NOMAD_MAX_ACC_TRANS; //maximum acc allowed for translation (in tenths of inchs/s^2)
  int NOMAD_MAX_VEL_STEER; //maximum speed allowed for steering (in tenths of inchs/s)
  int NOMAD_MAX_ACC_STEER; //maximum acc allowed for steering (in tenths of inchs/s^2)
  int REAL_ROBOT; // indicates whether to use real, simulated, or the actual state of robot
  int LOCKED; // indicates whether the turret movement is locked with the base movement or not
  int ZERO; // indicates whether to zero the robot or not upon inicialization of player server
  unsigned int CYCLE_TIME_US; //time before the next cycle happens
  char* HOST; //name of the host running Nserver connect to the robot
  unsigned int PORT; //port running Nserver
};

//Function to instantiate the class
Driver* NomadDriver_Init(ConfigFile* cf, int section)
{
  return((Driver*)(new NomadDriver(cf, section)));
}

//Function that register the driver in the driver table
void nomad_Register (DriverTable * table)
{
  table->AddDriver ("nomad_driver", NomadDriver_Init);
}

//zero the robot on startup
void NomadDriver::zeroOnStartup(){
    //setup connection parameters
    SERV_TCP_PORT = PORT;
    strcpy(SERVER_MACHINE_NAME,HOST);

    //stabilish connection
    if(! connect_robot(1)) {
      cout << "couldn't connect in order to 'zero' the robot... aborting..." << endl;
      return;
    };
    cout << " zeroing the robot position...";
    zr();
    ws(TRUE,TRUE,TRUE,0);
    cout << " done" << endl;

    //close connection to the server
    disconnect_robot(1);
};

//constructor
NomadDriver::NomadDriver (ConfigFile * cf, int section):
  Driver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  // Read options from the configuration file
  this->NOMAD_TIMEOUT_S = cf->ReadInt(section,"NOMAD_TIMEOUT_S",5);
  this->NOMAD_MAX_VEL_TRANS = cf->ReadInt(section,"NOMAD_MAX_VEL_TRANS",200);
  this->NOMAD_MAX_ACC_TRANS = cf->ReadInt(section,"NOMAD_MAX_ACC_TRANS",300);
  this->NOMAD_MAX_VEL_STEER = cf->ReadInt(section,"NOMAD_MAX_VEL_STEER",450);
  this->NOMAD_MAX_ACC_STEER = cf->ReadInt(section,"NOMAD_MAX_ACC_STEER",300);
  this->REAL_ROBOT = cf->ReadInt(section,"REAL_ROBOT",0);
  this->LOCKED = cf->ReadInt(section,"LOCKED",1);
  this->ZERO = cf->ReadInt(section,"ZERO",1);
  this->CYCLE_TIME_US = cf->ReadInt(section, "CYCLE_TIME_US", 100000);
  this->HOST = const_cast<char*>(cf->ReadString(section, "HOST", "localhost"));
  this->PORT = cf->ReadInt(section, "PORT", 7019);

  //zero the address holders
  memset (&this->position_addr, 0, sizeof (player_devaddr_t));
  memset (&this->bumper_addr, 0, sizeof (player_devaddr_t));
  memset (&this->sonar_addr, 0, sizeof(player_devaddr_t));
  memset (&this->ir_addr, 0, sizeof(player_devaddr_t));
  memset (&this->turret_addr,0,sizeof(player_devaddr_t));
  memset (&this->compass_addr,0,sizeof(player_devaddr_t));

  // Do we create a position2d interface?
  if (cf->ReadDeviceAddr(&(this->position_addr),section,"provides",PLAYER_POSITION2D_CODE,-1, NULL) == 0)
    {
      if (this->AddInterface(this->position_addr) !=0)
	{
	  this->SetError (-1);
	  return;
	}
    }

  // Do we create a bumper interface?
  if (cf->ReadDeviceAddr (&(this->bumper_addr),section,"provides",PLAYER_BUMPER_CODE, -1, NULL) == 0)
    {
      if (this->AddInterface (this->bumper_addr) != 0)
	{
	  this->SetError (-1);
	  return;
	}
    }

   // Do we create a sonar interface?
   if(cf->ReadDeviceAddr(&(this->sonar_addr), section, "provides",PLAYER_SONAR_CODE, -1, NULL) == 0)
     {
       if(this->AddInterface(this->sonar_addr) != 0)
 	{
 	  this->SetError(-1);
 	  return;
 	}
     }

   // Do we create an ir interface?
   if(cf->ReadDeviceAddr(&(this->ir_addr), section, "provides",PLAYER_IR_CODE, -1, NULL) == 0)
     {
       if(this->AddInterface(this->ir_addr) != 0)
 	{
 	  this->SetError(-1);
 	  return;
 	}
     }

  // Do we create a position1d interface (turret control)?
  if ((cf->ReadDeviceAddr(&(this->turret_addr),section,"provides",PLAYER_POSITION1D_CODE,-1,"turret") == 0) && (!LOCKED))
    {
      if (this->AddInterface(this->turret_addr) !=0)
	{
	  this->SetError (-1);
	  return;
	}
    }

  // Do we create a position1d interface (compass - return values only)?
  if ((cf->ReadDeviceAddr(&(this->compass_addr),section,"provides",PLAYER_POSITION1D_CODE,-1,"compass") == 0) && (!LOCKED))
    {
      if (this->AddInterface(this->compass_addr) !=0)
	{
	  this->SetError (-1);
	  return;
	}
    }

  // Do we "zero" the robot?
  if (ZERO) {
    this->zeroOnStartup();
  }
};

/// Set up the device.
/**
    Return 0 if things go well, and -1 otherwise.
*/
int NomadDriver::Setup ()
{
  cout << endl << "Nomad 200 :: Driver initialising" << endl;
  cout << "Nomad 200:: WARNING!!! - make sure there's enough free space around the robot"
       << endl;
  cout << endl << "Nomad 200 :: Connecting...";

  //setup connection parameters
  SERV_TCP_PORT = PORT;
  strcpy(SERVER_MACHINE_NAME,HOST);

  //stabilish connection
  if(! connect_robot(1)) {
    cout << "Nomad 200 :: couldn't connect... aborting..." << endl;
    return -1;
  };
  cout << " done" << endl;

  //setup nomad atributes
  cout << "Nomad 200:: Configuring...";
  if (REAL_ROBOT!=3) {
    if (REAL_ROBOT) {
      real_robot();
    }
    else {
      simulated_robot();
    };
  };
  conf_tm(NOMAD_TIMEOUT_S);
  ac(NOMAD_MAX_ACC_TRANS,NOMAD_MAX_ACC_STEER,NOMAD_MAX_ACC_STEER);
  sp(NOMAD_MAX_VEL_TRANS,NOMAD_MAX_VEL_STEER,NOMAD_MAX_VEL_STEER);

  //! Start the device thread; spawns a new thread and executes
  //! NomadDriver::Main(), which contains the main loop for the driver.
  StartThread ();

  // Initialize the holders for desired velocities
  VelTrans = 0;
  VelSteer = 0;
  VelTurret = 0;

  // Initialize the holder for position (odometry) data
  memset(&posdata,0,sizeof(posdata));

  return (0);
};

/// Shutdown the device
int NomadDriver::Shutdown ()
{
  cout << endl << "Nomad 200 :: Shutting driver down";
  //! Stop and join the driver thread
  StopThread ();
  //making sure that the robot stops when shutting down the device
  st();
  ws(TRUE,TRUE,TRUE,0);

  disconnect_robot(1);

  cout << endl << "Nomad 200 :: Shutting driver down - DONE" << endl;

  return (0);
};

/// Process messages
int NomadDriver::
ProcessMessage (QueuePointer &resp_queue,player_msghdr * hdr,void *data)
{
  //! Send a response if necessary, using Publish().
  //! If you handle the message successfully, return 0.  Otherwise,
  //! return -1, and a NACK will be sent for you, if a response is required.

  // Interface - position2d
  if (Message::MatchMessage (hdr,PLAYER_MSGTYPE_CMD,
			     PLAYER_POSITION2D_CMD_VEL,
			     this->position_addr))
    {
      // Get and send the latest motor commands
      player_position2d_cmd_vel_t position_cmd;
      position_cmd = *(player_position2d_cmd_vel_t *)data;

      position_cmd.vel.px = m2tenths_inch(position_cmd.vel.px); //[tenths of inches/s]
      position_cmd.vel.py = 0;
      position_cmd.vel.pa = rad2tenths_deg(position_cmd.vel.pa); //[tenths of degrees/s]

      // Desired velocities are stored
      VelTrans = position_cmd.vel.px; //[tenths of inches/s]
      VelSteer = position_cmd.vel.pa; //[tenths of degrees/s]

      //setting the desired velocities
      //if the movement of the turret is locked, use the same speed for base and turret
      if (LOCKED) {
        VelTurret = position_cmd.vel.pa;
        vm(static_cast<int>(position_cmd.vel.px),
           static_cast<int>(position_cmd.vel.pa),
           static_cast<int>(position_cmd.vel.pa));
      }
      else {
        vm(static_cast<int>(position_cmd.vel.px),
           static_cast<int>(position_cmd.vel.pa),
           static_cast<int>(VelTurret));
      };
      return(0);
    }
    // Command from above to set the odometry to a particular value
    else if (Message::MatchMessage (hdr,PLAYER_MSGTYPE_REQ,
			     PLAYER_POSITION2D_REQ_SET_ODOM,
			     this->position_addr))
    {
    // Get the desired odometry value
      player_position2d_data position_data;
      position_data = *(player_position2d_data *)data;

      position_data.pos.px = m2tenths_inch(position_data.pos.px); //[tenths of inches]
      position_data.pos.py = m2tenths_inch(position_data.pos.py); //[tenths of inches];
      position_data.pos.pa = rad2tenths_deg(position_data.pos.pa); //[tenths of degrees]

      //Set the desired odometry values...
      dp(static_cast<int>(position_data.pos.px),static_cast<int>(position_data.pos.py));
      if(LOCKED) {
        da(static_cast<int>(position_data.pos.pa),static_cast<int>(position_data.pos.pa));
      }
      else {
        da(static_cast<int>(position_data.pos.pa),State[STATE_CONF_TURRET]);
      }
		this->Publish(this->position_addr, resp_queue,
		    PLAYER_MSGTYPE_RESP_ACK,
		    PLAYER_POSITION2D_REQ_SET_ODOM);

        return (0);
	}
  else if (Message::MatchMessage (hdr,
				  PLAYER_MSGTYPE_REQ,
				  PLAYER_POSITION2D_REQ_GET_GEOM,
				  this->position_addr))
    {
      //reply to the request for the robot geometry
      //set up the geom data structure with the robot geometry
      player_position2d_geom_t geom;

      geom.pose.px = 0;
      geom.pose.py = 0;
      geom.pose.pz = 0;
      geom.pose.proll = 0;
      geom.pose.ppitch = 0;
      geom.pose.pyaw = 0;

      geom.size.sl = nomadRadius*2;
      geom.size.sw = nomadRadius*2;
      //sends the robot geometry to above
      this->Publish (this->position_addr,
		     resp_queue,
		     PLAYER_MSGTYPE_RESP_ACK,
		     PLAYER_POSITION2D_REQ_GET_GEOM,
		     (void *) &geom,
		     sizeof (geom),
		     NULL);

      return (0);
    }

    //Interface - bumper
    else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                PLAYER_BUMPER_REQ_GET_GEOM,
                                this->bumper_addr))
  {
      //reply to the request for the bumper geometry
      //set up the geom data structure with the bumper geometry
    player_bumper_geom_t geom;

    geom.bumper_def_count = 20;

    for (int i = 0; i < 20; ++i) {
      //bumper pose
      geom.bumper_def[i].pose.px = (nomadRadius)*cos((2*PI/20)*i);
      geom.bumper_def[i].pose.py = (nomadRadius)*sin((2*PI/20)*i);
      geom.bumper_def[i].pose.pz = 0;
      geom.bumper_def[i].pose.proll = 0;
      geom.bumper_def[i].pose.ppitch = 0;
      geom.bumper_def[i].pose.pyaw = (2*PI/20)*i;
      //bumper length
      geom.bumper_def[i].length = ((nomadRadius)*2*PI)/10 ;
      //bumper radius of curvature ??? - not sure about it
      geom.bumper_def[i].radius = (nomadRadius);
    };
    //sends the bumper geometry to above
    this->Publish(this->bumper_addr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_BUMPER_REQ_GET_GEOM,
                  (void*)&geom, sizeof(geom), NULL);

    return(0);
   }

  // Interface - sonar
  else if (Message::MatchMessage (hdr,PLAYER_MSGTYPE_REQ,
				  PLAYER_SONAR_REQ_GET_GEOM,
				  this->sonar_addr))
    {
      //reply to the request for the sonar geometry
      //set up the geom data structure with the sonar geometry
      player_sonar_geom_t geom;
      geom.poses_count = sonar_count;

      float sonar_ang = 0;

      //if the turret movement isn't locked add a delta to the sonars position
      if (!LOCKED) {
        sonar_ang = m2tenths_inch(State[STATE_CONF_TURRET]);
      };
      //set up sonars geometry
      for(unsigned int intCount = 0;intCount < geom.poses_count;++intCount)
      {
        geom.poses[intCount].proll = 0;
        geom.poses[intCount].ppitch = 0;
        geom.poses[intCount].pyaw = (intCount*((2*PI)/16)) + sonar_ang;
        geom.poses[intCount].px = cos(geom.poses[intCount].pyaw)*(nomadRadius);
        geom.poses[intCount].py = sin(geom.poses[intCount].pyaw)*(nomadRadius);
        geom.poses[intCount].pz = 0;
      };
      //sends sonar geometry to above
      this->Publish(this->sonar_addr, resp_queue,
		    PLAYER_MSGTYPE_RESP_ACK,
		    PLAYER_SONAR_REQ_GET_GEOM,
		    (void *) &geom,
		    sizeof(geom),
		    NULL);
      return (0);
    }

  // Interface - ir
  else if (Message::MatchMessage (hdr,PLAYER_MSGTYPE_REQ,
				 PLAYER_IR_REQ_POSE,
				  this->ir_addr))
    {
      //reply to the request for the ir geometry
      //set up the geom data structure with the ir geometry
      player_ir_pose_t geom;
      geom.poses_count = ir_count;

      float ir_ang = 0;

      //if the turret movement isn't locked add a delta to the ir's position
      if (!LOCKED) {
        ir_ang = m2tenths_inch(State[STATE_CONF_TURRET]);
      };

      //set up ir geometry
      for(unsigned int intCount = 0;intCount < geom.poses_count;++intCount)
      {
        geom.poses[intCount].proll = 0;
        geom.poses[intCount].ppitch = 0;
        geom.poses[intCount].pyaw = intCount*((2*PI)/16) + ir_ang;
        geom.poses[intCount].px = cos(geom.poses[intCount].pyaw)*(nomadRadius);
        geom.poses[intCount].py = sin(geom.poses[intCount].pyaw)*(nomadRadius);
        geom.poses[intCount].pz = 0;
      };

      //send ir geometry to above
      this->Publish(this->ir_addr, resp_queue,
		    PLAYER_MSGTYPE_RESP_ACK,
		    PLAYER_IR_REQ_POSE,
		    (void *) &geom,
		    sizeof(geom),
		    NULL);
      return (0);
    }

// Interface - position1d - turret control
  else if (Message::MatchMessage (hdr,PLAYER_MSGTYPE_CMD,
			     PLAYER_POSITION1D_CMD_VEL,
			     this->turret_addr))
    {
      // Get and send the latest motor commands
      player_position1d_cmd_vel_t vel_cmd;
      vel_cmd = *(player_position1d_cmd_vel_t *)data;

      vel_cmd.vel = rad2tenths_deg(vel_cmd.vel); //[tenths of degrees/s]

      // Desired velocities are stored
      VelTurret = vel_cmd.vel; //[tenths of degrees/s]

      //setting the desired velocities
      vm(static_cast<int>(VelTrans),
         static_cast<int>(VelSteer),
         static_cast<int>(VelTurret));
      return(0);
    }
    // TODO add position1d position control too
    // Command from above to set the odometry to a particular value
    else if (Message::MatchMessage (hdr,PLAYER_MSGTYPE_REQ,
			     PLAYER_POSITION1D_REQ_SET_ODOM,
			     this->turret_addr))
    {
    // Get the desired odometry value
      player_position1d_set_odom_req_t position_data;
      position_data = *(player_position1d_set_odom_req_t *)data;
      position_data.pos = rad2tenths_deg(position_data.pos); //[tenths of degrees]

      //Set the desired odometry values...
      da(State[STATE_CONF_STEER],static_cast<int>(position_data.pos));

		this->Publish(this->turret_addr, resp_queue,
		    PLAYER_MSGTYPE_RESP_ACK,
		    PLAYER_POSITION1D_REQ_SET_ODOM);

        return (0);
    }
  else if (Message::MatchMessage (hdr,
				  PLAYER_MSGTYPE_REQ,
				  PLAYER_POSITION1D_REQ_GET_GEOM,
				  this->turret_addr))
    {
      player_position1d_geom_t geom;

      geom.pose.px = 0;
      geom.pose.py = 0;
      geom.pose.pz = 0;
      geom.pose.proll = 0;
      geom.pose.ppitch = 0;
      geom.pose.pyaw = 0;

      geom.size.sl = nomadRadius*2;
      geom.size.sw = nomadRadius*2;

      this->Publish (this->turret_addr,
		     resp_queue,
		     PLAYER_MSGTYPE_RESP_ACK,
		     PLAYER_POSITION2D_REQ_GET_GEOM,
		     (void *) &geom,
		     sizeof (geom),
		     NULL);

      return (0);
    }
  // Interface - position1d - compass
  else if (Message::MatchMessage (hdr,
				  PLAYER_MSGTYPE_REQ,
				  PLAYER_POSITION1D_REQ_GET_GEOM,
				  this->compass_addr))
    {
      player_position1d_geom_t geom;

      geom.pose.px = 0;
      geom.pose.py = 0;
      geom.pose.pz = 0;
      geom.pose.proll = 0;
      geom.pose.ppitch = 0;
      geom.pose.pyaw = 0;

      geom.size.sl = nomadRadius*2;
      geom.size.sw = nomadRadius*2;
      //sends compass geometry to above
      this->Publish (this->compass_addr,
		     resp_queue,
		     PLAYER_MSGTYPE_RESP_ACK,
		     PLAYER_POSITION2D_REQ_GET_GEOM,
		     (void *) &geom,
		     sizeof (geom),
		     NULL);

      return (0);
    }

  // the msg wasn't expected
  else
		PLAYER_ERROR("Nomad 200:: Unhandled message");
    return (-1);
};

void NomadDriver::Main() {

  //set all data holders to zero
  player_position2d_data pos_data;
  memset(&pos_data,0,sizeof(pos_data));

  player_bumper_data_t bumperdata;
  memset(&bumperdata,0,sizeof(bumperdata));

  player_sonar_data_t sonar_data;
  memset(&sonar_data,0,sizeof(sonar_data));

  player_ir_data_t ir_data;
  memset(&ir_data,0,sizeof(ir_data));

  player_position1d_data_t turret_data;
  memset(&turret_data,0,sizeof(turret_data));

  player_position1d_data_t compass_data;
  memset(&compass_data,0,sizeof(compass_data));

  while(true) {
    // Test if we are supposed to cancel
    pthread_testcancel();

    // Process incoming messages
    ProcessMessages();

    //updates State vector that comes from NOMAD 200
    gs();

    //update position and velocities
    pos_data.pos.px = tenths_inch2m(State[STATE_CONF_X]);
    pos_data.pos.py = tenths_inch2m(State[STATE_CONF_Y]);
    pos_data.pos.pa = tenths_deg2rad(State[STATE_CONF_STEER]);
    pos_data.vel.px = tenths_inch2m(State[STATE_VEL_TRANS]);
    pos_data.vel.py = 0;
    pos_data.vel.pa = tenths_deg2rad(State[STATE_VEL_STEER]);

    this->Publish (this->position_addr,
         PLAYER_MSGTYPE_DATA,PLAYER_POSITION2D_DATA_STATE,
         (void *)&pos_data,sizeof (pos_data),NULL);


    //update turret data
    turret_data.pos = tenths_deg2rad(State[STATE_CONF_TURRET]);
    turret_data.vel = tenths_deg2rad(State[STATE_VEL_TURRET]);
    this->Publish(this->turret_addr,PLAYER_MSGTYPE_DATA,PLAYER_POSITION1D_DATA_STATE,(void*)&turret_data,sizeof(turret_data),NULL);

    //update compass data
    compass_data.pos = tenths_deg2rad(State[STATE_COMPASS]);
    compass_data.vel = 0;
    this->Publish(this->compass_addr,PLAYER_MSGTYPE_DATA,PLAYER_POSITION1D_DATA_STATE,(void*)&compass_data,sizeof(compass_data),NULL);

    // Update bumper data
    int bumper = State[STATE_BUMPER];
    bumperdata.bumpers_count = 20;
    for(int i = 0; i < 20; ++i) {
      if (bumper & (1 << i))
        bumperdata.bumpers[i] = 1;
      else
        bumperdata.bumpers[i] = 0;
    }

    this->Publish(this->bumper_addr,
         PLAYER_MSGTYPE_DATA, PLAYER_BUMPER_DATA_STATE,
         (void*)&bumperdata, sizeof(bumperdata), NULL);

    //update sonar data
    sonar_data.ranges_count = sonar_count;
    for(unsigned int i = 0; i < sonar_count; ++i) {
      sonar_data.ranges[i] = tenths_inch2m(10*State[STATE_SONAR_0 + i]);
    };

    this->Publish(this->sonar_addr,
          PLAYER_MSGTYPE_DATA, PLAYER_SONAR_DATA_RANGES,
         (void*)&sonar_data, sizeof(sonar_data), NULL);

    //update ir data
    ir_data.ranges_count = ir_count;
    for(unsigned int i = 0; i < ir_count; ++i) {
      ir_data.ranges[i] = tenths_inch2m(10*State[STATE_IR_0 + i]);
    };

    this->Publish(this->ir_addr,
          PLAYER_MSGTYPE_DATA, PLAYER_IR_DATA_RANGES,
         (void*)&ir_data, sizeof(ir_data), NULL);

    // Sleep (you might, for example, block on a read() instead)
    usleep (this->CYCLE_TIME_US);
};

};
