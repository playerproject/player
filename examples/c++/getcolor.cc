/*
 * followblob.cc - follow a blob (visual servoing for cmucam)
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <playerclient.h>

char host[256];
int port = 6665;

int main(int argc, char** argv)
{
  if (argc < 2) {
    printf("LOCALHOST...\n");
    strcpy(host,"localhost");
  } else if (argv[1][0] == '1') {
    printf("ROBOT #1 (65.103.106.61)...\n");
    strcpy(host,"65.103.105.61");
  } else if (argv[1][0] == '2') {
    printf("ROBOT #2 (65.103.106.63)...\n");
    strcpy(host,"65.103.105.63");
  } else {
    printf("Bad robot identifier.\n");
    exit(1);
  }
     
  //int maxspeed = 200;
  //unsigned int minarea = 2;
  //int newturnrate=0, newspeed=0;
  //int imagewidth = 80;
  //int misscnt = 0;

  printf("Please hold target directly in front of camera until AmigoBot begins to move...\n");

  /* Connect to the Player server */
  PlayerClient robot(host,port);
  BlobfinderProxy bp(&robot,0,'r');
  PositionProxy pp(&robot,0,'a');
  sleep(1);
  
  /* Turn off the motors */
  pp.SetMotorState(1);

//  bp.SetTrackingColor();
  bp.SetAutoGain(0);
  bp.SetColorMode(0);
  bp.SetContrast(128);
  bp.SetBrightness(128);

  sleep(1);
  //bp.SetTrackingColor(140,160,17,23,16,16);
  bp.SetTrackingColor();
  bp.SetTrackingColor();

  return(0);
}
    
