/*
 * $Id$
 *
 * a test for the C++ BumperProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_bumper(PlayerClient* client, int index)
{
  unsigned char access;
  BumperProxy sp(client,index,'c',robot);

  printf("device [bumper] index [%d]\n", index);

  TEST("subscribing (read)");
  if((sp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", sp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", sp.driver_name);

  // wait for P2OS to start up
  for(int i=0; i < 20; i++)
    client->Read();

  TEST("getting bumper geometry");

  player_bumper_geom_t bumper_geom;
  if(sp.GetBumperGeom( &bumper_geom ) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();
  printf( "Discovered %d bumper geometries\n", bumper_geom.bumper_count );
  for(int i=0;i<bumper_geom.bumper_count;i++)
    {
    printf("Bumper[%d]: (%4d,%4d,%4d) len: %4u radius: %4u\n", i, 
           bumper_geom.bumper_def[i].x_offset,
           bumper_geom.bumper_def[i].y_offset,
           bumper_geom.bumper_def[i].th_offset,
           bumper_geom.bumper_def[i].length,
           bumper_geom.bumper_def[i].radius );
  }

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }

    PASS();

    sp.Print();
  }


  TEST("unsubscribing");
  if((sp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

