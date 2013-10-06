// -*- mode:C++; tab-width:2; c-basic-offset:2; indent-tabs-mode:1; -*-

/*
 *  Copyright (C) 2010
 *     Ana Teresa Hernández Malagón <anat.hernandezm@gmail.com>
 *	Movirobotics <athernandez@movirobotics.com>
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
 */

/*
 *  This driver is adapted from the p2os driver of player 1.6.
 */

/*
	Movirobotic's mBase robot driver for Player 3.0.1 based on erratic driver developed by Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard. Developed by Ana Teresa Herández Malagón.
*/

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_mbasedriver mbasedriver
 * @brief mbase platform driver

This driver talks to the embedded computer in the mBase robot, which
mediates communication to the devices of the robot.

@par Compile-time dependencies

- none

@par Provides

The mbasedriver driver provides the following device interfaces, some of
them named:

- "odometry" @ref interface_position2d
  - This interface returns odometry data, and accepts velocity commands.

- @ref interface_power
  - Returns the current battery voltage (12 V when fully charged).

- @ref interface_aio
  - Returns data from analog and digital input pins

- @ref interface_ir
  - Returns ranges from IR sensors, assuming they're connected to the analog input pins

- @ref interface_sonar
  - Returns ranges from sonar sensors

@par Supported configuration requests

- "odometry" @ref interface_position2d :
  - PLAYER_POSITION_SET_ODOM_REQ
  - PLAYER_POSITION_MOTOR_POWER_REQ
  - PLAYER_POSITION_RESET_ODOM_REQ
  - PLAYER_POSITION_GET_GEOM_REQ
  - PLAYER_POSITION_VELOCITY_MODE_REQ
- @ref interface_ir :
  - PLAYER_IR_REQ_POSE
- @ref interface_sonar :
  - PLAYER_SONAR_GET_GEOM_REQ

@par Configuration file options

- port (string)
  - Default: "/dev/ttyS0"
- tipoMBase (integer)
  - Default: 5
  - Sets the type of mBase robot to use. It can be 5 for MR5 robot or 7 for MR7 robot.
- max_trans_vel (length)
  - Default: 1.5 m/s
  - Maximum translational velocity
- max_rot_vel (angle)
  - Default: 90 deg/s
  - Maximum rotational velocity
- trans_acel (length)
  - Default: 500 mm/s/s
  - Maximum: 1500 mm/s/s
  - Maximum translational acceleration, in length/sec/sec; nonnegative.
    Zero or negative means use the robot's default value.
- rot_acel (angle)
  - Default: 100 deg/s/s
  - Maximum rotational acceleration, in angle/sec/sec; nonnegative.
    Zero or negative means use the robot's default value. [this driver don't use that parameter]
- pid_p (integer)
  - Default: 10
  - PID setting; proportional gain.
    Negative means use the robot's default value.
- pid_i (integer)
  - Default: 10
  - PID setting; integral gain.
    Negative means use the robot's default value.
- pid_d (integer)
  - Default: 600
  - PID setting; derivative gain.
    Negative means use the robot's default value.
- driffactor (integer)
  - Default: 0
  - It straightens the robot's translation forward and backward.
- robotWidth (integer)
  - Default: 410 mm. for mBase and 495 for MR7.
  - Wheelbase.
    Negative means use the robot's default value.
- robotWheel (integer)
  - Default: 200 mm. for mBase and 310 for MR7.
  - Side wheel diameter.
    Negative means use the robot's default value.
- debug (integer)
  - Default: 0
  - A value different than zero shows debugging messages.
- ir_analog (integer)
  - Default: 0
  - Zero makes the IR parameter works with digital values using a 1.83 threshold. A different value of zero makes the IR parameter works with the values in volts.

@par Example

@verbatim
driver
(
  name "mbasedriver"

  provides [ "position2d:0"
             "power:0"
             "sonar:0"
             "aio:0"
             "ir:0" ]

  port "/dev/ttyS0"
)
@endverbatim

@author Joakim Arfvidsson, Brian Gerkey, Kasper Stoy, James McKenna, Ana Teresa Hernández Malagón
*/
/** @} */


// This must be first per pthread reference
#include <pthread.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <termios.h>
#include <stdlib.h> // for abs()
#include <sys/stat.h>

#include "mbasedriver.h"

#include "config.h"

bool debug_mbasedriver 		=	FALSE;
bool debug_send 		=	FALSE;
bool debug_receive_aio		=	FALSE;
bool debug_receive_sonar	=	FALSE;
bool debug_receive_motor	=	FALSE;
bool debug_subscribe		= 	FALSE;
bool debug_mbase_send_msj	=	FALSE;

/** Setting up, tearing down **/

// These get the driver in where it belongs, loading
Driver* mbasedriver_Init(ConfigFile* cf, int section)
{
	return (Driver*)(new mbasedriver(cf,section));
}

void mbasedriver_Register(DriverTable* table)
{
	table->AddDriver("mbasedriver", mbasedriver_Init);
}

#if 0
extern "C" 
{	
	int player_driver_init(DriverTable* table)
  	{
		mbasedriver_Register(table);
    		#define QUOTEME(x) #x
    		//#define DATE "##VIDERE_DATE##\"
    		printf("  mbasedriver robot driver %s, build date %s\n", mbasedriver_VERSION, mbasedriver_DATE);
    		return 0;
  	}
}
#endif

