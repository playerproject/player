/***************************************************************************
 * Desc: Test program for the Player C client
 * Author: Andrew Howard
 * Date: 13 May 2002
 # CVS: $Id$
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "playerc.h"

// Message macros
#define TEST(msg) printf(msg " ... "); fflush(stdout)
#define TEST1(msg, a) printf(msg " ... ", a); fflush(stdout)
#define PASS() (1 ? printf("pass\n"), fflush(stdout) : 0)
#define FAIL() (1 ? printf("\033[41mfail\033[0m\n%s\n", playerc_errorstr), fflush(stdout) : 0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))


/***************************************************************************
 * Test the position device
 **************************************************************************/

int test_position(playerc_client_t *client, int index)
{
  int t;
  playerc_position_t *position;

  printf("device [position] index [%d]\n", index);

  position = playerc_position_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_position_subscribe(position, PLAYER_ALL_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);
    if (playerc_client_read(client) != 0)
    {
      FAIL();
      return -1;
    }
    PASS();

    printf("position: [%6.3f] [%6.3f] [%6.3f] [%d]\n",
           position->px, position->py, position->pa, position->stall);
  }
  
  TEST("unsubscribing");
  if (playerc_position_unsubscribe(position) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_position_destroy(position);
  
  return 0;
}


/***************************************************************************
 * Test the laser device
 **************************************************************************/

int test_laser(playerc_client_t *client, int index)
{
  int t, i;
  playerc_laser_t *laser;

  double min, max;
  int resolution, intensity;

  printf("device [laser] index [%d]\n", index);

  laser = playerc_laser_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_laser_subscribe(laser, PLAYER_ALL_MODE) == 0)
    PASS();
  else
    FAIL();
  
  TEST("set configuration");
  min = -M_PI/2;
  max = +M_PI/2;
  resolution = 100;
  intensity = 1;
  if (playerc_laser_set_config(laser, min, max, resolution, intensity) == 0)
    PASS();
  else
    FAIL();

  TEST("get configuration");
  if (playerc_laser_get_config(laser, &min, &max, &resolution, &intensity) == 0)
    PASS();
  else
    FAIL();

  TEST("check configuration sanity");
  if (abs(min + M_PI/2) > 0.01 || abs(max - M_PI/2) > 0.01)
    FAIL();
  else if (resolution != 100 || intensity != 1)
    FAIL();
  else
    PASS();

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);
    if (playerc_client_read(client) != 0)
    {
      FAIL();
      break;
    }
    PASS();

    printf("laser: [%d] ", laser->scan_count);
    for (i = 0; i < 3; i++)
      printf("[%6.3f, %6.3f] ", laser->scan[i][0], laser->scan[i][1]);
    printf("\n");
  }
  
  TEST("unsubscribing");
  if (playerc_laser_unsubscribe(laser) == 0)
    PASS();
  else
    FAIL();
  
  playerc_laser_destroy(laser);
  
  return 0;
}


/***************************************************************************
 * Test the laserbeacon device
 **************************************************************************/

int test_laserbeacon(playerc_client_t *client, int index)
{
  int t, i;
  playerc_laserbeacon_t *laserbeacon;

  printf("device [laserbeacon] index [%d]\n", index);

  laserbeacon = playerc_laserbeacon_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_laserbeacon_subscribe(laserbeacon, PLAYER_ALL_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);
    if (playerc_client_read(client) != 0)
    {
      FAIL();
      return -1;
    }
    PASS();

    printf("laserbeacon: [%d] ", laserbeacon->beacon_count);
    for (i = 0; i < MIN(3, laserbeacon->beacon_count); i++)
      printf("[%d %6.3f, %6.3f, %6.3f] ", laserbeacon->beacons[i].id,
             laserbeacon->beacons[i].range, laserbeacon->beacons[i].bearing,
             laserbeacon->beacons[i].orient);
    printf("\n");
  }
  
  TEST("unsubscribing");
  if (playerc_laserbeacon_unsubscribe(laserbeacon) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_laserbeacon_destroy(laserbeacon);
  
  return 0;
}



int main(int argc, const char *argv[])
{
  playerc_client_t *client;
  const char *host;
  int port;
  int i;
  const char *opt, *val;

  // Default program options
  host = "localhost";
  port = 6665;

  // Read program options
  for (i = 1; i < argc - 1; i += 2)
  {
    opt = argv[i];
    val = argv[i + 1];
    
    if (strcmp(opt, "-h") == 0)
      host = val;
    if (strcmp(opt, "-p") == 0)
      port = atoi(val);
  }
  
  printf("host [%s:%d]\n", host, port);
  
  client = playerc_client_create(NULL, host, port);
  
  TEST("Connecting");
  if (playerc_client_connect(client) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  // Run the tests
  //test_position(client, 0);
  test_laser(client, 0);
  //test_laserbeacon(client, 0);
  
  TEST("Disconnecting");
  if (playerc_client_disconnect(client) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_client_destroy(client);

  return 0;
}


