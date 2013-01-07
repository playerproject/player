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

#include "hemisson_serial.h"

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define FAIL -1

#ifndef CRTSCTS
#ifdef IHFLOW
#ifdef OHFLOW
#define CRTSCTS ((IHFLOW) | (OHFLOW))
#endif
#endif
#endif

HemissonSerial::HemissonSerial(int debug, const char * port, const char * rate) : debug(debug)
{
	int beep[1];
	fd = -1;
	parity = 0;
	ttybuf[0] = 0;
        struct timespec ts;

	// open the serial port
	fd = serial_open(port);
	if (fd < 0)
	{
		fprintf(stderr, "Could not open serial device %s\n", port);
		return;
	}
	m_setparms(fd, rate, "N", "8", "1", 0, 0);
	tcflush(fd, TCIOFLUSH);
	// communication sanity check
	printf("Hemisson> %s\n", m_gets(fd, HEMISSON_SERIAL_TIMEOUT_USECS));
	printf("Hemisson> %s\n", m_gets(fd, HEMISSON_SERIAL_TIMEOUT_USECS));
	// clear the input buffer in case junk data is on the port
	HemissonCommand('B', 0, NULL, 0, NULL);
	tcflush(fd, TCIFLUSH);
	// try a test command
	beep[0] = 1;
	HemissonCommand('H', 1, beep, 0, NULL);
        ts.tv_sec = 0;
        ts.tv_nsec = 500000000;
        nanosleep(&ts, NULL);
	beep[0] = 0;
	HemissonCommand('H', 1, beep, 0, NULL);
}

HemissonSerial::~HemissonSerial()
{
	if (fd > 0)
	{
		serial_close(fd);
	}
}

int HemissonSerial::WriteInts(char command, int Count, int * Values)
{
	int i;

	if (fd < 0) return -1;
	buffer[0] = command;
	buffer[1] = '\0';
	int length;
	if (Values) for (i = 0; i < Count; ++ i)
	{
		length = strlen(buffer);
		snprintf(&buffer[length],HEMISSON_BUFFER_LEN - length - 2,",%d",Values[i]);
	}
	if (debug)
	{
	    printf(">>>> %s\n", buffer); fflush(stdout);
	}
	m_puts(fd, buffer);
	return 0;
}

#define MAX_RETRIES 5
// read ints from Hemisson
int HemissonSerial::ReadInts(char Header, int Count, int * Values)
{
	int i;

	buffer[0] = 0;
	if (fd < 0) return -1;
	for (i = 0; i < MAX_RETRIES; i++)
	{
	    strncpy(buffer, m_gets(fd, HEMISSON_SERIAL_TIMEOUT_USECS), HEMISSON_BUFFER_LEN);
	    if (debug)
	    {
		printf("[%d] %s\n", i, buffer); fflush(stdout);
	    }
	    if (buffer[0]) break;
	    m_puts(fd, "B");
	}
	if (buffer[0] != Header) return -1;
	char * pos = &buffer[2];
	if (Values) for (i = 0; i < Count; i++)
	{
		Values[i] = strtol(pos, &pos, 10);
		pos++;
	}
	return 0;
}

int HemissonSerial::HemissonCommand(char command, int InCount, int * InValues, int OutCount, int * OutValues)
{
        struct timespec ts;
        int ret2;

        WriteInts(command,InCount,InValues);
        ts.tv_sec = 0;
        ts.tv_nsec = 50000000;
        nanosleep(&ts, NULL);
        ret2 = ReadInts(command + 32, OutCount, OutValues);
        ts.tv_sec = 0;
        ts.tv_nsec = 50000000;
        nanosleep(&ts, NULL);
        return ret2;
}

char * HemissonSerial::m_gets(int fd, int tmout)
{
  int n, f, q;
  struct timeval tv;
  fd_set fds;
  char * buf;

  buf = ttybuf; f = 0; q = 0;

  for (;;)
  {
    buf[0]=0;
    // tv.tv_sec = tmout / 1000;
    // tv.tv_usec = (tmout % 1000) * 1000L;
    tv.tv_sec = 0;
    tv.tv_usec = tmout;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    if (select(fd + 1, &fds, NULL, NULL, &tv) > 0)
    {
      if (FD_ISSET(fd, &fds) > 0)
      {
        n = read(fd, buf, 1);
        if (n > 0)
        {
          if (q)
          {
            buf++;
            buf[0] = 0;
            if ((parity == 'M') || (parity == 'S')) ttybuf[0] &= 0x7f;
            return ttybuf;
          }
          switch(buf[0])
          {
          case 10:
            buf[0] = 0;
            if ((parity == 'M') || (parity == 'S')) ttybuf[0] &= 0x7f;
            return ttybuf;
          case 13:
            buf[0] = 0;
            break;
          case 16:
            buf[0] = 0;
            q = !0;
            break;
          default:
            buf++; f++;
            if (f >= TTYBUFFLEN)
            {
              buf[0] = 0;
              if ((parity == 'M') || (parity == 'S')) ttybuf[0] &= 0x7f;
              return ttybuf;
            }
          }
        }
      }
    } else break;
  }
  if ((parity == 'M') || (parity == 'S')) ttybuf[0] &= 0x7f;
  return ttybuf;
}

