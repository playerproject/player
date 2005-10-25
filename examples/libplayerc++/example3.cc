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
    client.SetFrequency(gFrequency);

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

  }
  catch (PlayerCc::PlayerError e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
  return 1;
}
