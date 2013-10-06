/**
	*  Copyright (C) 2010
	*     Ana Teresa Hernández Malagón <anat.hernandezm@gmail.com>
	*     Movirobotics <athernandez@movirobotics.com>
	*  Copyright (C) 2006
	*     Videre Design
	*  Copyright (C) 2000  
	*     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
	*
	*  Videre mbasedriver robot driver for Player
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

#ifndef _PACKET_H
#define _PACKET_H

#include <string.h>
#include <libplayercore/playercore.h>
//#include <stdint.h>

#define PACKET_LEN 256

enum receive_result_e {
	success = 0,
	failure = 1,
	timeout = 2
};


class mbasedriverPacket 
{
	public:
  		unsigned char packet[PACKET_LEN];
  		unsigned char size;

  		uint16_t CalcChkSum();

  		void Print();
		void PrintHex();
		int Build( unsigned char *data, unsigned char datasize );
		int Send( int fd );
		int Receive( int fd, uint16_t wait = 30 );
		bool Check();
  
  		bool operator!= ( mbasedriverPacket p ) 
		{
    			if ( size != p.size) return(true);

    			if ( memcmp( packet, p.packet, size ) != 0 ) return (true);

    			return(false);
  		}
};

#endif
