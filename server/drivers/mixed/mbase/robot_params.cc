/**
 *  Copyright (C) 2010
 *     Ana Teresa Hernández Malagón <anat.hernandezm@gmail.com>
 *     Movirobotics <athernandez@movirobotics.com>
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

/**
	Movirobotic's mBase robot driver for Player 3.0.1 based on erratic driver developed by Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard. Developed by Ana Teresa Herández Malagón.
**/

#include "robot_params.h"
#include <math.h>

#define DTOR(x) (M_PI * x / 180.0)

RobotParams_t mbasedriver_params =
{
	0.001534,	//AngleConvFactor; // 
	"Erratic",	//const char* Class;
	0.011,		//double DiffConvFactor; // 
	0.780,		//double DistConvFactor; // 
		//NO
		0,	//int FrontBumpers;
		0,	//double GyroScaler; // 
		0,	//int HasMoveCommand; // 
		0,	//int Holonomic; // 
		2,	//NumINFRADIG	infradig	int IRNum; // 
		3,	//NumINFRAAN	//int IRUnit; // 
		0,	//int LaserFlipped; // 
		0,	//char* LaserIgnore;
		0,	//char* LaserPort;
		0,	//int LaserPossessed; // 
		0,	//int LaserPowerControlled; // 
		0,	//int LaserTh; // 
		0,	//int LaserX; // 
		0,	//int LaserY; // 
		0,	//int MaxRVelocity; // 
		0,	//int MaxVelocity; // 
		0,	//int NewTableSensingIR; // 
		0,	//int NumFrontBumpers; // 
		0,	//int NumRearBumpers; // 
		0,	//double RangeConvFactor; // 
		0,	//int RearBumpers; // 
		0,	//int RequestEncoderPackets; // 
		0,	//int RequestIOPackets; // 
		0,	//int RobotDiagonal; // 
		//FIN NO (0)
	540,		//int RobotLength; // mm
		0,	//int RobotRadius; // NO (0)
	410,		//int RobotWidth; // mm
	230,		//int RobotAxleOffset; //  mm
	0,		//int RotAccel; // 
		0,	//int RotDecel; // NO (0)
	0,		//int RotVelMax; // 
		0,	//int SettableAccsDecs; // NO (0)
		0,	//int SettableVelMaxes; // NO (0)
	"MBase",	//const char* Subclass;
		0,	//int SwitchToBaudRate; // NO (0)
		0,	//int TableSensingIR; // NO (0)
	0,		//int TransAccel; // 
		0,	//int TransDecel; // NO (0)
	0,		//int TransVelMax; // 
	20,		//int Vel2Divisor; // 
	1.20482,	//double VelConvFactor; // 
	8,		//int NumSonars;
			//player_pose_t sonar_pose[32];
	{		// sonar poses, in m and rads
			//player 2.0.5
		//player 3.0.1
		/*{ 0.180, 0.110,  0.0, 0.0, 0.0, DTOR(90) },
			{ 0.200, 0.085,  0.0, 0.0, 0.0, DTOR(0) },
			{ 0.190, 0.065,  0.0, 0.0, 0.0, DTOR(-53) },
			{ 0.170, 0.045,  0.0, 0.0, 0.0, DTOR(-24) },
			{ 0.180, -0.110, 0.0, 0.0, 0.0, DTOR(-90) },
			{ 0.200, -0.085, 0.0, 0.0, 0.0, DTOR( 0) },
			{ 0.190, -0.065, 0.0, 0.0, 0.0, DTOR( 53) },
			{ 0.170, -0.045, 0.0, 0.0, 0.0, DTOR( 24) },*/
		{ 0.1662,	-0.0129,	0.0, 0.0, 0.0,	DTOR(-90) },
		{ 0.1894,	-0.0102,  	0.0, 0.0, 0.0,	DTOR(-59.83) },
		{ 0.2063, 	-0.0053,  	0.0, 0.0, 0.0,	DTOR(-34.83) },
		{ 0.2063, 	-0.0006,  	0.0, 0.0, 0.0,	DTOR(-9.83) },
		{ 0.2063,	0.0006, 	0.0, 0.0, 0.0,	DTOR(9.83) },
		{ 0.2063, 	0.0053, 	0.0, 0.0, 0.0,	DTOR(34.83) },
		{ 0.1894, 	0.0102, 	0.0, 0.0, 0.0,	DTOR(59.83) },
		{ 0.1662, 	0.0129, 	0.0, 0.0, 0.0,	DTOR(90) },
	},
	5,		//int NumIR;
			//player_pose_t IRPose[8];
		{
			{0.023,	0.085,	0.0, 0.0, 0.0,	0.0},	// ir poses in m and deg
			{0.023,	0.000, 	0.0, 0.0, 0.0,	0.0},
			{0.023, -0.085, 0.0, 0.0, 0.0,	0.0},
			{-0.5, 	0.000,	0.0, 0.0, 0.0,	DTOR(180)},
			{-0.25, -0.13,	0.0, 0.0, 0.0,	DTOR(-90)},		
		}
};

RobotParams_t *robotParams[1] = {&mbasedriver_params};

