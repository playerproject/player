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
 * client-side position device 
 */

#ifndef P2_POSITIONPROXY_H
#define P2_POSITIONPROXY_H

#include <positionproxy.h>
#include <playerclient.h>

/** The {\tt PositionProxy} class is used to control the {\tt position} device.
    The latest position data is contained in the attributes {\tt xpos, ypos}, etc.
 */
class P2PositionProxy : public PositionProxy
{

  public:
    /// Robot pose (according to odometry) in mm, mm, degrees.
    int xpos,ypos; unsigned short theta;

    /// Robot speed in mm/sec, degrees/sec.
    short speed, turnrate;

    /// Compass value (only valid if the compass is installed).
    unsigned short compass;

    /// Stall flag: 1 if the robot is stalled and 0 otherwise.
    unsigned char stalls;

    /** Accessors
     */
    virtual int32_t  Xpos () const { return xpos; }
    virtual int32_t  Ypos () const { return ypos; }
    virtual uint16_t Theta () const { return theta; }
    virtual int16_t  Speed () const { return speed; }
    virtual int16_t  TurnRate () const { return turnrate; }
    virtual unsigned short Compass () const { return compass; }
    virtual unsigned char Stalls () const { return stalls; }

    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    P2PositionProxy(PlayerClient* pc, unsigned short index,
                    unsigned char access ='c') :
        PositionProxy(pc,PLAYER_POSITION_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Send a motor command.
        Specify the linear and angular speed in mm/s and degrees/sec,
        respectively.\\
        Returns: 0 if everything's ok, -1 otherwise.
    */
    virtual int SetSpeed(short speed, short turnrate);

    /** Enable/disable the motors.
        Set {\tt state} to 0 to disable (default) or 1 to enable.
        Be VERY careful with this method!  Your robot is likely to run across the
        room with the charger still attached.
        
        Returns: 0 if everything's ok, -1 otherwise.
    */
    virtual int SetMotorState(unsigned char state);
    
    /** Select velocity control mode for the Pioneer 2.
        Set {\tt mode} to 0 for direct wheel velocity control (default),
        or 1 for separate translational and rotational control.

        Returns: 0 if everything's ok, -1 otherwise.
    */
    virtual int SelectVelocityControl(unsigned char mode);

    /** Reset odometry to (0,0,0).
        Returns: 0 if everything's ok, -1 otherwise.
    */
    virtual int ResetOdometry();

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

#endif
