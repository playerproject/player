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
 * randomwalk.c - sonar obstacle avoidance with random walk
 */

#include <stdio.h>
#include <string.h> /* for strcpy() */
#include <stdlib.h>  /* for atoi(3),rand(3) */
#include <playercclient.h>  /* the pure C client header */

#define USAGE \
  "USAGE: randomwalk [-h <host>] [-p <port>] [-m]\n" \
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

int main(int argc, char** argv)
{
  player_connection_t conn;
  player_sonar_data_t sonar;
  player_position_cmd_t poscmd;
  int randint;
  int randcount = 0;
  int avoidcount = 0;
  unsigned short minfrontdistance = 350;

  /* first, parse command line args */
  parse_args(argc,argv);

  /* Connect to the Player server */
  if(player_connect(&conn, host, portnum))
    exit(1);

  /* request read access on the sonars and write access to the wheels */
  if(player_request_device_access(&conn, PLAYER_POSITION_CODE, 0, 'w',NULL) == -1)
    exit(1);
  if(player_request_device_access(&conn, PLAYER_SONAR_CODE, 0, 'r',NULL) == -1)
    exit(1);

  /* maybe turn on the motors */
  /*if(turnOnMotors && robot.ChangeMotorState(1))
    exit(1);*/

  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(player_read_sonar(&conn, &sonar) == -1)
      exit(1);

    /* print current sensor data to console */
    /*player_print_sonar(sonar);*/
    /*player_print_position(position);*/

    if(sonar.ranges[2] < minfrontdistance ||
       sonar.ranges[3] < minfrontdistance ||
       sonar.ranges[4] < minfrontdistance ||
       sonar.ranges[5] < minfrontdistance ||
       avoidcount)
    {
      poscmd.speed = -150;

      /* once we start avoiding, continue avoiding for 2 seconds */
      /* (we run at about 10Hz, so 20 loop iterations is about 2 sec) */
      if(!avoidcount)
      {
        avoidcount = 15;
        randcount = 0;

        if(sonar.ranges[1]+sonar.ranges[15] < sonar.ranges[7]+sonar.ranges[8])
          poscmd.turnrate = -40;
        else
          poscmd.turnrate = 40;
      }
      avoidcount--;
    }
    else
    {
      avoidcount = 0;
      poscmd.speed = 200;

      /* update turnrate every 3 seconds */
      if(!randcount)
      {
        /* make random int tween -20 and 20 */
        randint = (1+(int)(40.0*rand()/(RAND_MAX+1.0))) - 20; 

        poscmd.turnrate = randint;
        randcount = 20;
      }
      randcount--;
    }

    /* write commands to robot */
    player_write_position(&conn, poscmd);
  }

  /* it's not necessary, but we could close the devices like this: */
  if(player_request_device_access(&conn, PLAYER_SONAR_CODE, 0, 'c',NULL))
    exit(1);
  if(player_request_device_access(&conn, PLAYER_POSITION_CODE, 0, 'c',NULL))
    exit(1);

  return(0);
}
    
