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
 * client-side gps device 
 */

#ifndef GPSPROXY_H
#define GPSPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

/** The {\tt GpsProxy} class is used to control the {\tt gps} device.
    The latest pose data is stored in three class attributes.  The robot
    can be ``teleported'' with the {\tt Warp()} method
  */
class GpsProxy : public ClientProxy
{

  public:
    // the latest GPS data

    /** The latest global pose (in mm, mm, and degrees, respectively)
     */
    int xpos,ypos,heading;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    GpsProxy(PlayerClient* pc, unsigned short index, 
              unsigned char access='c') :
            ClientProxy(pc,PLAYER_GPS_TYPE,index,access) {}

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

#endif
