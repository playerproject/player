/***************************************************************************
 * Desc: Laser device proxy
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_laser_putdata(playerc_laser_t *device, player_msghdr_t *header,
                           player_laser_data_t *data, size_t len);

// Create a new laser proxy
playerc_laser_t *playerc_laser_create(playerc_client_t *client, int index)
{
  playerc_laser_t *device;

  device = malloc(sizeof(playerc_laser_t));
  memset(device, 0, sizeof(playerc_laser_t));
  playerc_device_init(&device->info, client, PLAYER_LASER_CODE, index,
                      (playerc_putdata_fn_t) playerc_laser_putdata);
    
  return device;
}


// Destroy a laser proxy
void playerc_laser_destroy(playerc_laser_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the laser device
int playerc_laser_subscribe(playerc_laser_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the laser device
int playerc_laser_unsubscribe(playerc_laser_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_laser_putdata(playerc_laser_t *device, player_msghdr_t *header,
                           player_laser_data_t *data, size_t len)
{
  int i;
  double r, b, db;
    
  data->min_angle = ntohs(data->min_angle);
  data->max_angle = ntohs(data->max_angle);
  data->resolution = ntohs(data->resolution);
  data->range_count = ntohs(data->range_count);

  assert(data->range_count < ARRAYSIZE(device->scan));
  
  b = data->min_angle / 100.0 * M_PI / 180.0;
  db = data->resolution / 100.0 * M_PI / 180.0;    
  for (i = 0; i < data->range_count; i++)
  {
    r = (ntohs(data->ranges[i]) & 0x1FFF) / 1000.0;
    device->scan[i][0] = r;
    device->scan[i][1] = b;
    device->point[i][0] = r * cos(b);
    device->point[i][1] = r * sin(b);
    device->intensity[i] = ((ntohs(data->ranges[i]) & 0xE000) >> 13);
    b += db;
  }
  device->scan_count = data->range_count;
}


// Configure the laser
int playerc_laser_configure(playerc_laser_t *device, double min_angle, double max_angle,
                            double resolution, int intensity)
{
    player_laser_config_t config;

    config.min_angle = htons((int) (min_angle * 180.0 / M_PI * 100));
    config.max_angle = htons((int) (max_angle * 180.0 / M_PI * 100));
    config.resolution = htons((int) (resolution * 180.0 / M_PI * 100));
    config.intensity = (intensity ? 1 : 0);

    return playerc_client_request(device->info.client, &device->info,
                                  (char*) &config, sizeof(config), (char*) &config,
                                  sizeof(config));    
}
