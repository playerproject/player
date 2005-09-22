

/*
 * $Id$
 *
 * a test program with callbacks
 */

#include <iostream>

#include <boost/signal.hpp>
#include <boost/bind.hpp>

#include "playercc.h"

#include "test.h"

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

void read_callback4(PlayerClient* c)
{
  static uint i(0);
  std::cout << "read_client_callback_4: " << i << std::endl;
  if (++i>10)
    c->Stop();
}



int test_client(PlayerClient *client, int index)
{
  try
  {
    using namespace PlayerCc;

    TEST("PlayerClient");
    {
      TEST("SetFrequency");
      client->SetFrequency(30);
      PASS();
      TEST("SetDataMode");
      client->SetDataMode(PLAYER_DATAMODE_PUSH_NEW);
      PASS();
    }


    //ClientProxy c(client);
    CameraProxy cp(client, index);
    TEST("ClientProxy");

    //client->Read();
    //std::cout << cp << std::endl;

    TEST("Signal Connect");

    test_callback test1, test2;
    ClientProxy::connection_t conn1, conn2;

    conn1 = cp.ConnectSignal(&read_callback1);
    conn2 = cp.ConnectSignal(&read_callback2);
    cp.ConnectSignal(boost::bind(&test_callback::read_callback3, boost::ref(test1)));
    cp.ConnectSignal(boost::bind(&test_callback::read_callback3, boost::ref(test2)));
    PASS();

    TEST("user read");
    for (int i=0; i<10; ++i)
    {
      client->Read();

      if (4==i)
      {
        cp.DisconnectSignal(conn1);
        cp.DisconnectSignal(conn2);
      }
    }
    PASS();

    TEST("run");
    cp.ConnectSignal(boost::bind(&read_callback4, client));
    client->Run();
    PASS();

  }
  catch (PlayerCc::PlayerError e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
  return 1;
}
