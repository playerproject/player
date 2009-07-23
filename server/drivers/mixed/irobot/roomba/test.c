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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include "roomba_comms.h"

roomba_comm_t* r;

void
exit_handler(int sig)
{
  roomba_close(r);
  exit(0);
}

int
main(int argc, char** argv)
{
  int i;

  double tv,rv;

  tv = atof(argv[1]);
  rv = atof(argv[2]) * M_PI/180.0;

  r = roomba_create("/dev/ttyS2");
  if(roomba_open(r, 0) < 0)
    exit(-1);

  signal(SIGINT, exit_handler);

  roomba_set_speeds(r, tv, rv);
  usleep(1000000);
  roomba_set_speeds(r, -tv, -rv);
  usleep(1000000);
  roomba_set_speeds(r, 0.0, 0.0);
  //roomba_clean(r);

  for(i=0;i<100;i++)
  {
    if(roomba_get_sensors(r, -1) < 0)
    {
      exit(-1);
    }
    //roomba_print(r);
    usleep(500000);
  }
  roomba_close(r);
  return(0);
}
