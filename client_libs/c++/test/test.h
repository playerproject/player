/***************************************************************************
 * Desc: Test program for the Player C++ client
 * Author: Andrew Howard, Brian Gerkey, Richard Vaughan
 * Date: 28 May 2002
 * CVS: $Id$
 **************************************************************************/

#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "playerclient.h"

extern bool use_stage;

// Message macros
#define TEST(msg) (1 ? printf(msg " ... "), fflush(stdout) : 0)
#define TEST1(msg, a) (1 ? printf(msg " ... ", a), fflush(stdout) : 0)
#define PASS() (1 ? printf("pass\n"), fflush(stdout) : 0)
#define FAIL() (1 ? printf("\033[41mfail\033[0m\n"), fflush(stdout) : 0)

extern int robot;

int test_gps(PlayerClient* client, int index);
int test_position(PlayerClient* client, int index);
int test_position_control(PlayerClient* client, int index);
int test_sonar(PlayerClient* client, int index);
int test_power(PlayerClient* client, int index);
int test_dio(PlayerClient* client, int index);
int test_laser(PlayerClient* client, int index);
int test_ptz(PlayerClient* client, int index);
int test_speech(PlayerClient* client, int index);
int test_vision(PlayerClient* client, int index);
int test_lbd(PlayerClient* client, int index);
int test_broadcast(PlayerClient* client, int index);
int test_gripper(PlayerClient* client, int index);
int test_truth(PlayerClient* client, int index);
int test_idar(PlayerClient* client, int index);
int test_idarturret(PlayerClient* client, int index);
int test_bumper(PlayerClient* client, int index);
int test_wifi(PlayerClient* client, int index);
int test_mcom(PlayerClient* client, int index);
int test_fiducial(PlayerClient* client, int index);
int test_lookup(PlayerClient* client, int index);

//int test_bps(PlayerClient* client, int index);

#endif // TEST_H
