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
 * $Id$
 *
 * laserobstacleavoid.c - simple obstacle avoidance using SICK laser
 */

#include <stdio.h>
#include <stdlib.h>  /* for atoi(3) */
#include <playercclient.h>  /* for player client stuff */
#include <values.h>  /* for MAXINT */
#include <string.h> /* for strcmp() */

#define USAGE \
  "USAGE: laserobstacleavoid [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"

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
  player_connection_t conn;
  player_srf_data_t laser;
  player_position_cmd_t poscmd;
  int minR, minL;
  int j,l,r;

  parse_args(argc,argv);

  /* connect to the Player server */
  if(player_connect(&conn, host, portnum))
    exit(1);

  /* request read access on the laser and write access to the wheels */
  if(player_request_device_access(&conn, PLAYER_SRF_CODE, 0, 'r',NULL) == -1)
    exit(1);
  if(player_request_device_access(&conn, PLAYER_POSITION_CODE, 0, 'w',NULL) == -1)
    exit(1);

  /* maybe turn on the motors */
  /*if(turnOnMotors && robot.ChangeMotorState(1))
    exit(1);*/

  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(player_read_laser(&conn,&laser))
      exit(1);
    if(player_read_synch(&conn))
      exit(1);

    /* print current sensor data to console */
    /*player_print_laser(laser);*/

    /*
     * laser avoid (stolen from esben's java example)
     */
    minL=MAXINT; 
    minR=MAXINT;
    for(j=0; j<180; j++) {
      if (minR>laser.ranges[j]) minR=laser.ranges[j];
    }
    for (j=181; j<361; j++) {
      if (minL>laser.ranges[j]) minL=laser.ranges[j];
    }
    l=(100*minR)/500-100;
    r=(100*minL)/500-100;
    if (l>150) 
      l=150; 
    if (r>150) 
      r=150;
    poscmd.xspeed = r+l;
    poscmd.yawspeed = r-l;

    /* write commands to robot */
    if(player_write_position(&conn, poscmd) == -1)
      exit(1);
  }
}
