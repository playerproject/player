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
 * SICK laser
 */
#define DEFAULT_LASER_PORT "/dev/ttyS1"
/********************************************************************/

/********************************************************************
 * ACTS vision system
 */
/* this enum shouldn't really go here, but...*/
/* a variable of this type tells the vision device how to interact with ACTS */
typedef enum
{
  ACTS_VERSION_UNKNOWN = 0,
  ACTS_VERSION_1_0 = 1,
  ACTS_VERSION_1_2 = 2,
  ACTS_VERSION_2_0 = 3
} acts_version_t;
#define DEFAULT_ACTS_PORT 5001
/* default is to use older ACTS (until we change our robots) */
#define DEFAULT_ACTS_VERSION ACTS_VERSION_1_0
#define DEFAULT_ACTS_CONFIGFILE "/usr/local/acts/actsconfig"
/* default is to give no path for the binary; in this case, use execvp() 
 * and user's PATH */
#define DEFAULT_ACTS_PATH ""
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
 * Festival speech synthesis system.
 */
/* don't change this unless you change the Festival init scripts as well*/
#define DEFAULT_FESTIVAL_PORTNUM 1314
/* change this if Festival is installed somewhere else*/
#define DEFAULT_FESTIVAL_LIBDIR "/usr/local/festival/lib"
/* queue size */
#define SPEECH_MAX_STRING_LEN 256
#define SPEECH_MAX_QUEUE_LEN 4
/********************************************************************/

/********************************************************************
 * Broadcast communication device
 */
#define DEFAULT_BROADCAST_IP "10.255.255.255"
#define DEFAULT_BROADCAST_PORT 6013
/********************************************************************/

#endif
