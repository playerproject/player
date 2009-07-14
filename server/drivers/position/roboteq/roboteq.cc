/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003  Brian Gerkey gerkey@usc.edu
 *                           Andrew Howard ahoward@usc.edu
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
 
 // TODO: add config parameter to specify analog feedback.

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_roboteq roboteq
 * @brief Motor control driver for Roboteq AX2850
 
Provides position2d interface to the Roboteq AX2850 motor controller
http://www.roboteq.com/ax2850-folder.html

@par Compile-time dependencies

- none

@par Provides

- @ref interface_position2d
  @ref interface_power

@par Requires

- None

@par Configuration requests

- none

@par Configuration file options

- devicepath (string)
  - Default: none
  - The serial port to be used.

- baud (integer)
  - Default: 9600
  - The baud rate to be used.

- encoder_ppr
  - Default: 500
  - The number of pulses per revolution for the encoders. Optional if no encoders are present.
  
- wheel_circumference
  - Default: 1 meter
  - The wheel circumference in meters. Optional if no encoders present. 
  
- axle_length
  - Default: 1 meter
  - The distance between the centers of the two wheels. Optional if no encoders present.
  
- gear_ratio
  - Default: 1
  - The gear ratio from the motor to the wheel (the number of motor revolutions per one revolution of the wheel). Optional if no encoders present.
  
- controller_current_limit
  - Default: 105 amperes
  - The maximum current to the motors. See the Roboteq manual.
  
- acceleration
  - Default: 0x20
  - The acceleration time constant. See the Roboteq manual.
  
- encoder_time_base
  - Default: 0x16
  - The encoder time base. Optional if no encoders are present. See the Roboteq manual.
  
- encoder_distance_divider
  - Default: 0x08
  - The encoder distance divider. Optional if no encoders are present. See the Roboteq manual.
  
- invert_directions
  - Default: false (off)
  - Invert the motor directions, useful if you find the motors turning in the opposite direction from what you intended...
  
- rc_mode_on_shutdown
  - Default: true (on)
  - Set the motor controller to RC mode when the driver shuts down.

@par Example

@verbatim
driver
(
  name "roboteq"
  provides ["position2d:0" "power:0"]
  devicepath "/dev/ttyS0"
)
@endverbatim

@author Pablo Rivera rivera@cse.unr.edu
@author Mike Roddewig mrroddew@mtu.edu

*/
/** @} */

#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h> // ioctl
#include <unistd.h> // close(2),fcntl(2),getpid(2),usleep(3),execvp(3),fork(2)
#include <netdb.h> // for gethostbyname(3) 
#include <netinet/in.h>  // for struct sockaddr_in, htons(3) 
#include <sys/types.h>  // for socket(2) 
#include <sys/socket.h>  // for socket(2) 
#include <signal.h>  // for kill(2) 
#include <fcntl.h>  // for fcntl(2) 
#include <string.h>  // for strncpy(3),memcpy(3) 
#include <stdlib.h>  // for atexit(3),atoi(3) 
#include <pthread.h>  // for pthread stuff 
#include <math.h>

#include <libplayercore/playercore.h>

// settings
#define SERIAL_BUFF_SIZE			128
#define MAX_MOTOR_SPEED			127
#define ROBOTEQ_CON_TIMEOUT		10		// seconds to time-out on setting RS-232 mode
#define ROBOTEQ_DEFAULT_BAUD		9600 
#define INPUT_SWITCHES_FUNCTIONS 	0x01	// sets the input switches to function as an e-stop.

// Default parameter settings

#define DEFAULT_CONTROLLER_CURRENT_LIMIT	105		// Amperes
#define DEFAULT_ACCELERATION				0x20	// About 1 second from stop to full speed.
#define DEFAULT_ENCODER_TIME_BASE			0x16
#define DEFAULT_ENCODER_DISTANCE_DIVIDER	0x08
#define DEFAULT_GEAR_RATIO					1
#define DEFAULT_WHEEL_CIRCUMFERENCE		1
#define DEFAULT_AXLE_LENGTH				1
#define DEFAULT_ENCODER_PPR				500
#define DEFAULT_INVERT_DIRECTIONS			false
#define DEFAULT_RC_MODE_ON_SHUTDOWN		true

// Parameter addresses

#define CHANNEL1_OPERATING_MODE_ADDRESS	0x80
#define CHANNEL2_OPERATING_MODE_ADDRESS 	0x81
#define CONTROLLER_IDENTIFICATION_ADDRESS 	0x8A
#define CONTROLLER_STATUS_ADDRESS 			0x89
#define INPUT_CONTROL_MODE_ADDRESS		0x00
#define MOTOR_CONTROL_MODE_ADDRESS		0x01
#define CURRENT_LIMIT_ADDRESS				0x02
#define ACCELERATION_ADDRESS				0x03
#define INPUT_SWITCHES_FUNCTION_ADDRESS	0x04
#define ENCODER1_TIME_BASE_ADDRESS		0xA2
#define ENCODER2_TIME_BASE_ADDRESS		0xA3
#define ENCODER_DISTANCE_DIVIDER_ADDRESS	0xA5
#define EXPONENTIATION_CHANNEL1_ADDRESS	0x07
#define EXPONENTIATION_CHANNEL2_ADDRESS	0x08
#define PID_PROPORTIONAL_GAIN1_ADDRESS		0x82
#define PID_PROPORTIONAL_GAIN2_ADDRESS		0x83
#define PID_INTEGRAL_GAIN1_ADDRESS			0x84
#define PID_INTEGRAL_GAIN2_ADDRESS			0x85
#define PID_DIFFERENTIAL_GAIN1_ADDRESS		0x86
#define PID_DIFFERENTIAL_GAIN2_ADDRESS		0x87

