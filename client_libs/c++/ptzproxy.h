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
 * client-side ptz device 
 */

#ifndef PTZPROXY_H
#define PTZPROXY_H

#include <clientproxy.h>
#include <playerclient.h>


/** The {\tt PtzProxy} class is used to control the {\tt ptz} device.
    The state of the camera can be read from the {\tt pan, tilt, zoom}
    attributes and changed using the {\tt SetCam()} method.
 */
class PtzProxy : public ClientProxy
{

  public:
    /// Pan and tilt values (degrees).
    short pan, tilt;

    /// Zoom value (0 -- 1024, where 0 is wide angle and 1024 is telephoto).
    unsigned short zoom;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    PtzProxy(PlayerClient* pc, unsigned short index, unsigned char access='c'):
            ClientProxy(pc,PLAYER_PTZ_TYPE,index,access) {}

    // these methods are the user's interface to this device

    /** Change the camera state.
        Specify the new {\tt pan} and {\tt tilt} values (degrees)
        and the new {\tt zoom} value (0 -- 1024).
        Returns: 0 if everything's ok, -1 otherwise.
    */
    int SetCam(short pan, short tilt, unsigned short zoom);

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

#endif
