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
 * Driver for the Nomadics Nomad 200 robot. Should be easily adapted for other Nomads.
 * Authors: Richard Vaughan (vaughan@sfu.ca), Pawel Zebrowski (pzebrows@sfu.ca)
 *
 * Notes: this driver is an example of an alternative way to implement
 * a multi-interface driver. It is quite different to the P2OS driver.
 * 
 */

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_nomad nomad


The nomad driver controls the Nomadics NOMAD200 robot, and should be
easily modified to support close relatives.

This driver uses the Nomad-specific @ref player_interface_nomad
interface to talk over a serial port to a robot running Nomadics'
robot server program. The nomad interface gives access to most of
the Nomad's functionality. This driver also has a set of companion
drivers that provide generic (i.e. not robot-specific) Player
interfaces. These are: @ref player_driver_nomad_position (provides @ref
player_interface_position) and @ref player_driver_nomad_sonar (provides
@ref player_interface_sonar). The companion drivers rely on the nomad
driver to talk to the robot, so you MUST instantiate a nomad device in
your config file, followed by the devices using the companion drivers. See
below for an example config file umentation for the companion drivers
for example configfiles.

@par Compile-time dependencies

- none

@par Provides

- @ref player_interface_nomad

@par Requires

- none

@par Configuration requests

- none
  
@par Configuration file options

- serial_device (string)
  - Default: "/dev/ttyS0"
  - the serial port to which the Nomad is connected. This should match the setting on 
    the robot.

- serial_speed (integer)
  - Default: 9600
  - sets the speed (baud rate) of the serial port. This should match the setting on the robot.

@par Example 

@verbatim
driver
(
  name "nomad"
  provides ["nomad:0"]
  serial_device "/dev/ttyS0"
  serial_speed  "9600"
)

driver
(
  name "nomad_position"
  requires ["nomad:0"]
  provides ["position:0"]
)

driver
(
  name "nomad_sonar"
  requires ["nomad:0"]
  provides ["sonar:0"]
)

@endverbatim

@par Authors

Richard Vaughan, Pawel Zebrowski

*/
/** @} */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

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

#include <playertime.h>
extern PlayerTime* GlobalTime;

#define NOMAD_QLEN 5

// CHECK: is this the right number?
#define NOMAD_SONAR_RANGE_MAX_MM 5000

#include <driver.h>
#include <drivertable.h>

// this is the old 'official' Nomad interface code from Nomadics
// released GPL when Nomadics went foom.
#include "Nclient.h"


/////////////////////////////////////////////////////////////////////////
// converts tens of inches to millimeters
int inchesToMM(int inches)
{
  return (int)(inches * 2.54);
}

/////////////////////////////////////////////////////////////////////////
// converts millimeters to tens of inches
int mmToInches(int mm)
{
  return (int)(mm / 2.54);
}


class Nomad:public Driver 
{
  public:

  Nomad( ConfigFile* cf, int section);
  virtual ~Nomad();
  
  /* the main thread */
  virtual void Main();
  
  virtual int Setup();
  virtual int Shutdown();

protected:
  char* serial_device;
  int serial_speed;
};


// a factory creation function
Driver* Nomad_Init( ConfigFile* cf, int section)
{
  return((Driver*)(new Nomad( cf, section)));
}

// a driver registration function
void Nomad_Register(DriverTable* table)
{
  table->AddDriver("nomad",  Nomad_Init);
}

Nomad::Nomad( ConfigFile* cf, int section)
  : Driver(cf, section,  PLAYER_NOMAD_CODE, PLAYER_ALL_MODE,
           sizeof(player_nomad_data_t), 
           sizeof(player_nomad_cmd_t), 
           NOMAD_QLEN, NOMAD_QLEN )
{
  this->serial_device = (char*)cf->ReadString( section, "serial_device", NOMAD_SERIAL_PORT );
  this->serial_speed = cf->ReadInt( section, "serial_speed", NOMAD_SERIAL_BAUD );
  //this->server_host = cf->ReadString( section, "server_host", "" );
  //this->serverPort = cf->ReadInt( section, "server_port", 0 );
}

Nomad::~Nomad()
{
  puts( "Destroying Nomad driver" );
}