// Constants

#define MOTOR_CONTROL_MODE_CLOSED_LOOP	0xC5
#define MOTOR_CONTROL_MODE_OPEN_LOOP		0x01
#define MAX_PID_GAIN						63.0/8.0
#define EXPONENTIATION_LINEAR				0x00
#define EXPONENTIATION_STRONG_EXP			0x02
#define INPUT_CONTROL_MODE				0x01

// Message levels

#define MESSAGE_ERROR					0
#define MESSAGE_INFO						1
#define MESSAGE_DEBUG					2

#ifndef CRTSCTS
#ifdef IHFLOW
#ifdef OHFLOW
#define CRTSCTS ((IHFLOW) | (OHFLOW))
#endif
#endif
#endif

// *************************************
// some assumptions made by this driver:

// ROBOTEQ is in "mixed mode" where
// channel 1 is translation
// channel 2 is rotation

// ROBOTEQ is set to be in RC mode by default

// the robot is a skid-steer vehicle where
// left wheel(s) are on one output,
// right wheel(s) on the other.
// directionality is implied by the following
// macros (FORWARD,REVERSE,LEFT,RIGHT)
// so outputs may need to be switched

// *************************************

///////////////////////////////////////////////////////////////////////////

class roboteq : public ThreadedDriver
{
	private:
		int roboteq_fd;
		char serialin_buff[SERIAL_BUFF_SIZE];
		char serialout_buff[SERIAL_BUFF_SIZE];
		const char* devicepath;
		int roboteq_baud;
		double speed_to_rpm;
		double turning_circumference;
		char encoder_present;
		unsigned char motor_control_mode;
		double max_forward_velocity;
		double max_rotational_velocity;
		bool motors_enabled;
	
		// Config parameters.

		int controller_current_limit;
		unsigned char controller_current_limit_value;
		int acceleration;
		int encoder_time_base;
		int encoder_distance_divider;
		int encoder_ppr;
		double wheel_circumference;
		double axle_length;
		double speed_per_tick;
		double rad_per_tick;
		double gear_ratio;
		bool rc_mode_on_shutdown;
		bool invert_directions;
	
		int checkConfigParameter (char, int, int);
		void UpdatePowerData (player_power_data_t *);
		void UpdatePositionData (player_position2d_data_t *);
		int WriteMotorVelocity (double, double);

		player_position2d_data_t position_data;
		player_power_data_t power_data;
		player_devaddr_t position_addr;
		player_devaddr_t power_addr;
		player_pose2d current_position;

	public:
		roboteq(ConfigFile *, int);
    
		virtual int ProcessMessage(QueuePointer &, 
			player_msghdr *, void *);
		virtual int MainSetup();
		virtual void MainQuit();
		virtual void Main();
};


///////////////////////////////////////////////////////////////////////////
// initialization function
Driver* roboteq_Init( ConfigFile* cf, int section) {
	return((ThreadedDriver*)(new roboteq(cf, section)));
}


///////////////////////////////////////////////////////////////////////////
// a driver registration function
void roboteq_Register(DriverTable* table) {
	table->AddDriver("roboteq",  roboteq_Init);
}

