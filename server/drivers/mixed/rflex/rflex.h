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

#include <driver.h>
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

#define DEFAULT_RFLEX_BUMPER_ADDRESS 0x40
#define RFLEX_BUMPER_STYLE_BIT "bit"
#define RFLEX_BUMPER_STYLE_ADDR "addr"
#define DEFAULT_RFLEX_BUMPER_STYLE RFLEX_BUMPER_STYLE_ADDR

enum 
{
   BUMPER_BIT,
   BUMPER_ADDR
};

#define DEFAULT_RFLEX_POWER_OFFSET 0

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
  player_ir_data ir;
} __attribute__ ((packed)) player_rflex_data_t;

/*
typedef struct
{
  player_position_cmd_t position;
  player_gripper_cmd_t gripper;
  player_sound_cmd_t sound;
} __attribute__ ((packed)) player_rflex_cmd_t;
*/

// this is here because we need the above typedef's before including it.

class RFLEX : public Driver 
{
  private:
    player_device_id_t position_id;
    player_device_id_t sonar_id;
    player_device_id_t ir_id;
    player_device_id_t bumper_id;
    player_device_id_t power_id;
    player_device_id_t aio_id;
    player_device_id_t dio_id;

    int position_subscriptions;
    int sonar_subscriptions;
    int ir_subscriptions;
    int bumper_subscriptions;

    int rflex_fd;               // rflex device file descriptor
    
    // device used to communicate with rflex
    char rflex_serial_port[MAX_FILENAME_SIZE]; 
    double mm_odo_x;
    double mm_odo_y;
    double rad_odo_theta;

    void ResetRawPositions();
    int initialize_robot();
    void reset_odometry();
    void set_odometry(long,long,short);
    void update_everything(player_rflex_data_t* d);

    void set_config_defaults();

  public:
    RFLEX(ConfigFile* cf, int section);

    //override this in subclass, and fill with code to load options from
    //config file, default is just an empty dummy
    virtual void GetOptions(ConfigFile* cf,int section,rflex_config_t *rflex_configs);

    /* the main thread */
    virtual void Main();

    // we override these, because we will maintain our own subscription count
    virtual int Subscribe(player_device_id_t id);
    virtual int Unsubscribe(player_device_id_t id);

    virtual int Setup();
    virtual int Shutdown();

    static int joy_control;
};


#endif
