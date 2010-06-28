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
#include <list>
#include <algorithm>
#include <functional>

int main(int argc, char** argv)
{
  try
  {
    using namespace PlayerCc;
    // Let's subscribe to a couple of different clients, and
    // set up a proxy or two.
    PlayerClient client1("feyd");
    CameraProxy  cp1(&client1);

    PlayerClient client2("rabban");
    CameraProxy  cp2(&client2);

    // We can now create a list of pointers to PlayerClient,
    // and add two elements
    std::list<PlayerClient*> m_client;
    m_client.push_back(&client1);
    m_client.push_back(&client2);

    while (1)
    {
      // this will now iterate through the list of clients and read from
      // all of them that have data waiting
      std::for_each(m_client.begin(),
                    m_client.end(),
                    std::mem_fun(&PlayerClient::ReadIfWaiting));

      // output the proxies just for fun
      std::cout << cp1 << std::endl;
      std::cout << cp2 << std::endl;
    }

  }
  catch (PlayerCc::PlayerError & e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
  return 1;
}