/**
	- Constructor of the driver from configuration entry
*/
mbasedriver::mbasedriver(ConfigFile* cf, int section)
  : Driver(cf,section,true,PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
	memset(&this->position_id, 0, sizeof(player_devaddr_t));	
	memset(&this->power_id, 0, sizeof(player_devaddr_t));
  	memset(&this->aio_id, 0, sizeof(player_devaddr_t));
  	memset(&this->sonar_id, 0, sizeof(player_devaddr_t));

  	this->position_subscriptions = 0;
  	this->aio_ir_subscriptions = 0;
  	this->sonar_subscriptions = 0;

  	// intialise members
  	motor_packet = NULL;
  	mcount = 0;
  	memset(&this->mbasedriver_data,0,sizeof(this->mbasedriver_data));

  	// Do we create a robot position interface?
 	if(cf->ReadDeviceAddr(&(this->position_id), section, "provides", PLAYER_POSITION2D_CODE, -1, NULL) == 0) 
	{
    		if(this->AddInterface(this->position_id) != 0) 
		{
      			this->SetError(-1);
      			return;
    		}
  	}
  	// Do we create a power interface?
  	if(cf->ReadDeviceAddr(&(this->power_id), section, "provides", PLAYER_POWER_CODE, -1, NULL) == 0) 
	{
    		if(this->AddInterface(this->power_id) != 0) 
		{
      			this->SetError(-1);
      			return;
    		}
  	}
  	// Do we create a aio interface?
  	if(cf->ReadDeviceAddr(&(this->aio_id), section, "provides", PLAYER_AIO_CODE, -1, NULL) == 0) 
	{
    		if(this->AddInterface(this->aio_id) != 0) 
		{
      			this->SetError(-1);
      			return;
    		}
  	}
  	// Do we create an ir interface?
  	if(cf->ReadDeviceAddr(&(this->ir_id), section, "provides", PLAYER_IR_CODE, -1, NULL) == 0) 
	{
    		if(this->AddInterface(this->ir_id) != 0)
		{
      			this->SetError(-1);
      			return;
    		}
  	}
  	// Do we create a sonar interface?
  	if(cf->ReadDeviceAddr(&(this->sonar_id), section, "provides", PLAYER_SONAR_CODE, -1, NULL) == 0) 
	{
    		if(this->AddInterface(this->sonar_id) != 0) 
		{
      			this->SetError(-1);
      			return;
    		}
  	}

  	// build the table of robot parameters.
  	initialize_robot_params();

	///Leyendo los datos del fichero cfg y asignando valores a los parámetros correspondientes.
	int debug = cf->ReadInt(section, "debug", DEBUG);
	if(debug==0)	debug_usuario = false;
	else		debug_usuario = true;
	if(debug_usuario)	printf("Cargando parámetros de configuración...\n");
  	//port
  	this->psos_serial_port = cf->ReadString(section,"port",DEFAULT_VIDERE_PORT);
	//tipoMBase (timpoMBase asigna valores por defecto a detemrinados parámetros según la plataforma indicada por el usuario.
	//- robotWheel
	//- robotWidth
	int tipoMBase = cf->ReadInt(section, "tipoMBase", tipo_MR5);
	if(tipoMBase==tipo_MR7)
	{
		this->diametro = cf->ReadInt(section, "robotWheel", ROBOT_WHEEL_MR7);
		this->dist_ejes = cf->ReadInt(section, "robotWidth", ROBOT_WIDTH_MR7);
	}
	else if(tipoMBase==tipo_MR5)
	{
		this->diametro = cf->ReadInt(section, "robotWheel", ROBOT_WHEEL_MR5);
		this->dist_ejes = cf->ReadInt(section, "robotWidth", ROBOT_WIDTH_MR5);
	}
	else
	{
		this->diametro = cf->ReadInt(section, "robotWheel", ROBOT_WHEEL_MR5);
		this->dist_ejes = cf->ReadInt(section, "robotWidth", ROBOT_WIDTH_MR5);
	}
	//max_trans_vel
  	this->motor_max_speed = (int)rint(1e3 * cf->ReadLength(section, "max_trans_vel", MOTOR_DEF_MAX_SPEED));
	//max_rot_vel
  	this->motor_max_turnspeed = (int)rint(RTOD(cf->ReadAngle(section, "max_rot_vel", MOTOR_DEF_MAX_TURNSPEED)));
  
	//trans_acel
	this->motor_trans_acel = (int)rint(cf->ReadLength(section, "trans_acel", MOTOR_DEF_TRANS_ACEL));
	//rot_Acel
	this->motor_rot_acel = (int)rint(RTOD(cf->ReadAngle(section, "rot_acel", MOTOR_DEF_MAX_ROT_ACEL)));
	if(this->motor_trans_acel > MOTOR_MAX_TRANS_ACEL)	
		this->motor_trans_acel = MOTOR_MAX_TRANS_ACEL;
	else if (this->motor_trans_acel<0)	
		this->motor_trans_acel = MOTOR_DEF_TRANS_ACEL;
	if(this->motor_rot_acel > MOTOR_DEF_MAX_ROT_ACEL || this->motor_rot_acel<0) 
		this->motor_rot_acel = MOTOR_DEF_MAX_ROT_ACEL;
		
	//pid_p, pid_i, pid_d
	this->pid_p = cf->ReadInt(section, "pid_p", PID_P);
	this->pid_v = cf->ReadInt(section, "pid_d", PID_V);
	this->pid_i = cf->ReadInt(section, "pid_i", PID_I);

	//driffactor
	this->driffactor = cf->ReadInt(section, "driffactor", DRIFFACTOR);

	//ir_analog
	int IR_an = cf->ReadInt(section, "ir_analog", IR_AN);
	if(IR_an == 0)	this->ir_analog = false;
	else	this->ir_analog = true;

	//mostrar por pantalla parámetros de configuración a enviar a la plataforma
	if(debug_usuario)
	{	
		printf("\tport:	%s\n", this->psos_serial_port);
		if(tipoMBase==tipo_MR5)	printf("\ttipoMBase:	MR5\n");
		else if(tipoMBase==tipo_MR7)	printf("\ttipoMBase:	MR7\n");
		else	printf("\tValor tipoMBase incorrecto, se trabajará como si fuera un MR5\n");
		printf("\tmax_trans_vel:	%d\n", this->motor_max_speed);
		printf("\tmax_rot_vel:	%d\n", this->motor_max_turnspeed);
		printf("\ttrans_acel:	%d\n", this->motor_trans_acel);
		printf("\ttrans_rot:	%d\n", this->motor_rot_acel);
		printf("\tpid_p:	%d\n", this->pid_p);	
		printf("\tpid_i:	%d\n", this->pid_i);
		printf("\tpid_d:	%d\n", this->pid_v);
		printf("\tdriffactor:	%d\n", this->driffactor);
		printf("\trobotWidth:	%d\n", this->dist_ejes);
		printf("\tFrobotWheel:	%d\n", this->diametro);
	}

	//inicialización de variables globales
  	this->print_all_packets = 0;
  	debug_mbasedriver = 0;//cf->ReadInt(section, "debug", 0);
  	this->read_fd = -1;
  	this->write_fd = -1;

	// Data mutexes and semaphores
	pthread_mutex_init(&send_queue_mutex, 0);
	pthread_cond_init(&send_queue_cond, 0);
	pthread_mutex_init(&motor_packet_mutex, 0);

	if (Connect())
	{
		printf("Error connecting to mbasedriver robot\n");
		exit(1);
	}
}

/**
	Setup
	- Called by player when the driver is asked to connect
*/
int mbasedriver::Setup() 
{
	// We don't care, we connect at startup anyway
	return 0;
}

