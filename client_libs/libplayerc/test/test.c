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
  int all;
  int i;
  char *arg;
  const char *opt, *val;
  const char *device, *sindex; int index;

  // Default host, port
  host = "localhost";
  port = 6665;
  all = 1;

  // Read program options (host and port).
  for (i = 1; i < argc - 1; i += 2)
  {
    opt = argv[i];
    val = argv[i + 1];
    if (strcmp(opt, "-h") == 0)
      host = val;
    else if (strcmp(opt, "-p") == 0)
      port = atoi(val);
  }

  // If there are individual device arguments, dont do all tests.
  for (i = 1; i < argc; i++)
  {
    opt = argv[i];
    if (strncmp(opt, "--", 2) == 0)
      all = 0;
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

  if (all)
  {
    // Get the available device list from the server.
    TEST("querying interface list");
    if (playerc_client_get_devlist(client) < 0)
    {
      FAIL();
      return -1;
    }
    PASS();
  }
  else
  {
    // Override the auto-detected stuff with command line directives.
    for (i = 1; i < argc; i++)
    {
      if (strncmp(argv[i], "--", 2) != 0)
        continue;

      // Get device name and index
      arg = strdup(argv[i]);
      device = strtok(arg + 2, ":");
      sindex = strtok(NULL, "");
      index = (sindex ? atoi(sindex) : 0);

      client->ids[client->id_count].code = playerc_lookup_code(device);
      client->ids[client->id_count].index = index;
      client->id_count++;

      free(arg);
    }
  }

  // Print interface list.
  printf("selected interfaces:\n", host, port);
  for (i = 0; i < client->id_count; i++)
    printf("  %s:%d \n", playerc_lookup_name(client->ids[i].code), client->ids[i].index);

  // Run all tests
  for (i = 0; i < client->id_count; i++)
  {
    switch (client->ids[i].code)
    {
      // Broadcast device
      case PLAYER_COMMS_CODE:
        test_comms(client, client->ids[i].index);
        break;

      // GPS device
      case PLAYER_GPS_CODE:
        test_gps(client, client->ids[i].index);
        break;

      // Laser device
      case PLAYER_LASER_CODE:
        test_laser(client, client->ids[i].index);
        break;

      // Fiducial detector
      case PLAYER_FIDUCIAL_CODE:
        test_fiducial(client, client->ids[i].index);
        break;

      // Position device
      case PLAYER_POSITION_CODE:
        test_position(client, client->ids[i].index);
        break;

      // PTZ device
      case PLAYER_PTZ_CODE:
        test_ptz(client, client->ids[i].index);
        break;

      // Sonar device
      case PLAYER_SONAR_CODE:
        test_sonar(client, client->ids[i].index);
        break;

      // Truth device
      case PLAYER_TRUTH_CODE:
        test_truth(client, client->ids[i].index);
        break;

      // Blobfinder device
      case PLAYER_BLOBFINDER_CODE:
        test_blobfinder(client, client->ids[i].index);
        break;

      default:
        printf("no test for interface [%s]\n",
               playerc_lookup_name(client->ids[i].code));
        break;
    }
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


