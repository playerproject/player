/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2005 -
 *     Brian Gerkey
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
 * Functions for looking up the appropriate XDR pack/unpack function for a
 * given message type and subtype.
 */

#ifndef _FUNCTIONTABLE_H_
#define _FUNCTIONTABLE_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup libplayerxdr @{ */

/** Generic Prototype for a player XDR packing function */
typedef int (*player_pack_fn_t) (void* buf, size_t buflen, void* msg, int op);

/** @brief Look up the XDR packing function for a given message signature.
 *
 * @param interf : The interface
 * @param type : The message type
 * @param subtype : The message subtype
 *
 * @returns A pointer to the appropriate function, or NULL if one cannot be
 * found.
 */
player_pack_fn_t playerxdr_get_func(uint16_t interf, uint8_t type, 
                                    uint8_t subtype);

/** @brief Initialize the XDR function table.
 *
 * This function adds all the standard Player message types into the table
 * that is searched by playerxdr_get_func.
 *
 * @todo Add the ability to extend the function table for user-defined
 * message types.
 */
void playerxdr_ftable_init();

/** @} */

#ifdef __cplusplus
}
#endif

#endif
