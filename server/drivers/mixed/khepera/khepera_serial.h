/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006
 *     Toby Collett
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef KHEPERA_SERIAL_H
#define KHEPERA_SERIAL_H

#include <pthread.h>
#include <termios.h>

#define KHEPERA_DEFAULT_BAUD B38400
#define KHEPERA_BUFFER_LEN 255
#define KHEPERA_SERIAL_TIMEOUT_USECS 100000


class KheperaSerial
{
public:
	
	KheperaSerial(char * port, int rate = KHEPERA_DEFAULT_BAUD);
	~KheperaSerial();

	bool Open() {return fd >0;};
	int KheperaCommand(char command, int InCount, int * InValues, int OutCount, int * OutValues);

	void Lock();
	void Unlock();
protected:
	// serial port descriptor
	int fd;
	struct termios oldtio;
	
	// read/write buffer
	char buffer[KHEPERA_BUFFER_LEN+1];

	int WriteInts(char command, int Count = 0, int * Values = NULL);
	int ReadInts(char Header, int Count = 0, int * Values = NULL);
	
	pthread_mutex_t lock;
};

#endif
