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

#ifndef _POSITIONPROXY_H
#define _POSITIONPROXY_H

#include <clientproxy.h>

/**
 *  $Id$
 *
 * The {\tt PositionProxy} is the super class for all position-related devices.
 *  These devices report odometry information, and accept motor commands.
 */
class PositionProxy : public ClientProxy {

public:
    /** Constructor.
        Simply passes arguments along to ClientProxy
    */
    PositionProxy (PlayerClient* pc, uint16_t device_code,
                   unsigned short index, unsigned char access ='c')
        : ClientProxy(pc, device_code, index, access) {}

				
    // these methods are the user's interface to this device

    /** Send a motor command.
        Specify the linear and angular speed in m/s and degrees/sec,
        respectively.\\
        Returns: 0 if everything's ok, -1 otherwise.
    */
    virtual int SetSpeed (const int16_t speed, const int16_t turnrate) = 0;

    /** Enable/disable the motors.
        Set {\tt state} to 0 to disable (default) or 1 to enable.
        Be VERY careful with this method!  Your robot is likely to run across the
        room with the charger still attached.
        
        Returns: 0 if everything's ok, -1 otherwise.
    */
    virtual int SetMotorState (const unsigned char state) = 0;

    /** Select velocity control mode for robots like the Pioneer 2.
        Set {\tt mode} to 0 for direct wheel velocity control (default),
        or 1 for separate translational and rotational control.

        Returns: 0 if everything's ok, -1 otherwise.
    */
    virtual int SelectVelocityControl(unsigned char mode) = 0;

    
    /** Reset odometry to (0,0,0).
        Returns: 0 if everything's ok, -1 otherwise.
    */
    virtual int ResetOdometry () = 0;

    /** Accessors
     */
    virtual int32_t  Xpos () const = 0;
    virtual int32_t  Ypos () const = 0;
    virtual uint16_t Theta () const = 0;
    virtual int16_t  Speed () const = 0;
    virtual int16_t  TurnRate () const = 0;
    virtual unsigned short Compass () const = 0;
    virtual unsigned char Stalls () const = 0;
};

#endif // _POSITIONPROXY_H
