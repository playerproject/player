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
 *   declarations of various device-specific defaults.  you can change things 
 *   to here to reflect your local usage.  of course, you also just the config
 *   file (once it's written).
 */

#ifndef _PLAYER_DEFAULTS_H
#define _PLAYER_DEFAULTS_H

/********************************************************************
 * the Player server itself
 *
 * can be either absolute or relative, in which case it will be considered
 * relative to the top of the install path selected by INSTALL_PREFIX
 */
#define DEFAULT_PLAYER_CONFIGFILE "etc/config"
/********************************************************************/

/********************************************************************
 * Sony PTZ camera
 */
#define DEFAULT_PTZ_PORT "/dev/ttyS2"
/********************************************************************/

/********************************************************************
 * P2OS robot interface
 */
#define DEFAULT_P2OS_PORT "/dev/ttyS0"
/********************************************************************/

/********************************************************************
 * Broadcast communication device
 */
#define DEFAULT_BROADCAST_IP "10.255.255.255"
#define DEFAULT_BROADCAST_PORT 6013
/********************************************************************/

#endif
