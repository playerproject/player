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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>

// TODO: remove this include
#include <sys/poll.h>

#include "roomba_comms.h"

#define TWOSCOMP(v) (1 + ~v)

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

  printf("Opening connection to Roomba on %s...", r->serial_port);
  fflush(stdout);

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

  if(roomba_get_sensors(r, 500) < 0)
  {
    puts("roomba_open():failed to get data");
    close(r->fd);
    r->fd = -1;
    return(-1);
  }

  /* We know the robot is there; switch to blocking */
  /* ok, we got data, so now set NONBLOCK, and continue */
  if((flags = fcntl(r->fd, F_GETFL)) < 0)
  {
    perror("roomba_open():fcntl():");
    close(r->fd);
    r->fd = -1;
    return(-1);
  }
  if(fcntl(r->fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
  {
    perror("roomba_open():fcntl()");
    close(r->fd);
    r->fd = -1;
    return(-1);
  }

  puts("Done.");

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
  if(close(r->fd) < 0)
  {
    perror("roomba_close():close():");
    return(-1);
  }
  else
    return(0);
}

int
roomba_set_speeds(roomba_comm_t* r, double tv, double rv)
{
  unsigned char cmdbuf[5];
  int16_t tv_mm, rad_mm;
  int16_t tv_mm_tc, rad_mm_tc;

  tv_mm = (int16_t)rint(tv * 1e3);
  tv_mm = MAX(tv_mm, -ROOMBA_TVEL_MAX_MM_S);
  tv_mm = MIN(tv_mm, ROOMBA_TVEL_MAX_MM_S);
  tv_mm_tc = TWOSCOMP(tv_mm);

  if(rv == 0)
  {
    // Special case: drive straight
    rad_mm = 0x8000;
  }
  else if(tv == 0)
  {
    // Special cases: turn in place
    if(rv > 0)
      rad_mm = 1;
    else
      rad_mm = -1;
  }
  else
  {
    // General case: convert rv to turn radius
    rad_mm = (int16_t)rint(tv_mm / (rv * 1e3));
    rad_mm = MAX(rad_mm, -ROOMBA_RADIUS_MAX_MM);
    rad_mm = MIN(rad_mm, ROOMBA_RADIUS_MAX_MM);
  }

  rad_mm_tc = TWOSCOMP(rad_mm);

  cmdbuf[0] = ROOMBA_OPCODE_DRIVE;
  cmdbuf[1] = (unsigned char)(tv_mm_tc >> 8);
  cmdbuf[2] = (unsigned char)(tv_mm_tc & 0xFF);
  cmdbuf[3] = (unsigned char)(rad_mm_tc >> 8);
  cmdbuf[4] = (unsigned char)(rad_mm_tc & 0xFF);

  if(write(r->fd, cmdbuf, 5) < 0)
  {
    perror("roomba_set_speeds():write():");
    return(-1);
  }
  else
    return(0);
}

int
roomba_get_sensors(roomba_comm_t* r, int timeout)
{
  struct pollfd ufd[1];
  unsigned char cmdbuf[2];
  unsigned char databuf[ROOMBA_SENSOR_PACKET_SIZE];
  int retval;
  int numread;

  cmdbuf[0] = ROOMBA_OPCODE_SENSORS;
  /* Zero to get all sensor data */
  cmdbuf[1] = 0;

  if(write(r->fd, cmdbuf, 2) < 0)
  {
    perror("roomba_get_sensors():write():");
    return(-1);
  }

  ufd[0].fd = r->fd;
  ufd[0].events = POLLIN;

  for(;;)
  {
    if((retval = poll(ufd,1,timeout)) < 0)
    {
      if(errno == EINTR)
        continue;
      else
      {
        perror("roomba_get_sensors():poll():");
        return(-1);
      }
    }
    else if(retval == 0)
      return(-1);
    else
    {
      if((numread = read(r->fd,databuf,sizeof(databuf))) < 0)
      {
        perror("roomba_get_sensors():read()");
        return(-1);
      }
      else if(numread < sizeof(databuf))
      {
        puts("roomba_get_sensors():short read");
        return(-1);
      }
      else
      {
        roomba_parse_sensor_packet(r, databuf, (size_t)numread);
        return(0);
      }
    }
  }
}

int
roomba_parse_sensor_packet(roomba_comm_t* r, unsigned char* buf, size_t buflen)
{
  unsigned char flag;
  int16_t signed_int;
  uint16_t unsigned_int;
  double dist, angle;
  int idx;

  if(buflen != ROOMBA_SENSOR_PACKET_SIZE)
  {
    puts("roomba_parse_sensor_packet():packet is wrong size");
    return(-1);
  }

  idx = 0;

  /* Bumps, wheeldrops */
  flag = buf[idx++];
  r->bumper_right = (flag >> 0) & 0x01;
  r->bumper_left = (flag >> 1) & 0x01;
  r->wheeldrop_right = (flag >> 2) & 0x01;
  r->wheeldrop_left = (flag >> 3) & 0x01;
  r->wheeldrop_caster = (flag >> 4) & 0x01;

  r->wall = buf[idx++] & 0x01;
  r->cliff_left = buf[idx++] & 0x01;
  r->cliff_frontleft = buf[idx++] & 0x01;
  r->cliff_frontright = buf[idx++] & 0x01;
  r->cliff_right = buf[idx++] & 0x01;
  r->virtual_wall = buf[idx++] & 0x01;

  flag = buf[idx++];
  r->overcurrent_sidebrush = (flag >> 0) & 0x01;
  r->overcurrent_vacuum = (flag >> 1) & 0x01;
  r->overcurrent_mainbrush = (flag >> 2) & 0x01;
  r->overcurrent_driveright = (flag >> 3) & 0x01;
  r->overcurrent_driveleft = (flag >> 3) & 0x01;

  r->dirtdetector_left = buf[idx++];
  r->dirtdetector_right = buf[idx++];
  r->remote_opcode = buf[idx++];

  flag = buf[idx++];
  r->button_max = (flag >> 0) & 0x01;
  r->button_clean = (flag >> 1) & 0x01;
  r->button_spot = (flag >> 2) & 0x01;
  r->button_power = (flag >> 3) & 0x01;

  memcpy(&signed_int, buf+idx, 2);
  idx += 2;
  signed_int = (int16_t)ntohs((uint16_t)signed_int);
  dist = signed_int / 1e3;

  memcpy(&signed_int, buf+idx, 2);
  idx += 2;
  signed_int = (int16_t)ntohs((uint16_t)signed_int);
  angle = (2.0 * (signed_int / 1e3)) / ROOMBA_AXLE_LENGTH;

  /* First-order odometric integration */
  r->oa = NORMALIZE(r->oa + angle);
  r->ox += dist * cos(r->oa);
  r->oy += dist * sin(r->oa);

  r->charging_state = buf[idx++];

  memcpy(&unsigned_int, buf+idx, 2);
  idx += 2;
  unsigned_int = ntohs(unsigned_int);
  r->voltage = unsigned_int / 1e3;

  memcpy(&signed_int, buf+idx, 2);
  idx += 2;
  signed_int = (int16_t)ntohs((uint16_t)signed_int);
  r->current = signed_int / 1e3;

  r->temperature = (char)(buf[idx++]);

  memcpy(&unsigned_int, buf+idx, 2);
  idx += 2;
  unsigned_int = ntohs(unsigned_int);
  r->charge = unsigned_int / 1e3;

  memcpy(&unsigned_int, buf+idx, 2);
  idx += 2;
  unsigned_int = ntohs(unsigned_int);
  r->capacity = unsigned_int / 1e3;

  assert(idx == ROOMBA_SENSOR_PACKET_SIZE);

  return(0);
}
