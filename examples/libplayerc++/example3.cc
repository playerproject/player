#include <libplayerc++/playerc++.h>
#include <iostream>
#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include <cmath>

#include <time.h>
// these are our callback functions.  Currently, they all need to return void.
void read_callback1()
{
    std::cout << "callback_1" << std::endl;
}

void read_callback2(uint &aI)
{
    std::cout << "callback_2" << " " << ++aI << std::endl;
}

// we can also have callbacks in objects
class test_callback
{
  public:

    test_callback(int aId) : mId(aId) {};

    void read_callback3()
      { std::cout << "callback_3 " << mId << std::endl; }

    void read_callback4(uint aOpt = 0)
      { std::cout << "callback_3 " << mId << " " << aOpt << std::endl; }

    int mId;
};

int main(int argc, char** argv)
{
  try
  {
    using namespace PlayerCc;

    PlayerClient client("localhost", 6665);
    CameraProxy cp(&client, 0);
    PtzProxy   ptz(&client, 0);

    // Here, we're connecting a signal to a function.  We keep the connection_t
    // so we can later disconnect.
    ClientProxy::connection_t conn1;
    conn1 = cp.ConnectReadSignal(&read_callback1);

    ClientProxy::connection_t conn2;
    uint count = 0;
    conn2 = cp.ConnectReadSignal(boost::bind(&read_callback2, count));

    // here we're connecting a signal to a member function
    test_callback test1(1), test2(2);
    cp.ConnectReadSignal(boost::bind(&test_callback::read_callback3, boost::ref(test1)));
    cp.ConnectReadSignal(boost::bind(&test_callback::read_callback4, boost::ref(test2), 1));

    // now, we should see some signals each time Read() is called.
    client.StartThread();

    timespec sleep = {0, 1000000};
    for (;;)
    {
      //std::cout << ptz;
      //ptz.SetCam(ptz.GetPan()+DTOR(10*pow(-1, i%2)), 0, 0);
      ptz.SetCam(ptz.GetPan(), 0, 0);
      nanosleep(&sleep, NULL);
    }

    client.StopThread();
  }
  catch (PlayerCc::PlayerError e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
  return 1;
}
