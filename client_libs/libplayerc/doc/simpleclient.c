
#include <stdio.h>
#include "playerc.h"

int main(int argc, const char **argv)
{
  int i;
  playerc_client_t *client;
  playerc_position_t *position;

  // Create a client object and connect to the server; the server must
  // be running on "localhost" at port 6665
  client = playerc_client_create(NULL, "localhost", 6665);
  if (playerc_client_connect(client) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }

  // Create a position proxy (device id "position:0") and susbscribe
  // in read/write mode
  position = playerc_position_create(client, 0);
  if (playerc_position_subscribe(position, PLAYER_ALL_MODE) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }

  // Enable the robots motors
  playerc_position_enable(position, 1);

  // Start the robot turning slowing
  playerc_position_set_cmd_vel(position, 0, 0, 0.1, 1);

  for (i = 0; i < 200; i++)
  {
    // Read data from the server and display current robot position
    playerc_client_read(client);
    printf("position : %f %f %f\n",
           position->px, position->py, position->pa);
  } 

  // Shutdown and tidy up
  playerc_position_unsubscribe(position);
  playerc_position_destroy(position);
  playerc_client_disconnect(client);
  playerc_client_destroy(client);

  return 0;
}
