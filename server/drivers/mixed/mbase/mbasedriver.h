// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:1; -*-

/**
  *   Copyright (C) 2010
  *   	Ana Teresa Hernández Malagón <anat.hernandezm@gmail.com>
  *  	Movirobotics <athernandez@movirobotics.com>
  *  Copyright (C) 2006
  *     Videre Design
  *  Copyright (C) 2000
  *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
  *
  *  Videre mbasedriver robot driver for Player
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
	*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
**/

/**
  *  This driver is adapted from the p2os driver of player 1.6.
**/

/**
	Movirobotic's mBase robot driver for Player 3.0.1 based on erratic driver developed by Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard. Developed by Ana Teresa Herández Malagón.
**/

#ifndef _mbasedriverDEVICE_H
#define _mbasedriverDEVICE_H

#ifndef mbasedriver_VERSION
#define mbasedriver_VERSION "2.0"
#endif

#ifndef mbasedriver_DATE
#define mbasedriver_DATE "2010-08-31"
#endif

#ifndef mbase_author 
#define mbase_author "Hernandez Malagon, Ana Teresa"
#endif


#include <pthread.h>
#include <sys/time.h>
#include <queue>

#include <libplayercore/playercore.h>
#include <replace/replace.h>

#include "packet.h"
#include "robot_params.h"

//#include <stdint.h>

#define GradToRad(x) (M_PI * ((double)(x)) / 180.0)
#define RadToGrad(x) ((short)((x) * 180.0) / M_PI)

// angular constants, angular units are 4096 / rev
#define ATOR(x) (M_PI * ((double)(x)) / 2048.0)
#define ATOD(x) (180.0 * ((double)(x)) / 2048.0)
#define RTOA(x) ((short)((x) * 2048.0) / M_PI)

// Default max speeds
#define MOTOR_DEF_MAX_SPEED 0.5
#define MOTOR_DEF_MAX_TURNSPEED DTOR(100)

// This merely sets a delay policy in the initial connection
#define ROBOT_CYCLETIME 20000

/* mbasedriver constants */
#define VIDERE_NOMINAL_VOLTAGE 13.0

//mm/s/s
#define MOTOR_MAX_TRANS_ACEL	1500
#define MOTOR_DEF_TRANS_ACEL	500	
//gados/s/s
#define MOTOR_DEF_MAX_ROT_ACEL	135	
#define PID_P	10
#define PID_I	10
#define PID_V	6000
#define DRIFFACTOR	0 
#define ROBOT_WHEEL_MR5		190
#define ROBOT_WHEEL_MR7		310
#define ROBOT_WIDTH_MR5		410
#define ROBOT_WIDTH_MR7		495

//constantes para transformas voltage infrarrojo
#define INFRAmult	5
#define INFRAdiv	4096

//constantes para calcular distancia sonar
//para incrementar la frecuencia a la que trabaja el micro 
#define PLL		8 
#define FREC_OSC_MICRO	3.6864	
#define FACTOR_ESCALA	256

#define tipo_MR7	7
#define tipo_MR5	5
#define DEBUG	0
#define IR_AN	0

// Commands for the robot
typedef enum command 
{
	open_controller		=	0x01,//1,
	close_controller	=       0x02,//2,
	enable_motors 	=	0x04,//4,
	reset_origo	=       0x07,//7,
	trans_vel 	=       0x0B,//11,	// mm/s
	rot_vel 	=       0x15,//21,	// deg/s
	set_sonar 	=       0x1C,//28,
	stop 		=       0x1D,//29,
	set_analog 	=       0x47,//71,
	set_pid_p	=	0x50,//	80,
	set_pid_i	=	0x51,//	81,
	set_pid_v	=	0x52,//	82,
	set_trans_acel	=	0x5A,//	90,
	set_rot_acel	=	0x5B,//	91,
	set_driffactor	=	0x5C,//	92,
	set_robot_width =	0x5D,//	93,
	set_robot_wheel = 	0x5E,//	94
	set_odometria	=	0x5F,//95
	set_sensores	=	0x60
} command_e;

