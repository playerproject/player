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


int main(int argc, char **argv)
{ 
  int num;
  PlayerClient** clients;
  PositionProxy** pproxies;
  SonarProxy** sproxies;

  int baseport = PLAYER_PORTNUM;
  if(argc < 2)
  {
    puts("no num");
    exit(-1);
  }

  num = atoi(argv[1]); 
  if(argc > 2)
    baseport = atoi(argv[2]);
  printf("Starting %d clients\n", num);

  clients = new PlayerClient*[num];
  pproxies = new PositionProxy*[num];
  sproxies = new SonarProxy*[num];

  /* create a multiclient to control them all */
  PlayerMultiClient multi;

  for(int i=0;i<num;i++)
  {
    clients[i] = new PlayerClient(host,baseport+i);
    pproxies[i] = new P2PositionProxy(clients[i],0,'r');
    sproxies[i] = new SonarProxy(clients[i],0,'r');
    multi.AddClient(clients[i]);
    //usleep(10000);
  }

  for(;;)
  {
    if(multi.Read())
      exit(1);
  }

  return(0);
}

