/***************************************************************************
 * Desc: Test program for the Player C client
 * Author: Andrew Howard
 * Date: 13 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "playerc.h"
#include "test.h"



int main(int argc, const char *argv[])
{
  playerc_client_t *client;
  const char *host;
  int port;
  int i;
  char *arg;
  const char *opt, *val;
  const char *device, *sindex; int index;

  // Default host, port
  host = "localhost";
  port = 6665;

  // Read program options
  for (i = 1; i < argc - 1; i += 2)
  {
    opt = argv[i];
    val = argv[i + 1];
    
    if (strcmp(opt, "-h") == 0)
      host = val;
    else if (strcmp(opt, "-p") == 0)
      port = atoi(val);
  }
  
  printf("host [%s:%d]\n", host, port);
  
  client = playerc_client_create(NULL, host, port);
  
  TEST("connecting");
  if (playerc_client_connect(client) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  // Run the tests
  for (i = 1; i < argc; i++)
  {
    if (strncmp(argv[i], "--", 2) != 0)
      continue;

    // Get device name and index
    arg = strdup(argv[i]);
    device = strtok(arg + 2, ":");
    sindex = strtok(NULL, "");
    index = (sindex ? atoi(sindex) : 0);

    // BPS device
    if (strcmp(device, "bps") == 0 || strcmp(device, "all") == 0)
      test_bps(client, index);

    // Broadcast device
    if (strcmp(device, "broadcast") == 0 || strcmp(device, "all") == 0)
      test_broadcast(client, index);

    // GPS device
    if (strcmp(device, "gps") == 0 || strcmp(device, "all") == 0)
      test_gps(client, index);

    // Laser device
    if (strcmp(device, "laser") == 0 || strcmp(device, "all") == 0)
      test_laser(client, index);

    // LBD (laser beacon detector) device
    if (strcmp(device, "lbd") == 0 || strcmp(device, "all") == 0)
      test_lbd(client, index);

    // Position device
    if (strcmp(device, "position") == 0 || strcmp(device, "all") == 0)
      test_position(client, index);

    // PTZ device
    if (strcmp(device, "ptz") == 0 || strcmp(device, "all") == 0)
      test_ptz(client, index);

    // Sonar device
    if (strcmp(device, "sonar") == 0 || strcmp(device, "all") == 0)
      test_sonar(client, index);

    // Truth device
    if (strcmp(device, "truth") == 0 || strcmp(device, "stage") == 0)
      test_truth(client, index);

    // Vision device
    if (strcmp(device, "vision") == 0 || strcmp(device, "all") == 0)
      test_vision(client, index);
    
    free(arg);
  }
    
  TEST("disconnecting");
  if (playerc_client_disconnect(client) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_client_destroy(client);

  return 0;
}


