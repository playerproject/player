// mother.cc - wrappers for the Nomad client functions
// copyright Pawel Zebrowski, Richard Vaughan 2004, released under the GPL.
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////
// include the nomadic 200 supplied file.  This file talks only
// to Nserver, which can either simulate the robot, or forward the
// commands to the real robot over a network medium.
#include "Nclient.h"

/////////////////////////////////////////////////////////////////////////
// include the nomadic 200 supplied file.  Either include Nclient.h or
// Ndirect.h, not both.  Ndirect.h allows you to communicate directly 
// with the robot, not requiring Nserver.  Use this for final product
// #include "Ndirect.h"


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

/////////////////////////////////////////////////////////////////////////
// connects to the robot and performs any other connection setup that
// may be required
void connectToRobot()
{
  printf( "connecting to %s:%d\n", SERVER_MACHINE_NAME, SERV_TCP_PORT );
  
  // connect to the robot (connection parameters specified in a 
  // supplementary file, i believe)
  //connect_robot(1,MODEL_N200, SERVER_MACHINE_NAME, SERV_TCP_PORT );
  connect_robot(1);
  
  // set the robot timeout, in seconds.  The robot will stop if no commands
  // are sent before the timeout expires.
  conf_tm(5);
}

/////////////////////////////////////////////////////////////////////////
// cleans up and disconnects from the robot
void disconnectFromRobot()
{
	// disconnect from the one and only robot
	disconnect_robot(1);
}

/////////////////////////////////////////////////////////////////////////
// make the robot speak
void speak(char* s)
{
	//say string s
	tk(s);
}

/////////////////////////////////////////////////////////////////////////
// initializes the robot
void initRobot()
{
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
}

/////////////////////////////////////////////////////////////////////////
// update sensor data, to be used before reading/processing sensor data
void readRobot()
{
	//update all sensors
	gs();
}

/////////////////////////////////////////////////////////////////////////
// set the robot speed, turnrate, and turret in velocity mode.  
// Convert units first
void setSpeed(int speed, int turnrate, int turret)
{
	vm(mmToInches(speed), turnrate*10, turret*10);
}

/////////////////////////////////////////////////////////////////////////
// set te robot speed and turnrate in velocity mode.  Make the turret
// turn with the robot.  Convert units first
void setSpeed(int speed, int turnrate)
{
	//the sensors are located on the turret, so to give the illusion of
	//not having a turret, turn the turret at the same rate as the base
	vm(mmToInches(speed), turnrate*10, turnrate*10);
}

/////////////////////////////////////////////////////////////////////////
// set the odometry of the robot.  Set the turret the same as the base
void setOdometry(int x, int y, int theta)
{

	//translation
	dp(mmToInches(x), mmToInches(y));

	//rotation, base and turret
	da(theta*10, theta*10);
}

/////////////////////////////////////////////////////////////////////////
// reset the odometry.
void resetOdometry()
{
	zr();
}

/////////////////////////////////////////////////////////////////////////
// retreive the x position of the robot
int xPos()
{
	//conver units and return
	return inchesToMM( State[ STATE_CONF_X ] );
}

int yPos()
{
	//conver units and return
	return inchesToMM( State[ STATE_CONF_Y ] );
}

int theta()
{
	//conver units and return
	return (int)(State[ STATE_CONF_STEER ] / 10);
}

int speed()
{
	//conver units and return
	return inchesToMM( State[ STATE_VEL_TRANS ] );
}

int turnrate()
{
	//conver units and return
	return (int)(State[ STATE_VEL_STEER ] / 10);
}

/////////////////////////////////////////////////////////////////////////
// updates sonarData array with the latest sonar data.  Converts units
void getSonar(int sonarData[16])
{
	for (int i = 0; i < 16; i++)
	{
		//get the sonar data, in inches, and convert it to mm
		sonarData[i] = inchesToMM( State[ STATE_SONAR_0 + i] );
	}
}
