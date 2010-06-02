/*
Copyright (c) 2005, Brad Kratochvil, Toby Collett, Brian Gerkey, Andrew Howard, ...
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the Player Project nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <libplayerc++/playerc++.h>
#include <iostream>
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
// we can also have callbacks in objects
class test_callback
{
  public:

    void read_callback3()
    {
        std::cout << "read_client_callback_3 " << this << std::endl;
    }

};
// we'll use this to stop the client
void read_callback4(PlayerCc::PlayerClient* c)
{
  static uint32_t i(0);
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
    conn1 = cp.ConnectReadSignal(&read_callback1);

    // here we're connecting a signal to a member function
    test_callback test1, test2;
    cp.ConnectReadSignal(boost::bind(&test_callback::read_callback3, boost::ref(test1)));
    cp.ConnectReadSignal(boost::bind(&test_callback::read_callback3, boost::ref(test2)));

    // now, we should see some signals each time Read() is called.
    for (int i=0; i<10; ++i)
    {
      client.Read();
      if (4==i)
        cp.DisconnectReadSignal(conn1);
    }

    // Let's connect a signal to callback4.  This signal tells the client
    // to exit after 10 iterations
    cp.ConnectReadSignal(boost::bind(&read_callback4, &client));

    // Now, let's run the client.  This exits when the client->Stop() function
    // is called from the callback
    client.Run();
  }
  catch (PlayerCc::PlayerError & e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
  return 1;
}