///////////////////////////////////////////////////////////////////////////
roboteq::roboteq( ConfigFile* cf, int section) : ThreadedDriver(cf, section) {  
	
	memset (&this->position_addr, 0, sizeof (player_devaddr_t));
	memset (&this->power_addr, 0, sizeof (player_devaddr_t));
	
	// Check the config file to see if we are providing a position interface.
	if (cf->ReadDeviceAddr(&(this->position_addr), section, "provides",
		PLAYER_POSITION2D_CODE, -1, NULL) == 0) {
		if (this->AddInterface(this->position_addr) != 0) {
			PLAYER_ERROR("Error adding position interface.");
			SetError(-1);
			return;
		}
	}
	
	// Check the config file to see if we are providing a power interface.
	if (cf->ReadDeviceAddr(&(this->power_addr), section, "provides",
		PLAYER_POWER_CODE, -1, NULL) == 0) {
		if (this->AddInterface(this->power_addr) != 0) {
			PLAYER_ERROR("Error adding power interface.");
			SetError(-1);
			return;
		}
	}

	// Required parameter.
	
	if(!(this->devicepath = (char*)cf->ReadString(section, "devicepath", NULL))){
		PLAYER_ERROR("ROBOTEQ: you must specify the serial port device.");
		this->SetError(-1);
		return;
	}
	
	// Optional parameters.
	
	encoder_ppr = cf->ReadInt(section, "encoder_ppr", DEFAULT_ENCODER_PPR);
	if (encoder_ppr < 0) {
		PLAYER_ERROR("ROBOTEQ: 'encoder_ppr' must be positive.");
		this->SetError(-1);
		return;
	}

	wheel_circumference = cf->ReadFloat(section, "wheel_circumference", DEFAULT_WHEEL_CIRCUMFERENCE);
	if (encoder_ppr < 0.0) {
		PLAYER_ERROR("ROBOTEQ: 'wheel_circumference' must be positive.");
		this->SetError(-1);
		return;
	}
	
	axle_length = cf->ReadFloat(section, "axle_length", DEFAULT_AXLE_LENGTH);
	if (axle_length < 0.0) {
		PLAYER_ERROR("ROBOTEQ: 'axle_length' must be positive.");
		this->SetError(-1);
		return;
	}
	
	gear_ratio = cf->ReadFloat(section, "gear_ratio", DEFAULT_GEAR_RATIO);
	if (gear_ratio < 0.0) {
		PLAYER_ERROR("ROBOTEQ: 'gear_ratio' must be positive.");
		this->SetError(-1);
		return;
	}
	
	controller_current_limit = cf->ReadInt(section, "controller_current_limit", DEFAULT_CONTROLLER_CURRENT_LIMIT);
	
	// Compute the value to send to the microcontroller.
	if (controller_current_limit < 1) {
		PLAYER_ERROR("ROBOTEQ: The current limit must be greater than 0.");
		this->SetError(-1);
		return;
	}
	else if (controller_current_limit <= 30) {
		controller_current_limit_value = 0;
		controller_current_limit_value += (30 - controller_current_limit) << 4;
	}
	else if (controller_current_limit <= 45) {
		controller_current_limit_value = 1;
		controller_current_limit_value += (45 - controller_current_limit) << 4;
	}
	else if (controller_current_limit <= 60) {
		controller_current_limit_value = 2;
		controller_current_limit_value += (60 - controller_current_limit) << 4;
	}
	else if (controller_current_limit <= 75) {
		controller_current_limit_value = 3;
		controller_current_limit_value += (75 - controller_current_limit) << 4;
	}
	else if (controller_current_limit <= 90) {
		controller_current_limit_value = 4;
		controller_current_limit_value += (90 - controller_current_limit) << 4;
	}
	else if (controller_current_limit <= 105) {
		controller_current_limit_value = 5;
		controller_current_limit_value += (105 - controller_current_limit) << 4;
	}
	else if (controller_current_limit <= 120) {
		controller_current_limit_value = 6;
		controller_current_limit_value += (120 - controller_current_limit) << 4;
	}
	else {
		PLAYER_ERROR("ROBOTEQ: The current limit value must be less than 120 A.");
		this->SetError(-1);
		return;
	}

	acceleration = cf->ReadInt(section, "acceleration", DEFAULT_ACCELERATION);
	if (acceleration < 0 || acceleration > 53) {
		PLAYER_ERROR("ROBOTEQ: 'acceleration' must be a value between 0 and 53.");
		this->SetError(-1);
		return;
	}
	
	encoder_time_base = cf->ReadInt(section, "encoder_time_base", DEFAULT_ENCODER_TIME_BASE);
	if (encoder_time_base < 0 || encoder_time_base > 63) {
		PLAYER_ERROR("ROBOTEQ: 'encoder_time_base' must be a value between 0 and 63.");
		this->SetError(-1);
		return;
	}
	
	encoder_distance_divider = cf->ReadInt(section, "encoder_distance_divider", DEFAULT_ENCODER_DISTANCE_DIVIDER);
	if (encoder_distance_divider < 0 || encoder_distance_divider > 63) {
		PLAYER_ERROR("ROBOTEQ: 'encoder_distance_divider' must be a value between 0 and 63.");
		this->SetError(-1);
		return;
	}
	
	invert_directions = cf->ReadBool(section, "invert_directions", DEFAULT_INVERT_DIRECTIONS);

	roboteq_baud = cf->ReadInt(section, "baud", ROBOTEQ_DEFAULT_BAUD);
	
	rc_mode_on_shutdown = cf->ReadBool(section, "rc_mode_on_shutdown", DEFAULT_RC_MODE_ON_SHUTDOWN);

	memset(&position_data, 0, sizeof(position_data));
	memset(&power_data, 0, sizeof(power_data));
	roboteq_fd = -1;

	PLAYER_MSG1(MESSAGE_INFO, "Configuring Roboteq serial port at %s", devicepath);
	
	roboteq_fd = open(devicepath, O_RDWR|O_NDELAY);
	if (roboteq_fd == -1){
		PLAYER_ERROR("ROBOTEQ: Unable to configure serial port!");
		this->SetError(-1);
		return; 
	}
	else{
		struct termios options;
      
		tcgetattr(roboteq_fd, &options);

		// default is 9600 unless otherwise specified

		if (roboteq_baud == 4800) {
			cfsetispeed(&options, B4800);
			cfsetospeed(&options, B4800);
		}
		else if (roboteq_baud == 19200) {
			cfsetispeed(&options, B19200);
			cfsetospeed(&options, B19200);
		}
		else if (roboteq_baud == 38400) {
			cfsetispeed(&options, B38400);
			cfsetospeed(&options, B38400);
		}
		else {
			cfsetispeed(&options, B9600);
			cfsetospeed(&options, B9600);
		}

		// set to 7bit even parity, no flow control
		options.c_cflag |= (CLOCAL | CREAD);
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		options.c_cflag &= ~CSTOPB;
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS7;
		options.c_cflag &= ~CRTSCTS;

		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);    // non-canonical

		tcsetattr(roboteq_fd, TCSANOW, &options);
		ioctl(roboteq_fd, TCIOFLUSH, 2);
		tcflush(roboteq_fd, TCIFLUSH);
	}
	
	// Compute the encoder speed to RPM conversion factor.
	speed_to_rpm = (60.0 * 1000000.0) / (((double) encoder_ppr) * 4.0 * 256.0 * (((double) encoder_time_base) + 1.0));
	
	// Compute the speed value to m/s conversion factor.
	
	speed_per_tick = (speed_to_rpm * wheel_circumference) / (gear_ratio * 60);
	
	// Compute the turning circumference.
	turning_circumference = 2.0 * M_PI * axle_length;
	
	// Compute the speed value to rad/s conversion factor.
	
	rad_per_tick = (2.0 * M_PI * speed_per_tick) / turning_circumference;
	
	max_forward_velocity = speed_per_tick * 127;
	max_rotational_velocity = rad_per_tick * 127;
	
	PLAYER_MSG1(MESSAGE_INFO, "Computed maximum forward velocity of %f m/s.", max_forward_velocity);
	PLAYER_MSG1(MESSAGE_INFO, "Computed maximum rotational velocity of %f rad/s.", max_rotational_velocity);
	
	// Motors are disabled on startup.
	motors_enabled = false;
	
	// Enable new motor commands to overwrite old ones if they have not yet been processed.
	this->InQueue->AddReplaceRule(-1, -1, -1, -1, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, true);
	
	// Set the driver to always on so only one existence exists (is this the best way to do that?).
	this->alwayson = true;
	
	PLAYER_MSG0(MESSAGE_DEBUG, "Done.");
	
	return;
}

