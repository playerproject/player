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

#ifndef _RWI_BUMPERPROXY_H
#define _RWI_BUMPERPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

/** The {\tt RWIBumperProxy} class is used to read from the {\tt rwi_bumper}
	device.
 */
class RWIBumperProxy : public ClientProxy {

public:
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess}.
    */
    RWIBumperProxy (PlayerClient* pc, unsigned short index,
                   unsigned char access = 'c')
            : ClientProxy(pc,PLAYER_RWI_BUMPER_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Enable/disable the bumpers.
        Set {\tt state} to 1 to enable, 0 to disable.
        Note that when bumpers are disabled the client will still receive
        bumper data, but the states will always be the last value read from
        the bumpers before they were disabled.
        Returns 0 on success, -1 if there is a problem.
     */
    int SetBumperState (const unsigned char state) const;

    /** These methods return 1 if the specified bumper(s)
        have been bumped, 0 otherwise.
      */
    int Bumped (const unsigned int i) const;
    int BumpedAny () const;

    uint32_t GetBumpfield () const { return bumpfield; }

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    // interface that all proxies SHOULD provide
    void Print ();

private:
    /** Bitfield representing bumped state.
     */
    uint8_t  bumper_count;
    uint32_t bumpfield;
};

#endif // _RWI_BUMPERPROXY_H
