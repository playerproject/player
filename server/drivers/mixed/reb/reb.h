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

/* Copyright (C) 2002
 *   John Sweeney, UMASS, Amherst, Laboratory for Perceptual Robotics
 *
 * $Id$
 *
 * Header for the REB device.  This is the K-Team Robotics Extension
 * Board attached to their Kameleon 376BC.  We connect to it via
 * the serial port of our ADS Bitsy (StrongARM based).  SO the
 * architecture is similar to the P2OS device, in that the position, IR and
 * power services all need to go through a single serial port and
 * base device class.  So this code was copied from p2osdevice and
 * modified to taste.
 * 
 */

#ifndef _REBDEVICE_H
#define _REBDEVICE_H

#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

// for poll
#include <replace/replace.h>
//#include <sys/poll.h>

#include <driver.h>
#include <playercommon.h>
#include <player.h>
#include <drivertable.h>
#include <reb_params.h>


#define REB_CONFIG_BUFFER_SIZE 1024
#define REB_BAUDRATE B38400
#define REB_DEFAULT_SERIAL_PORT "/dev/ttySA1"

#define REB_MOTOR_RIGHT 0
#define REB_MOTOR_LEFT 2

#define REB_BATTERY_CHANNEL 15

#define REB_AD_OFF 0
#define REB_AD_ON 1

#define REB_FIXED_FACTOR 10000

#define REB_MAX_ACC 100
#define REB_MIN_ACC 10

#define REB_POS_MODE_STRAIGHT 0
#define REB_POS_MODE_ROTATION 1

#define REB_IR_START 1
#define REB_IR_STOP 0

#define CRLF "\r\n"
#define REB_RESTART_COMMAND "restart\r\n"
#define REB_COMMAND_PROMPT ":\r\n"

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

#ifndef SGN
#define SGN(x) ((x) < 0 ? -1 : 1)
#endif

typedef struct {
  player_position_data_t position;
  player_ir_data_t ir;
  player_power_data_t power;
} __attribute__ ((packed)) player_reb_data_t;

typedef struct {
  player_position_cmd_t position;
} __attribute__ ((packed)) player_reb_cmd_t;


class REB : public Driver 
{
public:
  
  REB(char *interface, ConfigFile *cf, int section);

  /* the main thread */
  virtual void Main();
  
  // we override these, because we will maintain our own subscription count
  virtual int Subscribe(void *client);
  virtual int Unsubscribe(void *client);
  
  virtual int Setup();
  virtual int Shutdown();
  
  virtual void PutData(unsigned char *, size_t maxsize,
		       uint32_t timestamp_sec, uint32_t timestamp_usec);

  void Restart();

  void ReadConfig();

  void SetOdometry(int, int, short);
  
  // handle IR
  void SetIRState(int);

  void UpdateData(void);

  void UpdateIRData(player_reb_data_t *);
  void UpdatePowerData(player_reb_data_t *);
  void UpdatePosData(player_reb_data_t *);

  // the following are all interface functions to the REB
  // this handles the A/D device which deals with IR for us
  void ConfigAD(int, int);
  unsigned short ReadAD(int);
  void ReadAllIR(uint16_t * ir);

  // this handles motor control
  void SetSpeed(int, int );
  int ReadSpeed(int);

  void SetPos(int, int);
  
  void SetPosCounter(int, int);
  int ReadPos(int);
  
  unsigned char ReadStatus(int, int *, int *);
  void ConfigPosPID(int, int, int, int);
  void ConfigSpeedPID(int, int, int, int);
  void ConfigSpeedProfile(int, int, int);

protected:

  void Lock();
  void Unlock();
  
  void SetupLock();
  void SetupUnlock();
  
  /* start a thread that will invoke Main() */
  virtual void StartThread();
  /* cancel (and wait for termination) of the thread */
  virtual void StopThread();

private:

  int write_serial(char *, int);
  int read_serial_until(char *, int, char *, int);
  int write_command(char *buf, int len, int maxsize);
  
  static pthread_t thread;
  
  // since we have several child classes that must use the same lock, we 
  // declare our own static mutex here and override Lock() and Unlock() to 
  // use this mutex instead of the one declared in Driver.
  static pthread_mutex_t reb_accessMutex;
  
  // and this one protects calls to Setup and Shutdown
  static pthread_mutex_t reb_setupMutex;
  
  // likewise, we need one REB-wide subscription count to manage calls to
  static int reb_subscriptions;
  
  static int ir_subscriptions;
  static int pos_subscriptions;
  static int power_subscriptions;
  
  static player_reb_data_t* data;
  static player_reb_cmd_t* command;
  
  static unsigned char* reqqueue;
  static unsigned char* repqueue;
    
  static int param_index;  // index in the RobotParams table for this robot
  static int reb_fd;               // reb device file descriptor
  
  static struct timeval last_position; // last position update
  static bool refresh_last_position;
  static int last_lpos, last_rpos;
  static int last_x_f, last_y_f;
  static double last_theta;

  static struct timeval last_pos_update; // time of last pos update
  static struct timeval last_power_update;
  static struct timeval last_ir_update;

  static int pos_update_period;

  static short desired_heading;

  static int ir_sequence;
  static struct timeval last_ir;

  static bool motors_enabled;
  static bool velocity_mode;
  static bool direct_velocity_control;

  // device used to communicate with reb
  static char reb_serial_port[MAX_FILENAME_SIZE]; 

  static struct timeval timeBegan_tv;
  
  // did we initialize the common data segments yet?
  static bool initdone;
  
  static int locks, slocks;

  static struct pollfd write_pfd, read_pfd;

  
};


#endif
