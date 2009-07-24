/*
Copyright (c) 2007, Brian Gerkey
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the Player Project nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <libplayersd/playersd.h>
#include <libplayercore/error.h>
#include <libplayercore/addr_util.h>
#include <libplayercore/interface_util.h>
#include <libplayerc/playerc.h>

#define MAX_DEVS 16
  
playerc_mclient_t* mclient;
playerc_client_t* clients[MAX_DEVS];
playerc_laser_t* lasers[MAX_DEVS];
int num_laserdevs;

void
device_cb(void* data)
{
  playerc_laser_t* laser = (playerc_laser_t*)data;
  printf("received data from %u:%u:%u:%u",
         laser->info.addr.host, 
         laser->info.addr.robot, 
         laser->info.addr.interf, 
         laser->info.addr.index);
  printf("  (%d scans)\n", laser->scan_count);
}

void
browse_cb(player_sd_t* sd, player_sd_dev_t* dev)
{
  if(dev->interf == PLAYER_LASER_CODE)
  {
    clients[num_laserdevs] = playerc_client_create(mclient, 
                                                   dev->hostname,
                                                   dev->robot);
    if(0 != playerc_client_connect(clients[num_laserdevs]))
      exit(-1);

    playerc_client_datamode(clients[num_laserdevs], PLAYERC_DATAMODE_PUSH);

    // Create and subscribe to a laser device.
    lasers[num_laserdevs] = playerc_laser_create(clients[num_laserdevs], 
                                                 dev->index);
    if(playerc_laser_subscribe(lasers[num_laserdevs], PLAYER_OPEN_MODE))
      exit(-1);

    // Add a callback to be invoked whenever we receive new data from this
    // laser
    playerc_client_addcallback(clients[num_laserdevs], 
                               &(lasers[num_laserdevs]->info),
                               device_cb, lasers[num_laserdevs]);

    num_laserdevs++;
    printf("subscribed to: %s:%u:%s:%u\n",
           dev->hostname,
           dev->robot,
           interf_to_str(dev->interf),
           dev->index);
    printf("Now receiving %d lasers\n", num_laserdevs);
  }
}

int
main(int argc, const char **argv)
{
  int i;

  // A service discovery object
  player_sd_t* sd;

  // Initialize multiclient
  mclient = playerc_mclient_create();

  // Initialize service discovery
  sd = player_sd_init();

  // Look for Player devices
  if(player_sd_browse(sd, 0.0, 1, browse_cb) != 0)
  {
    puts("player_sd_browse error");
    exit(-1);
  }

  for(;;)
  {
    // Update name service
    player_sd_update(sd,0.0);

    // Wait for new data from server
    playerc_mclient_read(mclient,10);
  }

  // Shutdown
  for(i=0;i<num_laserdevs;i++)
  {
    playerc_laser_unsubscribe(lasers[i]);
    playerc_laser_destroy(lasers[i]);
    playerc_client_disconnect(clients[i]);
    playerc_client_destroy(clients[i]);
  }

  playerc_mclient_destroy(mclient);

  return(0);
}
