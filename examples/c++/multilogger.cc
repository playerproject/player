/*
 * multilogger.cc
 *
 * a demo to show how to use the multi client.
 */

#include <playermulticlient.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char host[256] = "localhost";
int port = PLAYER_PORTNUM;


int main(int argc, char **argv)
{ 
  /* Connect to many Player servers */
  PlayerClient r0(host,port);
  PlayerClient r1(host,port+1);
  PlayerClient r2(host,port+2);

  PlayerClient r3(host,port);
  PlayerClient r4(host,port+1);
  PlayerClient r5(host,port+2);
  PlayerClient r6(host,port);
  PlayerClient r7(host,port+1);
  PlayerClient r8(host,port+2);

  /* Request sensor data */
  GpsProxy gp0(&r0,0,'r');
  GpsProxy gp1(&r1,0,'r');
  GpsProxy gp2(&r2,0,'r');

  GpsProxy gp3(&r3,0,'r');
  GpsProxy gp4(&r4,0,'r');
  GpsProxy gp5(&r5,0,'r');
  GpsProxy gp6(&r6,0,'r');
  GpsProxy gp7(&r7,0,'r');
  GpsProxy gp8(&r8,0,'r');

  /* create a multiclient to control them all */
  PlayerMultiClient multi;
  multi.AddClient(&r0);
  multi.AddClient(&r1);
  multi.AddClient(&r2);

  multi.AddClient(&r3);
  multi.AddClient(&r4);
  multi.AddClient(&r5);
  multi.AddClient(&r6);
  multi.AddClient(&r7);
  multi.AddClient(&r8);

  for(;;)
  {
    if(multi.Read())
      exit(1);

    gp0.Print();
    gp1.Print();
    gp2.Print();
    gp3.Print();
    gp4.Print();
    gp5.Print();
    gp6.Print();
    gp7.Print();
    gp8.Print();
  }

  return(0);
}