///////////////////////////////////////////////////////////////////////////
int roboteq::MainSetup() {
	int returned_value;
	int ret;
	int i;
	
	// initialize RoboteQ to RS-232 mode
	strcpy(serialout_buff, "\r");
	for (i=0; i<10; i++) { 
		write(roboteq_fd, serialout_buff, strlen(serialout_buff));
		tcdrain(roboteq_fd);
		usleep(25000);
	}
	
	// Disable the watchdog timer.
	strcpy(serialout_buff, "^00 01\r");
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	usleep(25000);
	
	strcpy(serialout_buff, "%rrrrrr\r");
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	sleep(2); // Sleep for two seconds to give the controller sufficient time to reboot.
	
	tcflush(roboteq_fd, TCIFLUSH); // Clear all the reboot crap out of the input buffer.
	
	// Read controller model to make sure the serial link is ok.
	strcpy(serialout_buff, "^8A\r");
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	usleep(25000);
	
	memset(serialin_buff, 0, SERIAL_BUFF_SIZE);
	ret = read(roboteq_fd, serialin_buff, SERIAL_BUFF_SIZE-1);
	serialin_buff[SERIAL_BUFF_SIZE-1] = 0x00; // Null terminate our buffer to make sure sscanf doesn't run amok.
	
	ret = sscanf(serialin_buff, "%*3c %2X", &returned_value);
	if (ret != 1) {
		PLAYER_ERROR("ROBOTEQ: Unable to communicate with the controller! Check the serial device.");
		this->SetError(-1);
		return -1;
	}
	else {
		if (returned_value & 0x01) {
			PLAYER_MSG0(MESSAGE_INFO, "AX500 found.");
		}
		else if (returned_value & 0x02) {
			PLAYER_MSG0(MESSAGE_INFO, "AX1500 found.");
		}
		else if (returned_value & 0x04) {
			PLAYER_MSG0(MESSAGE_INFO, "AX2500 found.");
		}
		else if (returned_value & 0x08) {
			PLAYER_MSG0(MESSAGE_INFO, "AX3500 found.");
		}
		else {
			PLAYER_WARN("Unknown controller found."); // Weird...this shouldn't happen.
		}
		
		if (returned_value & 0x20) {
			PLAYER_MSG0(MESSAGE_INFO, "Encoder present.");
			encoder_present = 1;
		}
		else {
			encoder_present = 0;
		}
		
		if (returned_value & 0x40) {
			PLAYER_MSG0(MESSAGE_INFO, "Short circuit detection capable.");
		}
	}
	
	if (encoder_present == 1) {
		motor_control_mode = MOTOR_CONTROL_MODE_CLOSED_LOOP;
	}
	else {
		motor_control_mode = MOTOR_CONTROL_MODE_OPEN_LOOP;
	}
	
	// Set configuration parameters

	if (this->checkConfigParameter('^', MOTOR_CONTROL_MODE_ADDRESS, motor_control_mode) != 0) {
		PLAYER_ERROR("ROBOTEQ: Error setting motor control mode.");
		this->SetError(-1);
		return -1;
	}
	if (this->checkConfigParameter('^', CURRENT_LIMIT_ADDRESS, controller_current_limit_value) != 0) {
		PLAYER_ERROR("ROBOTEQ: Error setting controller current limit.");
		this->SetError(-1);
		return -1;
	}
	if (this->checkConfigParameter('^', ACCELERATION_ADDRESS, acceleration) != 0) {
		PLAYER_ERROR("ROBOTEQ: Error setting acceleration profile.");
		this->SetError(-1);
		return -1;
	}
	if (this->checkConfigParameter('^', EXPONENTIATION_CHANNEL1_ADDRESS, EXPONENTIATION_LINEAR) != 0) {
		PLAYER_ERROR("ROBOTEQ: Error setting channel 1 exponentiation to linear.");
		this->SetError(-1);
		return -1;
	}
	if (this->checkConfigParameter('^', EXPONENTIATION_CHANNEL2_ADDRESS, EXPONENTIATION_LINEAR) != 0) {
		PLAYER_ERROR("ROBOTEQ: Error setting channel 2 exponentiation to linear.");
		this->SetError(-1);
		return -1;
	}
	if (encoder_present != 0) {
		// Set encoder parameters.
		
		if (this->checkConfigParameter('*', ENCODER1_TIME_BASE_ADDRESS, encoder_time_base) != 0) {
			PLAYER_ERROR("ROBOTEQ: Error setting encoder one time base.");
			this->SetError(-1);
			return -1;
		}
		if (this->checkConfigParameter('*', ENCODER2_TIME_BASE_ADDRESS, encoder_time_base) != 0) {
			PLAYER_ERROR("ROBOTEQ: Error setting encoder two time base.");
			this->SetError(-1);
			return -1;
		}
		if (this->checkConfigParameter('*', ENCODER_DISTANCE_DIVIDER_ADDRESS, encoder_distance_divider) != 0) {
			PLAYER_ERROR("ROBOTEQ: Error setting encoder distance divider.");
			this->SetError(-1);
			return -1;
		}
	}
	// Reboot the controller.
	strcpy(serialout_buff, "%rrrrrr\r");
        write(roboteq_fd, serialout_buff, strlen(serialout_buff));
        tcdrain(roboteq_fd);
        sleep(2); // Sleep for two seconds to give the controller sufficient time to reboot.

        tcflush(roboteq_fd, TCIFLUSH); // Clear all the reboot crap out of the input buffer.

	return 0;
}

