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

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "playerxdr.h"
#include "functiontable.h"

typedef struct
{
  uint16_t interface;
  uint8_t subtype;
  player_pack_fn_t func;
} playerxdr_function_t;


static playerxdr_function_t init_ftable[] =
{
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DEVLIST, (player_pack_fn_t)player_device_devlist_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DRIVERINFO, (player_pack_fn_t)player_device_driverinfo_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DEV, (player_pack_fn_t)player_device_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DATA, (player_pack_fn_t)player_device_data_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DATAMODE, (player_pack_fn_t)player_device_datamode_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DATAFREQ, (player_pack_fn_t)player_device_datafreq_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_AUTH, (player_pack_fn_t)player_device_auth_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_NAMESERVICE, (player_pack_fn_t)player_device_nameservice_req_pack},
  {0,0,NULL}
};

static playerxdr_function_t* ftable;
static int ftable_len;

void
playerxdr_ftable_init()
{
  ftable_len = 0;
  playerxdr_function_t* f;
  for(f = init_ftable; f->func; f++)
    ftable_len++;

  ftable = (playerxdr_function_t*)calloc(ftable_len,
                                         sizeof(playerxdr_function_t));
  assert(ftable);

  memcpy(ftable,init_ftable,ftable_len*sizeof(playerxdr_function_t));
}

void
playerxdr_ftable_add(playerxdr_function_t f)
{
  ftable = (playerxdr_function_t*)realloc(ftable,
                                          ((ftable_len+1)*
                                           sizeof(playerxdr_function_t)));
  assert(ftable);
  ftable[ftable_len++] = f;
}

player_pack_fn_t
playerxdr_get_func(uint16_t interface, uint8_t subtype)
{
  playerxdr_function_t* curr;
  int i;

  for(i=0;i<ftable_len;i++)
  {
    curr = ftable + i;
    if((curr->interface == interface) && (curr->subtype == subtype))
      return(curr->func);
  }
  return(NULL);
}

