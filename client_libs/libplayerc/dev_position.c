/***************************************************************************
 * Desc: Position device proxy
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"

// Local declarations
void playerc_position_putdata(playerc_position_t *device, player_msghdr_t *header,
                              player_position_data_t *data, size_t len);


// Create a new position proxy
playerc_position_t *playerc_position_create(playerc_client_t *client, int index)
{
  playerc_position_t *device;

  device = malloc(sizeof(playerc_position_t));
  memset(device, 0, sizeof(playerc_position_t));
  playerc_device_init(&device->info, client, PLAYER_POSITION_CODE, index,
                      (playerc_putdata_fn_t) playerc_position_putdata);

  
  return device;
}


// Destroy a position proxy
void playerc_position_destroy(playerc_position_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the position device
int playerc_position_subscribe(playerc_position_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the position device
int playerc_position_unsubscribe(playerc_position_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_position_putdata(playerc_position_t *device, player_msghdr_t *header,
                              player_position_data_t *data, size_t len)
{
  device->px = (long) ntohl(data->xpos) / 1000.0;
  device->py = (long) ntohl(data->ypos) / 1000.0;
  device->pa = (short) ntohs(data->theta) * M_PI / 180.0;
  device->vx = (short) ntohs(data->speed) / 1000.0;
  device->vy = (short) ntohs(data->sidespeed) / 1000.0;
  device->va = (short) ntohs(data->turnrate) * M_PI / 180.0;
  device->stall = data->stalls;
}


// Enable/disable the motors
int playerc_position_enable(playerc_position_t *device, int enable)
{
  player_position_config_t config;

  config.request = PLAYER_POSITION_MOTOR_POWER_REQ;
  config.value = enable;

  return playerc_client_request(device->info.client, &device->info,
                                (char*) &config, sizeof(config),
                                (char*) &config, sizeof(config));    
}


// Set the robot speed
int playerc_position_setspeed(playerc_position_t *device, double vx, double vy, double va)
{
  player_position_cmd_t cmd;

  cmd.speed = htons((int) (vx * 1000.0));
  cmd.sidespeed = htons((int) (vy * 1000.0));
  cmd.turnrate = htons((int) (va * 180.0 / M_PI));

  return playerc_client_write(device->info.client, &device->info, (char*) &cmd, sizeof(cmd));
}