int HemissonSerial::m_getchar(int fd, int tmout)
{
  int n;
  struct timeval tv;
  fd_set fds;

  // tv.tv_sec = tmout / 1000;
  // tv.tv_usec = (tmout % 1000) * 1000L;
  tv.tv_sec = 0;
  tv.tv_usec = tmout;

  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  if (select(fd + 1, &fds, NULL, NULL, &tv) > 0)
  {
    if (FD_ISSET(fd, &fds) > 0)
    {
      n = read(fd, ttybuf, 1);
      if (n > 0) return static_cast<int>(static_cast<unsigned char>(ttybuf[0]));
    }
  }
  return -1;
}

/*
 * Send a string to the modem.
 */
void HemissonSerial::m_puts(int fd, const char * s)
{
  struct timespec ts;
  char c;

  while (*s)
  {
  	if (*s == '^' && (*(s + 1)))
        {
  		s++;
  		if (*s == '^')
  			c = *s;
  		else
  			c = (*s) & 31;
  	} else c = *s;
  	if (c == '~')
        {
                ts.tv_sec = 1;
                ts.tv_nsec = 0;
                nanosleep(&ts, NULL);
  	} else write(fd, &c, 1);
  	s++;
  }
  c = 13;
  write(fd, &c, 1);
}

void HemissonSerial::m_putchar(int fd, int chr)
{
    char c;
    struct timespec ts;

    c = static_cast<char>(chr);
    if (c == '~')
    {
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
        nanosleep(&ts, NULL);
    } else write(fd, &c, 1);
}

/* Set hardware flow control. */
void HemissonSerial::m_sethwf(int fd, int on)
{
  struct termios tty;

  tcgetattr(fd, &tty);
  if (on)
	tty.c_cflag |= CRTSCTS;
  else
	tty.c_cflag &= ~CRTSCTS;
  tcsetattr(fd, TCSANOW, &tty);
}

/* Set RTS line. Sometimes dropped. Linux specific? */
void HemissonSerial::m_setrts(int fd)
{
#if defined(TIOCM_RTS) && defined(TIOCMGET)
  int mcs=0;

  ioctl(fd, TIOCMGET, &mcs);
  mcs |= TIOCM_RTS;
  ioctl(fd, TIOCMSET, &mcs);
#endif
}

/*
 * Drop DTR line and raise it again.
 */
void HemissonSerial::m_dtrtoggle(int fd, int sec)
{
  /* Posix - set baudrate to 0 and back */
  struct termios tty, old;
  struct timespec ts;

  tcgetattr(fd, &tty);
  tcgetattr(fd, &old);
  cfsetospeed(&tty, B0);
  cfsetispeed(&tty, B0);
  tcsetattr(fd, TCSANOW, &tty);
  if (sec > 0)
  {
    ts.tv_sec = sec;
    ts.tv_nsec = 0;
    nanosleep(&ts, NULL);
    tcsetattr(fd, TCSANOW, &old);
  }
}

/*
 * Send a break
 */
void HemissonSerial::m_break(int fd)
{
  tcsendbreak(fd, 0);
}

/*
 * Get the dcd status
 */
int HemissonSerial::m_getdcd(int fd)
{
#ifdef TIOCMGET
  int mcs=0;

  ioctl(fd, TIOCMGET, &mcs);
  return (mcs & TIOCM_CAR) ? 1 : 0;
#else
  fd = fd;
  return 0; /* Impossible!! (eg. Coherent) */
#endif
}

/*
 * Save the state of a port
 */
void HemissonSerial::m_savestate(int fd)
{
  tcgetattr(fd, &savetty);
#ifdef TIOCMGET
  ioctl(fd, TIOCMGET, &m_word);
#endif
}

/*
 * Restore the state of a port
 */
void HemissonSerial::m_restorestate(int fd)
{
  tcsetattr(fd, TCSANOW, &savetty);
#ifdef TIOCMSET
  ioctl(fd, TIOCMSET, &m_word);
#endif
}

/*
 * Set the line status so that it will not kill our process
 * if the line hangs up.
 */
/*ARGSUSED*/ 
void HemissonSerial::m_nohang(int fd)
{
  struct termios sgg;

  tcgetattr(fd, &sgg);
  sgg.c_cflag |= CLOCAL;
  tcsetattr(fd, TCSANOW, &sgg);
}

/*
 * Set hangup on close on/off.
 */