/**
	Connect
	- Establishes connection and starts threads
*/
int mbasedriver::Connect()
{
	unsigned short valor_abs;
 	printf("  mbasedriver connection initializing (%s)...",this->psos_serial_port); fflush(stdout);
	
  	//Try opening both our file descriptors
	if ((this->read_fd = open(this->psos_serial_port, O_RDONLY, S_IRUSR | S_IWUSR)) < 0) 
	{
    		perror("mbasedriver::Setup():open():");
    		return(1);
  	};
  	if ((this->write_fd = open(this->psos_serial_port, O_WRONLY, S_IRUSR | S_IWUSR)) < 0) 
	{
    		perror("mbasedriver::Setup():open():");
    		close(this->read_fd); this->read_fd = -1;
    		return(1);
  	}

  	// Populate term and set initial baud rate
  	int bauds[] = {B115200, B115200};
  	int numbauds = sizeof(bauds), currbaud = 0;
  	struct termios read_term, write_term;
  	{
    		if(tcgetattr( this->read_fd, &read_term ) < 0 ) 
		{
      			perror("mbasedriver::Setup():tcgetattr():");
      			close(this->read_fd); close(this->write_fd);
      			this->read_fd = -1; this->write_fd = -1;
      			return(1);
    		}
		cfmakeraw(&read_term);
    		//cfsetspeed(&read_term, bauds[currbaud]);
    		cfsetispeed(&read_term, bauds[currbaud]);
    		cfsetospeed(&read_term, bauds[currbaud]);
    		if(tcsetattr(this->read_fd, TCSAFLUSH, &read_term ) < 0) 
		{
      			perror("mbasedriver::Setup():tcsetattr(read channel):");
      			close(this->read_fd); close(this->write_fd);
      			this->read_fd = -1; this->write_fd = -1;
      			return(1);
    		}
  	}

  	// Empty buffers
  	if (tcflush(this->read_fd, TCIOFLUSH ) < 0) 
	{
    		perror("mbasedriver::Setup():tcflush():");
    		close(this->read_fd); close(this->write_fd);
    		this->read_fd = -1; this->write_fd = -1;
    		return(1);
  	}
  	if (tcflush(this->write_fd, TCIOFLUSH ) < 0) 
	{
   	 	perror("mbasedriver::Setup():tcflush():");
    		close(this->read_fd); close(this->write_fd);
    		this->read_fd = -1; this->write_fd = -1;
    		return(1);
  	}
  	// will send config packets until a response is gotten
  	int num_sync_attempts = 10;
  	int num_patience = 200;
  	int failed = 0;
 	enum { NO_SYNC, AFTER_FIRST_SYNC, AFTER_SECOND_SYNC, READY } communication_state = NO_SYNC;
  	mbasedriverPacket received_packet;
  	while(communication_state != READY && num_patience-- > 0) 
	{
    		// Receive a reply to see if we got it yet
    		uint8_t receive_error;
    		if((receive_error = received_packet.Receive(this->read_fd))) 
		{
      			if(receive_error == (receive_result_e)failure)	printf("Error receiving\n");
      			// If we still have retries, just get another packet
      			if((communication_state == NO_SYNC) && (num_sync_attempts >= 0)) 
			{
        			num_sync_attempts--;
        			usleep(ROBOT_CYCLETIME);
        			continue;
      			}
      			// Otherwise, try next speed or give up completely
      			else 
			{
      				// Couldn't connect; try different speed.
        			if(++currbaud < numbauds)
				{
        				// Set speeds for the descriptors
          				cfsetispeed(&read_term, bauds[currbaud]);
          				cfsetospeed(&write_term, bauds[currbaud]);
          				if( tcsetattr(this->read_fd, TCSAFLUSH, &read_term ) < 0 ) 
					{
            					perror("mbasedriver::Setup():tcsetattr():");
            					close(this->read_fd); close(this->write_fd);
            					this->read_fd = -1; this->write_fd = -1;
            					return(1);
          				}
          				if( tcsetattr(this->write_fd, TCSAFLUSH, &write_term ) < 0 ) 
					{
            					perror("mbasedriver::Setup():tcsetattr():");
            					close(this->read_fd); close(this->write_fd);
						this->read_fd = -1; this->write_fd = -1;
						return(1);
          				}
					// Flush the descriptors
					if(tcflush(this->read_fd, TCIOFLUSH ) < 0 ) 
					{
						perror("mbasedriver::Setup():tcflush():");
						close(this->read_fd); close(this->write_fd);
						this->read_fd = -1; this->write_fd = -1;
						return(1);
					}
					if(tcflush(this->write_fd, TCIOFLUSH ) < 0 ) 
					{
						perror("mbasedriver::Setup():tcflush():");
						close(this->read_fd); close(this->write_fd);
						this->read_fd = -1; this->write_fd = -1;
						return(1);
					}
					// Give same slack to new speed
					num_sync_attempts = 10;
					continue;
        			}
				// Tried all speeds, abort
				else failed = 1;
			}	
    		}
		// If we gave up
		if (failed) break;
		// If we successfully got a packet, check if it's the one we're waiting for
		if (received_packet.packet[3] == (reply_e)motor)// 0x80)//0x20) 
		{	
			printf("COMUNICACION ESTABLECIDA\n");
			communication_state = READY;
		}
		usleep(ROBOT_CYCLETIME);
	}

	if(failed) 
	{
    		printf("  Couldn't synchronize with mbasedriver robot.\n");
    		printf("  Most likely because the robot is not connected to %s\n", this->psos_serial_port);
    		close(this->read_fd); close(this->write_fd);
    		this->read_fd = -1; this->write_fd = -1;
    		return(1);
  	}
  	else if (communication_state != READY) 
	{
    		printf("  Couldn't synchronize with mbasedriver robot.\n");
    		printf("  We heard something though. Is the sending part of the cable dead?\n");
    		close(this->read_fd); close(this->write_fd);
    		this->read_fd = -1; this->write_fd = -1;
    		return(1);
  	}

  	char name[20], type[20], subtype[20];
	snprintf(name, 20, "%s", &received_packet.packet[5]);
	snprintf(type, 20, "%s", &received_packet.packet[25]);
	snprintf(subtype, 20, "%s", &received_packet.packet[45]);

	///Envío mensajes de configuración a la placa de control IOM
	///- OPEN_CONTROLLER
	///- STOP
	if(debug_usuario)	
		printf("Inicio del envio de mensajes de configuración al robot...\n");
	{	
    		mbasedriverPacket packet;
		if(debug_send)	printf(".... OPEN_CONTROLLER ...\n");
		unsigned char commandOpen = (command_e)open_controller;
		packet.Build(&commandOpen, 1);
		packet.Send(this->write_fd);
		//usleep(ROBOT_CYCLETIME);
		if(debug_send)	printf("... STOP ...\n");
		unsigned char commandStop = (command_e)stop;
		packet.Build(&commandStop, 1);
		packet.Send(this->write_fd);
		usleep(ROBOT_CYCLETIME);
	}
	
	param_idx = 0; //MBASE
	robotParams[this->param_idx]->RobotWidth = this->dist_ejes;
	// Create a packet and set initial odometry position (the mbasedriverMotorPacket is persistent)
	this->motor_packet = new mbasedriverMotorPacket(param_idx);
	//No se utilizará offset ya que la plataforma trabajará con la posición de odometría que devuelva la placa de control IOM.

	///- SET_TRANS_ACEL, si no se envía al menos una vez la plataforma no se moverá
	///- SET_ROT_ACEL -> No se envia porque el IOM no lo reconoce 
  	{
		mbasedriverPacket *accel_packet;
		unsigned char accel_command[4];
		if(this->motor_trans_acel > 0) 
		{
			accel_packet = new mbasedriverPacket();
			accel_command[0] = (command_e)set_trans_acel;
			accel_command[1] = (argtype_e)argint;
			accel_command[2] = this->motor_trans_acel & 0x00FF;
			accel_command[3] = (this->motor_trans_acel & 0xFF00) >> 8;
			if(debug_send)	printf("... SET_TRANS_ACEL ...\n");
 	     		accel_packet->Build(accel_command, 4);
      			this->Send(accel_packet);
    		}
    	/**	if(this->motor_rot_acel > 0) 
		{
      			accel_packet = new mbasedriverPacket();
      			accel_command[0] = (command_e)set_rot_acel;
      			accel_command[1] = (argtype_e)argint;
      			accel_command[2] = this->motor_rot_acel & 0x00FF;
      			accel_command[3] = (this->motor_rot_acel & 0xFF00) >> 8;
			if(debug_send)	printf("__ build SET_ROT_ACEL\n");
      			accel_packet->Build(accel_command, 4);
      			this->Send(accel_packet);
    		}
	*/
  	}
	///- SET_DRIFFACTOR
	{
		mbasedriverPacket *config_packet;
		unsigned char config_command[4];
		config_packet = new mbasedriverPacket();
		config_command[0] = (command_e)set_driffactor;
		if(this->driffactor >= 0)	config_command[1] = (argtype_e)argint;
    		else				config_command[1] = (argtype_e)argnint;
		valor_abs = (unsigned short)abs(this->driffactor);
      		config_command[2] = valor_abs & 0x00FF;
		config_command[3] = (valor_abs & 0xFF00) >> 8;
		if(debug_send)	printf("... SET_DRIFFACTOR ...\n");
      		config_packet->Build(config_command, 4);
      		this->Send(config_packet);
  	}
	///- SET_ROBOT_WIDTH
  	{	
		mbasedriverPacket *ejes_packet;
    		unsigned char ejes_command[4];
		ejes_packet = new mbasedriverPacket();
		if(this->dist_ejes >= 0)
		{
			ejes_command[0] = (command_e)set_robot_width;
			ejes_command[1] = (argtype_e)argint;
			ejes_command[2] = (this->dist_ejes) & 0x00FF;
			ejes_command[3] = ((this->dist_ejes) & 0xFF00) >> 8;
			if(debug_send)	printf("... SET_ROBOT_WIDTH ...\n");
			ejes_packet->Build(ejes_command, 4);
			this->Send(ejes_packet);	
		}
 	}
	///- SET_ROBOT_WHEEL
  	{
		mbasedriverPacket *diam_packet;
		unsigned char diam_command[4];
		diam_packet = new mbasedriverPacket();
		if(this->diametro >= 0)
		{
			diam_command[0] = (command_e)set_robot_wheel;
			diam_command[1] = (argtype_e)argint;
			diam_command[2] = (this->diametro) & 0x00FF;
			diam_command[3] = ((this->diametro) & 0xFF00) >> 8;
			if(debug_send)	printf("... SET_ROBOT_WHEEL ...\n");
			diam_packet->Build(diam_command, 4);
			this->Send(diam_packet);	
		}
  	}
  	///- SET_PID_P
	///- SET_PID_I
	///- SET_PID_D
  	{
		mbasedriverPacket *packet;
		unsigned char payload[4];	
		if(this->pid_p >= 0) 
		{
			packet = new mbasedriverPacket();
			payload[0] = (command_e)set_pid_p;
			payload[1] = (argtype_e)argint;
			payload[2] = this->pid_p & 0x00FF;
			payload[3] = (this->pid_p & 0xFF00) >> 8;
			if(debug_send)	printf("... SET_PID_P ...\n");
			packet->Build(payload, 4);
			this->Send(packet);
    		}
		if(this->pid_v >= 0) 
		{
			packet = new mbasedriverPacket();
			payload[0] = (command_e)set_pid_v;
			payload[1] = (argtype_e)argint;
			payload[2] = this->pid_v & 0x00FF;
			payload[3] = (this->pid_v & 0xFF00) >> 8;
			if(debug_send)	printf("... SET_PID_D ...\n");
			packet->Build(payload, 4);
			this->Send(packet);
		}
		if(this->pid_i >= 0) 
		{
			packet = new mbasedriverPacket();
			payload[0] = (command_e)set_pid_i;
			payload[1] = (argtype_e)argint;
			payload[2] = this->pid_i & 0x00FF;
			payload[3] = (this->pid_i & 0xFF00) >> 8;
			if(debug_send)
				printf("... SET_PID_I ...\n");
			packet->Build(payload, 4);
			this->Send(packet);
    		}
	}
	if(debug_usuario)	
		printf("...Fin del envio de mensajes de configuración al robot.\n");
  	/* now spawn reading thread */
  	((mbasedriver*)this)->StartThreads();
  	return(0);
}


