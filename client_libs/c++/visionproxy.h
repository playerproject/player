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
 * client-side vision device 
 */

#ifndef VISIONPROXY_H
#define VISIONPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

class Blob
{
  public:
    unsigned int area;
    unsigned char x;
    unsigned char y;
    unsigned char left;
    unsigned char right;
    unsigned char top;
    unsigned char bottom;
};


/** The {\tt VisionProxy} class is used to control the {\tt vision} device.
    It contains no methods.  The latest color blob data is stored in
    {\tt blobs}, a dynamically allocated 2-D array, indexed by color
    channel.
*/
class VisionProxy : public ClientProxy
{

  public:
    // the latest vision data

    /** Array containing the latest blob data.
        Each blob contains the following information:
        \begin{verbatim}
        unsigned int area;
        unsigned char x;
        unsigned char y;
        unsigned char left;
        unsigned char right;
        unsigned char top;
        unsigned char bottom;
        \end{verbatim}
        The number of blobs in each channel is given by
        {\tt num\_blobs}, i.e. the number of blobs in channel
        {\tt i} is {\tt num\_blobs[i]}.
     */
    char num_blobs[ACTS_NUM_CHANNELS];
    Blob* blobs[ACTS_NUM_CHANNELS];
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    VisionProxy(PlayerClient* pc, unsigned short index, 
                unsigned char access='c');
    ~VisionProxy();

    // these methods are the user's interface to this device

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

#endif
