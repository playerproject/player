/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) <insert dates here>
 *     <insert author's name(s) here>
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
#ifndef _ADDR_UTIL_H
#define _ADDR_UTIL_H

/** @ingroup libplayerinterface
    @defgroup addrutil Address Utilities
Utilities for converting between IP address formats.
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <playerconfig.h>

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERINTERFACE_EXPORT
  #elif defined (playerinterface_EXPORTS)
    #define PLAYERINTERFACE_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERINTERFACE_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERINTERFACE_EXPORT
#endif
/** @ingroup addrutil
@{
*/
/// Convert a packed address to a dotted IP address string
/// @param dest The destination buffer for the IP address string
/// @param len The length of the destination buffer
/// @param addr The packed address to be converted
PLAYERINTERFACE_EXPORT void packedaddr_to_dottedip(char* dest, size_t len, uint32_t addr);
/// Convert a hostname to a packed IP address
/// @param dest The destination for the packed IP address
/// @param hostname The null-terminated string containing the host name.
//  @return 0 if successful, nonzero otherwise
PLAYERINTERFACE_EXPORT int hostname_to_packedaddr(uint32_t* dest, const char* hostname);
/** @} */
#ifdef __cplusplus
}
#endif
#endif