// Check the value of the configuration parameter located at the given address, 
// set it to the value if it does not equal the provided value.
int roboteq::checkConfigParameter(char prefix, int address, int value) {
	unsigned int returned_value;
	char command_status;
	int ret;


	sprintf(serialout_buff, "%c%.2X\r", prefix, address);
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	usleep(25000);
	
	memset(serialin_buff, 0, SERIAL_BUFF_SIZE);
	ret = read(roboteq_fd, serialin_buff, SERIAL_BUFF_SIZE-1);
	serialin_buff[SERIAL_BUFF_SIZE-1] = 0x00; // Null terminate our buffer to make sure sscanf doesn't run amok.
	
	ret = sscanf(serialin_buff, "%*3c %2X", &returned_value);
	if (ret != 1) {
		returned_value = value + 1;
		// Yes, it's hackish. BUT, for some reason if you try and read a value that has not been set instead of returning
		// say -1 the Roboteq controller will return an error which will screw this function up. But, once you write to
		// the address everything is just peachy. So, we basically ensure that we write to the address. And we do look 
		// for an error further down the line.
	}
	
	if ((char) returned_value != value) {
		sprintf(serialout_buff, "%c%.2X %.2X\r", prefix, address, value);
		write(roboteq_fd, serialout_buff, strlen(serialout_buff));
		tcdrain(roboteq_fd);
		usleep(25000);
		
		// Check that the command was received ok.
		
		memset(serialin_buff, 0, SERIAL_BUFF_SIZE);
		ret = read(roboteq_fd, serialin_buff, SERIAL_BUFF_SIZE-1);
		serialin_buff[SERIAL_BUFF_SIZE-1] = 0x00; // Null terminate our buffer to make sure sscanf doesn't run amok.
		ret = sscanf(serialin_buff, "%*6c %1c", &command_status);
		if (ret != 1 || command_status != '+') {
			return -2; // An error occured writing the command.
		}
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////
void roboteq::MainQuit() {
	// Shut off the motors.
	
	WriteMotorVelocity(0.0, 0.0);
	
	if (rc_mode_on_shutdown == true) {
		// Reset the input exponentiation mode to strong exponential.
	
		if (this->checkConfigParameter('^', EXPONENTIATION_CHANNEL1_ADDRESS, EXPONENTIATION_STRONG_EXP) != 0) {
			PLAYER_WARN("ROBOTEQ: Error setting channel 1 exponentiation to strong exponential");
		}
		if (this->checkConfigParameter('^', EXPONENTIATION_CHANNEL2_ADDRESS, EXPONENTIATION_STRONG_EXP) != 0) {
			PLAYER_WARN("ROBOTEQ: Error setting channel 2 exponentiation to strong exponential.");
		}
		strcpy(serialout_buff, "^00 00\r");
		write(roboteq_fd, serialout_buff, strlen(serialout_buff));
		tcdrain(roboteq_fd);
		usleep(25000);
		strcpy(serialout_buff, "%rrrrrr\r");
		write(roboteq_fd, serialout_buff, strlen(serialout_buff));
		tcdrain(roboteq_fd);
		usleep(25000);
	}

	close(roboteq_fd);
  
	return;
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
////////////////////////////////////////////////////////////////////////////////
int roboteq::ProcessMessage (QueuePointer &resp_queue, player_msghdr * hdr, void * data) {
	
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, position_addr)) {

		assert(hdr->size == sizeof(player_position2d_cmd_vel_t));

		player_position2d_cmd_vel_t & command 
			= *reinterpret_cast<player_position2d_cmd_vel_t *> (data);
		
		WriteMotorVelocity(command.vel.px, command.vel.pa);
	}
	else if (Message::MatchMessage(hdr , PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SET_ODOM, position_addr)) {
		assert(hdr->size == sizeof(player_position2d_set_odom_req_t));
		
		player_position2d_set_odom_req_t & odom_req = * reinterpret_cast<player_position2d_set_odom_req_t *> (data);
		
		current_position.px = odom_req.pose.px;
		current_position.py = odom_req.pose.py;
		current_position.pa = odom_req.pose.pa;
		
		this->Publish(position_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_SET_ODOM);
	}
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_RESET_ODOM, position_addr)) {		
		current_position.px = 0.0;
		current_position.py = 0.0;
		current_position.pa = 0.0;
		
		this->Publish(position_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_RESET_ODOM);
	}
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_MOTOR_POWER)) {
		assert(hdr->size == sizeof(player_position2d_power_config_t));
		
		player_position2d_power_config_t * power_config = reinterpret_cast<player_position2d_power_config_t *> (data);
		motors_enabled = (bool) power_config->state;
		
		this->Publish(position_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_MOTOR_POWER);
	}
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SPEED_PID)) {
		assert(hdr->size == sizeof(player_position2d_speed_pid_req_t));
		
		player_position2d_speed_pid_req_t * pid_req = reinterpret_cast<player_position2d_speed_pid_req_t *> (data);
		
		// Check the gain values we received.
		
		if ((pid_req->kp < 0 || pid_req->kp > MAX_PID_GAIN) || (pid_req->ki < 0 || pid_req->ki > MAX_PID_GAIN) || 
			(pid_req->kd < 0 || pid_req->kd > MAX_PID_GAIN)) {
			PLAYER_WARN("ROBOTEQ: Invalid PID gain parameter(s).");

			this->Publish(position_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_POSITION2D_REQ_SPEED_PID);
		}
		else {
		
			if (this->checkConfigParameter('^', PID_PROPORTIONAL_GAIN1_ADDRESS, (unsigned char) (pid_req->kp * 8)) != 0) {
				PLAYER_WARN("ROBOTEQ: Error setting channel one proportional gain.");
			}
			if (this->checkConfigParameter('^', PID_PROPORTIONAL_GAIN2_ADDRESS, (unsigned char) (pid_req->kp * 8)) != 0) {
				PLAYER_WARN("ROBOTEQ: Error setting channel two proportional gain.");
			}
			if (this->checkConfigParameter('^', PID_INTEGRAL_GAIN1_ADDRESS,(unsigned char) (pid_req->ki * 8)) != 0) {
				PLAYER_WARN("ROBOTEQ: Error setting channel one integral gain.");
			}
			if (this->checkConfigParameter('^', PID_INTEGRAL_GAIN2_ADDRESS, (unsigned char) (pid_req->ki * 8)) != 0) {
				PLAYER_WARN("ROBOTEQ: Error setting channel two integral gain.");
			}
			if (this->checkConfigParameter('^', PID_DIFFERENTIAL_GAIN1_ADDRESS, (unsigned char) (pid_req->kd * 8)) != 0) {
				PLAYER_WARN("ROBOTEQ: Error setting channel one differential gain.");
			}
			if (this->checkConfigParameter('^', PID_DIFFERENTIAL_GAIN2_ADDRESS, (unsigned char) (pid_req->kd * 8)) != 0) {
				PLAYER_WARN("ROBOTEQ: Error setting channel two differential gain.");
			}
			
			this->Publish(position_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_SPEED_PID);
		}
	}
	else {
		this->Publish(position_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);
	}
	
	return 0;
}

