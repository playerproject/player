/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *
 *		David Feil-Seifer
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
 *
 * Driver for the "ER" robots, made by Evolution Robotics.    
 *
 * Some of this code is borrowed and/or adapted from the player
 * module of trogdor; thanks to the author of that module.
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#define PI 3.1415927

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */
#include <math.h>

#include <driver.h>
#include <drivertable.h>
#include <devicetable.h>
#include <player.h>

#include <sys/ioctl.h>
#include <asm/ioctls.h> /* not portable - fails in OS X  - rtv*/
#include "er_constants.h"

static void StopRobot(void* erdev);

class ER : public Driver
{
  private:
    // this function will be run in a separate thread
    virtual void Main();
    
    // bookkeeping
    bool fd_blocking;
	bool stopped;
	bool debug;
    double px, py, pa;  // integrated odometric pose (m,m,rad)
    int last_ltics, last_rtics;
    bool odom_initialized;
	bool need_to_set_speed;
	
	double axle_length;
	int motor_0_dir;
	int motor_1_dir;


    // methods for internal use
    int WriteBuf(unsigned char* s, size_t len);
    int ReadBuf(unsigned char* s, size_t len);
    int SendCommand(unsigned char * cmd, int cmd_len, unsigned char * ret_val, int ret_len);

	int InitOdom( );
    int GetOdom(int *ltics, int *rtics, int *lvel, int *rvel);
    void UpdateOdom(int ltics, int rtics);
    int GetBatteryVoltage(int* voltage);
	int GetRangeSensor( int s, float * val );
	void MotorSpeed(  );
	void SpeedCommand( unsigned char * cmd, double speed, int dir );
    float BytesToFloat(unsigned char *ptr);
    void Int32ToBytes(unsigned char* buf, int i);
    int ValidateChecksum(unsigned char *ptr, size_t len);
    unsigned char ComputeChecksum(unsigned char *ptr, size_t len);
	void InstToChars( int x, unsigned char * y, int l );
    int ComputeTickDiff(int from, int to);
    int ChangeMotorState(int state);
    int InitRobot();
    int BytesToInt32(unsigned char *ptr);
	unsigned char * GetRangeCode( int s );
	void Motor0ToMotor1( unsigned char * x );

	int *tc_num;
  public:
    int fd; // device file descriptor
    const char* serial_port; // name of dev file

    // public, so that it can be called from pthread cleanup function
    int SetVelocity(double lvel, double rvel);
	void Stop( int StopMode );

    ER(ConfigFile* cf, int section);

    virtual int Setup();
    virtual int Shutdown();
};


// initialization function
Driver* ER_Init(ConfigFile* cf, int section)
{
  return((Driver*)(new ER(cf, section)));
}

// a driver registration function
void 
ER_Register(DriverTable* table)
{
  table->AddDriver("er1", ER_Init);
}

ER::ER(ConfigFile* cf, int section) 
        : Driver(cf, section, PLAYER_POSITION_CODE, PLAYER_ALL_MODE,
                 sizeof(player_position_data_t),
                 sizeof(player_position_cmd_t),1,1)
{
  fd = -1;
  tc_num = new int[3];
  tc_num[0] = 2;
  tc_num[1] = 0;
  tc_num[2] = 185;
  this->serial_port = cf->ReadString(section, "port", ER_DEFAULT_PORT);
	need_to_set_speed = true;
	
  //read in axle radius
	axle_length = cf->ReadFloat( section, "axle", ER_DEFAULT_AXLE_LENGTH );

  //read in left motor and right motor direction
  	int dir = cf->ReadInt(section,"motor_dir", 1);
	motor_0_dir = dir * ER_DEFAULT_MOTOR_0_DIR;
	motor_1_dir = dir * ER_DEFAULT_MOTOR_1_DIR;

	debug = 1== cf->ReadInt( section, "debug", 0 );

}

int
ER::InitRobot()
{

	// initialize the robot
	unsigned char initstr[4];
	unsigned char buf[6];

	InstToChars( ER_MOTOR_0_INIT, initstr, 4 );

	usleep(ER_DELAY_US);
	if(WriteBuf(initstr,sizeof(initstr)) < 0)
	{
		PLAYER_WARN("failed to initialize robot");
		return -1;
	}
	usleep(ER_DELAY_US);
	if(ReadBuf(buf,6) < 0)
	{
		PLAYER_ERROR("failed to read status");
		return(-1);
	}
	/* TODO check return value to match 0x00A934100013 */
	Motor0ToMotor1( initstr );
	if(WriteBuf(initstr,sizeof(initstr)) < 0)
	{
		PLAYER_WARN("failed to initialize robot");
		return -1;
	}
	usleep(ER_DELAY_US);
	if(ReadBuf(buf,6) < 0)
	{
		PLAYER_ERROR("failed to read status");
		return(-1);
	}
	/* TODO check return value to match 0x00A934100013 */

	tc_num[2] = 25;
	stopped = true;
	return(0);
}