int Nomad::Setup()
{
  printf("Nomad Setup: connecting to robot... ");
  fflush(stdout);
  
  // connect to the robot 
  int success = FALSE;  
  if( SERV_TCP_PORT > 0 ) // if we were compiled using Nclient
    {
      printf( "TCP: %s:%d... ", SERVER_MACHINE_NAME, SERV_TCP_PORT ); 
      fflush(stdout);
      success = connect_robot(1); // uses default parameters
    }
  else
    {
      printf( "Serial: %s:%d... ", this->serial_device, this->serial_speed );
      fflush(stdout);
      success = connect_robot(1, MODEL_N200, this->serial_device, this->serial_speed );
    }
  
  if( !success )
    {
      printf( "Nomad setup failed!\n" );
      return( 1 ); 
    }
  
  // set the robot timeout, in seconds.  The robot will stop if no commands
  // are sent before the timeout expires.
  conf_tm(5);

  // zero all counters
  zr();
  
  // set the robots acceleration for translation, steering, and turret
  // ac (translation acc, steering acc, turret ac), which is measure 
  // in 0.1inches/s/s, where the maximum is 300 = 30inches/s/s for 
  // all axes.
  ac(300, 300, 300);
  
  // set the robots maximum speed in each axis
  // sp(translation speed, steering speed, turret speed)
  // measured in 0.1inches/s for translation and 0.1deg/s for steering
  // and turret.  Maximum values are (200, 450, 450)
  sp(200, 450, 450);
  
  /* now spawn reading thread */
  StartThread();

  puts( "Nomad setup done." );
  return(0);
}

int Nomad::Shutdown()
{
  StopThread();
  puts("Nomad has been shutdown");
  return(0);
}


void 
Nomad::Main()
{  
  unsigned char config[NOMAD_CONFIG_BUFFER_SIZE];
  
  for(;;)
    {
      // handle configs --------------------------------------------------------
      
      void* client;
      size_t config_size = 0;
      
      if((config_size = GetConfig(&client, (void*)config, 
                                  sizeof(config),NULL)))
	switch( config[0] )
	  {
	  default:
	    puts("Nomad got unknown config request");
	    if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL))
	      PLAYER_ERROR("failed to PutReply");
	    break;
	  }
      
      // handle commands--------------------------------------------------------
      
      /* read the latest Player client commands */
      player_nomad_cmd_t command;
      GetCommand((unsigned char*)&command, sizeof(command),NULL);
      
      /* write the command to the robot */      
      int v = ntohl(command.vel_trans);
      int w = ntohl(command.vel_steer);
      int turret = ntohl(command.vel_turret);
      printf( "command: vel_trans:%d vel_steer:%d turret:%d\n", v,w,turret ); 

      // set the speed
      vm(mmToInches(v), w*10, turret*10);


      // produce data-------------------------------------------------------

      /* read the latest data from the robot */
      //printf( "read data from robot" );
      gs(); // fills the structure 'State'
      
      player_nomad_data_t data;
      memset(&data,0,sizeof(data));
      
      data.x = inchesToMM( State[ STATE_CONF_X ] );
      data.y = inchesToMM( State[ STATE_CONF_Y ] );
      data.a = State[ STATE_CONF_STEER ] / 10;
      data.vel_trans = inchesToMM( State[ STATE_VEL_TRANS ] );
      data.vel_steer = State[ STATE_VEL_STEER ] / 10;
      data.vel_turret = State[ STATE_VEL_TURRET ] / 10;

      // player's byteswapping
      data.x = htonl(data.x);
      data.y = htonl(data.y);
      data.a = htonl(data.a);
      data.vel_trans = htonl(data.vel_trans);
      data.vel_trans = htonl(data.vel_steer);
      data.vel_turret = htonl(data.vel_turret);

      double sonar_scale = NOMAD_SONAR_RANGE_MAX_MM / 255.0;

      //printf( "Nomad sonar: " );
      for( int i=0; i<NOMAD_SONAR_COUNT; i++ )
	{
	  //printf( " %d", State[ STATE_SONAR_0 + i ] );
	  data.sonar[i] 
	    = htons((uint16_t)(sonar_scale*State[STATE_SONAR_0+i]));
	}
	  //puts("");

      PutData((uint8_t*)&data, sizeof(data),NULL);
      
      //usleep(1);

    }
  pthread_exit(NULL);
}


