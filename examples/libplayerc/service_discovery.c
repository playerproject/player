#include <stdio.h>
#include <stdlib.h>

#include <libplayersd/playersd.h>
#include <libplayercore/error.h>
#include <libplayercore/addr_util.h>
#include <libplayercore/interface_util.h>
#include <libplayerc/playerc.h>

int
main(int argc, const char **argv)
{
  int i;
  playerc_client_t *client=NULL;
  playerc_position2d_t *position2d;
  char host[16];

  // A service discovery object
  player_sd_t* sd;
  player_sd_dev_t* sddev;

  // Initialize service discovery
  sd = player_sd_init();

  // Look for Player devices
  if(player_sd_browse(sd, 1.0, 0, NULL) != 0)
  {
    puts("player_sd_browse error");
    exit(-1);
  }

  // Did we find any position2d devices?
  for(i=0;i<sd->devs_len;i++)
  {
    if(sd->devs[i].valid)
    {
      printf("device with interf: %d\n", sd->devs[i].addr.interf);
      if(sd->devs[i].addr.interf == PLAYER_POSITION2D_CODE)
      {
        sddev = sd->devs + i;
        packedaddr_to_dottedip(host,sizeof(host),sddev->addr.host);

        client = playerc_client_create(NULL, host, sddev->addr.robot);
        if(0 != playerc_client_connect(client))
          exit(-1);

        // Create and subscribe to a position2d device.
        position2d = playerc_position2d_create(client, sddev->addr.index);
        if(playerc_position2d_subscribe(position2d, PLAYER_OPEN_MODE))
          exit(-1);

        break;
      }
    }
  }

  if(!client)
  {
    puts("no devices found");
    exit(-1);
  }

  // Make the robot spin!
  if (0 != playerc_position2d_set_cmd_vel(position2d, 0, 0, DTOR(40.0), 1))
    return -1;

  for (i = 0; i < 200; i++)
  {
    // Wait for new data from server
    playerc_client_read(client);

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
