/*
 * $Id$
 *
 * a test for the C++ LaserProxy
 */

#include "test.h"
#if !defined (WIN32)
  #include <unistd.h>
#endif
#include <math.h>

#if defined (WIN32)
  #include <replace.h>
#endif

int
test_laser(PlayerClient* client, int index)
{
  TEST("laser");
  LaserProxy lp(client,index);

  // wait for the laser to warm up
  for(int i=0;i<20;i++)
    client->Read();

  TEST("set configuration");
  int min = -90;
  int max = +90;
  int resolution = 100;
  int range_res = 1;
  bool intensity = true;
  double scanning_frequency = 10;
  lp.Configure(DTOR(min), DTOR(max), resolution, range_res, intensity, scanning_frequency);

  TEST("get configuration");
  lp.RequestConfigure();
  
  TEST("check configuration sanity");
#if defined (WIN32)
  if((((int)round(RTOD(lp.GetMinAngle()))) != min) ||
     (((int)round(RTOD(lp.GetMaxAngle()))) != max))
#else
  if((((int)rint(RTOD(lp.GetMinAngle()))) != min) ||
     (((int)rint(RTOD(lp.GetMaxAngle()))) != max))
#endif
  {
    FAIL();
    return(-1);
  }
#if defined (WIN32)
  else if((((int)round(RTOD(lp.GetScanRes())*100.0)) != resolution) ||
          (lp.GetRangeRes() != range_res))
#else
  else if((((int)rint(RTOD(lp.GetScanRes())*100.0)) != resolution) ||
          (lp.GetRangeRes() != range_res))
#endif
  {
    FAIL();
    return(-1);
  }
  else
    PASS();

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    client->Read();

    std::cerr << "read laser data: " << std::endl << lp << std::endl;
    PASS();
  }



  PASS();
  return 0;
}

