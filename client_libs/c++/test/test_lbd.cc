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
  FiducialProxy lbp(client,index);

  printf("device [laserbeacon] index [%d]\n", index);

  TEST("subscribing (read)");
  if((lbp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    return -1;
  }
  PASS();

  // wait for the laser to warm up
  for(int i=0;i<20;i++)
    client->Read();

  TEST("set bit counts and size");
  if(lbp.SetBits(5, 102) >= 0)
    PASS();
  else
  {
    FAIL();
    return(-1);
  }

  TEST("set thresholds");
  if(lbp.SetThresh(60, 60) >= 0)
    PASS();
  else
  {
    FAIL();
    return(-1);
  }

  TEST("get configuration");
  if(lbp.GetConfig() == 0)
    PASS();
  else
  {
    FAIL();
    return(-1);
  }

  TEST("check configuration sanity");
  if(lbp.bit_count != 5 ||
     abs(lbp.bit_size - 102) > 1 ||
     abs(lbp.one_thresh - 60) > 1 ||
     abs(lbp.zero_thresh - 60) > 1)
  {
    FAIL();
    return(-1);
  }
  PASS();

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

