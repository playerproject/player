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

#include <string.h>
#include <stdlib.h>

#include "playerc.h"
#include "error.h"

// Local declarations
void playerc_actarray_putmsg(playerc_actarray_t *device,
                             player_msghdr_t *header,
                             player_actarray_data_t *data, size_t len);

// Create an actarray proxy
playerc_actarray_t *playerc_actarray_create(playerc_client_t *client, int index)
{
  playerc_actarray_t *device;

  device = malloc(sizeof(playerc_actarray_t));
  memset(device, 0, sizeof(playerc_actarray_t));
  playerc_device_init(&device->info, client, PLAYER_ACTARRAY_CODE, index,
                       (playerc_putmsg_fn_t) playerc_actarray_putmsg);

  return device;
}

// Destroy an actarray proxy
void playerc_actarray_destroy(playerc_actarray_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}

// Subscribe to the actarray device
int playerc_actarray_subscribe(playerc_actarray_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}

// Un-subscribe from the actarray device
int playerc_actarray_unsubscribe(playerc_actarray_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}

void playerc_actarray_putmsg(playerc_actarray_t *device,
                             player_msghdr_t *header,
                             player_actarray_data_t *data, size_t len)
{
  int i = 0;

  if((header->type == PLAYER_MSGTYPE_DATA) && (header->subtype == PLAYER_ACTARRAY_DATA_STATE))
  {
    device->actuators_count = data->actuators_count;
    for (i = 0; i < device->actuators_count; i++)
    {
      device->actuators_data[i].position = data->actuators[i].position;
      device->actuators_data[i].speed = data->actuators[i].speed;
      device->actuators_data[i].acceleration = data->actuators[i].acceleration;
      device->actuators_data[i].state = data->actuators[i].state;
      device->actuators_data[i].current = data->actuators[i].current;
    }
    device->motor_state = data->motor_state;
  }
  else
    PLAYERC_WARN2("skipping actarray message with unknown type/subtype: %s/%d\n",
                  msgtype_to_str(header->type), header->subtype);
}

// Get the actarray geometry
int playerc_actarray_get_geom(playerc_actarray_t *device)
{
  player_actarray_geom_t geom;
  int ii = 0, result = 0;

  if((result = playerc_client_request(device->info.client, &device->info,
      PLAYER_ACTARRAY_REQ_GET_GEOM, NULL, (void*)&geom, sizeof(geom))) < 0)
    return result;

  for (ii = 0; ii < device->actuators_count; ii++)
  {
    device->actuators_geom[ii].type = geom.actuators[ii].type;
    device->actuators_geom[ii].length = geom.actuators[ii].length;
    device->actuators_geom[ii].orientation.proll = geom.actuators[ii].orientation.proll;
    device->actuators_geom[ii].orientation.ppitch = geom.actuators[ii].orientation.ppitch;
    device->actuators_geom[ii].orientation.pyaw = geom.actuators[ii].orientation.pyaw;
    device->actuators_geom[ii].axis.px = geom.actuators[ii].axis.px;
    device->actuators_geom[ii].axis.py = geom.actuators[ii].axis.py;
    device->actuators_geom[ii].axis.pz = geom.actuators[ii].axis.pz;
    device->actuators_geom[ii].min = geom.actuators[ii].min;
    device->actuators_geom[ii].centre = geom.actuators[ii].centre;
    device->actuators_geom[ii].max = geom.actuators[ii].max;
    device->actuators_geom[ii].home = geom.actuators[ii].home;
    device->actuators_geom[ii].config_speed = geom.actuators[ii].config_speed;
    device->actuators_geom[ii].hasbrakes = geom.actuators[ii].hasbrakes;
  }
  device->base_pos = geom.base_pos;
  device->base_orientation = geom.base_orientation;
  return 0;
}

// Command a joint in the array to move to a specified position
int playerc_actarray_position_cmd(playerc_actarray_t *device, int joint, float position)
{
  player_actarray_position_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.joint = joint;
  cmd.position = position;

  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_ACTARRAY_CMD_POS,
                              &cmd, NULL);
}

