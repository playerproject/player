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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * ptz.c
 *
 * a simple demo to show how to send commands to and get feedback from
 * the Sony PTZ camera.  this program will pan the camera in a loop
 * from side to side
 */

#define USAGE \
  "USAGE: ptz [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"

#include <playercclient.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char host[256];
int portnum;
char motorson;

/* parse command-line args */
void
parse_args(int argc, char** argv)
{
  int i;

  strcpy(host,"localhost");
  portnum = PLAYER_PORTNUM;

  i=1;
  while(i<argc)
  {
    if(!strcmp(argv[i],"-h"))
    {
      if(++i<argc)
        strcpy(host,argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc)
        portnum = atoi(argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-m"))
    {
      motorson = 1;
    }
    else
    {
      puts(USAGE);
      exit(1);
    }
    i++;
  }
}

int main(int argc, char **argv)
{ 
  int dir, i;
  player_connection_t conn;
  player_ptz_data_t ptzdata;
  player_ptz_cmd_t ptzcmd;

  parse_args(argc,argv);

  ptzcmd.zoom = 0;
  ptzcmd.tilt = 0;

  /* Connect to Player server */
  if(player_connect(&conn, host, portnum))
    exit(1);

  /* Request sensor data */
  if(player_request_device_access(&conn, PLAYER_PTZ_CODE, 0, 'a',NULL) == -1)
    exit(1);

  dir = 1;
  for(;;)
  {
    if(player_read_ptz(&conn, &ptzdata))
      exit(1);
    if(player_read_synch(&conn))
      exit(1);

    player_print_ptz(ptzdata);

    if(ptzdata.pan > 80 || ptzdata.pan < -80)
    {
      ptzcmd.pan = dir*70;
      player_write_ptz(&conn, ptzcmd);
      for(i=0;i<10;i++)
      {
        if(player_read_ptz(&conn, &ptzdata))
          exit(1);
      }
      player_print_ptz(ptzdata);
      dir = -dir;
    }
    ptzcmd.pan = ptzdata.pan + dir * 5;
    player_write_ptz(&conn, ptzcmd);
  }

  return(0);
}

