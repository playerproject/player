/*
 * $Id$
 *
 * a simple c++ client application
 */
#include <iostream>

#include "playerc++.h"

int main(int argc, char** argv)
{
  try
  {
    PlayerCc::PlayerClient client("localhost", 6665);
    std::cout << client << std::endl;

    PlayerCc::CameraProxy cp(&client, 0);

    for (int i=0; i<10; ++i)
    {
      client.Read();
      std::cout << cp << std::endl;
    }
  }
  catch (PlayerCc::PlayerError e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
  return 1;
}
