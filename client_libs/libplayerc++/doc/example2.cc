/*
 * $Id$
 *
 * a more complicated C++ example with signals
 */

#include "playercc.h"

#include <boost/signal.hpp>
#include <boost/bind.hpp>

// these are our callback functions.  Currently, they all need to return void.

void read_callback1()
{
    std::cout << "read_client_callback_1" << std::endl;
}

void read_callback2()
{
    std::cout << "read_client_callback_2" << std::endl;
}

class test_callback
{
  public:

    void read_callback3()
    {
        std::cout << "read_client_callback_3 " << this << std::endl;
    }

};

void read_callback4(PlayerCc::PlayerClient* c)
{
  static uint i(0);
  std::cout << "read_client_callback_4: " << i << std::endl;
  if (++i>10)
    c->Stop();
}

int main(int argc, char** argv)
{
  try
  {
    using namespace PlayerCc;

    PlayerClient client("localhost", 6665);
    CameraProxy cp(&client, 0);

    // Here, we're connecting a signal to a function.  We keep the connection_t
    // so we can later disconnect.
    ClientProxy::connection_t conn1;
    conn1 = cp.ConnectSignal(&read_callback1);

    // here we're connecting a signal to a member function
    test_callback test1, test2;
    cp.ConnectSignal(boost::bind(&test_callback::read_callback3, boost::ref(test1)));
    cp.ConnectSignal(boost::bind(&test_callback::read_callback3, boost::ref(test2)));

    // now, we should see some signals each time Read() is called.
    for (int i=0; i<10; ++i)
    {
      client.Read();
      if (4==i)
        cp.DisconnectSignal(conn1);
    }

    // Let's connect a signal to callback4.  This signal tells the client
    // to exit after 10 iterations
    cp.ConnectSignal(boost::bind(&read_callback4, &client));

    // Now, let's run the client.  This exits when teh client->Stop() function
    // is called from the callback
    client.Run();
  }
  catch (PlayerCc::PlayerError e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
  return 1;
}
