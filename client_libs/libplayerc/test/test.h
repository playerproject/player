/***************************************************************************
 * Desc: Test program for the Player C client
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "playerc.h"

// Message macros
#define TEST(msg) (1 ? printf(msg " ... "), fflush(stdout) : 0)
#define TEST1(msg, a) (1 ? printf(msg " ... ", a), fflush(stdout) : 0)
#define PASS() (1 ? printf("pass\n"), fflush(stdout) : 0)
#define FAIL() (1 ? printf("\033[41mfail\033[0m\n%s\n", playerc_errorstr), fflush(stdout) : 0)

// Basic broadcast test
int test_broadcast(playerc_client_t *client, int index);

// Basic laser test
int test_laser(playerc_client_t *client, int index);

// Basic test for the LBD (laser beacon detector) device.
int test_lbd(playerc_client_t *client, int index);

// Basic test for position device.
int test_position(playerc_client_t *client, int index);

// Basic vision test.
int test_vision(playerc_client_t *client, int index);


#endif // TEST_H