/**
	Shutdown
	- Called by player when the driver is supposed to disconnect
*/
int mbasedriver::Shutdown() 
{
	// kill with destructor instead of here
  	return 0;
}


/**
	Disconnect
	- Is theoretically able to disconnect, but we don't do that 
*/
int mbasedriver::Disconnect() 
{
	printf("Shutting mbasedriver driver down\n");
  	this->StopThreads();
  	// If we're connected, send some kill commands before killing connections
  	if (this->write_fd > -1) 
	{
    		unsigned char command[20],buffer[20];
    		mbasedriverPacket packet;
   		memset(buffer,0,20);

    		command[0] = (command_e)stop;
    		packet.Build(command, 1);
    		packet.Send(this->write_fd);
    		usleep(ROBOT_CYCLETIME);

		command[0] = (command_e)close_controller;
		packet.Build(command, 1);
		packet.Send(this->write_fd);
		usleep(ROBOT_CYCLETIME);

		close(this->write_fd);
		this->write_fd = -1;
  	}
  	if (this->read_fd > -1) 
	{
		close(this->read_fd);
		this->read_fd = -1;
  	}
  	if (this->motor_packet) 
	{
    		delete this->motor_packet;
    		this->motor_packet = NULL;
  	}
  	printf("mbasedriver has been shut down");
  	return(0);
}


/**
	StrarThreads
	- These call the supplied Driver::StartThread() method, but adds additional threads
*/
void mbasedriver::StartThreads() 
{
	pthread_create(&send_thread, 0, &SendThreadDummy, this);
  	pthread_create(&receive_thread, 0, &ReceiveThreadDummy, this);
}

/**
	StopThreads
*/
void mbasedriver::StopThreads() 
{
	pthread_cancel(send_thread);
  	pthread_cancel(receive_thread);

	// Cleanup mutexes
	pthread_mutex_trylock(&send_queue_mutex);
	pthread_mutex_unlock(&send_queue_mutex);
	
	pthread_mutex_trylock(&motor_packet_mutex);
	pthread_mutex_unlock(&motor_packet_mutex);
}

/**
	Subscribe
	- Subscription is overridden to add a subscription count of our own
*/
int mbasedriver::Subscribe(player_devaddr_t id) 
{
	if(debug_subscribe)	printf("[mbasedriver]SUBSCRIBE: ");
 	int setupResult;

 	if((setupResult = Driver::Subscribe(id)) == 0)
  	{
    		if(Device::MatchDeviceAddress(id, this->position_id))
		{
			this->position_subscriptions++;
			if(debug_subscribe)	printf(" motor\n");
			this->ToggleMotorPower(1);
		}
		if(Device::MatchDeviceAddress(id, this->aio_id))
		{
			this->aio_ir_subscriptions++;
			if(debug_subscribe)	printf(" aio\n");
			this->ToggleAIn(1);
		}
		if(Device::MatchDeviceAddress(id, this->ir_id))
		{
			this->aio_ir_subscriptions++;
			if(debug_subscribe)	printf(" ir\n");
			this->ToggleAIn(1);
			//quitar?
		}
		if(Device::MatchDeviceAddress(id, this->sonar_id))
		{
			this->sonar_subscriptions++;
			if(debug_subscribe)	printf(" sonar\n");
			this->ToggleSonar(1);
		}
	}
	return(setupResult);
}


