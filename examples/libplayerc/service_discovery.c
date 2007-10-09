#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <libplayersd/playersd.h>
#include <libplayercore/error.h>
#include <libplayercore/addr_util.h>
#include <libplayercore/interface_util.h>
#include <libplayerc/playerc.h>

#define MAX_POSITION_DEVS 16

void
browse_cb(player_sd_t* sd, player_sd_dev_t* dev)
{
  printf("found new device: %s:%u:%s:%u\n",
         dev->hostname,
         dev->robot,
         interf_to_str(dev->interf),
         dev->index);
}

int
main(int argc, const char **argv)
{
  int i;
  playerc_client_t *client=NULL;
  playerc_position2d_t *position2d;

  // A service discovery object
  player_sd_t* sd;
  // An array to store matching devices
  player_sd_dev_t positiondevs[MAX_POSITION_DEVS];
  int num_positiondevs;

  // Initialize service discovery
  sd = player_sd_init();

  // Look for Player devices
  if(player_sd_browse(sd, 0.0, 1, browse_cb) != 0)
  {
    puts("player_sd_browse error");
    exit(-1);
  }

  while((num_positiondevs = player_sd_find_devices(sd, positiondevs, 
                                                   MAX_POSITION_DEVS,
                                                   NULL, NULL, -1,
                                                   PLAYER_POSITION2D_CODE, 
                                                   -1)) < 1)
  {
    // Update name service
    puts("player_sd_update");
    player_sd_update(sd,0.1);
  }

  printf("found %d position2d devices\n", num_positiondevs);

  // Subscribe to the first one
  client = playerc_client_create(NULL, 
                                 positiondevs[0].hostname,
                                 positiondevs[0].robot);
  if(0 != playerc_client_connect(client))
    exit(-1);

  // Create and subscribe to a position2d device.
  position2d = playerc_position2d_create(client, positiondevs[0].index);
  if(playerc_position2d_subscribe(position2d, PLAYER_OPEN_MODE))
    exit(-1);

  // Make the robot move
  if(0 != playerc_position2d_set_cmd_vel(position2d, 0.35, 0, DTOR(40.0), 1))
    return -1;

  for(i = 0; i < 200; i++)
  {
    // Wait for new data from server
    playerc_client_read(client);

    // Update name service
    player_sd_update(sd,0.0);

    // Print current robot pose
    printf("position2d : %f %f %f\n",
           position2d->px, position2d->py, position2d->pa);
  }

  // Shutdown
  playerc_position2d_unsubscribe(position2d);
  playerc_position2d_destroy(position2d);
  playerc_client_disconnect(client);
  playerc_client_destroy(client);


  return(0);
}
