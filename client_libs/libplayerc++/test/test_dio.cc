/*
 * $Id$
 *
 * a test for the C++ PositionProxy
 */

#include "test.h"
#include <unistd.h>

using namespace PlayerCc;

int
test_dio(PlayerClient* client, int index)
{
  TEST("dio");
  DioProxy dp(client,index);

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    client->Read();

    PASS();

    std::cerr << dp << std::endl;
  }

  PASS();

  return(0);
}