/**
	Unsubscribe
*/
int mbasedriver::Unsubscribe(player_devaddr_t id) 
{
	int shutdownResult;
	if((shutdownResult = Driver::Unsubscribe(id)) == 0)
	{
		// also decrement the appropriate subscription counter
		if(Device::MatchDeviceAddress(id, this->position_id))
		{
			this->position_subscriptions--;
			assert(this->position_subscriptions >= 0);
			this->ToggleMotorPower(0);
		//	this->ResetRawPositions();
		}
		if(Device::MatchDeviceAddress(id, this->aio_id))
		{
			this->aio_ir_subscriptions--;
			this->ToggleAIn(0);
		}
    		if(Device::MatchDeviceAddress(id, this->ir_id))
		{  
			this->aio_ir_subscriptions--;
			this->ToggleAIn(0);
		}
    		if(Device::MatchDeviceAddress(id, this->sonar_id))
		{
			this->sonar_subscriptions--;
			this->ToggleSonar(0);
		}
  	}
  	return(shutdownResult);
}


/**
	ReceiveThread
	- Listens to the robot
*/
void mbasedriver::ReceiveThread()
{
	int aux_ain=0;
	int valor_IOM = 0;
	double calc = 0.0;
  	for (;;) 
	{
    		pthread_testcancel();
	
		// Receive next packet from robot
		mbasedriverPacket packet;
		uint32_t waited = 0;
		uint8_t error_code;
		while ((error_code = packet.Receive(this->read_fd, 5000))) 
		{
			waited += 5;
			printf("Lost serial communication with mbasedriver (%d) - no data received for %i seconds\n", error_code, waited);
    		}

    		if (waited)	printf("Connection re-established\n");

    		if (print_all_packets) 
		{
      			printf("Got: ");
      			packet.PrintHex();
    		}

    		// Process the packet
    		int count, maxcount;
    		switch(packet.packet[3])
      		{
			case (reply_e)motor:
			case (reply_e)motor+2:
			case (reply_e)motor+3:
			if(debug_receive_motor)
			{
				switch( packet.packet[3])
				{
					case (reply_e)motor:
						printf("[motor]	");	
						break;
					case (reply_e)motor+2:
						printf("[motor2] ");
						break;
					case (reply_e)motor+3:
						printf("[motor3] ");
						break;
				}
				packet.PrintHex();
			}
	
			if (motor_packet->Parse(&packet.packet[3], packet.size-3)) 
			{
				motor_packet->Fill(&mbasedriver_data);
				PublishPosition2D();
				PublishPower();
			}
			break;

    		/*	Mensaje no implementado en la placa de control
			case (reply_e)config:
				break;
		*/
	
      			case (reply_e)ain:
				aux_ain = 0;
				valor_IOM=0;
				calc=0;
				if(debug_receive_aio)
				{
					printf("[ain] ");
					packet.PrintHex();
				}
				// This data goes in two places, analog input and ir rangers
				mbasedriver_data.aio.voltages_count = robotParams[this->param_idx]->NumINFRAAN;
				//RobotParams[this->param_idx]->NumINFRADIG;//packet.packet[4];
				mbasedriver_data.ir.voltages_count = robotParams[this->param_idx]->NumINFRAAN;
				mbasedriver_data.ir.ranges_count = robotParams[this->param_idx]->NumIR;
				unsigned int i_voltage;
				//dVol = (iTotal * INFRAmult)/INFRAdiv;
				aux_ain = 0;
				valor_IOM = 0;
				if(mbasedriver_data.aio.voltages)	
					delete[] mbasedriver_data.aio.voltages;
				mbasedriver_data.aio.voltages = new float [robotParams[this->param_idx]->NumINFRAAN + robotParams[this->param_idx]->NumINFRADIG];
				if(mbasedriver_data.ir.voltages)
					delete[] mbasedriver_data.ir.voltages;
				mbasedriver_data.ir.voltages = new float[robotParams[this->param_idx]->NumINFRAAN + robotParams[this->param_idx]->NumINFRADIG];
				mbasedriver_data.sonar.ranges_count = maxcount;
				if(mbasedriver_data.ir.ranges)	delete[] mbasedriver_data.ir.ranges;
				mbasedriver_data.ir.ranges = new float[robotParams[this->param_idx]->NumINFRAAN + robotParams[this->param_idx]->NumINFRADIG];
				for (i_voltage = 0; i_voltage < mbasedriver_data.aio.voltages_count ;i_voltage++) 
				{
					//  if (i_voltage >= PLAYER_AIO_MAX_INPUTS)	continue;
					valor_IOM =  (packet.packet[4+aux_ain+i_voltage]+ 256*packet.packet[5+aux_ain+i_voltage]);
					calc = (double)(valor_IOM) * INFRAmult / INFRAdiv;
					mbasedriver_data.aio.voltages[i_voltage] =  calc;
					if(this->ir_analog)
					{	
						mbasedriver_data.ir.voltages[i_voltage] = mbasedriver_data.aio.voltages[i_voltage];
					}
					else
					{
						if(valor_IOM > 1500)	mbasedriver_data.ir.voltages[i_voltage] = 1;
						else	mbasedriver_data.ir.voltages[i_voltage] = 0;
					}
					mbasedriver_data.ir.ranges[i_voltage] = mbasedriver_data.ir.voltages[i_voltage];
					aux_ain++;
				}
				// add digital inputs, four E port bits
				mbasedriver_data.aio.voltages_count += robotParams[this->param_idx]->NumINFRADIG;
				mbasedriver_data.ir.voltages_count += robotParams[this->param_idx]->NumINFRADIG;
				for (int i=0; i < robotParams[this->param_idx]->NumINFRADIG; i++) 
				{
					//if (i_voltage >= PLAYER_AIO_MAX_INPUTS)	continue;
					valor_IOM = packet.packet[4+aux_ain+i_voltage] ;
					if(valor_IOM==0)	
						mbasedriver_data.aio.voltages[i_voltage+i] = 0;
					else	
						mbasedriver_data.aio.voltages[i_voltage+i] = 1.0;
					mbasedriver_data.ir.voltages[i_voltage+i] = mbasedriver_data.aio.voltages[i_voltage+i];
					//(packet.packet[4+i_voltage*2] & (0x1 << i+4)) ? 1.0 : 0.0;
					mbasedriver_data.ir.ranges[i_voltage+i] = mbasedriver_data.aio.voltages[i_voltage+i];
					//if(debug_mbase_receive_ain)	printf("Dig:	V.Rec.= %02x	%d	V.Cal.= %.2f\n",packet.packet[5+aux_ain+i_voltage], valor_IOM, mbasedriver_data.aio.voltages[i_voltage+i]);
					aux_ain++;
				}
				PublishAIn();
				PublishIR();	
				break;

      			case (reply_e)sonar:
				if(debug_receive_sonar)
				{
					printf("[sonar] ");
					packet.PrintHex();
				}
				maxcount = robotParams[this->param_idx]->NumSonars;
				count = packet.packet[4];
				if(mbasedriver_data.sonar.ranges_count != (unsigned int)maxcount)
				{
         				mbasedriver_data.sonar.ranges_count = maxcount;
          				if(mbasedriver_data.sonar.ranges)
            					delete[] mbasedriver_data.sonar.ranges;
          				mbasedriver_data.sonar.ranges = new float[mbasedriver_data.sonar.ranges_count];
        			}
				for (int i = 0; i < count; i++)
	  			{
	    				int ch = packet.packet[5+i*3]; 
	    				if (ch >= maxcount)	continue;
	    				mbasedriver_data.sonar.ranges[ch] = .001 * (double)(packet.packet[6+i*3] + 256*packet.packet[7+i*3]);
					//printf("Sonar %d: %.2f\n", ch, mbasedriver_data.sonar.ranges[ch]);
	  			}
				PublishSonar();
				break;

		/*	Mensaje no implementado en la placa de control
      			case (reply_e)debug:
				if (debug_mbasedriver) 
				{
	  				printf("Debug message: ");
	  				for (uint8_t i = 3; i < packet.size-2 ;i++)
	    					printf("%c", packet.packet[i]);
	  				printf("\n");
				}
				break;
		*/

      			default:
				if (debug_mbasedriver) 
				{
					printf("Unrecognized packet: ");
					packet.Print();
				}
      		}
  	}
}


