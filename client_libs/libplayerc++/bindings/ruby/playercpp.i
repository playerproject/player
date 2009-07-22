%module playercpp
%{
#include "libplayerc++/playerc++.h"
%}

/* ignore for boost related gotchas */
%ignore PlayerCc::PlayerClient::mutex_t;
%ignore PlayerCc::PlayerClient::mMutex;
%ignore PlayerCc::ClientProxy::DisconnectReadSignal;

/* handle std::string and so on */
%include stl.i
%include stdint.i

%include "libplayerc++/playerclient.h"
%include "libplayerc++/clientproxy.h"
%include "libplayerc++/playerc++.h"
%include "libplayerinterface/player.h"
