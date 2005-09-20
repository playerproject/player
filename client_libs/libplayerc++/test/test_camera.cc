/*
 * $Id$
 *
 * a test for the C++ CameraProxy
 */

#include "playercc.h"
#include "test.h"

int test_camera(PlayerClient *client, int index)
{
  TEST("camera");
  try
  {
    using namespace std;
    using namespace PlayerCc;

    CameraProxy cp(client, index);

    cout << static_cast<ClientProxy>(cp) << endl;

    for (int i=0; i<10; ++i)
    {
      TEST("read camera");
      client->Read();
      PASS();

      cout << cp << endl;

      if (i>5)
      {
        TEST("save frame");
        cp.SaveFrame("test_");
        PASS();
      }
    }
  }
  catch (PlayerCc::PlayerError e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
  return 1;
}
