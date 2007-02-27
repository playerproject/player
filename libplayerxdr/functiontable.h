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

/** @addtogroup libplayerxdr libplayerxdr @{ */

/** Generic Prototype for a player XDR packing function */
typedef int (*player_pack_fn_t) (void* buf, size_t buflen, void* msg, int op);
/** Generic Prototype for a player message structure deep copy function */
typedef unsigned int (*player_dpcpy_fn_t) (const void* src, void* dest);
/** Generic Prototype for a player message structure cleanup function */
typedef void (*player_cleanup_fn_t) (void* msg);

/** Structure to link an (interface,type,subtype) tuple with an XDR
 * pack/unpack function, a deep copy function and a delete function */
typedef struct
{
  uint16_t interf;
  uint8_t type;
  uint8_t subtype;
  player_pack_fn_t packfunc;
  player_dpcpy_fn_t dpcpyfunc;
  player_cleanup_fn_t cleanupfunc;
} playerxdr_function_t;

/** @brief Look up the XDR packing function for a given message signature.
 *
 * @param interf : The interface
 * @param type : The message type
 * @param subtype : The message subtype
 *
 * @returns A pointer to the appropriate function, or NULL if one cannot be
 * found.
 */
player_pack_fn_t playerxdr_get_packfunc(uint16_t interf, uint8_t type,
                                    uint8_t subtype);

player_dpcpy_fn_t playerxdr_get_dpcpyfunc(uint16_t interf, uint8_t type,
                                    uint8_t subtype);

player_cleanup_fn_t playerxdr_get_cleanupfunc(uint16_t interf, uint8_t type,
                                    uint8_t subtype);

/** @brief Add an entry to the function table.
 *
 * @param f : the message signature and function to add
 * @param replace : whether any existing function for the same signature
 *                  should be replaced.
 *
 * @returns 0 on success (new entry was entered, replacing old one if
 * necessary), -1 on failure (an existing entry matched the given
 * signature, but @p replace was 0)
 */
int playerxdr_ftable_add(playerxdr_function_t f, int replace);

/** @brief Initialize the XDR function table.
 *
 * This function adds all the standard Player message types into the table
 * that is searched by the playerxdr_get_* functions.
 *
 * @todo Add the ability to extend the function table for user-defined
 * message types.
 */
void playerxdr_ftable_init(void);

/** @brief Deep copy a message structure.
 *
 * Copies the dynamically allocated parts of a message structure from src to
 * dest using the message type/subtype's deep copy function.
 *
 * @param src : The source message
 * @param dest : The destination message
 *
 * @returns : The number of bytes copied.
 */
unsigned int playerxdr_deepcopy_message(void* src, void* dest, uint16_t interf, uint8_t type,
                                    uint8_t subtype);

/** @brief Delete a message structure's dynamic elements.
 *
 * Deletes any dynamically allocated data used by a message structure. NOTE:
 * Does not delete the message structure itself, even if it is dynamically
 * allocated. Only data pointed to by the message structure's members will be
 * deleted.
 *
 * @param msg : The message to clean up.
 *
 * @returns: Nothing.
 */
void playerxdr_delete_message(void* msg, uint16_t interf, uint8_t type,
                                    uint8_t subtype);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
