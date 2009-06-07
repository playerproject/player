#include <stdio.h>
#include <libplayerc/playerc.h>

int main(int argc, const char **argv)
{
  int i, stop;
  playerc_client_t *client;
  playerc_ir_t *ir1;

  client = playerc_client_create(NULL, "localhost", 6665);
  if(playerc_client_connect(client) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }
  ir1 = playerc_ir_create(client, 0);
  if(playerc_ir_subscribe(ir1, PLAYERC_OPEN_MODE) != 0)
  {
    fprintf(stderr, "IRRerror: %s\n", playerc_error_str());
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

  /* It Read the IR sensors until an object distance 2cm or lesser, *
   * from any of eight sensors.                                     */
  stop = 0;
  while(!stop)
  {
    playerc_client_read(client);

    for(i=0; i < ir1->data.ranges_count; i++)
    {
      printf("ir%d: %f   ", i, ir1->data.ranges[i]);
      if(ir1->data.ranges[i] < 0.02)
        stop = 1;
    }
      printf("\n");
  }

  /* Shutdown and tidy up */
  playerc_ir_unsubscribe(ir1);
  playerc_ir_destroy(ir1);
  playerc_client_disconnect(client);
  playerc_client_destroy(client);

  return 0;
}
