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

#ifndef SONARPROXY_H
#define SONARPROXY_H

#include <clientproxy.h>
#include <playerclient.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

/** The {\tt SonarProxy} class is used to control the {\tt sonar} device.
    The most recent sonar range measuremts can be read from the {\tt range}
    attribute, or using the the {\tt []} operator.
 */
class SonarProxy : public ClientProxy
{

  public:
    /** The latest sonar scan data.
        Range is measured in mm.
     */
    unsigned short ranges[PLAYER_NUM_SONAR_SAMPLES];

    /** Positions of sonars
     */
    player_sonar_geom_t sonar_pose;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using {\tt PlayerProxy::RequestDeviceAccess}.
    */
    SonarProxy(PlayerClient* pc, unsigned short index, 
               unsigned char access = 'c') :
            ClientProxy(pc,PLAYER_SONAR_CODE,index,access)
    { bzero(&sonar_pose,sizeof(sonar_pose)); }

    // these methods are the user's interface to this device
    
    /** Enable/disable the sonars.
        Set {\tt state} to 1 to enable, 0 to disable.
        Note that when sonars are disabled the client will still receive sonar
        data, but the ranges will always be the last value read from the sonars
        before they were disabled.\\
        Returns 0 on success, -1 if there is a problem.
     */
    int SetSonarState(unsigned char state);

    int GetSonarGeom();

    /** Range access operator.
        This operator provides an alternate way of access the range data.
        For example, given a {\tt SonarProxy} named {\tt sp}, the following
        expressions are equivalent: \verb+sp.ranges[0]+ and \verb+sp[0]+.
     */
    unsigned short operator [](unsigned int index) 
    { 
      if(index < sizeof(ranges))
        return(ranges[index]);
      else 
        return(0);
    }

    /** Get the pose of a particular sonar.
        This is a convenience function that returns the pose of any
        sonar on a Pioneer2DX robot.  It will {\em not} return valid
        poses for other configurations.
    */
    void GetSonarPose(int s, double* px, double* py, double* pth);
    
    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

#endif
