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
 * parent class for client-side device proxies
 */

#ifndef CLIENTPROXY_H
#define CLIENTPROXY_H

#include <sys/time.h>
#include <playercclient.h>

// forward declaration for friending
class PlayerClient;


/* Base class for all proxy devices.
 */
class ClientProxy
{
  // GCC 3.0 requires this new syntax
  friend class PlayerClient; // ANSI C++ syntax?
  // friend PlayerClient; // syntax deprecated

 
  public:         
    PlayerClient* client;  // our controlling client object

    // if this generic proxy is not subclassed, the most recent 
    // data and header get copied in here. that way we can use this
    // base class on it's own as a generic proxy
    unsigned char last_data[4096];
    player_msghdr_t last_header;

    unsigned char access;   // 'r', 'w', or 'a' (others?)
    unsigned short device; // the name by which we identify this kind of device
    unsigned short index;  // which device we mean

    struct timeval timestamp;  // time at which this data was sensed
    struct timeval senttime;   // time at which this data was sent
    struct timeval receivedtime; // time at which this data was received

    /* Constructor.
        The constructor will try to get access to the device
        (unless \p req_device is 0 or \p req_access is 'c').
    */
    ClientProxy(PlayerClient* pc, 
		unsigned short req_device,
		unsigned short req_index,
		unsigned char req_access = 'c' );

    // destructor will try to close access to the device
    virtual ~ClientProxy();

    unsigned char GetAccess() { return(access); };  

    // methods for changin access mode
    int ChangeAccess(unsigned char req_access, 
                     unsigned char* grant_access=NULL );
    int Close() { return(ChangeAccess('c')); }

    // interface that all proxies must provide
    virtual void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    virtual void Print();
};

#endif