void roboteq::UpdatePowerData(player_power_data_t * power_data) {
	int ret;
	int current_1, current_2, voltage;
	
	power_data->valid &= (PLAYER_POWER_MASK_VOLTS | PLAYER_POWER_MASK_WATTS); // We only return the voltage and power (in watts).
	
	// Request the battery voltage.
	strcpy(serialout_buff, "?E\r");
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	usleep(25000);
	
	memset(serialin_buff, 0, SERIAL_BUFF_SIZE);
	ret = read(roboteq_fd, serialin_buff, SERIAL_BUFF_SIZE-1);
	serialin_buff[SERIAL_BUFF_SIZE-1] = 0x00; // Null terminate our buffer to make sure sscanf doesn't run amok.
	
	ret = sscanf(serialin_buff, "%*2c %2X", &voltage);
	if (ret == 1) {
		power_data->volts = ((double) voltage) * (55.0/256.0);
	}
	
	// Request the current flowing to each channel. We sum the result to compute the power.
	strcpy(serialout_buff, "?A\r");
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	usleep(25000);
	
	memset(serialin_buff, 0, SERIAL_BUFF_SIZE);
	ret = read(roboteq_fd, serialin_buff, SERIAL_BUFF_SIZE-1);
	serialin_buff[SERIAL_BUFF_SIZE-1] = 0x00; // Null terminate our buffer to make sure sscanf doesn't run amok.

	ret = sscanf(serialin_buff, "%*2c %2X %2X", &current_1, &current_2);
	if (ret == 2) {
		power_data->watts = power_data->volts * (current_1 + current_2);
	}
}