// Command all joints in the array to move with a specified current
int playerc_actarray_multi_position_cmd(playerc_actarray_t *device, float positions[PLAYER_ACTARRAY_NUM_ACTUATORS])
{
  player_actarray_multi_position_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  memcpy(&cmd.positions,positions, sizeof(float) * PLAYER_ACTARRAY_NUM_ACTUATORS);
  cmd.positions_count = device->actuators_count;

  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_ACTARRAY_CMD_MULTI_POS,
                              &cmd, NULL);
}


// Command a joint in the array to move at a specified speed
int playerc_actarray_speed_cmd(playerc_actarray_t *device, int joint, float speed)
{
  player_actarray_speed_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.joint = joint;
  cmd.speed = speed;

  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_ACTARRAY_CMD_SPEED,
                              &cmd, NULL);
}


// Command all joints in the array to move with a specified current
int playerc_actarray_multi_speed_cmd(playerc_actarray_t *device, float speeds[PLAYER_ACTARRAY_NUM_ACTUATORS])
{
  player_actarray_multi_speed_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  memcpy(&cmd.speeds,speeds,sizeof(float) * PLAYER_ACTARRAY_NUM_ACTUATORS);
  cmd.speeds_count = device->actuators_count;

  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_ACTARRAY_CMD_MULTI_SPEED,
                              &cmd, NULL);
}


// Command a joint (or, if joint is -1, the whole array) to go to its home position
int playerc_actarray_home_cmd(playerc_actarray_t *device, int joint)
{
  player_actarray_home_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.joint = joint;

  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_ACTARRAY_CMD_HOME,
                              &cmd, NULL);
}

// Command a joint in the array to move with a specified current
int playerc_actarray_current_cmd(playerc_actarray_t *device, int joint, float current)
{
  player_actarray_current_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.joint = joint;
  cmd.current = current;

  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_ACTARRAY_CMD_CURRENT,
                              &cmd, NULL);
}

// Command all joints in the array to move with a specified current
int playerc_actarray_multi_current_cmd(playerc_actarray_t *device, float currents[PLAYER_ACTARRAY_NUM_ACTUATORS])
{
  player_actarray_multi_current_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  memcpy(&cmd.currents,currents,sizeof(float) * PLAYER_ACTARRAY_NUM_ACTUATORS);
  cmd.currents_count = device->actuators_count;

  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_ACTARRAY_CMD_MULTI_CURRENT,
                              &cmd, NULL);
}

// Turn the power to the array on or off
int playerc_actarray_power(playerc_actarray_t *device, uint8_t enable)
{
  player_actarray_power_config_t config;

  config.value = enable;

  return playerc_client_request(device->info.client, &device->info,
                                PLAYER_ACTARRAY_REQ_POWER,
                                &config, NULL, 0);
}

// Turn the brakes of all actuators in the array that have them on or off
int playerc_actarray_brakes(playerc_actarray_t *device, uint8_t enable)
{
  player_actarray_brakes_config_t config;

  config.value = enable;

  return playerc_client_request(device->info.client, &device->info,
                                PLAYER_ACTARRAY_REQ_BRAKES,
                                &config, NULL, 0);
}

// Set the speed of a joint (-1 for all joints) for all subsequent movement commands
int playerc_actarray_speed_config(playerc_actarray_t *device, int joint, float speed)
{
  player_actarray_speed_config_t config;

  config.joint = joint;
  config.speed = speed;

  return playerc_client_request(device->info.client, &device->info,
                                PLAYER_ACTARRAY_REQ_SPEED,
                                &config, NULL, 0);
}

// Set the speed of a joint (-1 for all joints) for all subsequent movement commands
int playerc_actarray_accel_config(playerc_actarray_t *device, int joint, float accel)
{
  player_actarray_accel_config_t config;

  config.joint = joint;
  config.accel = accel;

  return playerc_client_request(device->info.client, &device->info,
                                PLAYER_ACTARRAY_REQ_ACCEL,
                                &config, NULL, 0);
}



