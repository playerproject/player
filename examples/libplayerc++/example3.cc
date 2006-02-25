// For anyone who looks at this file:
//
// I'm using this as a playground for *contemplating* new functionality of the
// libplayerc++ library, so it may not make any sense.
//
// You've been warned...

#include <libplayerc++/playerc++.h>
#include <iostream>
#include <unistd.h>

#include "args.h"

template<typename T>
void Print(T t)
{
  std::cout << t << std::endl;
}

template<typename T>
void Print_Ref(T t)
{
  std::cout << *t << std::endl;
}

int main(int argc, char** argv)
{
  try
  {
    using namespace PlayerCc;

    parse_args(argc, argv);

    PlayerClient client(gHostname, gPort);

    client.SetDataMode(gDataMode);

    client.RequestDeviceList();
    std::list<playerc_device_info_t> dlist(client.GetDeviceList());

    for_each(dlist.begin(), dlist.end(), Print<playerc_device_info_t>);

    std::list<ClientProxy*> mProxyList;

    CameraProxy  cp(&client, gIndex);
    PtzProxy  pp(&client, gIndex);

    mProxyList.push_back(&cp);
    mProxyList.push_back(&pp);

    for_each(mProxyList.begin(),
             mProxyList.end(),
             Print_Ref<ClientProxy*>);

    // testing PlayerMultiClient

    PlayerClient client1("localhost");
    PlayerClient client2("feyd");

    std::list<PlayerClient*> m_client;

    m_client.push_back(&client1);
    m_client.push_back(&client2);

    CameraProxy  cp1(&client1);
    PtzProxy     pp1(&client1);

    std::for_each(m_client.begin(),
                  m_client.end(),
                  std::mem_fun(&PlayerClient::ReadIfWaiting));

  }
  catch (PlayerCc::PlayerError e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
  return 1;
}
