/*
 * $Id$
 *
 * a test for the C++ LaserProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>
#include <math.h>

int
test_laser(PlayerClient* client, int index)
{
  unsigned char access;
  LaserProxy lp(client,index,'c');
  int min, max;
  int resolution, intensity;

  printf("device [laser] index [%d]\n", index);

  TEST("subscribing (read)");
  if((lp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", lp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", lp.driver_name);

  // wait for the laser to warm up
  for(int i=0;i<20;i++)
    client->Read();

  TEST("set configuration");
  min = -90*100;
  max = +90*100;
  resolution = 100;
  intensity = 1;
  if(lp.Configure(min, max, resolution, intensity) >= 0)
    PASS();
  else
  {
    FAIL();
    return(-1);
  }

  TEST("get configuration");
  if(lp.GetConfigure() == 0)
    PASS();
  else
  {
    FAIL();
    return(-1);
  }

  TEST("check configuration sanity");
  if(abs(lp.min_angle + (90*100)) || 
     abs(lp.max_angle - (90*100)))
  {
    FAIL();
    return(-1);
  }
  else if(lp.resolution != 100 || lp.intensity != 1)
  {
    FAIL();
    return(-1);
  }
  else
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
    lp.Print();
  }

  TEST("unsubscribing");
  if((lp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

