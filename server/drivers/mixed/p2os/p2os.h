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

#include <driver.h>
#include <drivertable.h>
#include <packet.h>
#include <player.h>
#include <robot_params.h>
   
// Default max speeds
#define MOTOR_DEF_MAX_SPEED 500
#define MOTOR_DEF_MAX_TURNSPEED 100

/* 
 * Apparently, newer kernel require a large value (200000) here.  It only 
 * makes the initialization phase take a bit longer, and doesn't have any 
 * impact on the speed at which packets are received from P2OS 
 */
#define P2OS_CYCLETIME_USEC 200000

/* p2os constants */

/* Command numbers */
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
#define TTY2 42		// Added in AmigOS 1.2
#define GETAUX 43	// Added in AmigOS 1.2
#define JOYDRIVE 47
#define GYRO 58         // Added in AROS 1.8
#define TTY3 66		// Added in AmigOS 1.3
#define GETAUX2 67	// Added in AmigOS 1.3
#define SOUND 90
#define PLAYLIST 91

/* Server Information Packet (SIP) types */
#define STATUSSTOPPED	0x32
#define STATUSMOVING	0x33
#define	ENCODER		0x90
#define SERAUX		0xB0
#define SERAUX2		0xB8	// Added in AmigOS 1.3
#define GYROPAC         0x98    // Added AROS 1.8
//#define PLAYLIST	0xD0

/* Argument types */
#define ARGINT		0x3B	// Positive int (LSB, MSB)
#define ARGNINT		0x1B	// Negative int (LSB, MSB)
#define ARGSTR		0x2B	// String (Note: 1st byte is length!!)

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

/* CMUcam stuff */
#define CMUCAM_IMAGE_WIDTH	80
#define CMUCAM_IMAGE_HEIGHT	143
#define CMUCAM_MESSAGE_LEN	10

#define DEFAULT_P2OS_PORT "/dev/ttyS0"

typedef struct
{
  player_position_data_t position;
  player_sonar_data_t sonar;
  player_gripper_data_t gripper;
  player_power_data_t power;
  player_bumper_data_t bumper;
  player_dio_data_t dio;
  player_aio_data_t aio;
  player_blobfinder_data_t blobfinder;
  player_position_data_t compass;
  player_position_data_t gyro;
} __attribute__ ((packed)) player_p2os_data_t;

// this is here because we need the above typedef's before including it.
#include <sip.h>

class SIP;

class P2OS : public Driver 
{
  private:
    player_p2os_data_t p2os_data;
    
    player_device_id_t position_id;
    player_device_id_t sonar_id;
    player_device_id_t aio_id;
    player_device_id_t dio_id;
    player_device_id_t gripper_id;
    player_device_id_t bumper_id;
    player_device_id_t power_id;
    player_device_id_t compass_id;
    player_device_id_t gyro_id;
    player_device_id_t blobfinder_id;
    player_device_id_t sound_id;

    // bookkeeping to only send new gripper I/O commands
    bool sent_gripper_cmd;
    player_gripper_cmd_t last_gripper_cmd;

    // bookkeeping to only send new sound I/O commands
    bool sent_sound_cmd;
    player_sound_cmd_t last_sound_cmd;

    int position_subscriptions;
    int sonar_subscriptions;

    SIP* sippacket;
  
    int SendReceive(P2OSPacket* pkt);
    void ResetRawPositions();
    /* toggle sonars on/off, according to val */
    void ToggleSonarPower(unsigned char val);
    /* toggle motors on/off, according to val */
    void ToggleMotorPower(unsigned char val);
    void HandleConfig(void);
    void GetCommand(void);
    void PutData(void);
    void HandlePositionCommand(player_position_cmd_t position_cmd);
    void HandleGripperCommand(player_gripper_cmd_t gripper_cmd);
    void HandleSoundCommand(player_sound_cmd_t sound_cmd);

    int param_idx;  // index in the RobotParams table for this robot
    int direct_wheel_vel_control; // false -> separate trans and rot vel
    int psos_fd;               // p2os device file descriptor
    const char* psos_serial_port;
    struct timeval lastblob_tv;

    // Max motor speeds
    int motor_max_speed;
    int motor_max_turnspeed;

    // Bound the command velocities
    bool use_vel_band; 

    int radio_modemp; // are we using a radio modem?
    int joystickp; // are we using a joystick?

  public:

    P2OS(ConfigFile* cf, int section);

    int Subscribe(player_device_id_t id);
    int Unsubscribe(player_device_id_t id);

    /* the main thread */
    void Main();

    int Setup();
    int Shutdown();

    void CMUcamReset();
    void CMUcamTrack(int rmin=0, int rmax=0, int gmin=0,
                          int gmax=0, int bmin=0, int bmax=0);
    void CMUcamStopTracking();
};


#endif
