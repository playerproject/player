/*
 * $Id$
 *
 * a test for the C++ SonarProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_fiducial(PlayerClient* client, int index)
{
  unsigned char access;
  FiducialProxy fp(client,index,'c',robot);

  printf("device [fiducialfinder] index [%d]\n", index);

  TEST("subscribing (read)");
  if((fp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", fp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", fp.driver_name);

  // wait for P2OS to start up
  for(int i=0; i < 20; i++)
    client->Read();

  fp.Print();
  
  TEST("unsubscribing");
  if((fp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

