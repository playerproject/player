/*
 *  test_coopobject.c : a Player client library test
 *  Copyright (C) Andrew Howard and contributors 2002-2007
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 */
/***************************************************************************
 * Desc: Test for the Cooperating Object proxy
 * Author: Adrian Jimenez Gonzalez
 * Date: 15 Aug 2011
 **************************************************************************/

#include "test.h"
#include "playerc.h"

// Basic Cooperating Object test
int test_coopobject(playerc_client_t *client, int index)
{
  int t, i;//, j;
  void *rdevice;
  playerc_coopobject_t *device;

  printf("device [wsn] index [%d]\n", index);

  device = playerc_coopobject_create(client, index);

  TEST("subscribing (read)");
  if (playerc_coopobject_subscribe(device, PLAYER_OPEN_MODE) == 0)
    PASS();
  else
    FAIL();

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);
    
    if (rdevice == device)
    {
      PASS();
      printf("Node type: %d, with ID %d and parent %d holds:\n"
		"Time Stamp  : [%f]\n",
		device->node_type, device->node_id, device->node_parent_id,
		(device->timeSec)+(1e-6)*((float)(device->timeUsec)));

		if (device->sensor_data != NULL && device->sensor_data_count >0) {
			printf("Sensor data   :\n");
			for (i = 0; i < device->sensor_data_count; i++){
			if(device->sensor_data[i].type !=0 )
				printf ("\t sensor %d is of type %d: [%d]\n", i, device->sensor_data[i].type, device->sensor_data[i].value);
			}
		}
		if (device->alarm_data != NULL && device->alarm_data_count >0) {
			printf("Alarm data    :\n");
			for (i = 0; i < device->alarm_data_count; i++){
				if(device->alarm_data[i].type != 0)
					printf ("\t alarm %d is of type %d  = [%d]\n", i, device->alarm_data[i].type, device->alarm_data[i].value);
			}
		}
		if (device->user_data != NULL && device->user_data_count >0) {
			printf("user %d data	:\n", device->user_type);
			for (i = 0; i < device->user_data_count; i++){
				printf ("\t data %d = [%d]\n", i, device->user_data[i]);
			}
		}
		printf("RSSI data    :\n"
		"\t Mobile ID         : [%d]\n"
		"\t Fixed ID          : [%d]\n"
		"\t RSSI              : [%d]\n"
		"\t Stamp             : [%d]\n"
		"\t Node time         : [%f]\n"
		"\t Position (x,y,z)  : [%d,%d,%d]\n",
		device->RSSImobileId,
		device->RSSIfixedId,
		device->RSSIvalue,
		device->RSSIstamp,
		(device->RSSInodeTimeHigh+1e-6*device->RSSInodeTimeLow),
		device->x, device->y,
		device->z);
    }
    else
    {
      FAIL();
      break;
    }
  }
/*  
  TEST("setting the data frequency rate");
  if(playerc_coopobject_datafreq(device, -1, -1, 1) < 0)
    FAIL();
  else
  {
    sleep(3);
    PASS();
  }
  
  TEST("enabling all LEDs");
  if(playerc_coopobject_set_devstate(device, -1, -1, 3, 7) < 0)
    FAIL();
  else
  {
    sleep(3);
    PASS();
  }
  
/*  TEST("going to sleep");
  if(playerc_coopobject_power(device, 2, -1, 0) < 0)
    FAIL();
  else
  {
    sleep(3);
    PASS();
  }
  
  TEST("waking up");
  if(playerc_coopobject_power(device, 1, -1, 1) < 0)
    FAIL();
  else
  {
    sleep(3);
    PASS();
  }
*/  

  TEST("unsubscribing");
  if (playerc_coopobject_unsubscribe(device) == 0)
    PASS();
  else
    FAIL();
  
  playerc_coopobject_destroy(device);
  
  return 0;
}
