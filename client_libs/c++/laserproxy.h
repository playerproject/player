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


/** The {\tt LaserProxy} class is used to control the {\tt laser} device.
    The latest scan data is held in two arrays: {\tt ranges} and {\tt
    intensity}.  The laser scan range, resolution and so on can be
    configured using the {\tt Configure()} method.
*/
class LaserProxy : public ClientProxy
{

  public:

    // the latest laser scan data
    
    /** Scan range for the latest set of data.
        Angles are measured in units of $0.01^{\circ}$,
        in the range -9000 ($-90^{\circ}$) to
        +9000 ($+90^{\circ}$).
    */
    short min_angle; short max_angle;

    /** Scan resolution for the latest set of data.
        Resolution is measured in units of $0.01^{\circ}$.
        Valid values are: 25, 50, and 100.
    */
    unsigned short resolution;

    /** Whether or not reflectance values are returned.
      */
    bool intensity;

    /// The number of range measurements in the latest set of data.
    unsigned short range_count;

    /// The range values (in mm).
    unsigned short ranges[PLAYER_NUM_LASER_SAMPLES];

    // TODO: haven't verified that intensities work yet:
    /// The reflected intensity values (arbitrary units in range 0-7).
    unsigned char intensities[PLAYER_NUM_LASER_SAMPLES];

    // What is this?
    unsigned short min_right,min_left;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    LaserProxy(PlayerClient* pc, unsigned short index, 
               unsigned char access='c'):
        ClientProxy(pc,PLAYER_LASER_CODE,index,access) {}

    // these methods are the user's interface to this device

    // returns the local rectangular coordinate of the i'th beam strike
    int CartesianCoordinate( int i, int *x, int *y );

    // configure the laser scan.
    /** Configure the laser scan pattern.
        Angles {\tt min\_angle} and {\tt max\_angle} are measured in
        units of $0.1^{\circ}$, in the range -9000
        ($-90^{\circ}$) to +9000 ($+90^{\circ}$).  {\tt
        resolution} is also measured in units of $0.1^{\circ}$; valid
        values are: 25 ($0.25^{\circ}$), 50 ($0.5^{\circ}$) and $100
        (1^{\circ}$).  Set {\tt intensity} to {\tt true} to enable
        intensity measurements, or {\tt false} to disable.\\
        Returns the 0 on success, or -1 of there is a problem.
    */
    int Configure(short min_angle, short max_angle, 
                  unsigned short resolution, bool intensity);

    /** Get the current laser configuration; it is read into the
        relevant class attributes.\\
        Returns the 0 on success, or -1 of there is a problem.
      */
    int GetConfigure();

    /** Range access operator.
        This operator provides an alternate way of access the range data.
        For example, given a {\tt LaserProxy} named {\tt lp}, the following
        expressions are equivalent: \verb+lp.ranges[0]+ and \verb+lp[0]+.
    */
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
