/***************************************************************************
 * Desc: LaserBeacon device proxy
 * Author: Andrew Howard
 * Date: 15 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_laserbeacon_putdata(playerc_laserbeacon_t *device, player_msghdr_t *header,
                                 player_laserbeacon_data_t *data, size_t len);

// Create a new laserbeacon proxy
playerc_laserbeacon_t *playerc_laserbeacon_create(playerc_client_t *client, int index)
{
  playerc_laserbeacon_t *device;

  device = malloc(sizeof(playerc_laserbeacon_t));
  memset(device, 0, sizeof(playerc_laserbeacon_t));
  playerc_device_init(&device->info, client, PLAYER_LASERBEACON_CODE, index,
                      (playerc_putdata_fn_t) playerc_laserbeacon_putdata);

  return device;
}


// Destroy a laserbeacon proxy
void playerc_laserbeacon_destroy(playerc_laserbeacon_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the laserbeacon device
int playerc_laserbeacon_subscribe(playerc_laserbeacon_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the laserbeacon device
int playerc_laserbeacon_unsubscribe(playerc_laserbeacon_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_laserbeacon_putdata(playerc_laserbeacon_t *device, player_msghdr_t *header,
                                 player_laserbeacon_data_t *data, size_t len)
{
  int i;

  device->beacon_count = ntohs(data->count);

  for (i = 0; i < device->beacon_count; i++)
  {
    device->beacons[i].id = data->beacon[i].id;
    device->beacons[i].range = ntohs(data->beacon[i].range) / 1000.0;
    device->beacons[i].bearing = ((int) (int16_t) ntohs(data->beacon[i].bearing)) * M_PI / 180;
    device->beacons[i].orient = ((int) (int16_t) ntohs(data->beacon[i].orient)) * M_PI / 180;
  }
}


// Configure the laserbeacon device
int playerc_laserbeacon_configure(playerc_laserbeacon_t *device, int bit_count, double bit_width)
{
  player_laserbeacon_setbits_t config;

  config.subtype = PLAYER_LASERBEACON_SUBTYPE_SETBITS;
  config.bit_count = bit_count;
  config.bit_size = htons(bit_width * 1000);
    
  return playerc_client_request(device->info.client, &device->info,
                                (char*) &config, sizeof(config),
                                (char*) &config, sizeof(config));    
}
