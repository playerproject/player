/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006 -
 *     Brian Gerkey
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <termios.h>

#include "roomba_comms.h"


roomba_comm_t*
roomba_create(const char* serial_port)
{
  roomba_comm_t* r;

  r = calloc(1,sizeof(roomba_comm_t));
  assert(r);
  r->fd = -1;
  r->mode = ROOMBA_MODE_OFF;
  strncpy(r->serial_port,serial_port,sizeof(r->serial_port)-1);
  return(r);
}

int
roomba_open(roomba_comm_t* r)
{
  struct termios term;
  int flags;

  if(r->fd >= 0)
  {
    puts("roomba connection already open!");
    return(-1);
  }

  // Open it.  non-blocking at first, in case there's no roomba
  if((r->fd = open(r->serial_port, 
                   O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 )
  {
    perror("roomba_open():open():");
    return(-1);
  }

  if(tcflush(r->fd, TCIFLUSH) < 0 )
  {
    perror("roomba_open():tcflush():");
    close(r->fd);
    r->fd = -1;
    return(-1);
  }
  if(tcgetattr(r->fd, &term) < 0 )
  {
    perror("roomba_open():tcgetattr():");
    close(r->fd);
    r->fd = -1;
    return(-1);
  }
  
  // TODO: handle not having cfmakeraw available
  cfmakeraw(&term);
  cfsetispeed(&term, B57600);
  cfsetospeed(&term, B57600);
  
  if(tcsetattr(r->fd, TCSAFLUSH, &term) < 0 )
  {
    perror("roomba_open():tcsetattr():");
    close(r->fd);
    r->fd = -1;
    return(-1);
  }

  if(roomba_init(r) < 0)
  {
    puts("failed to initialize connection");
    close(r->fd);
    r->fd = -1;
    return(-1);
  }

  return(0);
}

int
roomba_init(roomba_comm_t* r)
{
  unsigned char cmdbuf[1];

  usleep(ROOMBA_DELAY_MODECHANGE_MS * 1e3);

  cmdbuf[0] = ROOMBA_OPCODE_START;
  if(write(r->fd, cmdbuf, 1) < 0)
  {
    perror("roomba_init():write():");
    return(-1);
  }
  r->mode = ROOMBA_MODE_PASSIVE;

  usleep(ROOMBA_DELAY_MODECHANGE_MS * 1e3);
  
  cmdbuf[0] = ROOMBA_OPCODE_CONTROL;
  if(write(r->fd, cmdbuf, 1) < 0)
  {
    perror("roomba_init():write():");
    return(-1);
  }
  r->mode = ROOMBA_MODE_SAFE;

  usleep(ROOMBA_DELAY_MODECHANGE_MS * 1e3);

  cmdbuf[0] = ROOMBA_OPCODE_CONTROL;
  if(write(r->fd, cmdbuf, 1) < 0)
  {
    perror("roomba_init():write():");
    return(-1);
  }
  r->mode = ROOMBA_MODE_FULL;

  return(0);
}

int
roomba_close(roomba_comm_t* r)
{
  unsigned char cmdbuf[1];

  roomba_set_speeds(0,0);
}
