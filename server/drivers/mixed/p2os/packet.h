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
 *   part of the P2OS parser.  this class has methods for building,
 *   printing, sending and receiving P2OS packets.
 *
 */

#ifndef _PACKET_H
#define _PACKET_H

#include <string.h>

#include <libplayercore/globals.h>
#include <libplayercore/wallclocktime.h>

#define PACKET_LEN 256

class P2OSPacket 
{
 public:
  unsigned char packet[PACKET_LEN];
  unsigned char size;
  double timestamp;

  int CalcChkSum();

  void Print();
  void PrintHex();
  int Build( unsigned char *data, unsigned char datasize );
  int Send( int fd );
  int Receive( int fd, bool ignore_checksum );
  bool Check( bool ignore_checksum = false );

  bool operator!= ( P2OSPacket p ) {
    if ( size != p.size) return(true);

    if ( memcmp( packet, p.packet, size ) != 0 ) return (true);

    return(false);
  }
};

#endif
