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
 *   the clodbuster device.   there's a thread here that
 *   actually interacts with grasp board via the serial line.  the other
 *   "devices" communicate with this thread by putting into and getting
 *   data out of shared buffers.
 */
#ifndef _CLODBUSTERDEVICE_H
#define _CLODBUSTERDEVICE_H

#include <pthread.h>
#include <sys/time.h>

#include <device.h>
#include <drivertable.h>
#include <packet.h>
#include <player.h>
//#include <robot_params.h>
/*  
#define P2OS_MOTORS_REQUEST_ON 0
#define P2OS_MOTORS_ON 1
#define P2OS_MOTORS_REQUEST_OFF 2
#define P2OS_MOTORS_OFF 3
*/
/* data for the clodbuster */
#define CLODBUSTER_CYCLETIME_USEC 50000

/* Grasp Board Command numbers */
#define SYNC 255

#define SET_SERVO_THROTTLE 0
#define SET_SERVO_FRONTSTEER 1
#define SET_SERVO_BACKSTEER 2
#define SET_SERVO_PAN 3

#define ECHO_SERVO_VALUES 64 // 0x40
#define ECHO_MAX_SERVO_LIMITS 65 //0x41
#define ECHO_MIN_SERVO_LIMITS 66 //0x42
#define ECHO_CEN_SERVO_LIMITS 67 //0x43

#define ECHO_ENCODER_COUNTS 112 // 0x70
#define ECHO_ENCODER_COUNTS_TS 113 // 0x71
#define ECHO_ADC 128 //0x80
#define READ_ID 97

#define SET_SLEEP_MODE 144 // 0x90
#define ECHO_SLEEP_MODE 145 // 0x91

#define SLEEP_MODE_ON 1
#define SLEEP_MODE_OFF 0

#define SERVO_CHANNELS 8

// I think this might be useful.. so leaving it in for the time being
/* Argument types */
#define ARGINT		0x3B	// Positive int (LSB, MSB)
#define ARGNINT		0x1B	// Negative int (LSB, MSB)
#define ARGSTR		0x2B	// String (Note: 1st byte is length!!)

#define CLODBUSTER_CONFIG_BUFFER_SIZE 256

#define DEFAULT_CLODBUSTER_PORT "/dev/ttyUSB0" // This has to be USB - serial port.

typedef struct
{
  player_position_data_t position;
  
} __attribute__ ((packed)) player_clodbuster_data_t;

typedef struct
{
  player_position_cmd_t position;
} __attribute__ ((packed)) player_clodbuster_cmd_t;

typedef struct
{ 
  uint32_t time_count;
  int32_t left,right;
} __attribute__ ((packed)) clodbuster_encoder_data_t;

class PIDGains
{
 private:
     float kp, ki, kd, freq, k1, k2, k3;
     void findK()
	  {
	       float T=1.0/freq;
	       k1 = kp + .5*T*ki + kd/T;
	       k2 = -kp - 2.0*kd/T + .5*ki*T;
	       k3 = kd/T;
	       printf("Gain constants set to K1 = %f, K2 = %f, K3 = %f\n",k1,k2,k3);
	  };
 public:
     PIDGains(float kp_, float ki_, float kd_, float freq_)
	  :kp(kp_), ki(ki_), kd(kd_),freq(freq_)
	  {
	       findK();
	  };
     //~PIDGains();
     void SetKp(float k)
	  {
	       kp=k;
	       findK();
	  };
     void SetKi(float k)
	  {
	       ki=k;
	       findK();
	  };
     void SetKd(float k)
	  {
	       kd=k;
	       findK();
	  };
     void SetFreq(float f)
	  {
	       freq=f;
	       findK();
	  };
     float K1(){return(k1);};
     float K2(){return(k2);};
     float K3(){return(k3);};
};

class ClodBuster:public CDevice 
{
  private:
    static pthread_t thread;
  
    // since we have several child classes that must use the same lock, we 
    // declare our own static mutex here and override Lock() and Unlock() to 
    // use this mutex instead of the one declared in CDevice.
    static pthread_mutex_t clodbuster_accessMutex;

    // and this one protects calls to Setup and Shutdown
    static pthread_mutex_t clodbuster_setupMutex;

    // likewise, we need one clodbuster-wide subscription count to manage calls to
    // Setup() and Shutdown()
    static int clodbuster_subscriptions;

    static unsigned char* reqqueue;
    static unsigned char* repqueue;

    void ResetRawPositions();
    clodbuster_encoder_data_t ReadEncoders();

     static int clodbuster_fd;               // clodbuster device file descriptor
    
    // device used to communicate with GRASP IO Board
    static char clodbuster_serial_port[MAX_FILENAME_SIZE]; 
   
    static struct timeval timeBegan_tv;
    int kp,ki,kd;

    // did we initialize the common data segments yet?
    static bool initdone;
    clodbuster_encoder_data_t encoder_offset;
    clodbuster_encoder_data_t encoder_measurement;
    clodbuster_encoder_data_t old_encoder_measurement;
    float EncV, EncOmega, EncVleft, EncVright;

    static bool direct_command_control;
    unsigned char max_limits[SERVO_CHANNELS],center_limits[SERVO_CHANNELS],min_limits[SERVO_CHANNELS];
    void GetGraspBoardParams();

    // CB geometry parameters
    float WheelRadius;
    float WheelBase;
    float WheelSeparation;
    unsigned int CountsPerRev;
    float Kenc; // counts --> distance traveled

    // control parameters
    float LoopFreq;
    PIDGains *Kv, *Kw;

    void IntegrateEncoders();
    void DifferenceEncoders();

 protected:
    static player_clodbuster_data_t* data;
    static player_clodbuster_cmd_t* command;

    // Max motor speeds
    static int motor_max_speed;
    static int motor_max_turnspeed;
    
    // Bound the command velocities
    static bool use_vel_band; 
        
    void Lock();
    void Unlock();

    /* start a thread that will invoke Main() */
    virtual void StartThread();
    /* cancel (and wait for termination) of the thread */
    virtual void StopThread();

  public:

    ClodBuster(char* interface, ConfigFile* cf, int section);
    virtual ~ClodBuster();

    /* the main thread */
    virtual void Main();

    // we override these, because we will maintain our own subscription count
    virtual int Subscribe(void *client);
    virtual int Unsubscribe(void *client);

    virtual int Setup();
    virtual int Shutdown();

    virtual void PutData(unsigned char *, size_t maxsize,
                         uint32_t timestamp_sec, uint32_t timestamp_usec);
size_t GetData(void* client, unsigned char *dest, size_t maxsize,
                                 uint32_t* timestamp_sec, 
			   uint32_t* timestamp_usec);
 void PutCommand(void* client, unsigned char *src, size_t size );

    unsigned char SetServo(unsigned char chan, int value);
    void SetServo(unsigned char chan, unsigned char cmd);
    /*
    void CMUcamReset();
    void CMUcamTrack(int rmin=0, int rmax=0, int gmin=0,
                          int gmax=0, int bmin=0, int bmax=0);
    void CMUcamStopTracking();

    */ // don't want to have this right now.. but maybe eventually.
};


#endif