/**
	ReceiveThreadDummy
*/
void* mbasedriver::ReceiveThreadDummy(void *driver) 
{
	((mbasedriver*)driver)->ReceiveThread();
  	return 0;
}

/**
	SendThread
*/
void mbasedriver::SendThread() 
{
	for (;;) 
	{
    		pthread_testcancel();
    		// Get rights to the queue
    		pthread_mutex_lock(&send_queue_mutex);

    		// If there is nothing, we wait for signal
    		if (!send_queue.size())
      			pthread_cond_wait(&send_queue_cond, &send_queue_mutex);

    		// Get first element and give the queue back
		mbasedriverPacket *packet = 0;
		if (send_queue.size()) 
		{
			packet = send_queue.front();
			send_queue.pop();
    		}
    		pthread_mutex_unlock(&send_queue_mutex);

    		// Send the packet and destroy it
    		if (packet) 
		{
      			if (print_all_packets) 
			{
				printf("Just about to send: ");
				packet->Print();
      			}
      			packet->Send(this->write_fd);
			// To not overload buffers on the robot, hold off a little bit
      			usleep(15000);
    		}
    		delete(packet);
  	}
}

/**
	SedThreadDummy
*/
void* mbasedriver::SendThreadDummy(void *driver) 
{
	((mbasedriver*)driver)->SendThread();
  	return 0;
}


/**
	Send
	- Queues for sending
*/
void mbasedriver::Send(mbasedriverPacket *packet) 
{
  pthread_mutex_lock(&send_queue_mutex); {
    send_queue.push(packet);
    pthread_cond_signal(&send_queue_cond);
  } pthread_mutex_unlock(&send_queue_mutex);
}

/**
	ResetRawPositions
	- Tells the robot to reset the odometry center
*/
void mbasedriver::ResetRawPositions() 
{
	mbasedriverPacket *pkt;
  	unsigned char mbasedrivercommand[4];
	if(debug_usuario)	printf("Reset raw odometry\n");

  	if(this->motor_packet) 
	{
    		pkt = new mbasedriverPacket();
		this->motor_packet->xpos = 0;
		this->motor_packet->ypos = 0;
    		mbasedrivercommand[0] = (command_e)reset_origo;
    		mbasedrivercommand[1] = (argtype_e)argint;
		//quitar?
    		pkt->Build(mbasedrivercommand, 2);
    		this->Send(pkt);
  	}
}


/**
	ToggleMotorPower
	- Enable or disable motors
*/
void mbasedriver::ToggleMotorPower(unsigned char val) 
{
	if(debug_usuario)	printf("TOGGLEMOTORPOWER %d\n", val);
	unsigned char command[4];
	mbasedriverPacket *packet = new mbasedriverPacket();
	command[0] = (command_e)enable_motors;
	command[1] = (argtype_e)argint;
	//command[2] = val;
	command[2] = val ? 1 : 0;
	command[3] = 0;
	if(debug_usuario)	
	{
		if(command[2]==1)	printf("ENABLE MOTORS\n");
		else			printf("DISABLE MOTORS\n");
	}
	packet->Build(command, 4);
	Send(packet);
}


/**
	ToggleAIn
	- Enable or disable analog input reporting
*/
void mbasedriver::ToggleAIn(unsigned char val)
{
	if(debug_usuario)	printf("TOGGLEAIN\n");
	unsigned char command[4];
	mbasedriverPacket *packet = new mbasedriverPacket();
	command[0] = (command_e)set_analog;
	command[1] = (argtype_e)argint;
	command[2] = val ? 1 : 0;
	command[3] = 0;
	if(debug_usuario)	
	{
		if(command[2]==1)	printf("ENABLE ANALOG\n");
		else			printf("DISABLE ANALOG\n");
	}
	packet->Build(command, 4);
	Send(packet);
}

/**
	ToggleSonar
	- Enable or disable sonar reporting
*/
void mbasedriver::ToggleSonar(unsigned char val)
{
	if(debug_usuario)	printf("TOGGLESONAR\n");
	unsigned char command[4];
	mbasedriverPacket *packet = new mbasedriverPacket();
	command[0] = (command_e)set_sonar;
	command[1] = (argtype_e)argint;
	command[2] = val ? 1 : 0;
	command[3] = 0;
	if(debug_usuario)	
	{
		if(command[2]==1)	printf("ENABLE SONAR\n");
		else			printf("DISABLE SONAR\n");
	}
	packet->Build(command, 4);
	Send(packet);
}


/**
	Main
	- Main entry point for the worker thread
*/
void mbasedriver::Main() 
{
	int last_position_subscrcount=0;
	int last_aio_ir_subscriptions=0;
	int last_sonar_subscriptions=0;

  	for(;;)
  	{
    		pthread_testcancel();
    		// Wait for some instructions
    		//    Wait();
    		usleep(10000);	// Wait() blocks too much, doesn't get subscriptions

		// we want to reset the odometry and enable the motors if the first
		// client just subscribed to the position device, and we want to stop
		// and disable the motors if the last client unsubscribed.
		/**    if(!last_position_subscrcount && this->position_subscriptions)
		{
			this->ToggleMotorPower(0);
			this->ResetRawPositions();
		}
		else if(last_position_subscrcount && !(this->position_subscriptions))
		{
			// enable motor power
			this->ToggleMotorPower(1);
		}
		*/
    		last_position_subscrcount = this->position_subscriptions;

		// We'll ask the robot to enable analog packets if we just got our
		// first subscriber
		/**    if(!last_aio_ir_subscriptions && this->aio_ir_subscriptions)
			this->ToggleAIn(1);
		else if(last_aio_ir_subscriptions && !(this->aio_ir_subscriptions))
			this->ToggleAIn(0);
		*/
    		last_aio_ir_subscriptions = this->aio_ir_subscriptions;

		// We'll ask the robot to enable sonar packets if we just got our
		// first subscriber
		/**    if(!last_sonar_subscriptions && this->sonar_subscriptions)
			this->ToggleSonar(1);
		else if(last_sonar_subscriptions && !(this->sonar_subscriptions))
			this->ToggleSonar(0);
		*/
    		last_sonar_subscriptions = this->sonar_subscriptions;

		// handle pending messages
		//printf( "Will look for incoming messages\n" );
		if(!this->InQueue->Empty())
		{
			ProcessMessages();
		}
		else
		{
			// if no pending msg, resend the last position cmd.
			//TODO reenable?
			//this->HandlePositionCommand(this->last_position_cmd);
		}
  	}
}

