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

/** The {\tt TruthProxy} gets and sets the {\em true} pose of a truth
    device [worldfile tag: truth()]. This may be different from the
    pose returned by a device such as GPS or Position. If you want to
    log what happened in an experiment, this is the device to
    use. 

    Setting the position of a truth device moves its parent, so you
    can put a truth device on robot and teleport it around the place. 
 */
class TruthProxy : public ClientProxy
{
  
  public:

  /** These vars store the current device pose (x,y,a) as
      (m,m,radians). The values are updated at regular intervals as
      data arrives. You can read these values directly but setting
      them does NOT change the device's pose!. Use {\tt
      TruthProxy::SetPose()} for that.  
*/
  double x, y, a; 


  /** Constructor.
      Leave the access field empty to start unconnected.
      You can change the access later using 
      {\tt PlayerProxy::RequestDeviceAccess}.
  */
  TruthProxy(PlayerClient* pc, unsigned short index, 
             unsigned char access = 'c') :
    ClientProxy(pc,PLAYER_TRUTH_CODE,index,access) {};

    
  // interface that all proxies must provide
  void FillData(player_msghdr_t hdr, const char* buffer);
    
  // interface that all proxies SHOULD provide
  void Print();

  /** Query Player about the current pose - requests the pose from the
      server, then fills in values for the arguments
      (m,m,radians). Usually you'll just read the {\tt x,y,a}
      attributes but this function allows you to get pose direct from
      the server if you need too. Returns 0 on success, -1 if there is
      a problem.  
  */
  int GetPose( double *px, double *py, double *pa );

  /** Request a change in pose (m,m,radians). Returns 0 on success, -1
      if there is a problem.  
  */
  int SetPose( double px, double py, double pa );


};

#endif




