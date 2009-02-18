/*
 * $Id$
 *
 * a test for the C++ SonarProxy
 */

#include "test.h"
#if !defined (WIN32)
  #include <unistd.h>
#endif

#include <playerconfig.h>
#if !HAVE_USLEEP
  #include <replace.h>
#endif

using namespace PlayerCc;

int
test_gripper(PlayerClient* client, int index)
{
  GripperProxy gp(client,index);

  // wait for P2OS to start up
  for(int i=0; i < 20; i++)
    client->Read();

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);
    client->Read();

    std::cerr << "got gripper data: " << std::endl << gp << std::endl;

    PASS();
  }

  TEST("gripper open");
  gp.Open();
  usleep(5000000);
  PASS();

  TEST("gripper close");
  gp.Close();
  usleep(8000000);
  PASS();

  TEST("gripper open");
  gp.Open();
  usleep(5000000);
  PASS();

  TEST("gripper store object (only on some grippers, e.g. stage)");
  gp.Store();
  usleep(3000000);
  PASS();

  TEST("gripper retrieve object (only on some grippers, e.g. stage");
  gp.Retrieve();
  usleep(3000000);
  PASS();

  PASS();
  return(0);
}