int roboteq::WriteMotorVelocity(double forward_velocity, double rotational_velocity) {
	unsigned char forward_value, rotational_value;
	char returned_value;
	int ret;
	
	// We hard limit the input velocities.
	if (fabs(forward_velocity) > max_forward_velocity) {
		forward_value = 0x7F;
	}
	else {
		forward_value = (unsigned char) (fabs(forward_velocity) / speed_per_tick);
	}
	
	if (fabs(rotational_velocity) > max_rotational_velocity) {
		rotational_value = 0x7F;
	}
	else {
		rotational_value = (unsigned char) (fabs(rotational_velocity) / rad_per_tick);
	}
	
	// Software enable/disable.
	
	if (motors_enabled == false) {
		forward_value = 0;
		rotational_value = 0;
		
		// Give the user an informational message to hopefully save them hours of torment. >:D
		PLAYER_MSG0(MESSAGE_INFO, "Warning, the motors are disabled! Enable them before use.");
	}
	
	// Check if we need to invert the velocities.
	
	if (invert_directions == true) {
		forward_velocity = -forward_velocity;
		rotational_velocity = -rotational_velocity;
	}

	// Write forward velocity and check result.
	
	if (forward_velocity < 0) {
		sprintf(serialout_buff, "!b%.2X\r", forward_value);
	} 
	else {
		sprintf(serialout_buff, "!B%.2X\r", forward_value);
	}
	
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	usleep(25000);
		
	memset(serialin_buff, 0, SERIAL_BUFF_SIZE);
	ret = read(roboteq_fd, serialin_buff, SERIAL_BUFF_SIZE-1);
	serialin_buff[SERIAL_BUFF_SIZE-1] = 0x00; // Null terminate our buffer to make sure sscanf doesn't run amok.

	ret = sscanf(serialin_buff, "%*4c %1c", &returned_value);
		
	if (returned_value != '+') {
		// Some kind of error happened.
		
		PLAYER_WARN("ROBOTEQ: Error writing forward velocity command.");
		return -1;
	}
	
	// Write rotational velocity and check result.
	
	if (rotational_velocity < 0) {
		sprintf(serialout_buff, "!a%.2X\r", rotational_value);
	} 
	else {
		sprintf(serialout_buff, "!A%.2X\r", rotational_value);
	}
	
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	usleep(25000);
		
	memset(serialin_buff, 0, SERIAL_BUFF_SIZE);
	ret = read(roboteq_fd, serialin_buff, SERIAL_BUFF_SIZE-1);
	serialin_buff[SERIAL_BUFF_SIZE-1] = 0x00; // Null terminate our buffer to make sure sscanf doesn't run amok.

	ret = sscanf(serialin_buff, "%*4c %1c", &returned_value);
		
	if (returned_value != '+') {
		// Some kind of error happened.
		
		PLAYER_WARN("ROBOTEQ: Error writing rotational velocity command.");
		return -1;
	}
	
	return 0;
}

