/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 *
 *   the P2OS device.  it's the parent device for all the P2 'sub-devices',
 *   like gripper, position, sonar, etc.  there's a thread here that
 *   actually interacts with P2OS via the serial line.  the other
 *   "devices" communicate with this thread by putting into and getting
 *   data out of shared buffers.
 */
#ifndef _P2OSDEVICE_H
#define _P2OSDEVICE_H

#include <pthread.h>
#include <sys/time.h>

#include <device.h>
#include <packet.h>
#include <playercommon.h>
#include <messages.h>
#include <robot_params.h>
   
#define P2OS_MOTORS_REQUEST_ON 0
#define P2OS_MOTORS_ON 1
#define P2OS_MOTORS_REQUEST_OFF 2
#define P2OS_MOTORS_OFF 3

/* data for the p2-dx robot from p2 operation manual */
#define P2OS_CYCLETIME_USEC 100000

/* p2os constants */
#define SYNC0 0
#define SYNC1 1
#define SYNC2 2

#define PULSE 0
#define OPEN 1
#define CLOSE 2
#define ENABLE 4
#define SETV 6
#define SETO 7
#define VEL 11
#define RVEL 21
#define SONAR 28
#define STOP 29
#define VEL2 32
#define GRIPPER 33
#define GRIPPERVAL 36
#define TTY2 42
#define GETAUX 43

/* gripper stuff */
#define GRIPopen   1
#define GRIPclose  2
#define GRIPstop   3
#define LIFTup     4
#define LIFTdown   5
#define LIFTstop   6
#define GRIPstore  7
#define GRIPdeploy 8
#define GRIPhalt   15
#define GRIPpress  16
#define LIFTcarry  17

#define P2OS_CONFIG_BUFFER_SIZE 256

typedef struct
{
  player_position_data_t position;
  player_sonar_data_t sonar;
  player_gripper_data_t gripper;
  player_misc_data_t misc;
} __attribute__ ((packed)) player_p2os_data_t;

typedef struct
{
  player_position_cmd_t position;
  player_gripper_cmd_t gripper;
} __attribute__ ((packed)) player_p2os_cmd_t;

// this is here because we need the above typedef's before including it.
#include <sip.h>

class SIP;

class P2OS:public CDevice 
{
  private:
    static pthread_t thread;
    //static pthread_mutex_t serial_mutex;
    static SIP* sippacket;
  
    // since we have several child classes that must use the same lock, we 
    // declare our own static mutex here and override Lock() and Unlock() to 
    // use this mutex instead of the one declared in CDevice.
    static pthread_mutex_t p2os_accessMutex;

    // and this one protects calls to Setup and Shutdown
    static pthread_mutex_t p2os_setupMutex;

    // likewise, we need one P2OS-wide subscription count to manage calls to
    // Setup() and Shutdown()
    static int p2os_subscriptions;

    static player_p2os_data_t* data;
    static player_p2os_cmd_t* command;

    static unsigned char* reqqueue;
    static unsigned char* repqueue;

    int SendReceive(P2OSPacket* pkt);
    void ResetRawPositions();

    static int param_idx;  // index in the RobotParams table for this robot
    static bool direct_wheel_vel_control; // false -> separate trans and rot vel
    static char num_loops_since_rvel;  
    static int psos_fd;               // p2os device file descriptor
    
    // device used to communicate with p2os
    static char psos_serial_port[MAX_FILENAME_SIZE]; 
    static bool radio_modemp; // are we using a radio modem?

    static struct timeval timeBegan_tv;

    // did we initialize the common data segments yet?
    static bool initdone;

  protected:
    void Lock();
    void Unlock();

    /* start a thread that will invoke Main() */
    virtual void StartThread();
    /* cancel (and wait for termination) of the thread */
    virtual void StopThread();

  public:

    P2OS(int argc, char** argv);

    /* the main thread */
    virtual void Main();

    // we override these, because we will maintain our own subscription count
    virtual int Subscribe(void *client);
    virtual int Unsubscribe(void *client);

    virtual int Setup();
    virtual int Shutdown();

    virtual void PutData(unsigned char *, size_t maxsize,
                         uint32_t timestamp_sec, uint32_t timestamp_usec);
};


#endif
