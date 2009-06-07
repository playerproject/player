#include <stdio.h>
#include <libplayerc/playerc.h>

int main(int argc, const char **argv)
{
  playerc_client_t *client;
  playerc_position2d_t *position2d;

  client = playerc_client_create(NULL, "localhost", 6665);
  if(playerc_client_connect(client) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }
  position2d = playerc_position2d_create(client, 0);
  if(playerc_position2d_subscribe(position2d, PLAYERC_OPEN_MODE) != 0)
  {
    fprintf(stderr, "P2error: %s\n", playerc_error_str());
    return -1;
  }

  if(playerc_client_datamode(client, PLAYERC_DATAMODE_PULL) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }
  if(playerc_client_set_replace_rule(client, -1, -1,
                                     PLAYER_MSGTYPE_DATA, -1, 1) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }

  /* Goes 10cm forward */
  playerc_position2d_set_cmd_vel(position2d, 0.01, 0, 0, 1);
  while(position2d->px < 0.1)
  {
    playerc_client_read(client);
    printf("position (x,y,theta): %f %f %f\n",
           position2d->px, position2d->py, position2d->pa);
  }

  /* Shutdown and tidy up */
  playerc_position2d_unsubscribe(position2d);
  playerc_position2d_destroy(position2d);
  playerc_client_disconnect(client);
  playerc_client_destroy(client);

  return 0;
}
