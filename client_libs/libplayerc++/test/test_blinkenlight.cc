/*
 * $Id$
 *
 * a test for the C++ Blinkenlight proxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_blinkenlight(PlayerClient* client, int index)
{
  unsigned char access;
  BlinkenlightProxy bp(client,index,'c');

  printf("device [blinkenlightfinder] index [%d]\n", index);

  TEST("subscribing (all)");
  if((bp.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", bp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", bp.driver_name);

  // wait for P2OS to start up
  for(int i=0; i < 10; i++)
    client->Read();

  bp.Print();

  // store the current state so we can reset it later
  int init_enable = bp.enable;
  int init_period_ms = bp.period_ms;

  TEST( "setting the light to flash at 100ms" );
  if( bp.SetLight( true, 100 ) < 0 )
    FAIL();
  else
    PASS();

  for(int i=0; i < 20; i++)
    client->Read();  

  TEST( "setting the light to flash at 200ms" );
  if( bp.SetLight( true, 200 ) < 0 )
    FAIL();
  else
    PASS();

  for(int i=0; i < 20; i++)
    client->Read();  

  TEST( "setting the light to flash at 400ms" );
  if( bp.SetLight( true, 400 ) < 0 )
    FAIL();
  else
    PASS();

  for(int i=0; i < 20; i++)
    client->Read();  

  TEST( "setting the light to flash at 1000ms" );
  if( bp.SetLight( true, 1000 ) < 0 )
    FAIL();
  else
    PASS();
  
  for(int i=0; i < 20; i++)
    client->Read();  

  TEST( "re-setting light to original state" );
  if( bp.SetLight( init_enable, init_period_ms ) < 0 )
    FAIL();
  else
    PASS();

    TEST("unsubscribing");
    if((bp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
       (access != PLAYER_CLOSE_MODE))
      {
	FAIL();
	return -1;
      }
    
    PASS();
    
    return(0);
}

