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
 *   this file was adapted from the P2OS device for the RWI RFLEX robots
 *
 *   the RFLEX device.  it's the parent device for all the RFLEX 'sub-devices',
 *   like, position, sonar, etc.  there's a thread here that
 *   actually interacts with RFLEX via the serial line.  the other
 *   "devices" communicate with this thread by putting into and getting
 *   data out of shared buffers.
 */
#ifndef _RFLEXDEVICE_H
#define _RFLEXDEVICE_H

#include <pthread.h>
#include <sys/time.h>

#include <device.h>
#include <drivertable.h>
#include <player.h>
   
#include "rflex_commands.h"
#include "rflex-io.h"

#include "rflex_configs.h"

#define RFLEX_MOTORS_REQUEST_ON 0
#define RFLEX_MOTORS_ON 1
#define RFLEX_MOTORS_REQUEST_OFF 2
#define RFLEX_MOTORS_OFF 3

#define RFLEX_CONFIG_BUFFER_SIZE 256

#define DEFAULT_RFLEX_PORT "/dev/ttyS0"

#define MAX_NUM_LOOPS                 30
#define B_STX                         0x02
#define B_ETX                         0x03
#define B_ESC                         0x1b

typedef struct
{
  player_position_data_t position;
  player_sonar_data_t sonar;
  player_gripper_data_t gripper;
  player_power_data_t power;
  player_bumper_data_t bumper;
  player_dio_data_t dio;
  player_aio_data_t aio;
} __attribute__ ((packed)) player_rflex_data_t;

typedef struct
{
  player_position_cmd_t position;
  player_gripper_cmd_t gripper;
  player_sound_cmd_t sound;
} __attribute__ ((packed)) player_rflex_cmd_t;

// this is here because we need the above typedef's before including it.

class RFLEX:public CDevice 
{
  private:
    static pthread_t thread;
    //static pthread_mutex_t serial_mutex;
  
    // since we have several child classes that must use the same lock, we 
    // declare our own static mutex here and override Lock() and Unlock() to 
    // use this mutex instead of the one declared in CDevice.
    static pthread_mutex_t rflex_accessMutex;

    // and this one protects calls to Setup and Shutdown
    static pthread_mutex_t rflex_setupMutex;

    // likewise, we need one RFLEX-wide subscription count to manage calls to
    // Setup() and Shutdown()
    static int rflex_subscriptions;

    static player_rflex_data_t* data;
    static player_rflex_cmd_t* command;

    static unsigned char* reqqueue;
    static unsigned char* repqueue;

    static int rflex_fd;               // rflex device file descriptor
    
    // device used to communicate with rflex
    static char rflex_serial_port[MAX_FILENAME_SIZE]; 
    static double mm_odo_x;
    static double mm_odo_y;
    static double rad_odo_theta;

    static struct timeval timeBegan_tv;

    // did we initialize the common data segments yet?
    static bool initdone;

    void ResetRawPositions();
    int initialize_robot();
    void reset_odometry();
    void set_odometry(long,long,short);
    void update_everything(player_rflex_data_t* d, CDevice* sonarp);    

    void set_config_defaults();
  protected:
    void Lock();
    void Unlock();

    /* start a thread that will invoke Main() */
    virtual void StartThread();
    /* cancel (and wait for termination) of the thread */
    virtual void StopThread();

  public:

    RFLEX(char* interface, ConfigFile* cf, int section);

    //override this in subclass, and fill with code to load options from
    //config file, default is just an empty dummy
    virtual void GetOptions(ConfigFile* cf,int section,rflex_config_t *rflex_configs);
    virtual ~RFLEX();

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
