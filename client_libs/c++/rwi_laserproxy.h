/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2002
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *     Nik Melchior
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

#ifndef _RWI_LASERPROXY_H
#define _RWI_LASERPROXY_H

#include <clientproxy.h>
#include <playerclient.h>


/** The {\tt LaserProxy} class is used to control the {\tt rwi_laser} device.
    The latest scan data is held in the {\tt ranges} array.
*/
class RWILaserProxy : public ClientProxy {
public:
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    RWILaserProxy (PlayerClient* pc, unsigned short index,
                   unsigned char access='c')
        : ClientProxy(pc,PLAYER_RWI_LASER_CODE,index,access) {}

    // these methods are the user's interface to this device

    // returns the local rectangular coordinate of the i'th beam strike
    int CartesianCoordinate (const int i, int *x, int *y) const;

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char *buffer);

    // interface that all proxies SHOULD provide
    void Print ();

    /** Accessors
     */
    int  RangeCount () const { return range_count; }
    uint16_t Ranges (const unsigned int index) const
    {
    	if (index < range_count)
    		return ranges[index];
    	else
    		return 0;
    }
    uint16_t MinLeft () const { return min_left; }
    uint16_t MinRight () const { return min_right; }
    /** Range access operator.
        This operator provides an alternate way of access the range data.
        For example, given an {\tt RWILaserProxy} named {\tt lp}, the following
        expressions are equivalent: \verb+lp.Ranges(0)+ and \verb+lp[0]+.
    */
    uint16_t operator [] (unsigned int index) const
    {
      return Ranges(index);
    }

private:
    // the latest laser scan data
    /// The range values (in mm).
    uint16_t range_count;
    uint16_t ranges[PLAYER_NUM_LASER_SAMPLES];

    // The shortest distances towards the `right' or `left' side of the robot
    uint16_t min_left, min_right;
};

#endif // _RWI_LASERPROXY_H
