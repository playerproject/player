#include "test.h"
#if !defined (WIN32)
  #include <unistd.h>
#endif

int
test_ranger(PlayerClient* client, int index)
{
  TEST("ranger");
  RangerProxy rp(client,index);

  // wait for P2OS to start up, throwing away data as fast as possible
  for(int i=0; i < 60; i++)
  {
    client->Read();
  }

  rp.RequestGeom();

  std::cout << "There are " << rp.GetElementCount() << " individual range sensors.\n";

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    client->Read();

    PASS();
    std::cout << rp << std::endl;

  }

  PASS();
  return 0;
}

