/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 *
 * client-side speech device 
 */

#ifndef SPEECHPROXY_H
#define SPEECHPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

/** The {\tt SpeechProxy} class is used to control the
    {\tt speech} device.
    Use the {\tt say} method to send things to say.
*/
class SpeechProxy : public ClientProxy
{

  public:
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    SpeechProxy(PlayerClient* pc, unsigned short index, 
                unsigned char access='c'):
            ClientProxy(pc,PLAYER_SPEECH_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Send a phrase to say.
        The phrase is an ASCII string.
        Returns the 0 on success, or -1 of there is a problem.
    */
    int Say(char* str);
};

#endif
