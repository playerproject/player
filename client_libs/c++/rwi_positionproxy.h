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

#ifndef _RWI_POSITIONPROXY_H
#define _RWI_POSITIONPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

/** The {\tt RWIPositionProxy} class is used to control the {\tt rwi_position} device.
    The latest position data is contained in the attributes {\tt xpos, ypos}, etc.
 */
class RWIPositionProxy : public ClientProxy {

public:
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    RWIPositionProxy (PlayerClient* pc, unsigned short index,
                      unsigned char access ='c')
        : ClientProxy(pc,PLAYER_RWI_POSITION_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Send a motor command.
        Specify the linear and angular speed in m/s and degrees/sec,
        respectively.\\
        Returns: 0 if everything's ok, -1 otherwise.
    */
    int SetSpeed (const int16_t speed, const int16_t turnrate) const;

    /** Enable/disable the motors.
        Set {\tt state} to 0 to disable (default) or 1 to enable.
        Be VERY careful with this method!  Your robot is likely to run across the
        room with the charger still attached.
        
        Returns: 0 if everything's ok, -1 otherwise.
    */
    int SetMotorState (const unsigned char state) const;
    
    /** Reset odometry to (0,0,0).
        Returns: 0 if everything's ok, -1 otherwise.
    */
    int ResetOdometry () const;

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print ();

    /** Accessors
     */
    int32_t  Xpos () const { return xpos; }
    int32_t  Ypos () const { return ypos; }
    uint16_t Theta () const { return theta; }
    int16_t  Speed () const { return speed; }
    int16_t  TurnRate () const { return turn_rate; }
    unsigned short Compass () const { return compass; }
    unsigned char Stalls () const { return stalls; }

private:
    /// Robot pose (according to odometry) in mm, mm, degrees.
    int32_t  xpos, ypos;
    uint16_t theta;

    /// Robot speed in mm/s and degrees/s.
    int16_t speed, turn_rate;

    /// Compass value (only valid if the compass is installed).
    unsigned short compass;

    /// Stall flag: 1 if the robot is stalled and 0 otherwise.
    unsigned char stalls;
};

#endif // _RWI_POSITIONPROXY_H
