/* 
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002-2003
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
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2003
 *                      
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/***************************************************************************
 * Desc: Utility functions.
 * Author: Andrew Howard
 * Date: 16 Aug 2002
 * CVS: $Id$
 **************************************************************************/

#include <string.h>

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
    case PLAYER_GPS_CODE:
      return PLAYER_GPS_STRING;
    case PLAYER_LASER_CODE:
      return PLAYER_LASER_STRING;
    case PLAYER_LOCALIZE_CODE:
      return PLAYER_LOCALIZE_STRING;
    case PLAYER_GRIPPER_CODE:
      return PLAYER_GRIPPER_STRING;
    case PLAYER_POSITION_CODE:
      return PLAYER_POSITION_STRING;
    case PLAYER_POSITION3D_CODE:
      return PLAYER_POSITION3D_STRING;
    case PLAYER_POWER_CODE:
      return PLAYER_POWER_STRING;
    case PLAYER_PTZ_CODE:
      return PLAYER_PTZ_STRING;
    case PLAYER_SONAR_CODE:
      return PLAYER_SONAR_STRING;
    case PLAYER_WIFI_CODE:
      return PLAYER_WIFI_STRING;
      return PLAYER_LOCALIZE_STRING;
    default:
      break;
  }
  return "unknown";
}


// Get the device code for a give name.
int playerc_lookup_code(const char *name)
{
  if (strcmp(name, PLAYER_AIO_STRING) == 0)
    return PLAYER_AIO_CODE;
  if (strcmp(name, PLAYER_BLOBFINDER_STRING) == 0)
    return PLAYER_BLOBFINDER_CODE;
  if (strcmp(name, PLAYER_BUMPER_STRING) == 0)
    return PLAYER_BUMPER_CODE;
  if (strcmp(name, PLAYER_COMMS_STRING) == 0)
    return PLAYER_COMMS_CODE;
  if (strcmp(name, PLAYER_DIO_STRING) == 0)
    return PLAYER_DIO_CODE;
  if (strcmp(name, PLAYER_FIDUCIAL_STRING) == 0)
    return PLAYER_FIDUCIAL_CODE;
  if (strcmp(name, PLAYER_GPS_STRING) == 0)
    return PLAYER_GPS_CODE;
  if (strcmp(name, PLAYER_LASER_STRING) == 0)
    return PLAYER_LASER_CODE;
  if (strcmp(name, PLAYER_LOCALIZE_STRING) == 0)
    return PLAYER_LOCALIZE_CODE;
  if (strcmp(name, PLAYER_GRIPPER_STRING) == 0)
    return PLAYER_GRIPPER_CODE;
  if (strcmp(name, PLAYER_POSITION_STRING) == 0)
    return PLAYER_POSITION_CODE;
  if (strcmp(name, PLAYER_POSITION3D_STRING) == 0)
    return PLAYER_POSITION3D_CODE;
  if (strcmp(name, PLAYER_POWER_STRING) == 0)
    return PLAYER_POWER_CODE;
  if (strcmp(name, PLAYER_PTZ_STRING) == 0)
    return PLAYER_PTZ_CODE;
  if (strcmp(name, PLAYER_SONAR_STRING) == 0)
    return PLAYER_SONAR_CODE;
  if (strcmp(name, PLAYER_WIFI_STRING) == 0)
    return PLAYER_WIFI_CODE;
  return -1;
}