void HemissonSerial::m_hupcl(int fd, int on)
{
  struct termios sgg;

  tcgetattr(fd, &sgg);
  if (on) sgg.c_cflag |= HUPCL;
  else sgg.c_cflag &= ~HUPCL;
  tcsetattr(fd, TCSANOW, &sgg);
}

/*
 * Flush the buffers
 */
void HemissonSerial::m_flush(int fd)
{
/* Should I Posixify this, or not? */
#ifdef TCFLSH
  ioctl(fd, TCFLSH, 2);
#endif
#ifdef TIOCFLUSH
  ioctl(fd, TIOCFLUSH, NULL);
#endif
}

/*
 * Set baudrate, parity and number of bits.
 */
void HemissonSerial::m_setparms(int fd, const char * baudr, const char * par, const char * bits, const char * stopb, int hwf, int swf)
{
  int spd = -1;
  int newbaud;
  int bit = bits[0];

  struct termios tty;

  tcgetattr(fd, &tty);

  /* We generate mark and space parity ourself. */
  parity = par[0];
  if (bit == '7' && (par[0] == 'M' || par[0] == 'S'))
	bit = '8';

  /* Check if 'baudr' is really a number */
  if ((newbaud = (atol(baudr) / 100)) == 0 && baudr[0] != '0') newbaud = -1;

  switch(newbaud) {
  	case 0:		spd = B0;	break;
  	case 3:		spd = B300;	break;
  	case 6:		spd = B600;	break;
  	case 12:	spd = B1200;	break;
  	case 24:	spd = B2400;	break;
  	case 48:	spd = B4800;	break;
  	case 96:	spd = B9600;	break;
#ifdef B19200
  	case 192:	spd = B19200;	break;
#else /* B19200 */
#ifdef EXTA
	case 192:	spd = EXTA;	break;
#else /* EXTA */
	case 192:	spd = B9600;	break;
#endif /* EXTA */
#endif /* B19200 */
#ifdef B38400
	case 384:	spd = B38400;	break;
#else /* B38400 */
#ifdef EXTB
	case 384:	spd = EXTB;	break;
#else /* EXTB */
	case 384:	spd = B9600;	break;
#endif /* EXTB */
#endif /* B38400 */
#ifdef B57600
	case 576:	spd = B57600;	break;
#endif
#ifdef B115200
	case 1152:	spd = B115200;	break;
#endif
#ifdef B230400
	case 2304:	spd = B230400;	break;
#endif
  }
  if (spd != -1) {
	cfsetospeed(&tty, (speed_t)spd);
	cfsetispeed(&tty, (speed_t)spd);
  }
  switch (bit) {
  	case '5':
  		tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS5;
  		break;
  	case '6':
  		tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS6;
  		break;
  	case '7':
  		tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS7;
  		break;
  	case '8':
	default:
  		tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  		break;
  }		
  /* Set into raw, no echo mode */
  tty.c_iflag =  IGNBRK;
  tty.c_lflag = 0;
  tty.c_oflag = 0;
  tty.c_cflag |= CLOCAL | CREAD;
#ifdef _DCDFLOW
  tty.c_cflag &= ~CRTSCTS;
#endif
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 5;

  if (swf)
	tty.c_iflag |= IXON | IXOFF;
  else
	tty.c_iflag &= ~(IXON|IXOFF|IXANY);

  tty.c_cflag &= ~(PARENB | PARODD);
  if (par[0] == 'E')
	tty.c_cflag |= PARENB;
  else if (par[0] == 'O')
	tty.c_cflag |= (PARENB | PARODD);

  if (stopb[0] == '2')
    tty.c_cflag |= CSTOPB;
  else
    tty.c_cflag &= ~CSTOPB;

  tcsetattr(fd, TCSANOW, &tty);

  m_setrts(fd);

#ifndef _DCDFLOW
  m_sethwf(fd, hwf);
#endif
}

int HemissonSerial::serial_open(const char * devname)
{
    int f, portfd;
    struct stat stt;

    ttybuf[0] = 0;
    ttybuf[TTYBUFFLEN] = 0;
    if (stat(devname, &stt)==-1)
    {
        fprintf(stderr, "Cannot stat device: %s\n", devname);
        return FAIL;
    }
    portfd=open(devname, O_RDWR|O_NDELAY);
    if (portfd<0)
    {
        fprintf(stderr, "Cannot open device: %s\n", devname);
        return FAIL;
    }
    f = fcntl (portfd, F_GETFL, 0);
    fcntl(portfd, F_SETFL, f & ~O_NDELAY);
    m_savestate(portfd);
    m_nohang(portfd);
    m_hupcl(portfd, 1);
    m_flush(portfd);
    return portfd;
}

void HemissonSerial::serial_close(int fd)
{
    if (fd >= 0)
    {
        m_restorestate(fd);
        close(fd);
    }
}
