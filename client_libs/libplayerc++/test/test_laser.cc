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
  int resolution, intensity, range_res;

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
  min = -90;
  max = +90;
  resolution = 100;
  range_res = 1;
  intensity = 1;
  if(lp.Configure(DTOR(min), DTOR(max), resolution, range_res, intensity) >= 0)
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
  
  lp.PrintConfig();

  TEST("check configuration sanity");
  if((((int)rint(RTOD(lp.min_angle))) != min) ||
     (((int)rint(RTOD(lp.max_angle))) != max))
  {
    FAIL();
    return(-1);
  }
  else if((((int)rint(RTOD(lp.scan_res)*100.0)) != resolution) ||
          (lp.intensity != intensity))
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

