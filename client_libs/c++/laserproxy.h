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
 * client-side laser device 
 */

#ifndef LASERPROXY_H
#define LASERPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

class LaserProxy : public ClientProxy
{

  public:
    // the latest laser scan data
    short min_angle;
    short max_angle;
    unsigned short resolution;
    unsigned short range_count;
    unsigned short ranges[PLAYER_NUM_LASER_SAMPLES];
    // TODO: haven't verified that intensities work yet:
    unsigned char intensities[PLAYER_NUM_LASER_SAMPLES];
    unsigned short min_right,min_left;
   
    // the client calls this method to make a new proxy
    //   leave access empty to start unconnected
    LaserProxy(PlayerClient* pc, unsigned short index, 
               unsigned char access='c'):
            ClientProxy(pc,PLAYER_LASER_CODE,index,access) {}

    // these methods are the user's interface to this device

    // configure the laser scan.
    int Configure(short min_angle, short max_angle, 
                  unsigned short resolution, bool intensity);

    // a different way to access the range data
    unsigned short operator [](unsigned int index)
    { 
      if(index < sizeof(ranges))
        return(ranges[index]);
      else 
        return(0);
    } 
    
    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

#endif
