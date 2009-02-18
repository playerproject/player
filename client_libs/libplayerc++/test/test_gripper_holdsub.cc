/*
 * $Id$
 *
 * a test for the C++ SonarProxy
 */

#include "test.h"
#if !defined (WIN32)
  #include <unistd.h>
#endif

using namespace PlayerCc;

int
test_gripper_holdsubscribe(PlayerClient* client, int index)
{
  GripperProxy gp(client,index);

  // wait for P2OS to start up
  for(int i=0; i < 20; i++)
    client->Read();

  while(1)
  {
    client->Read();

    std::cerr << "got gripper data: " << std::endl << gp << std::endl;

  }

  return(-1);
}


 	  	 
