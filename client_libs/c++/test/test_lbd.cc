/*
 * $Id$
 *
 * a test for the C++ LaserProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_lbd(PlayerClient* client, int index)
{
  unsigned char access;
  FiducialProxy lbp(client,index,'c');

  printf("device [laserbeacon] index [%d]\n", index);

  TEST("subscribing (read)");
  if((lbp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", lbp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", lbp.driver_name);

  // wait for the laser to warm up
  for(int i=0;i<20;i++)
    client->Read();

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }

    PASS();
    lbp.Print();
  }

  TEST("unsubscribing");
  if((lbp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

