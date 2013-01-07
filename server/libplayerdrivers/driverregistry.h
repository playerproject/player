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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
/********************************************************************
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 ********************************************************************/

/*
 * $Id$
 */

/** 
@defgroup libplayerdrivers libplayerdrivers
@brief Library of standard Player drivers

This library contains all the drivers that were built during the Player
installation.  The contents will vary from system to system, depending on
which drivers' prerequisites are satisfied and which options are supplied
by the user to the <b>configure</b> script.

See @ref install for <b>configure</b> options to enable and disable
compilation of particular drivers.
*/

/** @ingroup libplayerdrivers
@defgroup drivers Drivers
@brief The drivers themselves
*/

/** @ingroup libplayerdrivers
 * @{ */

#ifndef _DRIVERREGISTRY_H
#define _DRIVERREGISTRY_H

/** @brief Register available drivers

This function adds each driver that is included in libplayerdrivers to the
global driverTable.    From there, the drivers can be instantiated and
bound to interfaces. If you use libplayerdrivers, you should call this function during program intialization. 
*/
void player_register_drivers();
/** @} */

#endif