void roboteq::UpdatePositionData(player_position2d_data_t * position_data) {
	int ret;
	int speed1_value, speed2_value;
	int encoder1_count, encoder2_count;
	double rpm1, rpm2;
	double speed1, speed2, speed_diff;
	double distance1, distance2, distance_diff;
	
	// Ok, the math for this section is a bit tricky. Wheel 1 is assumed to be on the right
	// and a positive rotational velocity indicates the robot is turning left. To determine 
	// the rotational velocity we compute the difference in wheel velocities and then 
	// compute the rotational velocity by converting m/s to rad/s on a circle of radius 
	// axle length.
	
	// Read in the current speed value.
	strcpy(serialout_buff, "?Z\r");
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	usleep(25000);
	
	memset(serialin_buff, 0, SERIAL_BUFF_SIZE);
	ret = read(roboteq_fd, serialin_buff, SERIAL_BUFF_SIZE-1);
	serialin_buff[SERIAL_BUFF_SIZE-1] = 0x00;
	ret = sscanf(serialin_buff, "%*2c %2X %2X", &speed1_value, &speed2_value);
	if (ret != 2) {
		return; // Well, best not to update the data without any data...
	}
	
	// Compute the speed (in m/s).
	speed1 = ((double) speed1_value) * speed_per_tick;
	speed2 = ((double) speed2_value) * speed_per_tick;
	
	if (invert_directions == true) {
		speed1 = -speed1;
		speed2 = -speed2;
	}
	
	speed_diff = speed1 - speed2;
	
	position_data->vel.pa = (speed_diff / turning_circumference) * (2.0 * M_PI);
	position_data->vel.px = 0.0;
	
	if (fabs(speed1) > fabs(speed2)) {
		position_data->vel.py = speed2;
	}
	else {
		position_data->vel.py = speed1;
	}
	
	// Compute a new position. We don't use the speed data.

	// Get the encoder counters. We use the relative mode.
	strcpy(serialout_buff, "?Q4\r");
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	usleep(25000);
	
	memset(serialin_buff, 0, SERIAL_BUFF_SIZE);
	ret = read(roboteq_fd, serialin_buff, SERIAL_BUFF_SIZE-1);
	serialin_buff[SERIAL_BUFF_SIZE-1] = 0x00;
	ret = sscanf(serialin_buff, "%*3c %X", &encoder1_count);
	if (ret != 1) {
		return;
	}
	
	strcpy(serialout_buff, "?Q5\r");
	write(roboteq_fd, serialout_buff, strlen(serialout_buff));
	tcdrain(roboteq_fd);
	usleep(25000);
	
	memset(serialin_buff, 0, SERIAL_BUFF_SIZE);
	ret = read(roboteq_fd, serialin_buff, SERIAL_BUFF_SIZE-1);
	serialin_buff[SERIAL_BUFF_SIZE-1] = 0x00;
	ret = sscanf(serialin_buff, "%*3c %X", &encoder2_count);
	if (ret != 1) {
		return;
	}
	
	// NOTE!!! There could be a bug here. If the driver is pre-empted or the delay
	// between reading encoder one and two is too long then there will be an error
	// in the computed position. It seems odd that Roboteq wouldn't provide a
	// command that would read out the values in both encoders simultaneously
	// but they don't. Solution, anyone?
	
	rpm1 = (((double) encoder1_count) / ((double) encoder_ppr)) / gear_ratio;
	rpm2 = (((double) encoder2_count) / ((double) encoder_ppr)) / gear_ratio;
	
	distance1 = rpm1 * wheel_circumference;
	distance2 = rpm2 * wheel_circumference;
	
	if (invert_directions == true) {
		distance1 = -distance1;
		distance2 = -distance2;
	}
	
	distance_diff = distance1 - distance2;
	
	// We make an approximation here. Technically, if the speed in the wheels was different
	// then the robot was turning in a circle. Which means I would have to try and calculate
	// the arc but I really don't want to so I assume the motion is linear. If the position is 
	// updated often enough this really shouldn't matter, but then again, why are you 
	// relying on this data to be accurate? I would also need an accurate time since the last
	// measurement which is rather difficult to obtain accurately (preemption, anyone?).
	
	if (fabs(distance1) > fabs(distance2)) {
		current_position.px += cos(current_position.pa) * distance2;
		current_position.py += sin(current_position.pa) * distance2;
	}
	else {
		current_position.px += cos(current_position.pa) * distance1;
		current_position.py += sin(current_position.pa) * distance1;
	}
	
	current_position.pa += (distance_diff / turning_circumference) * (2.0 * M_PI);
	current_position.pa = fmod(current_position.pa, (2.0 * M_PI)); // Constrain the angle to be within 2*pi.
	
	position_data->pos = current_position;
}

///////////////////////////////////////////////////////////////////////////
// Main driver thread runs here.
///////////////////////////////////////////////////////////////////////////
void roboteq::Main() {
	for (;;) {
		pthread_testcancel();
		ProcessMessages();
		
		// Update data
		UpdatePowerData(&(this->power_data));
		
		if (encoder_present != 0) {
			UpdatePositionData(&(this->position_data));
		}
		
		// Publish data
		this->Publish(position_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, 
			(unsigned char *) &position_data, sizeof(position_data), NULL);
		this->Publish(power_addr, PLAYER_MSGTYPE_DATA, PLAYER_POWER_DATA_STATE,
			(unsigned char *) &power_data, sizeof(power_data), NULL);
		
		usleep(25000);
	}
	  
	return;
}




