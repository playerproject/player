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
 * client-side bps device 
 */

#ifndef BPSPROXY_H
#define BPSPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

/** The {\tt BpsProxy} class is used to control the {\tt bps} device.
    The current robot pose can be read from the {\tt px, py, pa} attributes.
    Use the {\tt AddBeacon} method to indicate the location of beacons.
 */
class BpsProxy : public ClientProxy
{

  public:

    /// Current robot pose (global coordinates) in mm, mm, degrees.
    int px, py, pa;

    /// Uncertainty associated with the current pose in mm, mm, degrees.
    int ux, uy, ua;

    // error
    int err;
   
    /** Proxy constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using {\tt PlayerProxy::RequestDeviceAccess}.
    */
    BpsProxy(PlayerClient* pc, unsigned short index, unsigned char access='c') :
            ClientProxy(pc,PLAYER_BPS_CODE,index,access) {}

    /** Add a beacon to the {\tt BPS} device's internal map.
        The beacon pose (global coordinates) must be specified in mm, mm, degrees.
    */
    int AddBeacon(char id, int px, int py, int pa);

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

#endif
