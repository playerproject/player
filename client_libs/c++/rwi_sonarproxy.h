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

#ifndef _RWI_SONARPROXY_H
#define _RWI_SONARPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

/** The {\tt SonarProxy} class is used to control the {\tt rwi_sonar} device.
    The most recent sonar range measuremts can be read from {\tt Ranges()}
    or thru the operator[] accessors.
 */
class RWISonarProxy : public ClientProxy {

public:
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess}.
    */
    RWISonarProxy (PlayerClient* pc, unsigned short index,
                   unsigned char access = 'c')
            : ClientProxy(pc,PLAYER_RWI_SONAR_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Enable/disable the sonars.
        Set {\tt state} to 1 to enable, 0 to disable.
        Note that when sonars are disabled the client will still receive sonar
        data, but the ranges will always be the last value read from the
        sonars before they were disabled.
        Returns 0 on success, -1 if there is a problem.
     */
    int SetSonarState (const unsigned char state) const;

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    // interface that all proxies SHOULD provide
    void Print ();

    /** Accessors
     */
    uint16_t Ranges (const unsigned int index) const
    {
    	if (index < range_count)
    		return ranges[index];
    	else
    		return 0;
    }

    uint16_t operator [] (unsigned int index) const
    {
      return Ranges(index);
    }


private:
    /** The latest sonar scan data.
        Range is measured in mm.
     */
    uint16_t range_count;
    uint16_t ranges[PLAYER_NUM_SONAR_SAMPLES];
};

#endif // _RWI_SONARPROXY_H