//- Publishes the indicated interface data
/**
	PublishPosition2D
*/
void mbasedriver::PublishPosition2D() 
{
	this->Publish(this->position_id,
    			PLAYER_MSGTYPE_DATA,
			PLAYER_POSITION2D_DATA_STATE,
			(void*)&(this->mbasedriver_data.position),
			sizeof(player_position2d_data_t),
			NULL);
}


/**
	PublishPower
*/
void mbasedriver::PublishPower() 
{
	this->Publish(this->power_id,
			PLAYER_MSGTYPE_DATA,
			PLAYER_POWER_DATA_STATE,
			(void*)&(this->mbasedriver_data.power),
			sizeof(player_power_data_t),
			NULL);
}


/**
	PublishAIn
*/
void mbasedriver::PublishAIn() 
{
	this->Publish(this->aio_id,
			PLAYER_MSGTYPE_DATA,
			PLAYER_AIO_DATA_STATE,
			(void*)&(this->mbasedriver_data.aio),
			sizeof(player_aio_data_t),
			NULL);
}


/**
	PublishIR
*/
void mbasedriver::PublishIR() 
{
	this->Publish(this->ir_id,
    			PLAYER_MSGTYPE_DATA,
			PLAYER_IR_DATA_RANGES,
			(void*)&(this->mbasedriver_data.ir),
			sizeof(player_ir_data_t),
			NULL);
}


/**
	PublishSonar
*/
void mbasedriver::PublishSonar() 
{
	this->Publish(this->sonar_id,
    			PLAYER_MSGTYPE_DATA,
   			PLAYER_SONAR_DATA_RANGES,
    			(void*)&(this->mbasedriver_data.sonar),
    			sizeof(player_sonar_data_t),
   			NULL);
}


/**
	PublishAllData
*/
void mbasedriver::PublishAllData() 
{
  	this->PublishPosition2D();
	this->PublishPower();
	this->PublishAIn();
	this->PublishIR();
	this->PublishSonar();
}


/**
	ProcessMessage
	- Gets called from ProcessMessages to handle one message
*/
int mbasedriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data) 
{
	// Look for configuration requests
  	if(hdr->type == PLAYER_MSGTYPE_REQ)		
		return(this->HandleConfig(resp_queue,hdr,data));
  	else if(hdr->type == PLAYER_MSGTYPE_CMD)
		return(this->HandleCommand(hdr,data));
  	else
    		return(-1);
}


/**
	HandleConfig
	- Handles one Player command with a configuration request
*/
int mbasedriver::HandleConfig(QueuePointer &resp_queue, player_msghdr * hdr, void * data) 
{
	// check for position config requests
  	if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
					PLAYER_POSITION2D_REQ_SET_ODOM,
					this->position_id))
  	{
    		if(hdr->size != sizeof(player_position2d_set_odom_req_t))
    		{
      			PLAYER_WARN("Arg to odometry set requests wrong size; ignoring");
      			return(-1);
    		}
    		player_position2d_set_odom_req_t* set_odom_req = (player_position2d_set_odom_req_t*)data;

		mbasedriverPacket *odom_packet;
    		unsigned char odom_command[8];
		odom_packet = new mbasedriverPacket();
		odom_command[0] = (command_e)set_odometria;
		odom_command[1] = (argtype_e)argint;
		odom_command[2] = ((int )rint(set_odom_req->pose.px*1e3)) & 0x00FF;
		odom_command[3] = (((int )rint(set_odom_req->pose.px*1e3)) & 0xFF00) >> 8;
		odom_command[4] = ((int )rint(set_odom_req->pose.py*1e3)) & 0x00FF;
		odom_command[5] = (((int )rint(set_odom_req->pose.py*1e3)) & 0xFF00) >> 8;
		odom_command[6] = ((int)rint(RadToGrad(set_odom_req->pose.pa))) & 0x00FF;
		odom_command[7] = (((int)rint(RadToGrad(set_odom_req->pose.pa))) & 0xFF00) >> 8;
		if(debug_usuario)	
			printf("... SET_ODOM... %f %f %f ...\n", set_odom_req->pose.px, set_odom_req->pose.py, set_odom_req->pose.pa);
		odom_packet->Build(odom_command, 8);
		this->Send(odom_packet);

    		this->Publish(this->position_id, resp_queue,
                  		PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_SET_ODOM);
    		return(0);
  	}
  	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                    	PLAYER_POSITION2D_REQ_MOTOR_POWER,
                                    	this->position_id)) 
	{
    		if(hdr->size != sizeof(player_position2d_power_config_t)) 
		{
      			PLAYER_WARN("Arg to motor state change request wrong size; ignoring");
      			return(-1);
    		}
    		player_position2d_power_config_t* power_config = (player_position2d_power_config_t*)data;
    		this->ToggleMotorPower(power_config->state);

    		this->Publish(this->position_id, resp_queue,
      				PLAYER_MSGTYPE_RESP_ACK,
                  		PLAYER_POSITION2D_REQ_MOTOR_POWER);
    		return(0);
  	} 
	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                      PLAYER_POSITION2D_REQ_RESET_ODOM,
                                      this->position_id)) 
	{
    		if(hdr->size != 0) 
		{
			PLAYER_WARN("Arg to reset position request is wrong size; ignoring");
			return(-1);
    		}

		mbasedriverPacket *reset_odom_packet;
    		unsigned char reset_odom_command[2];
		reset_odom_packet = new mbasedriverPacket();
		reset_odom_command[0] = (command_e)reset_origo;
		reset_odom_command[1] = (argtype_e)argint;
		this->motor_packet->xpos = 0;
		this->motor_packet->ypos = 0;
		if(debug_usuario)	printf("... RESET_ORIGO ...\n");
		reset_odom_packet->Build(reset_odom_command, 2);
		this->Send(reset_odom_packet);

    		this->Publish(this->position_id, resp_queue,
      				PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_RESET_ODOM);
    		return(0);
  	}
  	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                    	PLAYER_POSITION2D_REQ_GET_GEOM,
                                   	this->position_id)) 
	{
		if(hdr->size != 0) 
		{
			PLAYER_WARN("Arg get robot geom is wrong size; ignoring");
			return(-1);
		}
    		player_position2d_geom_t geom;

		geom.pose.px = -robotParams[param_idx]->RobotAxleOffset / 1e3;
		geom.pose.py = 0.0;
		geom.pose.pyaw = 0.0;

		geom.size.sl = robotParams[param_idx]->RobotLength / 1e3;
		geom.size.sw = robotParams[param_idx]->RobotWidth / 1e3;
		
		this->Publish(this->position_id, resp_queue,
				PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_POSITION2D_REQ_GET_GEOM,
				(void*)&geom, sizeof(geom), NULL);
    		return(0);
  	}
	// Request for getting IR locations
  	else if(Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                    	PLAYER_IR_REQ_POSE,
                                    	this->ir_id)) 
	{
    		player_ir_pose_t pose;
    		pose.poses_count = robotParams[param_idx]->NumIR;
		pose.poses = new player_pose3d_t[pose.poses_count];
    		for (uint16_t i = 0; i < pose.poses_count ;i++)
      			pose.poses[i] = robotParams[param_idx]->IRPose[i];
    		this->Publish(this->ir_id, resp_queue,
                  		PLAYER_MSGTYPE_RESP_ACK,
                  		PLAYER_IR_REQ_POSE,
                  		(void*)&pose, sizeof(pose), NULL);
    		return(0);
  	}
  	// Request for getting sonar locations two types of requests...
  	else if (Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
                                    	PLAYER_SONAR_DATA_GEOM,
                                    	this->sonar_id))
    	{
      		player_sonar_geom_t pose;
      		pose.poses_count = robotParams[param_idx]->NumSonars;
      		for (uint16_t i = 0; i < pose.poses_count ;i++)
        		pose.poses[i] = robotParams[param_idx]->sonar_pose[i];
      		this->Publish(this->sonar_id, resp_queue,
                    		PLAYER_MSGTYPE_RESP_ACK,
                    		PLAYER_SONAR_DATA_GEOM,
                    		(void*)&pose, sizeof(pose), NULL);
      		return(0);
    	}
  	// Request for getting sonar locations two types of requests...
  	else if (Message::MatchMessage(hdr,PLAYER_MSGTYPE_REQ,
        				PLAYER_SONAR_REQ_GET_GEOM,
                                    	this->sonar_id))
    	{
      		player_sonar_geom_t pose;
      		pose.poses_count = robotParams[param_idx]->NumSonars;
		pose.poses = new player_pose3d_t[pose.poses_count];
      		for (uint16_t i = 0; i < pose.poses_count ;i++)
	  		pose.poses[i] = robotParams[param_idx]->sonar_pose[i];
      		this->Publish(this->sonar_id, resp_queue,
                    		PLAYER_MSGTYPE_RESP_ACK,
                    		PLAYER_SONAR_REQ_GET_GEOM,
                    		(void*)&pose, sizeof(pose), NULL);
      		return(0);
    	}
	else
    	{
      		PLAYER_WARN("unknown config request to mbasedriver driver");
      		return(-1);
    	}
}


