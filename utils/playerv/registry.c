/*
 *  PlayerViewer
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
/**************************************************************************
 * Desc: Device registry.
 * Author: Andrew Howard
 * Date: 16 Aug 2002
 * CVS: $Id$
 *************************************************************************/

#include "playerv.h"

// Create the appropriate GUI proxy for a given set of device info.
void create_proxy(device_t *device, opt_t *opt, mainwnd_t *mainwnd, playerc_client_t *client)
{
  switch (device->code)
  {
    case PLAYER_BLOBFINDER_CODE:
      device->proxy = blobfinder_create(mainwnd, opt, client, device->robot, device->index,
                                        device->drivername, device->subscribe);
      device->fndestroy = (fndestroy_t) blobfinder_destroy;
      device->fnupdate = (fnupdate_t) blobfinder_update;
      break;
    case PLAYER_FIDUCIAL_CODE:
      device->proxy = fiducial_create(mainwnd, opt, client, device->robot,
                                      device->index, device->drivername, device->subscribe);
      device->fndestroy = (fndestroy_t) fiducial_destroy;
      device->fnupdate = (fnupdate_t) fiducial_update;
      break;
    case PLAYER_LASER_CODE:
      device->proxy = laser_create(mainwnd, opt, client, device->index,
                                   device->index, device->drivername, device->subscribe);
      device->fndestroy = (fndestroy_t) laser_destroy;
      device->fnupdate = (fnupdate_t) laser_update;
      break;
    case PLAYER_POSITION_CODE:
      device->proxy = position_create(mainwnd, opt, client, device->index, 
                                      device->index, device->drivername, device->subscribe);
      device->fndestroy = (fndestroy_t) position_destroy;
      device->fnupdate = (fnupdate_t) position_update;
      break;
    case PLAYER_POWER_CODE:
      device->proxy = power_create(mainwnd, opt, client, device->index, 
                                      device->index, device->drivername, device->subscribe);
      device->fndestroy = (fndestroy_t) power_destroy;
      device->fnupdate = (fnupdate_t) power_update;
      break;
    case PLAYER_PTZ_CODE:
      device->proxy = ptz_create(mainwnd, opt, client, device->index, 
                                 device->index, device->drivername, device->subscribe);
      device->fndestroy = (fndestroy_t) ptz_destroy;
      device->fnupdate = (fnupdate_t) ptz_update;
      break;
    case PLAYER_SONAR_CODE:
      device->proxy = sonar_create(mainwnd, opt, client, device->index,
                                   device->index, device->drivername, device->subscribe);
      device->fndestroy = (fndestroy_t) sonar_destroy;
      device->fnupdate = (fnupdate_t) sonar_update;
      break;
    case PLAYER_LOCALIZATION_CODE:
      device->proxy = localization_create(mainwnd, opt, client, device->index,
                                 device->index, device->drivername, device->subscribe);
      device->fndestroy = (fndestroy_t) localization_destroy;
      device->fnupdate = (fnupdate_t) localization_update;
      break;
    default:
      device->proxy = NULL;
      device->fndestroy = NULL;
      device->fnupdate = NULL;
      break;
  }
  return;
}
