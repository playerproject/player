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
 * client-side sonar device 
 */

#ifndef TRUTHPROXY_H
#define TRUTHPROXY_H

#include <clientproxy.h>
#include <playerclient.h>


// Convert radians to degrees
//
#ifndef RTOD
#define RTOD(r) ((r) * 180 / M_PI)
#endif

// Convert degrees to radians
//
#ifndef DTOR
#define DTOR(d) ((d) * M_PI / 180)
#endif

// Normalize angle to domain -pi, pi
//
#ifndef NORMALIZE
#define NORMALIZE(z) atan2(sin(z), cos(z))
#endif


class TruthProxy : public ClientProxy
{
  
  public:
  ///////////////////////////////////////////////////////////
  // system interface
  //
  // the client calls this method to make a new proxy
  //   leave access empty to start unconnected
  TruthProxy(PlayerClient* pc, unsigned short index, 
             unsigned char access = 'c') :
    ClientProxy(pc,PLAYER_TRUTH_CODE,index,access) {};

    
  // interface that all proxies must provide
  void FillData(player_msghdr_t hdr, const char* buffer);
    
  ////////////////////////////////////////////////////////////////////
  // user interface 
  //
  // these methods are the user's interface to this device to get and
  // set the device's position

  // interface that all proxies SHOULD provide
  void Print();

  // these are updated at regular intervals as data arrives, you can
  // read these values directly (setting them does NOT change the
  // device's pose!)
  double x, y, a; // meters, meters, degrees

  // query Player about the current pose - wait for a reply
  int GetPose( double *px, double *py, double *pa );

  // request a change in pose
  int SetPose( double px, double py, double pa );


};

#endif




