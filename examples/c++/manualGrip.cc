/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
///////////////////////////////////////////////////////////////////////////
//
// Desc: Very simple example to control the gripper manually
//
// When started, console waits for user to input keyboard command followed by enter. 
// Supported command :
//
// Command    : Character to enter (always followed by enter)
// -----------------------------------------------------------
// Open grips   : o
// Close grips  : c
// Stop grips   : x
// Lift up      : u
// Lift down    : d
// Lift stop    : s
// Deploy       : w
// Store        : q
// Halt         : h
//
// Author: Carle Cote (Laborius - Universite de Sherbrooke - Sherbrooke, Quebec, Canada)
// Date: 23 february 2004
// CVS: 
//
///////////////////////////////////////////////////////////////////////////

#include <playerclient.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <stdlib.h>

#include <iostream>

char host[256] = "localhost";
int port = PLAYER_PORTNUM;

/* parse command-line args */
void
parse_args(int argc, char** argv)
{
  int i;

  i=1;
  while(i<argc)
  {
    if(!strcmp(argv[i],"-h"))
    {
      if(++i<argc)
        strcpy(host,argv[i]);
      else
      {
        //puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc)
        port = atoi(argv[i]);
      else
      {
        //puts(USAGE);
        exit(1);
      }
    }
    else
    {
      //puts(USAGE);
      exit(1);
    }
    i++;
  }
}

int main(int argc, char **argv)
{ 
  parse_args(argc,argv);

  /* Connect to Player server */
  PlayerClient robot(host,port);

  /* Request sensor data */
  GripperProxy gp(&robot,0,'a');
  
  puts("RESET");
  gp.SetGrip(GRIPhalt,0);

  for(;;)
  {
    if(robot.Read())
      exit(1);

    std::string str;
    std::cin >> str;
    
    if(str == "o")
    {
      puts("GRIP OPEN");
      gp.SetGrip(GRIPopen,0);
    }
    else if (str == "c")
    {
       puts("GRIP CLOSE");
       gp.SetGrip(GRIPclose,0);
    }
    else if (str == "x")
    {
       puts("GRIP STOP");
       gp.SetGrip(GRIPstop,0);
    }
    else if(str == "u")
    {
      puts("LIFT UP");
      gp.SetGrip(LIFTup,0);
    }
    else if (str == "d")
    {
       puts("LIFT DOWN");
       gp.SetGrip(LIFTdown,0);
    }
    else if (str == "s")
    {
       puts("LIFT STOP");
       gp.SetGrip(LIFTstop,0);
    }
    else if (str == "q")
    {
       puts("GRIP STORE");
       gp.SetGrip(GRIPstore,0);
    }
    else if (str == "w")
    {
       puts("GRIP DEPLOY");
       gp.SetGrip(GRIPdeploy,0);
    }
    else if (str == "h")
    {
       puts("GRIPPER HALT");
       gp.SetGrip(GRIPhalt,0);
    }
    
    else
    {
       puts("UNKNOWN COMMAND");
    }
    
  }

  return(0);
}

