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
 * client-side gripper device 
 */

#ifndef GRIPPERPROXY_H
#define GRIPPERPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

/* gripper stuff */
#define GRIPopen   1
#define GRIPclose  2
#define GRIPstop   3
#define LIFTup     4
#define LIFTdown   5
#define LIFTstop   6
#define GRIPstore  7
#define GRIPdeploy 8
#define GRIPhalt   15
#define GRIPpress  16
#define LIFTcarry  17

class GripperProxy : public ClientProxy
{

  public:
    // the latest gripper data
    unsigned char state,beams;
   
    // the client calls this method to make a new proxy
    //   leave access empty to start unconnected
    GripperProxy(PlayerClient* pc, unsigned short index, 
                 unsigned char access='c') :
            ClientProxy(pc,PLAYER_GRIPPER_CODE,index,access) {}

    // these methods are the user's interface to this device

    // send a gripper command
    //
    // Returns:
    //   0 if everything's ok
    //   -1 otherwise (that's bad)
    int SetGrip(unsigned char cmd, unsigned char arg);

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

#endif
