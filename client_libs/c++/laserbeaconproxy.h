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
 * client-side laser beacon device 
 */

#ifndef LASERBEACONPROXY_H
#define LASERBEACONPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

/** The {\tt LaserBeaconProxy} class is used to control the
    {\tt laserbeacon} device.
    The latest set of detected beacons is stored in the
    {\tt beacons} array.
    The {\tt laserbeacon} device may be configured using
    the {\tt SetBits()} and {\tt SetThresh()} methods.
*/
class LaserbeaconProxy : public ClientProxy
{

  public:
    // the latest laser beacon data

    /** The latest laser beacon data.
        Each beacon has the following information:
        \begin{verbatim}
        uint8_t id;
        uint16_t range;
        int16_t bearing;
        int16_t orient;
        \end{verbatim}
        where {\tt range} is measured in mm, and {\tt bearing} and
        {\tt orient} are measured in degrees.
     */
    player_laserbeacon_item_t beacons[PLAYER_MAX_LASERBEACONS];

    /** The number of beacons detected
     */
    unsigned short count;

    /** The current bit count of the laserbeacon device.
        See the Player manual for information on this setting.
      */
    unsigned char bit_count;

    /** The current bit size (in mm) of the laserbeacon device.
        See the Player manual for information on this setting.
      */
    unsigned short bit_size;

    /** The current zero threshold of the laserbeacon device.
        See the Player manual for information on this setting.
      */
    unsigned short zero_thresh;

    /** The current one threshold of the laserbeacon device.
        See the Player manual for information on this setting.
      */
    unsigned short one_thresh;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    LaserbeaconProxy(PlayerClient* pc, unsigned short index,
                     unsigned char access='c'):
            ClientProxy(pc,PLAYER_LASERBEACON_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Set the bit properties of the beacons.
        Set {\tt bit\_count} to the number of bits in your beacons
        (usually 5 or 8).
        Set {\tt bit\_size} to the width of each bit (in mm).\\
        Returns the 0 on success, or -1 of there is a problem.
     */
    int SetBits(unsigned char bit_count, unsigned short bit_size);

    /** Set the identification thresholds.
        Thresholds must be in the range 0--99.
        See the Player manual for a full discussion of these settings.\\
        Returns the 0 on success, or -1 of there is a problem.
     */
    int SetThresh(unsigned short zero_thresh, unsigned short one_thresh);

    /** Get the current configuration.
        Fills the current device configuration into the corresponding
        class attributes.\\
        Returns the 0 on success, or -1 of there is a problem.
     */
    int GetConfig();
    
    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

#endif