int 
ER::Setup()
{

	struct termios term;
	int flags;
//	int ltics,rtics,lvel,rvel;
	player_position_cmd_t cmd;
	player_position_data_t data;

	cmd.xpos = cmd.ypos = cmd.yaw = 0;
	cmd.xspeed = cmd.yspeed = cmd.yawspeed = 0;
	data.xpos = data.ypos = data.yaw = 0;
	data.xspeed = data.yspeed = data.yawspeed = 0;

	this->px = this->py = this->pa = 0.0;
	this->odom_initialized = false;

	printf("Evolution Robotics evolution_rcm connection initializing (%s)...", serial_port);
	fflush(stdout);

	// open it.  non-blocking at first, in case there's no robot
	if((this->fd = open(serial_port, O_RDWR | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 )
	{
		PLAYER_ERROR1("open() failed: %s", strerror(errno));
		return(-1);
	}  
 
	if(tcgetattr(this->fd, &term) < 0 )
	{
    	PLAYER_ERROR1("tcgetattr() failed: %s", strerror(errno));
		close(this->fd);
		this->fd = -1;
		return(-1);
	}

	cfmakeraw( &term );
	cfsetispeed(&term, B230400);
	cfsetospeed(&term, B230400);

	if(tcsetattr(this->fd, TCSADRAIN, &term) < 0 )
	{
		PLAYER_ERROR1("tcsetattr() failed: %s", strerror(errno));
		close(this->fd);
		this->fd = -1;
		return(-1);
	}

	fd_blocking = false;
	if(InitRobot() < 0)
	{
		PLAYER_ERROR("failed to initialize robot");
		close(this->fd);
		this->fd = -1;
		return(-1);
	}

	/* ok, we got data, so now set NONBLOCK, and continue */
	if((flags = fcntl(this->fd, F_GETFL)) < 0)
	{
		PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
		close(this->fd);
		this->fd = -1;
		return(-1);
	}

	if(fcntl(this->fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
	{
		PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
		close(this->fd);
		this->fd = -1;
		return(-1);
	}
	fd_blocking = true;
	puts("Done.");

	/*  This might be a good time to reset the odometry values */
	if( InitOdom() < 0 ) {
		PLAYER_ERROR("InitOdom failed" );
		close(this->fd);
		this->fd = -1;
		return -1;
	}

	// zero the command buffer
	PutCommand(this->device_id,(unsigned char*)&cmd,sizeof(cmd),NULL);
	PutData((unsigned char*)&data,sizeof(data),NULL);

	// start the thread to talk with the robot
	StartThread();

	return(0);
}

int
ER::Shutdown()
{

  if(this->fd == -1)
    return(0);

  StopThread();

  // the robot is stopped by the thread cleanup function StopRobot(), which
  // is called as a result of the above StopThread()
  
  if(SetVelocity(0,0) < 0)
    PLAYER_ERROR("failed to stop robot while shutting down");
  

  usleep(ER_DELAY_US);

  if(close(this->fd))
    PLAYER_ERROR1("close() failed:%s",strerror(errno));
  this->fd = -1;
  puts("ER has been shutdown");
  
  return(0);
}

void 
ER::Main()
{

	player_position_cmd_t command;
	player_position_data_t data;
	double last_final_lvel, last_final_rvel;
	int rtics, ltics, lvel, rvel;
	last_final_rvel = last_final_lvel = 0;
	char config[256];
	int config_size;
	void* client;
	double lvel_mps, rvel_mps;
	double command_rvel, command_lvel;
	double final_lvel = 0, final_rvel = 0;
	double rotational_term;
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

	// push a pthread cleanup function that stops the robot
	pthread_cleanup_push(StopRobot,this);

	for(;;)
	{
		pthread_testcancel();
    
		/* position command */

		GetCommand((void*)&command, sizeof(player_position_cmd_t),
                           NULL);
		command.yawspeed = ntohl(command.yawspeed) / 1;
		command.xspeed = ntohl(command.xspeed) / 1;
		
		// convert (tv,rv) to (lv,rv) and send to robot
		rotational_term = (command.yawspeed * PI / 180.0) * (axle_length*1000.0) / 2.0;
		command_rvel = (command.xspeed) + rotational_term;
		command_lvel = (command.xspeed) - rotational_term;
    
		// sanity check on per-wheel speeds
		if(fabs(command_lvel) > ER_MAX_WHEELSPEED) {
			if(command_lvel > 0) {
				command_lvel = ER_MAX_WHEELSPEED;
				command_rvel *= ER_MAX_WHEELSPEED/command_lvel;
			}
			else {
				command_lvel = - ER_MAX_WHEELSPEED;
				command_rvel *= -ER_MAX_WHEELSPEED/command_lvel;
			}
		}
		if(fabs(command_rvel) > ER_MAX_WHEELSPEED) {
			if(command_rvel > 0) {
				command_rvel = ER_MAX_WHEELSPEED;
				command_lvel *= ER_MAX_WHEELSPEED/command_rvel;
			}
			else {
				command_rvel = - ER_MAX_WHEELSPEED;
				command_lvel *= -ER_MAX_WHEELSPEED/command_rvel;
			}
		}

		final_lvel = command_lvel;
		final_rvel = command_rvel;

		// TODO: do this min threshold smarter, to preserve desired travel 
		// direction


		if((final_lvel != last_final_lvel) ||
			(final_rvel != last_final_rvel)) {

			if( final_lvel * last_final_lvel < 0 || final_rvel * last_final_rvel < 0 ) {
//				printf( "Changing motor direction, soft stop\n" );
				SetVelocity(0,0);
			}

			if(SetVelocity(final_lvel/10.0,final_rvel/10.0) < 0) {
				PLAYER_ERROR("failed to set velocity");
				pthread_exit(NULL);
			}
			last_final_lvel = final_lvel;
			last_final_rvel = final_rvel;
			MotorSpeed();
		}

		/* get battery voltage */

/*

		TODO:
			get hex to voltage to percentage conversion function

    	int volt;
	    if(GetBatteryVoltage(&volt) < 0) {
			PLAYER_WARN("failed to get voltage");
		}
		else printf("volt: %d\n", volt);

*/
	
		/* Get the 13 range sensor values */

/*

		TODO: 	read from config file to judge where the IRS are and what
			type they are
			
				hex => raw => distance for each type

		float val;
		for( int i = 0; i < 13; i++ ) {
			if( GetRangeSensor( i, &val ) < 0) {
				PLAYER_WARN("failed to get range sensor");
			}
		} 

*/

		/* get the odometry values */
		GetOdom( &ltics, &rtics, &lvel, &rvel );
		UpdateOdom( ltics, rtics );


		double tmp_angle;
		data.xpos = htonl((int32_t)rint(this->px * 1e3));
		data.ypos = htonl((int32_t)rint(this->py * 1e3));
		if(this->pa < 0)
			tmp_angle = this->pa + 2*M_PI;
		else
			tmp_angle = this->pa;

		data.yaw = htonl((int32_t)floor(RTOD(tmp_angle)));

		data.yspeed = 0;
		lvel_mps = lvel * ER_MPS_PER_TICK;
		rvel_mps = rvel * ER_MPS_PER_TICK;
		data.xspeed = htonl((int32_t)rint(1e3 * (lvel_mps + rvel_mps) / 2.0));
		data.yawspeed = htonl((int32_t)rint(RTOD((rvel_mps-lvel_mps) / 
                                             axle_length)));

		PutData((void*)&data,sizeof(data),NULL);

		player_position_power_config_t* powercfg;

		if((config_size = GetConfig(&client,(void*)config,
                                            sizeof(config),NULL)) > 0)
		{
			switch(config[0])
			{
				case PLAYER_POSITION_GET_GEOM_REQ:
				/* Return the robot geometry. */
					if(config_size != 1)
					{
						PLAYER_WARN("Get robot geom config is wrong size; ignoring");
						if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
							PLAYER_ERROR("failed to PutReply");
						break;
					}

					//TODO : get values from somewhere.
					player_position_geom_t geom;
					geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
					geom.pose[0] = htons((short) (0));
					geom.pose[1] = htons((short) (0));
					geom.pose[2] = htons((short) (0));
					geom.size[0] = htons((short) (450));
					geom.size[1] = htons((short) (450));

					if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &geom, sizeof(geom),NULL))
					PLAYER_ERROR("failed to PutReply");
					break;
				case PLAYER_POSITION_MOTOR_POWER_REQ:
					// NOTE: this doesn't seem to actually work
					if(config_size != sizeof(player_position_power_config_t))
					{
						PLAYER_WARN("Motor state change request wrong size; ignoring");
						if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
							PLAYER_ERROR("failed to PutReply");
						break;
					}
					powercfg = (player_position_power_config_t*)config;
					printf("got motor power req: %d\n", powercfg->value);
					if(ChangeMotorState(powercfg->value) < 0)
					{
						if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
							PLAYER_ERROR("failed to PutReply");
					}
					else
					{
						if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
							PLAYER_ERROR("failed to PutReply");
					}
					break;

				default:
					PLAYER_WARN1("received unknown config type %d\n", config[0]);
					PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL);
			} /* switch */
		} /* if */

	    usleep(ER_DELAY_US);

	} /* for */
	pthread_cleanup_pop(1);
  
}

int
ER::ReadBuf(unsigned char* s, size_t len)
{

	int thisnumread;
	size_t numread = 0;

  while(numread < len)
  {
//  	printf( "len=%d numread=%d\n", len, numread );
    if((thisnumread = read(this->fd,s+numread,len-numread)) < 0)
    {
      PLAYER_ERROR1("read() failed: %s", strerror(errno));
      return(-1);
    }
    if(thisnumread == 0)
      PLAYER_WARN("short read");
    numread += thisnumread;
  }
  
//  printf("read:\n");
//  for(size_t i=0;i<numread;i++) printf("0x%02x\n", s[i]);
  
  return(0);
}

int
ER::WriteBuf(unsigned char* s, size_t len)
{

  size_t numwritten;
  int thisnumwritten;
  numwritten=0;
  while(numwritten < len)
  {
    if((thisnumwritten = write(this->fd,s+numwritten,len-numwritten)) < 0)
    {
      if(!this->fd_blocking && errno == EAGAIN)
      {
        usleep(ER_DELAY_US);
        continue;
      }
      PLAYER_ERROR1("write() failed: %s", strerror(errno));
      return(-1);
    }
    numwritten += thisnumwritten;
  }
 
	ioctl( this->fd, TIOCMSET, tc_num );
	if( tc_num[0] == 2 ) { tc_num[0] = 0; }
	else { tc_num[0] = 2; }

  return 0;
}

int 
ER::BytesToInt32(unsigned char *ptr)
{
  unsigned char char0,char1,char2,char3;
  int data = 0;

  char0 = ptr[3];
  char1 = ptr[2];
  char2 = ptr[1];
  char3 = ptr[0];

  data |=  ((int)char0)        & 0x000000FF;
  data |= (((int)char1) << 8)  & 0x0000FF00;
  data |= (((int)char2) << 16) & 0x00FF0000;
  data |= (((int)char3) << 24) & 0xFF000000;

  return data;
}

float 
ER::BytesToFloat(unsigned char *ptr)
{
//  unsigned char char0,char1,char2,char3;
  float data = 0;

	sscanf( (const char *) ptr, "%f", &data );

  return data;
}

int
ER::GetBatteryVoltage(int* voltage)
{
	
	unsigned char buf[6];
	unsigned char ret[4];
	
	InstToChars( ER_GET_VOLTAGE_LOW, buf, 4 );
	InstToChars( ER_GET_VOLTAGE_HIGH, &(buf[4]), 2 );

//	puts("sending for voltage");
	SendCommand( buf, 6, ret, 4 );
  
// 	printf( "voltage?: %f\n", (float) BytesToFloat(buf) );
  
	return(0);
}

int
ER::GetRangeSensor(int s, float * val )
{
	
	unsigned char * buf = GetRangeCode( s );
	unsigned char ret[4];

//	printf("sending for range value for sensor: %d\n", s );
	if( SendCommand( buf, 6, ret, 4 ) < 0 ) {
		PLAYER_ERROR( "ERROR on send command" );
		return -1;
	}
	
	/* this is definately wrong, need to fix this */
	float range = (float) BytesToFloat( buf );
//	printf( "sensor value?: %d\n", s );
	val = &range;

	return 0;
}

void
ER::Int32ToBytes(unsigned char* buf, int i)
{
  buf[3] = (i >> 0)  & 0xFF;
  buf[2] = (i >> 8)  & 0xFF;
  buf[1] = (i >> 16) & 0xFF;
  buf[0] = (i >> 24) & 0xFF;
}

int
ER::GetOdom(int *ltics, int *rtics, int *lvel, int *rvel)
{

	unsigned char buf[4];
	unsigned char ret[6];

	/* motor 0 */
	InstToChars( ER_ODOM_PROBE, buf, 4 );
	SendCommand( buf, 4, ret, 6 );
	*ltics = motor_0_dir * BytesToInt32(&(ret[2]));

	int diff = BytesToInt32( &(ret[2]) );


//	printf( "0x%08x\n", diff);

	/* motor 1 */
	InstToChars( ER_ODOM_PROBE, buf, 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 4, ret, 6 );
	*rtics = motor_1_dir * BytesToInt32(&(ret[2]));

	diff = BytesToInt32( &(ret[2]) );

//	printf( "0x%08x\n", diff);
	/* hmmm, what to do here ??? */
	/*
	index += 4;
	*rvel = BytesToInt32(buf+index);
	index += 4;
	*lvel = BytesToInt32(buf+index);
	*/
	
//	printf("ltics: %d rtics: %d\n", *ltics, *rtics);

//	puts("got good odom packet");

	return(0);
}

int
ER::InitOdom()
{
	/*function to send all the motor commands to reset the wheel ticks */
	
	unsigned char buf[8];
	unsigned char ret[8];
	/* Start with motor 0*/
	InstToChars( ER_MOTOR_0_INIT, buf, 4 );
	SendCommand( buf, 4, ret, 6 );
	/* TODO check that ret value matches 0x00A934100013 */
	
	InstToChars( ER_ODOM_RESET_1, buf, 4 );
	SendCommand( buf, 4, ret, 2 );
	/* TODO check that this value matches 0x01FF */
	
	InstToChars( ER_ODOM_RESET_2_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_2_2, &(buf[4]), 2 );
	SendCommand( buf, 6, ret, 2 );
	
	InstToChars( ER_ODOM_RESET_3_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_3_2, &(buf[4]), 2 );
	SendCommand( buf, 6, ret, 2 );
	
	InstToChars( ER_ODOM_RESET_4_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_4_2, &(buf[4]), 2 );
	SendCommand( buf, 6, ret, 2 );
	
	InstToChars( ER_ODOM_RESET_5_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_5_2, &(buf[4]), 4 );
	SendCommand( buf, 8, ret, 2 );

	InstToChars( ER_ODOM_RESET_6_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_6_2, &(buf[4]), 4 );
	SendCommand( buf, 8, ret, 2 );

	InstToChars( ER_ODOM_RESET_7_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_7_2, &(buf[4]), 4 );
	SendCommand( buf, 8, ret, 2 );

	InstToChars( ER_MOTOR_EXECUTE_1, buf, 4 );
	SendCommand( buf, 4, ret, 2 );


	/* Translate for motor 1 */
	InstToChars( ER_MOTOR_0_INIT, buf, 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 4, ret, 6 );
	/* TODO check that this value matches 0x00A934100013 */
	
	InstToChars( ER_ODOM_RESET_1, buf, 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 4, ret, 2 );
	/* TODO check that this value matches 0x01FF */
	
	InstToChars( ER_ODOM_RESET_2_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_2_2, &(buf[4]), 2 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 6, ret, 2 );
	
	InstToChars( ER_ODOM_RESET_3_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_3_2, &(buf[4]), 2 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 6, ret, 2 );
	
	InstToChars( ER_ODOM_RESET_4_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_4_2, &(buf[4]), 2 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 6, ret, 2 );
	
	InstToChars( ER_ODOM_RESET_5_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_5_2, &(buf[4]), 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 8, ret, 2 );

	InstToChars( ER_ODOM_RESET_6_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_6_2, &(buf[4]), 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 8, ret, 2 );

	InstToChars( ER_ODOM_RESET_7_1, buf, 4 );
	InstToChars( ER_ODOM_RESET_7_2, &(buf[4]), 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 8, ret, 2 );

	InstToChars( ER_MOTOR_EXECUTE_1, buf, 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 4, ret, 2 );	
	
	last_ltics = 0;
	last_rtics = 0;
	
	return 0;
}

int 
ER::ComputeTickDiff(int from, int to) 
{
  int diff1, diff2;

  // find difference in two directions and pick shortest
  if(to > from) 
  {
    diff1 = to - from;
    diff2 = (-ER_MAX_TICKS - from) + (to - ER_MAX_TICKS);
  }
  else 
  {
    diff1 = to - from;
    diff2 = (from - ER_MAX_TICKS) + (-ER_MAX_TICKS - to);
  }

  if(abs(diff1) < abs(diff2)) 
    return(diff1);
  else
    return(diff2);
	

	return 0;
}

void
ER::UpdateOdom(int ltics, int rtics)
{
	
	int ltics_delta, rtics_delta;
	double l_delta, r_delta, a_delta, d_delta;
	int max_tics;
	static struct timeval lasttime;
	struct timeval currtime;
	double timediff;

	if(!this->odom_initialized)
	{
		this->last_ltics = ltics;
		this->last_rtics = rtics;
		gettimeofday(&lasttime,NULL);
		this->odom_initialized = true;
		return;
	}
  

//	ltics_delta = ComputeTickDiff(last_ltics,ltics);
//	rtics_delta = ComputeTickDiff(last_rtics,rtics);

	ltics_delta = ltics - this->last_ltics;
	rtics_delta = rtics - this->last_rtics;

  // mysterious rollover code borrowed from CARMEN
/* 
  if(ltics_delta > SHRT_MAX/2)
    ltics_delta += SHRT_MIN;
  if(ltics_delta < -SHRT_MIN/2)
    ltics_delta -= SHRT_MIN;
  if(rtics_delta > SHRT_MAX/2)
    rtics_delta += SHRT_MIN;
  if(rtics_delta < -SHRT_MIN/2)
    rtics_delta -= SHRT_MIN;
*/

  gettimeofday(&currtime,NULL);
  timediff = (currtime.tv_sec + currtime.tv_usec/1e6)-
             (lasttime.tv_sec + lasttime.tv_usec/1e6);
  max_tics = (int)rint(ER_MAX_WHEELSPEED / ER_M_PER_TICK / timediff);
  lasttime = currtime;
	
	if( debug ) {
	  printf("ltics: %d\trtics: %d\n", ltics,rtics);
	  printf("ldelt: %d\trdelt: %d\n", ltics_delta, rtics_delta);
	}
  //printf("maxtics: %d\n", max_tics);

  if(abs(ltics_delta) > max_tics || abs(rtics_delta) > max_tics)
  {
    PLAYER_WARN("Invalid odometry change (too big); ignoring");
    return;
  }

  l_delta = ltics_delta * ER_M_PER_TICK;
  r_delta = rtics_delta * ER_M_PER_TICK;


  a_delta = (r_delta - l_delta) / ( axle_length );
  d_delta = (l_delta + r_delta) / 2.0;

  this->px += d_delta * cos(this->pa + (a_delta / 2));
  this->py += d_delta * sin(this->pa + (a_delta / 2));
  this->pa += a_delta;
  this->pa = NORMALIZE(this->pa);
  
	if( debug ) {
		printf("er: pose: %f,%f,%f\n", this->px,this->py,this->pa * 180.0 / PI);
	}
  this->last_ltics = ltics;
  this->last_rtics = rtics;
}

void
ER::Stop( int StopMode ) {
	
	unsigned char buf[8];
	unsigned char ret[8];
	
	/* Start with motor 0*/
	if( StopMode == FULL_STOP ) {

		InstToChars( ER_STOP_1_1, buf, 4 );
		InstToChars( ER_STOP_1_2, &(buf[4]), 2 );
		SendCommand( buf, 6, ret, 2 );
	}
	else {
		
		InstToChars( ER_SET_ACCL_2_1, buf, 4 );
		InstToChars( ER_SET_ACCL_2_2, &(buf[4]), 2 );
		SendCommand( buf, 6, ret, 2 );
		
	}
	if( StopMode == FULL_STOP ) {

		InstToChars( ER_STOP_2_1, buf, 4 );
		InstToChars( ER_STOP_2_2, &(buf[4]), 2 );
		SendCommand( buf, 6, ret, 2 );
	
		InstToChars( ER_STOP_3_1, buf, 4 );
		InstToChars( ER_STOP_3_2, &(buf[4]), 2 );
		SendCommand( buf, 6, ret, 2 );

		InstToChars( ER_STOP_4_1, buf, 4 );
		InstToChars( ER_STOP_4_2, &(buf[4]), 2 );
		SendCommand( buf, 6, ret, 2 );

	}

	InstToChars( ER_STOP_5_1, buf, 4 );
	InstToChars( ER_STOP_5_2, &(buf[4]), 4 );
	SendCommand( buf, 8, ret, 2 );

	if( StopMode == FULL_STOP ) {

		InstToChars( ER_STOP_6_1, buf, 4 );
		InstToChars( ER_STOP_6_2, &(buf[4]), 4 );
		SendCommand( buf, 8, ret, 2 );

		InstToChars( ER_STOP_7_1, buf, 4 );
		InstToChars( ER_STOP_7_2, &(buf[4]), 4 );
		SendCommand( buf, 8, ret, 2 );

	}

	InstToChars( ER_MOTOR_EXECUTE_1, buf, 4 );
	SendCommand( buf, 4, ret, 2 );

	InstToChars( ER_STOP_8_1, buf, 4 );
	SendCommand( buf, 4, ret, 2 );

	/* now motor 1*/
	if( StopMode == FULL_STOP ) {

		InstToChars( ER_STOP_1_1, buf, 4 );
		InstToChars( ER_STOP_1_2, &(buf[4]), 2 );
		Motor0ToMotor1( buf );
		SendCommand( buf, 6, ret, 2 );

	}
	else {
		
		InstToChars( ER_SET_ACCL_2_1, buf, 4 );
		InstToChars( ER_SET_ACCL_2_2, &(buf[4]), 2 );
		Motor0ToMotor1( buf );
		SendCommand( buf, 6, ret, 2 );
	
	}

	if( StopMode == FULL_STOP ) {

		InstToChars( ER_STOP_2_1, buf, 4 );
		InstToChars( ER_STOP_2_2, &(buf[4]), 2 );
		Motor0ToMotor1( buf );
		SendCommand( buf, 6, ret, 2 );

		InstToChars( ER_STOP_3_1, buf, 4 );
		InstToChars( ER_STOP_3_2, &(buf[4]), 2 );
		Motor0ToMotor1( buf );
		SendCommand( buf, 6, ret, 2 );

		InstToChars( ER_STOP_4_1, buf, 4 );
		InstToChars( ER_STOP_4_2, &(buf[4]), 2 );
		Motor0ToMotor1( buf );
		SendCommand( buf, 6, ret, 2 );

	}

	InstToChars( ER_STOP_5_1, buf, 4 );
	InstToChars( ER_STOP_5_2, &(buf[4]), 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 8, ret, 2 );

	if( StopMode == FULL_STOP ) {

		InstToChars( ER_STOP_6_1, buf, 4 );
		InstToChars( ER_STOP_6_2, &(buf[4]), 4 );
		Motor0ToMotor1( buf );
		SendCommand( buf, 8, ret, 2 );

		InstToChars( ER_STOP_7_1, buf, 4 );
		InstToChars( ER_STOP_7_2, &(buf[4]), 4 );
		Motor0ToMotor1( buf );
		SendCommand( buf, 8, ret, 2 );

	}

	InstToChars( ER_MOTOR_EXECUTE_1, buf, 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 4, ret, 2 );

	InstToChars( ER_STOP_8_1, buf, 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 4, ret, 2 );
	
}

void
ER::MotorSpeed() {
	
	unsigned char buf[8];
	unsigned char ret[8];
	
	/* Start with motor 0*/
	InstToChars( ER_SET_ACCL_1, buf, 4 );
	SendCommand( buf, 4, ret, 4 );

	InstToChars( ER_SET_ACCL_2_1, buf, 4 );
	InstToChars( ER_SET_ACCL_2_2, &(buf[4]), 2 );
	SendCommand( buf, 6, ret, 2 );

	InstToChars( ER_SET_ACCL_3_1, buf, 4 );
	InstToChars( ER_SET_ACCL_3_2, &(buf[4]), 4 );
	SendCommand( buf, 8, ret, 2 );

	/* now motor 1*/
	InstToChars( ER_SET_ACCL_1, buf, 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 4, ret, 4 );

	InstToChars( ER_SET_ACCL_2_1, buf, 4 );
	InstToChars( ER_SET_ACCL_2_2, &(buf[4]), 2 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 6, ret, 2 );

	InstToChars( ER_SET_ACCL_3_1, buf, 4 );
	InstToChars( ER_SET_ACCL_3_2, &(buf[4]), 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 8, ret, 2 );

	/* send execute command */
	InstToChars( ER_MOTOR_EXECUTE_1, buf, 4 );
	SendCommand( buf, 4, ret, 2 );

	InstToChars( ER_MOTOR_EXECUTE_1, buf, 4 );
	Motor0ToMotor1( buf );
	SendCommand( buf, 4, ret, 2 );
	
}


// Validate XOR checksum
int
ER::ValidateChecksum(unsigned char *ptr, size_t len)
{
	return 0;
}

// Compute XOR checksum
unsigned char
ER::ComputeChecksum(unsigned char *ptr, size_t len)
{
  	return (unsigned char) 0;
}

int
ER::SendCommand(unsigned char * cmd, int cmd_len, unsigned char * ret_val, int ret_len )
{
	if(WriteBuf(cmd,cmd_len) < 0)
	{
		PLAYER_ERROR("failed to send command");
		return(-1);
	}
	if( ret_len > 0 ) {
		if(ReadBuf(ret_val,ret_len) < 0)
		{
			PLAYER_ERROR("failed to read");
			return(-1);
		}
	}
	
	return(0);
}

int
ER::SetVelocity(double lvel, double rvel)
{

	unsigned char buf[8];
	unsigned char ret[8];

//	printf( "lvel: %f rvel: %f\n", lvel, rvel );

		InstToChars( ER_SET_SPEED_1, buf, 4 );
		SendCommand( buf, 4, ret, 4 );

		if( stopped ) {
			InstToChars( ER_SET_SPEED_2_1, buf, 4 );
			InstToChars( ER_SET_SPEED_2_2, &(buf[4]), 2 );
			SendCommand( buf, 6, ret, 2 );

			InstToChars( ER_SET_SPEED_3_1, buf, 4 );
			InstToChars( ER_SET_SPEED_3_2, &(buf[4]), 2 );
			SendCommand( buf, 6, ret, 2 );
	
			InstToChars( ER_SET_SPEED_4_1, buf, 4 );
			InstToChars( ER_SET_SPEED_4_2, &(buf[4]), 2 );
			SendCommand( buf, 6, ret, 2 );
		}
		else {

			InstToChars( ER_SET_ACCL_2_1, buf, 4 );
			InstToChars( ER_SET_ACCL_2_2, &(buf[4]), 2 );
			SendCommand( buf, 6, ret, 2 );

		}
		
		/* compute command 5 */
		SpeedCommand( buf, lvel, motor_0_dir );
		/* send command 5 */
		SendCommand( buf, 8, ret, 2 );
		
		InstToChars( ER_SET_SPEED_6_1, buf, 4 );
		InstToChars( ER_SET_SPEED_6_2, &(buf[4]), 4 );
		SendCommand( buf, 8, ret, 2 );


		/* motor 1 */
	
		InstToChars( ER_SET_SPEED_1, buf, 4 );
		Motor0ToMotor1( buf );
		SendCommand( buf, 4, ret, 4 );
	
		if( stopped ) {
			InstToChars( ER_SET_SPEED_2_1, buf, 4 );
			InstToChars( ER_SET_SPEED_2_2, &(buf[4]), 2 );
			Motor0ToMotor1( buf );
			SendCommand( buf, 6, ret, 2 );
			stopped = false;

			InstToChars( ER_SET_SPEED_3_1, buf, 4 );
			InstToChars( ER_SET_SPEED_3_2, &(buf[4]), 2 );
			Motor0ToMotor1( buf );
			SendCommand( buf, 6, ret, 2 );
	
			InstToChars( ER_SET_SPEED_4_1, buf, 4 );
			InstToChars( ER_SET_SPEED_4_2, &(buf[4]), 2 );
			Motor0ToMotor1( buf );
			SendCommand( buf, 6, ret, 2 );
		

		}
		else {
			InstToChars( ER_SET_ACCL_2_1, buf, 4 );
			InstToChars( ER_SET_ACCL_2_2, &(buf[4]), 2 );
			Motor0ToMotor1( buf );
			SendCommand( buf, 6, ret, 2 );
		}
	
		/* compute command 5 */
		SpeedCommand( buf, rvel, motor_1_dir );
		/* send command 5 */
		Motor0ToMotor1( buf );
		SendCommand( buf, 8, ret, 2 );
	
		InstToChars( ER_SET_SPEED_6_1, buf, 4 );
		InstToChars( ER_SET_SPEED_6_2, &(buf[4]), 4 );
		Motor0ToMotor1( buf );
		SendCommand( buf, 8, ret, 2 );
		
	
		InstToChars( ER_MOTOR_EXECUTE_1, buf, 4 );
		SendCommand( buf, 4, ret, 2 );

		InstToChars( ER_MOTOR_EXECUTE_1, buf, 4 );
		Motor0ToMotor1( buf );
		SendCommand( buf, 4, ret, 2 );

	return(0);
}

void
ER::SpeedCommand(unsigned char * cmd, double speed, int dir ) {
	cmd[0] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x11;
	
	int whole = dir * (int) (speed * 16819.8);
	unsigned int frac = 0x100;
	
	Int32ToBytes( &(cmd[4]), whole );

	/* sets the checksum value */
	frac -= cmd[3];
	frac -= cmd[4];
	frac -= cmd[5];
	frac -= cmd[6];
	frac -= cmd[7];
	cmd[1] = (unsigned char) frac;
//	printf( "speed: %f checksum: 0x%02x value: 0x%08x\n", speed, cmd[1], whole );
	
}

static void
StopRobot(void* erdev)
{
  ER* er = (ER*)erdev;

//  if(er->Stop( FULL_STOP ) < 0)
//    PLAYER_ERROR("failed to stop robot on thread exit");

	er->Stop(FULL_STOP );
}

unsigned char *
ER::GetRangeCode( int s ) {
	unsigned char * x;
	x = new unsigned char[6];
	int a = s / 8;
	int b = s % 8;
	x[0] = a;
	x[1] = 17 - b - a;
	x[2] = 0x00;
	x[3] = 0xEF;
	x[4] = 0x00;
	x[5] = b;
	return x;
}

void
ER::InstToChars( int x, unsigned char * y, int l ) {
	unsigned char * z = (unsigned char *) &x;
	for( int i = 0; i < l; i++ ) {
		y[i] = z[l-i-1];
	}
}

void
ER::Motor0ToMotor1( unsigned char * x ) {
	x[0]++;
	x[1]--;
}

int 
ER::ChangeMotorState(int state)
{
/*
  unsigned char buf[1];
  if(state)
    buf[0] = TROGDOR_ENABLE_VEL_CONTROL;
  else
    buf[0] = TROGDOR_DISABLE_VEL_CONTROL;
  return(WriteBuf(buf,sizeof(buf)));
*/
	return 0;
}
