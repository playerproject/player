/***************************************************************************
 * Desc: Common device functions
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "playerc.h"
#include "error.h"


// Initialise the device 
void playerc_device_init(playerc_device_t *device, playerc_client_t *client,
                         int code, int index, playerc_putdata_fn_t putdata)
{
  device->client = client;
  device->code = code;
  device->index = index;
  device->subscribed = 0;
  device->callback_count = 0;
  device->putdata = putdata;

  playerc_client_adddevice(device->client, device);
}


// Finalize the device
void playerc_device_term(playerc_device_t *device)
{
  playerc_client_deldevice(device->client, device);
}


// Subscribe/unsubscribe the device
int playerc_device_subscribe(playerc_device_t *device, int access)
{
  if (playerc_client_subscribe(device->client, device->code, device->index,
                               access, device->drivername, sizeof(device->drivername)) != 0)
    return -1;
  device->subscribed = 1;
  return 0;
}



// Subscribe/unsubscribe the device
int playerc_device_unsubscribe(playerc_device_t *device)
{
  device->subscribed = 0;
  return playerc_client_unsubscribe(device->client, device->code, device->index);
}

