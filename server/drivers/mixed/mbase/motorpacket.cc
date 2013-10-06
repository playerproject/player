// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:1; -*-

/**
  *  Copyright (C) 2010
  *     Ana Teresa Hernández Malagón <anat.hernandezm@gmail.com>
  *	Movirobotics <athernandez@movirobotics.com>
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

#include <stdio.h>
#include <limits.h>
#include <math.h>  /* rint(3) */
#include <sys/types.h>
#include <netinet/in.h>
#include "motorpacket.h"
#include <stdlib.h> /* for abs() */
#include <unistd.h>


/**
	Print
*/
void mbasedriverMotorPacket::Print() 
{
  printf("lwstall:%d rwstall:%d\n", lwstall, rwstall);

  printf("battery: %d\n", battery);
  printf("xpos: %d ypos:%d\n", xpos, ypos);
  printf("angle: %d lvel: %d rvel: %d\n", angle, lvel, rvel);
}

/**
	Parse
	-Parses and absorbs a standard packet from the robot
*/
bool mbasedriverMotorPacket::Parse( unsigned char *buffer, int length ) 
{
	//cnt empieza en 1 xq el 0 del buffer es el COMANDO
  	int cnt = 1;
	if(debug_receive_motor)	
	{
		printf("buffer[x]=	[0]	%02x	[1]	%02x	[2]	%02x	[3]	%02x\n",buffer[cnt], buffer[cnt+1], buffer[cnt+2], buffer[cnt+3]);
		printf("buffer[y]=	[4]	%02x	[5]	%02x	[6]	%02x	[7]	%02x\n",buffer[cnt+4], buffer[cnt+5], buffer[cnt+6], buffer[cnt+7]);
		printf("buffer[t]=	[8]	%02x	[9]	%02x\n",buffer[cnt+8], buffer[cnt+9]);
		printf("buffer[lvel]=	[10]	%02x	[11]	%02x\n",buffer[cnt+10], buffer[cnt+11]);
		printf("buffer[rvel]=	[12]	%02x	[13]	%02x\n",buffer[cnt+12], buffer[cnt+13]);
		printf("buffer[bat]=	[14]	%02x	[15]	%02x\n",buffer[cnt+14], buffer[cnt+15]);
		printf("buffer[stall]=	[16]	%02x\n",buffer[cnt+16]);
	}

	ypos = (long)(buffer[cnt] | (buffer[cnt+1] << 8) | (buffer[cnt+2] << 16) | (buffer[cnt+3] << 24)); //valor devuelto por el IOM
	cnt += 4;
	xpos = (long)(buffer[cnt] | (buffer[cnt+1] << 8) | (buffer[cnt+2] << 16) | (buffer[cnt+3] << 24)); //valor devuelto por el IOM
	cnt += 4;
	
	angle = (short)(buffer[cnt] | (buffer[cnt+1] << 8)); 
  	cnt += sizeof(short);
	lvel = (short) rint(((short)(buffer[cnt] | (buffer[cnt+1] << 8))) *robotParams[param_idx]->VelConvFactor);
  	cnt += sizeof(short);
	
	rvel = (short) rint(((short)(buffer[cnt] | (buffer[cnt+1] << 8))) *robotParams[param_idx]->VelConvFactor);
  	cnt += sizeof(short);
	
	battery = (int)(buffer[cnt] | (buffer[cnt+1] << 8));//buffer[cnt];
	//printf("bat buffer[%d]=	%02x	buffer[%d]= %02x	battery=%d\n",cnt, buffer[cnt],cnt+1, buffer[cnt+1], battery);	
  	cnt += sizeof(int);

	lwstall = 0;
  	rwstall = 0;
  	cnt += sizeof(unsigned char);

/**	Descomentar cuando el IOM este preparado para dar un valor correcto de stall
  	lwstall = buffer[cnt] & 0x01;
  	//cnt += sizeof(unsigned char);
  	rwstall = buffer[cnt] & 0x01;
  	cnt += sizeof(unsigned char);
*/

	if(debug_receive_motor)	
		printf("PARSE	xpos= %d	ypos= %d	angle= %d	lvel= %d	rvel= %d	battery %d	lwstall= %d\n",xpos, ypos, angle, lvel, rvel, battery, lwstall);

	return true;
}

/**
	Fill
	- Spits out information that was previously parsed
*/
void mbasedriverMotorPacket::Fill(player_mbasedriver_data_t* data)
{
	 // Odometry data
  	{
    		data->position.pos.px = (double)(this->xpos / 1e3);
    		data->position.pos.py = (double)(this->ypos / 1e3);     		data->position.pos.pa = GradToRad(this->angle);

   		data->position.vel.py = 0.0;
		//obtengo la velrot y veltrans (funcion inversa del veldrift)
		int veltransCalculado = (int)(ceil (static_cast<double>((this->lvel+this->rvel)/2)));
		data->position.vel.px = (double)veltransCalculado / 1e3;
		data->position.vel.pa = (((double)this->lvel-this->rvel)/(robotParams[param_idx]->RobotWidth));

    		data->position.stall = (unsigned char)(this->lwstall || this->rwstall);
	}

  	// Battery data
  	{
   		data->power.valid = PLAYER_POWER_MASK_VOLTS | PLAYER_POWER_MASK_PERCENT;
    		///De momento el IOM no devuleve voltios si no un valor que hay que transformar
		double R1 = 1.512;
		double R2 = 0.512;
		double dVol = (double)this->battery * 5.0 / pow(2.0, 12);
		data->power.volts = R1 * dVol / R2;
    		data->power.percent = 1e2 * (data->power.volts / VIDERE_NOMINAL_VOLTAGE);
	//	printf("FILL ODOM batt=%.2f	percent=%.2f\n",data->power.volts, data->power.percent);
  	}

/*	// gyro
	{
  		memset(&(data->gyro),0,sizeof(data->gyro));
  		data->gyro.pos.pa = (double)gyro_rate;//DTOR(this->gyro_rate);
	}

	//accel
	{
		memset(&(data->accel),0,sizeof(data->accel));
		data->accel.pos.px = (double)accel_x;
		data->accel.pos.py = (double)accel_y;
	}
*/

	if(debug_receive_motor)
		printf("FILL ODOM pos.px= %.2f	pos.py= %.2f	pos.pa= %.2f	vel.py= %.2f	vel.px= %.2f	vel.pa= %.2f	stall= %d batt=%.2f\n", data->position.pos.px, data->position.pos.py, data->position.pos.pa, data->position.vel.py, data->position.vel.px, data->position.vel.pa, data->position.stall, data->power.volts);
		//printf("FILL ODOM pos.px= %.2f	pos.py= %.2f	pos.pa= %.2f	vel.py= %.2f	vel.px= %.2f	vel.pa= %.2f	stall= %d batt=%.2f gyro=%.2f	acel_x=%.2f	acel_y=%.2f\n", data->position.pos.px, data->position.pos.py, data->position.pos.pa, data->position.vel.py, data->position.vel.px, data->position.vel.pa, data->position.stall, data->power.volts, data->gyro.pos.pa, data->accel.pos.px, data->accel.pos.py);
}

