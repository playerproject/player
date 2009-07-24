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

#ifndef RFLEX_IO_H
#define RFLEX_IO_H

#include "rflex-info.h"

#define MAX_NAME_LENGTH  256

long  bytesWaiting( int sd );

void  DEVICE_set_params( RFLEX_Device dev );

void  DEVICE_set_baudrate( RFLEX_Device dev, int brate );

int   DEVICE_connect_port( RFLEX_Device *dev );

#define BUFFER_LENGTH         512

int   writeData( int fd, unsigned char *buf, int nChars );

int   waitForAnswer( int fd, unsigned char *buf, int *len );

#endif
