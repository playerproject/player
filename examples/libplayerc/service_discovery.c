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
browse_cb(player_sd_t* sd, player_sd_dev_t* dev)
{
  if(dev->interf == PLAYER_LASER_CODE)
  {
    clients[num_laserdevs] = playerc_client_create(mclient, 
                                                   dev->hostname,
                                                   dev->robot);
    if(0 != playerc_client_connect(clients[num_laserdevs]))
      exit(-1);

    // Create and subscribe to a laser device.
    lasers[num_laserdevs] = playerc_laser_create(clients[num_laserdevs], 
                                                 dev->index);
    if(playerc_laser_subscribe(lasers[num_laserdevs], PLAYER_OPEN_MODE))
      exit(-1);

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
    // Wait for new data from server
    playerc_mclient_read(mclient,100);

    // Update name service
    player_sd_update(sd,0.0);

    /*
    // Print current robot pose
    printf("position2d : %f %f %f\n",
           position2d->px, position2d->py, position2d->pa);
           */
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