/**
	getms
	- function to get the time in ms
*/
int mbasedriver:: getms()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
  	return tv.tv_sec*1000 + tv.tv_usec/1000;
}


/**
	HandlePositionCommand
	- Handles one Player command detailing a velocity
*/
void mbasedriver::HandlePositionCommand(player_position2d_cmd_vel_t position_cmd)
{
	int speedDemand, turnRateDemand;
	unsigned short absspeedDemand, absturnRateDemand;
	unsigned char motorcommand[4];
	mbasedriverPacket *motorpacket;
	
	speedDemand = (int)rint(position_cmd.vel.px * 1e3);
	turnRateDemand = (int)rint(RTOD(position_cmd.vel.pa));

	if(trans_ant == speedDemand && rot_ant == turnRateDemand)	return;
	trans_ant = speedDemand;
	rot_ant = turnRateDemand;
	int ms = getms();
	if (mcount == 0) mcount = ms-200;
	if (ms < mcount + 50)          // at least 100 ms have to elapse
	return;
	mcount = ms;
/*	if(speedDemand==0 && turnRateDemand==0)
	{
		if(debug_usuario)	printf("TRANS=0, ROT=0 -> STOP!!!!!!!!!!!!!!!!!!!!!!\n");
		mbasedriverPacket packet;
		if(debug_usuario)	printf("STOP\n");
    		unsigned char command = (command_e)stop;
    		packet.Build(&command, 1);
 	   	packet.Send(this->write_fd);
	    	usleep(ROBOT_CYCLETIME);
	}
*/
//	else
//	{
		motorcommand[0] = (command_e)trans_vel;
		if(speedDemand >= 0)	motorcommand[1] = (argtype_e)argint;
		else			motorcommand[1] = (argtype_e)argnint;
		
		absspeedDemand = (unsigned short)abs(speedDemand);
		if(absspeedDemand < this->motor_max_speed)
		{
			motorcommand[2] = absspeedDemand & 0x00FF;
			motorcommand[3] = (absspeedDemand & 0xFF00) >> 8;
			if(debug_usuario)	printf("...TRANS_VEL: %d...\n",speedDemand);
		}
		else
		{
			motorcommand[2] = this->motor_max_speed & 0x00FF;
			motorcommand[3] = (this->motor_max_speed & 0xFF00) >> 8;
			if(debug_usuario)	printf("TRANS_VEL: %d...\n",this->motor_max_speed);
		}
		motorpacket = new mbasedriverPacket();
		motorpacket->Build(motorcommand, 4);
		//motorpacket->PrintHex();
		Send(motorpacket);
		usleep(ROBOT_CYCLETIME);
		
		motorcommand[0] = (command_e)rot_vel;
		if(turnRateDemand >= 0)	motorcommand[1] = (argtype_e)argint;
		else			motorcommand[1] = (argtype_e)argnint;
		
		absturnRateDemand = (unsigned short)abs(turnRateDemand);
		if(absturnRateDemand < this->motor_max_turnspeed)
		{
			motorcommand[2] = absturnRateDemand & 0x00FF;
			motorcommand[3] = (absturnRateDemand & 0xFF00) >> 8;
			if(debug_usuario)	printf("...ROT_VEL: %d...\n",turnRateDemand);
		}
		else
		{
			motorcommand[2] = this->motor_max_turnspeed & 0x00FF;
			motorcommand[3] = (this->motor_max_turnspeed & 0xFF00) >> 8;
			if(debug_usuario)	printf("...ROT_VEL: %d...\n",this->motor_max_turnspeed);
		}
		
		motorpacket = new mbasedriverPacket();
		if(debug_send)	printf("__ build SET_ROT_VEL %d\n", turnRateDemand);
		motorpacket->Build(motorcommand, 4);
		Send(motorpacket);
		usleep(ROBOT_CYCLETIME);
//	}	
}

#define SERVO_NEUTRAL 1650	// in servo counts
#define SERVO_COUNTS_PER_DEGREE 6.5
#define SERVO_MAX_COUNT 2300
#define SERVO_MIN_COUNT 1100

/**
	HandleCommand
	- Switchboard for robot commands, called from ProcessMessage
	- Note that these don't perform a response...
*/
int mbasedriver::HandleCommand(player_msghdr * hdr, void* data)
{
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, this->position_id))
  	{
      		player_position2d_cmd_vel_t position_cmd = *(player_position2d_cmd_vel_t*)data;
      		this->HandlePositionCommand(position_cmd);
    	}
  	else	return(-1);
  	return(0);
}
