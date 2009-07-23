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


// Basic test for an dio device.
int test_dio(playerc_client_t *client, int index) {
    int t;
    void *rdevice;
    playerc_dio_t *device;
    int i;
    unsigned int do_value;
    const unsigned int do_count = 8;

    printf("device [dio] index [%d]\n", index);

    device = playerc_dio_create(client, index);

    TEST("subscribing (read/write)");
    if (playerc_dio_subscribe(device, PLAYER_OPEN_MODE) < 0) {
        FAIL();
        return -1;
    }
    PASS();

    for (t = 0; t < 5; t++) {
        TEST1("reading data (attempt %d)", t);

        do
            rdevice = playerc_client_read(client);
        while (rdevice == client);

        if (rdevice == device) {
            PASS();

            printf("dio: [%8.3f] MSB...LSB:[ ",
                   device->info.datatime);
            //printf("%d :  ",device->digin);

            for (i=0 ; i != device->count ; i++) {
                //Extract the values of the single bits (easier on the eyes)
                int dix=(device->digin & ( 1<<(device->count-(i+1)) ) )? 1 : 0 ;
                printf("%1d",dix);
                if (((i+1) % 4)==0) {
                    printf(" ");
                }

            }
            printf("]\n");

        }
        else {
            //printf("error: %s", playerc_error_str());
            FAIL();
            break;
        }
    }


    do_value = 0;
    for (t = 0; t < 5; t++) {

        TEST1("writing data (attempt %d)", t);
        TEST1("  DO Value: %d",do_value);

        do_value++;

        if (playerc_dio_set_output(device, do_count, do_value  )  != 0) {
            FAIL();
            break;
        }
        PASS();
        usleep(200000);  //some time to see the effect
    }

    //turn everything off:
    do_value = 0;
    playerc_dio_set_output(device, do_count, do_value  );


    TEST("unsubscribing");
    if (playerc_dio_unsubscribe(device) != 0) {
        FAIL();
        return -1;
    }
    PASS();

    playerc_dio_destroy(device);

    return 0;
}
