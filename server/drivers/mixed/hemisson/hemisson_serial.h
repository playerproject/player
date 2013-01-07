/*
 *  Serial communication helper class for Hemisson robot driver
 *  Copyright (C) 2010 Paul Osmialowski
 *  Based on Minicom code released on the same license
 *  Minicom is Copyright (C) 1991,1992,1993,1994,1995,1996
 *      Miquel van Smoorenburg.
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

#ifndef HEMISSON_SERIAL_H
#define HEMISSON_SERIAL_H

#include <stddef.h>
#include <termios.h>

#define HEMISSON_BAUDRATE "115200"
#define HEMISSON_DEFAULT_SERIAL_PORT "/dev/rfcomm0"
#define HEMISSON_BUFFER_LEN 255
#define HEMISSON_SERIAL_TIMEOUT_USECS 100000

#define TTYBUFFLEN 255
#define TTYBUFFSIZE (TTYBUFFLEN + 1)

class HemissonSerial
{
public:
	HemissonSerial(int debug = 0, const char * port = HEMISSON_DEFAULT_SERIAL_PORT, const char * rate = HEMISSON_BAUDRATE);
	virtual ~HemissonSerial();

	bool Open() { return fd >0; };
	int HemissonCommand(char command, int InCount, int * InValues, int OutCount, int * OutValues);

protected:
	// serial port descriptor
	int fd;
	// read/write buffer
	char buffer[HEMISSON_BUFFER_LEN + 1];

	int WriteInts(char command, int Count = 0, int * Values = NULL);
	int ReadInts(char Header, int Count = 0, int * Values = NULL);

	int debug;

private:
	char ttybuf[TTYBUFFSIZE];
	char parity;
	struct termios savetty;
	int m_word;

	char * m_gets(int fd, int tmout);
	int m_getchar(int fd, int tmout);
	void m_puts(int fd, const char * s);
	void m_putchar(int fd, int chr);
	void m_dtrtoggle(int fd, int sec);
	void m_break(int fd);
	int m_getdcd(int fd);
	void m_flush(int fd);
	void m_setparms(int fd, const char * baudr, const char * par, const char * bits, const char * stopb, int hwf, int swf);
	int serial_open(const char * devname);
	void serial_close(int fd);

	void m_sethwf(int fd, int on);
	void m_setrts(int fd);
	void m_savestate(int fd);
	void m_restorestate(int fd);
	void m_nohang(int fd);
	void m_hupcl(int fd, int on);
};

#endif

