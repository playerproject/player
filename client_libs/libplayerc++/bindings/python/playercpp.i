/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2009
 *     Piotr Trojanek
 *                      
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

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

