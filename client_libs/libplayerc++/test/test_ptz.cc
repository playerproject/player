/*
 * $Id$
 *
 * a test for the C++ PositionProxy
 */

#include "test.h"
#if !defined (WIN32)
  #include <unistd.h>
#endif

#include <replace.h>

using namespace PlayerCc;

int
test_ptz(PlayerClient* client, int index)
{
  TEST("ptz");
  try {
    PtzProxy zp(client,index);

    for(int t = 0; t < 3; t++)
    {
      TEST1("reading data (attempt %d)", t);

      client->Read();
      PASS();

      std::cerr << zp << std::endl;
    }

    TEST("panning left");
    zp.SetCam(DTOR(90),0,0);
    usleep(3000000);
    PASS();

    TEST("panning right");
    zp.SetCam(DTOR(-90),0,0);
    usleep(3000000);
    PASS();

    TEST("tilting up");
    zp.SetCam(0,DTOR(25),0);
    usleep(3000000);
    PASS();

    TEST("tilting down");
    zp.SetCam(0,DTOR(-25),0);
    usleep(3000000);
    PASS();

    TEST("zooming in");
    zp.SetCam(0,0,DTOR(10));
    usleep(3000000);
    PASS();

    TEST("zooming out");
    zp.SetCam(0,0,DTOR(60));
    usleep(3000000);
    PASS();


  } catch(std::exception& e) {
    FAIL();
    std::cerr << e.what() << std::endl;
    return -1;
  }

  PASS();
  return 0;
}