// Argument types used in robot commands
typedef enum argtype {
	argint =  0x3B,
	argnint = 0x1B,
	argstr =  0x2B
} argtype_e;

// Types of replies from the robot
typedef enum reply {
	debug	=	0x15,
	config 	=  	0x20,
	stopped = 	0x32,
	moving 	=  	0x33,
	motor 	=   	0x80,
	encoder = 	0x90,
	ain 	=     	0x9a,
	sonar 	=  	0x9b,
	sensores	=	0x90
} reply_e;


#define DEFAULT_VIDERE_PORT "/dev/ttyS0"

typedef struct player_mbasedriver_data
{
  player_position2d_data_t position;
  player_power_data_t power;
  player_aio_data_t aio;
  player_ir_data ir;
  player_sonar_data sonar;
} __attribute__ ((packed)) player_mbasedriver_data_t;

// this is here because we need the above typedef's before including it.
#include "motorpacket.h"

extern bool debug_mbasedriver;
extern bool debug_send;
extern bool debug_receive_aio;
extern bool debug_receive_sonar;
extern bool debug_receive_motor;
extern bool debug_susbcribe;
extern bool debug_mbase_send_msj;

class mbasedriverMotorPacket;

class mbasedriver : public Driver
{
	private:
		int mcount;
		player_mbasedriver_data_t mbasedriver_data;

		player_devaddr_t position_id;
		player_devaddr_t power_id;
		player_devaddr_t aio_id;
		player_devaddr_t ir_id;
		player_devaddr_t sonar_id;

		int position_subscriptions;
		int aio_ir_subscriptions;
		int sonar_subscriptions;

		//mbasedriverMotorPacket* sippacket;
		mbasedriverMotorPacket *motor_packet;
		pthread_mutex_t motor_packet_mutex;

		int Connect();
		int Disconnect();
		
		void ResetRawPositions();
		void ToggleMotorPower(unsigned char val);

		void ToggleAIn(unsigned char val);
		void ToggleSonar(unsigned char val);

		int HandleConfig(QueuePointer &resp_queue, player_msghdr * hdr, void* data);
		int HandleCommand(player_msghdr * hdr, void * data);
		void HandlePositionCommand(player_position2d_cmd_vel_t position_cmd);

		void PublishAllData();
		void PublishPosition2D();
		void PublishPower();
		void PublishAIn();
		void PublishIR();
		void PublishSonar();

		void StartThreads();
		void StopThreads();

		void Send(mbasedriverPacket *packet);
		void SendThread();
		static void *SendThreadDummy(void *driver);
		void ReceiveThread();
		static void *ReceiveThreadDummy(void *driver);

		int read_fd, write_fd;
		const char* psos_serial_port;

		player_position2d_cmd_vel_t last_position_cmd;
		player_position2d_cmd_car_t last_car_cmd;

		std::queue<mbasedriverPacket *> send_queue;
		pthread_mutex_t send_queue_mutex;
		pthread_cond_t send_queue_cond;

		pthread_t send_thread;
		pthread_t receive_thread;

		// Parameters
  		bool print_all_packets;
		int param_idx;  // index in the RobotParams table for this robot

  		// Max motor speeds (mm/sec,deg/sec)
  		int motor_max_speed;
  		int motor_max_turnspeed;

		int getms();

		//mbase
		int motor_trans_acel;
		int motor_rot_acel;
		int trans_ant;
		int rot_ant;
		int16_t pid_p, pid_v, pid_i;
		int16_t driffactor;
		int16_t dist_ejes;
		int16_t diametro;
		bool debug_usuario;
		bool ir_analog;

	public:
		mbasedriver(ConfigFile* cf, int section);

		virtual int Subscribe(player_devaddr_t id);
		virtual int Unsubscribe(player_devaddr_t id);

		/* the main thread */
		virtual void Main();

		virtual int Setup();
		virtual int Shutdown();

		// MessageHandler
		virtual int ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data);
};


#endif
