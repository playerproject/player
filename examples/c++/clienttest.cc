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

/* 
 * clienttest.cc: a horrible monolithic program that we used
 *                to test various bits of Player.  probably useless
 *                and bug-filled at this point.  use at your own risk.
 */

#include <signal.h>  /* for signal(2) */
#include <stdio.h>
#include <playerclient.h>


#define USAGE "USAGE: testCasper [-h host]"

/* catch pipe signal to avoud default exit on connection break */
void CatchSigPipe( int signo )
{
  if(signo == SIGPIPE)
    puts("** SIGPIPE! **");

  exit(0);
}

int main(int argc, char *argv[])
{
  CRobot robot;
  char localhost[] = "localhost";
  char* host = localhost;
  int i = 1;
  //int req = 0;

  while(i<argc)
  {
    if(!strcmp(argv[i++],"-h"))
    {
      if(i<argc)
        host = argv[i];
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else
    {
      puts(USAGE);
      exit(1);
    }
    i++;
  }
  /* set up to handle SIGPIPE (happens when the client dies) */
  if(signal(SIGPIPE,CatchSigPipe) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGPIPE");
    exit(1);
  }

  /* Connect to Casper server */
  if(robot.Connect(host))
    exit(0);

  /* Request sensor data */
  //if (robot.Request( "pa" , 2 ) ) {
  //if (robot.Request( "va" , 2 ) ) {
  if(robot.Request( "vr" , 2 )) 
    exit(0);
  
  /*
  if(robot.SetFrequency(1))
  {
    printf("SetFrequency() fucked up.");
    exit(1);
  }
  if(robot.SetReqRep())
  {
    printf("SetReqRep() fucked up.");
    exit(1);
  }
  req = 0;
  */

  for(;;)
  {
    /*
    if(req < 5)
    {
      req++;
      usleep(2000000);
      if(!robot.ReqData())
      {
        printf("ReqData() fucked up.");
        exit(1);
      }
    }
    else if(req == 5)
    {
      req++;
      if(!robot.SetCont())
      {
        printf("SetCont() fucked up.");
        exit(1);
      }
    }
    */
    robot.Read();
    robot.Print();

    //robot.pan = 90;
    //robot.tilt = 20;
    //robot.zoom = 1023;
    //robot.zoom = 1023;
    //robot.newturnrate = 20;
    //robot.newspeed = 0;

    /*
    robot.newgrip = ayGRIPSTORE;
    robot.newgrip = ayGRIPDEPLOY;
    robot.newturnrate = -20;
    robot.newspeed = -75;
    if ( robot.sonar[0]<robot.sonar[7] )
      robot.newturnrate = -20;
    else
      robot.newturnrate = 20;
    */


    /*
    if (robot.sonar[7]<500) 
      robot.GripperCommand( GRIPopen);
    else if (robot.sonar[7]<1000) 
      robot.GripperCommand( GRIPstop);
    else
      robot.GripperCommand( GRIPclose);
    robot.Write();

    if (robot.sonar[0]<500) 
      robot.GripperCommand( LIFTup);
    else if (robot.sonar[0]<1000) 
      robot.GripperCommand( LIFTstop);
    else
      robot.GripperCommand( LIFTdown);
    robot.Write();
    */

    /*
    if (robot.pan < 10 )  
      robot.newspeed = 0;
    else
      robot.newspeed = 100;

    if ( robot.pan < 0 ) 
      robot.newturnrate = -10;
    else
      robot.newturnrate = +10;
    */

    /*
    if(robot.vision->NumBlobs[1])
    {
      robot.tilt = 0;
      robot.zoom = 0;
      if(robot.vision->Blobs[1][0].x - 80 > 5)
        robot.pan -= 10;
      else if(80 - robot.vision->Blobs[1][0].x > 5)
        robot.pan += 10;
      else
      {
        puts("no diff");
        robot.newturnrate = 0;
      }
    }
    else
    {
      puts("no blob");
      robot.newturnrate = 0;
    }
    */

    robot.Write();
  }
    
  /* never gets here */
  /* close everything */
  robot.Request( "scgc" , 4 );

  return(0);
}

