/**
  *  Copyright (C) 2010
  *     Ana Teresa Hernández Malagón <anat.hernandezm@gmail.com>
  *	Movirobotics <athernandez@movirobotics.com>
  *  Videre Erratic robot driver for Player
  *                   
  *  Copyright (C) 2006
  *     Videre Design
  *  Copyright (C) 2000  
  *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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
**/

/**
	Movirobotic's mBase robot driver for Player 3.0.1 based on erratic driver developed by Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard. Developed by Ana Teresa Herández Malagón.
**/

#ifndef _ROBOT_PARAMS_H
#define _ROBOT_PARAMS_H

#include "libplayerinterface/player.h"

void initialize_robot_params(void);

#define PLAYER_NUM_ROBOT_TYPES 30


typedef struct
{
	double AngleConvFactor; // 
	const char* Class;
	double DiffConvFactor; // 
	double DistConvFactor; // 
	//NO
		int FrontBumpers; // 
		double GyroScaler; // 
		int HasMoveCommand; // 
		int Holonomic; // 
		int NumINFRADIG; // ; int IRNum; // -> Infra digitales
		int NumINFRAAN;//int IRUnit; // 
		int LaserFlipped; // 
		char* LaserIgnore;
		char* LaserPort;
		int LaserPossessed; // 
		int LaserPowerControlled; // 
		int LaserTh; // 
		int LaserX; // 
		int LaserY; // 
		int MaxRVelocity; // 
		int MaxVelocity; // 
		int NewTableSensingIR; // 
		int NumFrontBumpers; // 
		int NumRearBumpers; // 
		double RangeConvFactor; // 
		int RearBumpers; // 
		int RequestEncoderPackets; // 
		int RequestIOPackets; // 
		int RobotDiagonal; // 
	//FIN NO (0)
	int RobotLength; // mm
		int RobotRadius; // NO (0)
	int RobotWidth; // mm
	int RobotAxleOffset; // 
	int RotAccel; // 
		int RotDecel; // NO (0)
	int RotVelMax; // 
		int SettableAccsDecs; // NO (0)
		int SettableVelMaxes; // NO (0)
	const char* Subclass;
		int SwitchToBaudRate; // NO (0)
		int TableSensingIR; // NO (0)
	int TransAccel; // 
		int TransDecel; // NO (0)
	int TransVelMax; // 
	int Vel2Divisor; // 
	double VelConvFactor; // 
	int NumSonars;
	player_pose3d_t sonar_pose[32];
	int NumIR;
	player_pose3d_t IRPose[8];
} RobotParams_t;

extern RobotParams_t *robotParams[];

#endif
