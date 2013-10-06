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

/*
 * $Id: motorpacket.h 4308 2007-12-13 20:23:08Z gerkey $
 *
 * part of the P2OS parser.  methods for filling and parsing server
 * information packets (mbasedriverMotorPackets)
 */

/**
	Movirobotic's mBase robot driver for Player 3.0.1 based on erratic driver developed by Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard. Developed by Ana Teresa Herández Malagón.
**/

#ifndef _mbasedriverMotorPacket_H
#define _mbasedriverMotorPacket_H

#include <limits.h>

#include "mbasedriver.h"


class mbasedriverMotorPacket 
{
	private:
  		int param_idx; // index of our robot's data in the parameter table

 	public:
  		// these values are returned in every standard mbasedriverMotorPacket
  		bool lwstall, rwstall;
  		int battery;
  		short angle, lvel, rvel;
  		int xpos, ypos;

  		bool Parse( unsigned char *buffer, int length );
		void Print();
		void Fill(player_mbasedriver_data_t* data);

		mbasedriverMotorPacket(int idx) 
		{
			param_idx = idx;
			
			xpos = INT_MAX;
			ypos = INT_MAX;
		}
};

#endif
