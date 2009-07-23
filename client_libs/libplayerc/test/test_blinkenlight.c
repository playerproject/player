/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2003
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/***************************************************************************
 * Desc: Tests for the dio device
 * Author: Alexis Maldonado
 * Date: 3 May 2007
 **************************************************************************/

#include <unistd.h>

#include "test.h"
#include "playerc.h"

const uint32_t LIGHTCOUNT = 5;

// Basic test for an dio device.
int test_blinkenlight(playerc_client_t *client, int index) {
    playerc_blinkenlight_t *device;
    unsigned long shortsleep = 200000L;
    unsigned long longsleep = 500000L;
    int u;
    int lightnum;
    double rate;
    double duty;

    printf("device [blinkenlight] index [%d]\n", index);

    device = playerc_blinkenlight_create(client, index);

    TEST("subscribing (read/write)");
    if (playerc_blinkenlight_subscribe(device, PLAYER_OPEN_MODE) < 0) {
        FAIL();
        return -1;
    }
    PASS();

/*     for (t = 0; t < 5; t++) { */
/*       TEST1("reading light state (attempt %d)", t); */

/*       do */
/* 	rdevice = playerc_client_read(client); */
/*       while (rdevice == client); */

/*       if (rdevice == device)  */
/* 	{ */
/* 	  PASS(); */

/* 	  printf("blinkenlight: enable %u color (%hhX,%hhX,%hhX) period %.3f duty %.3f\n", */
/* 		 device->enable, */
/* 		 device->red, */
/* 	       device->green, */
/* 		 device->blue, */
/* 		 device->period, */
/* 		 device->duty ); */

/* 	} */
/*       else { */
/* 	//printf("error: %s", playerc_error_str()); */
/* 	FAIL(); */
/* 	break; */
/*       } */
/*     } */

    TEST("Turning light on");
    if (playerc_blinkenlight_enable( device, 1 )  != 0)
	FAIL();
    PASS();
    usleep( longsleep );

    TEST("Turning light off");
    if (playerc_blinkenlight_enable( device, 0 )  != 0)
	FAIL();
    PASS();
    usleep( longsleep );

    TEST("Turning light on");
    if (playerc_blinkenlight_enable( device, 1 )  != 0)
      FAIL();
    PASS();
    usleep( longsleep );

    TEST( "Setting colors RED");
    for( u=5; u<256; u+=50 )
      {
	for( index=0; index<LIGHTCOUNT; index++ )
	  {
	    if (playerc_blinkenlight_color(device, index, u,0,0 )  != 0)
	      {
		FAIL();
		break;
	    }
	    usleep( shortsleep );
	  }
      }
    PASS();

    TEST( "Setting colors GREEN");
    for( u=5; u<256; u+=50 )
      {
	for( lightnum=0; lightnum<LIGHTCOUNT; lightnum++ )
	  {
	    if (playerc_blinkenlight_color(device, lightnum, 0,u,0 )  != 0)
	      {
		FAIL();
		break;
	      }
	    usleep( shortsleep );
	  }
      }
    PASS();

    TEST( "Setting colors BLUE");
    for( u=5; u<256; u+=50 )
      {
	for( lightnum=0; lightnum<LIGHTCOUNT; lightnum++ )
	  {
	    if (playerc_blinkenlight_color(device, lightnum, 0,0,u )  != 0)
	      {
		FAIL();
		break;
	      }
	    usleep( shortsleep );
	  }
      }
    PASS();

    TEST( "Setting colors R+G+B");
    for( u=5; u<256; u+=50 )
      {
	for( lightnum=0; lightnum<LIGHTCOUNT; lightnum++ )
	{
	  if (playerc_blinkenlight_color(device, lightnum, u,u,u )  != 0)
	    {
	      FAIL();
	      break;
	    }
	  usleep( shortsleep );
	}
      }
    PASS();

    TEST( "Setting colors randomly");
    for( u=0; u<10; u++ )
      {
	for( lightnum=0; lightnum<LIGHTCOUNT; lightnum++ )
	  {
	    if (playerc_blinkenlight_color(device,
					   lightnum,
					   random()%255,
					   random()%255,
					   random()%255) != 0)
	      {
		FAIL();
		break;
	      }
	    usleep( shortsleep );
	  }
      }
    PASS();
    usleep(longsleep);  //some time to see the effect

    TEST("Varying blink rate");

    for( rate=3; rate<=10; rate++ )
      for( duty=0.1; rate<=1.0; rate+=0.1 )
	for( lightnum=0; lightnum<6; lightnum++ )
	  if (playerc_blinkenlight_blink( device, lightnum, rate, duty  )  != 0)
	    {
	      FAIL();
	      break;
	    }
    PASS();

    TEST("Turning light off");
    if (playerc_blinkenlight_enable( device, 0 )  != 0)
	FAIL();
    PASS();

    TEST("unsubscribing");
    if (playerc_blinkenlight_unsubscribe(device) != 0)
      FAIL();
    PASS();

    playerc_blinkenlight_destroy(device);

    return 0;
}

