/* 
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/***************************************************************************
 * Desc: Utility functions.
 * Author: Andrew Howard
 * Date: 16 Aug 2002
 * CVS: $Id$
 **************************************************************************/

#include "playerc.h"
#include "error.h"


// Get the name for a given device code.
const char *playerc_lookup_name(int code)
{
  switch (code)
  {
    case PLAYER_AIO_CODE:
      return PLAYER_AIO_STRING;
    case PLAYER_BLOBFINDER_CODE:
      return PLAYER_BLOBFINDER_STRING;
    case PLAYER_BUMPER_CODE:
      return PLAYER_BUMPER_STRING;
    case PLAYER_COMMS_CODE:
      return PLAYER_COMMS_STRING;
    case PLAYER_DIO_CODE:
      return PLAYER_DIO_STRING;
    case PLAYER_FIDUCIAL_CODE:
      return PLAYER_FIDUCIAL_STRING;
    case PLAYER_FRF_CODE:
      return PLAYER_FRF_STRING;
    case PLAYER_GRIPPER_CODE:
      return PLAYER_GRIPPER_STRING;
    case PLAYER_POSITION_CODE:
      return PLAYER_POSITION_STRING;
    case PLAYER_POWER_CODE:
      return PLAYER_POWER_STRING;
    case PLAYER_PTZ_CODE:
      return PLAYER_PTZ_STRING;
    case PLAYER_SRF_CODE:
      return PLAYER_SRF_STRING;
    default:
      break;
  }
  return "unknown";
}


