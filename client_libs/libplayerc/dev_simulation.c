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
 * Desc: Simulation device proxy
 * Author: Douglas S. Blank
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "playerc.h"

// Local declarations
void playerc_simulation_putmsg(playerc_simulation_t *device,
                               player_msghdr_t *header,
                               player_simulation_data_t *data, size_t len);


// Create a new simulation proxy
playerc_simulation_t *playerc_simulation_create(playerc_client_t *client, int index)
{
  playerc_simulation_t *device;

  device = malloc(sizeof(playerc_simulation_t));
  memset(device, 0, sizeof(playerc_simulation_t));
  playerc_device_init(&device->info, client, PLAYER_SIMULATION_CODE, index,
                      (playerc_putmsg_fn_t)playerc_simulation_putmsg);
  return device;
}


// Destroy a simulation proxy
void playerc_simulation_destroy(playerc_simulation_t *device)
{
  playerc_device_term(&device->info);
  free(device);

  return;
}


// Subscribe to the simulation device
int playerc_simulation_subscribe(playerc_simulation_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the simulation device
int playerc_simulation_unsubscribe(playerc_simulation_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_simulation_putmsg(playerc_simulation_t *device, player_msghdr_t *header,
                              player_simulation_data_t *data, size_t len)
{
  // device->data 
}


// Set the target pose
int playerc_simulation_set_pose2d(playerc_simulation_t *device, char* name, double gx, double gy,
                                  double ga)
{
  player_simulation_pose2d_req_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  strncpy(cmd.name, name, PLAYER_SIMULATION_IDENTIFIER_MAXLEN);
  cmd.name[PLAYER_SIMULATION_IDENTIFIER_MAXLEN-1]='\0';
  cmd.name_count = strlen(cmd.name) + 1;
  cmd.pose.px = gx;
  cmd.pose.py = gy;
  cmd.pose.pa = ga;

  return playerc_client_request(device->info.client, &device->info, 
                                PLAYER_SIMULATION_REQ_SET_POSE2D,
                                &cmd, NULL);
}

// Get the current pose
int playerc_simulation_get_pose2d(playerc_simulation_t *device, char* identifier, 
				  double *x, double *y, double *a)
{
  player_simulation_pose2d_req_t cfg, *resp;
  
  memset(&cfg, 0, sizeof(cfg));
  strncpy(cfg.name, identifier, PLAYER_SIMULATION_IDENTIFIER_MAXLEN);
  cfg.name[PLAYER_SIMULATION_IDENTIFIER_MAXLEN-1]='\0';
  cfg.name_count = strlen(cfg.name) + 1;
  if (playerc_client_request(device->info.client, &device->info, 
                             PLAYER_SIMULATION_REQ_GET_POSE2D,
			     &cfg, (void**)&resp) < 0)
    return (-1);
  *x =  resp->pose.px;
  *y =  resp->pose.py;
  *a =  resp->pose.pa;
  player_simulation_pose2d_req_t_free(resp);
  return 0;
}

// Set the target pose in 3D
int playerc_simulation_set_pose3d(playerc_simulation_t *device, char* name, double gx, double gy,
                                  double gz, double groll, double gpitch, double gyaw)
{
  player_simulation_pose3d_req_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  strncpy(cmd.name, name, PLAYER_SIMULATION_IDENTIFIER_MAXLEN);
  cmd.name[PLAYER_SIMULATION_IDENTIFIER_MAXLEN-1]='\0';
  cmd.name_count = strlen(cmd.name) + 1;
  cmd.pose.px = gx;
  cmd.pose.py = gy;
  cmd.pose.pz = gz;
  cmd.pose.ppitch = gpitch;
  cmd.pose.proll = groll;
  cmd.pose.pyaw = gyaw;

  return playerc_client_request(device->info.client, &device->info, 
                                PLAYER_SIMULATION_REQ_SET_POSE3D,
                                &cmd, NULL);
}

// Get the current pose in 3D
int playerc_simulation_get_pose3d(playerc_simulation_t *device, char* identifier, 
          double *x, double *y, double *z, double *roll, double *pitch, double *yaw, double *time)
{
  player_simulation_pose3d_req_t cfg, *resp;
  
  memset(&cfg, 0, sizeof(cfg));
  strncpy(cfg.name, identifier, PLAYER_SIMULATION_IDENTIFIER_MAXLEN);
  cfg.name[PLAYER_SIMULATION_IDENTIFIER_MAXLEN-1]='\0';
  cfg.name_count = strlen(cfg.name) + 1;
  if (playerc_client_request(device->info.client, &device->info, 
                             PLAYER_SIMULATION_REQ_GET_POSE3D,
           &cfg, (void**)&resp) < 0)
    return (-1);
  *x =  resp->pose.px;
  *y =  resp->pose.py;
  *z =  resp->pose.pz;
  *pitch =  resp->pose.ppitch;
  *roll =  resp->pose.proll;
  *yaw =  resp->pose.pyaw;
  *time = resp->simtime;
  player_simulation_pose3d_req_t_free(resp);
  return 0;
}

// Set a property value */
int playerc_simulation_set_property(playerc_simulation_t *device, 
				    char* name,
				    char* property,
				    void* value,
				    size_t value_len )
{
  player_simulation_property_req_t req;

  memset(&req, 0, sizeof(req));
  strncpy(req.name, name, PLAYER_SIMULATION_IDENTIFIER_MAXLEN);
  req.name[PLAYER_SIMULATION_IDENTIFIER_MAXLEN-1]='\0';
  req.name_count = strlen(req.name) + 1;
  
  strncpy(req.prop, property, PLAYER_SIMULATION_IDENTIFIER_MAXLEN);
  req.prop[PLAYER_SIMULATION_IDENTIFIER_MAXLEN-1]='\0';
  req.prop_count = strlen(req.prop) + 1;

  if( value_len > PLAYER_SIMULATION_PROPERTY_DATA_MAXLEN )
    {
      PLAYER_WARN2( "Simulation property data exceeds maximum length (%d/%d bytes).",
		   value_len,  PLAYER_SIMULATION_PROPERTY_DATA_MAXLEN );
      value_len = PLAYER_SIMULATION_PROPERTY_DATA_MAXLEN;
    }
  
  memcpy( req.value, value, value_len );
  req.value_count = value_len;
  
  return playerc_client_request(device->info.client, &device->info, 
                                PLAYER_SIMULATION_REQ_SET_PROPERTY,
                                &req, NULL);
}

// Get a property value */
int playerc_simulation_get_property(playerc_simulation_t *device, 
				    char* name,
				    char* property,
				    void* value,
				    size_t value_len )
{
  player_simulation_property_req_t req, *resp;

  memset(&req, 0, sizeof(req));
  strncpy(req.name, name, PLAYER_SIMULATION_IDENTIFIER_MAXLEN);
  req.name[PLAYER_SIMULATION_IDENTIFIER_MAXLEN-1]='\0';
  req.name_count = strlen(req.name) + 1;
  
  strncpy(req.prop, property, PLAYER_SIMULATION_IDENTIFIER_MAXLEN);
  req.prop[PLAYER_SIMULATION_IDENTIFIER_MAXLEN-1]='\0';
  req.prop_count = strlen(req.prop) + 1;

  if( value_len > PLAYER_SIMULATION_PROPERTY_DATA_MAXLEN )
    {
      PLAYER_WARN2( "Simulation property data exceeds maximum length (%d/%d bytes).",
		   value_len,  PLAYER_SIMULATION_PROPERTY_DATA_MAXLEN );
      value_len = PLAYER_SIMULATION_PROPERTY_DATA_MAXLEN;
    }
  
  req.value_count = value_len;
  
  if( playerc_client_request(device->info.client, &device->info, 
                                PLAYER_SIMULATION_REQ_GET_PROPERTY,
                                &req, (void**)&resp) < 0)
    return -1;

  memcpy(value, resp->value, value_len);
  player_simulation_property_req_t_free(resp);
  return 0;
}
