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
 * client-side position device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
    
// send a motor command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PositionProxy::SetSpeed(int speed, int turnrate)
{
  if(!client)
    return(-1);

  player_position_cmd_t cmd;

  cmd.xspeed = (int)htonl(speed);
  cmd.yawspeed = (int)htonl(turnrate);

  return(client->Write(PLAYER_POSITION_CODE,index,
                       (const char*)&cmd,sizeof(cmd)));
}

/* sets the desired heading to theta, with the translational
 * and rotational velocity contraints xspeed and yawspeed, respectively
 *
 * returns: 0 if ok, -1 else
 */
int
PositionProxy::DoDesiredHeading(short theta, int xspeed, int yawspeed)
{
  if (!client) {
    return -1;
  }

  player_position_cmd_t cmd;

  // the desired heading is the yaw member
  cmd.yaw = htons(theta);
  
  // set velocity constraints
  cmd.xspeed = htons(xspeed);
  cmd.yawspeed = htons(yawspeed);

  return client->Write(PLAYER_POSITION_CODE, index, 
		       (const char *)&cmd, sizeof(cmd));
}
  

/* if the robot is in position mode, this will make it perform
 * a straightline translation by trans mm. (negative values will be backwards)
 * undefined effect if in velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int
PositionProxy::DoStraightLine(int trans)
{
  if (!client) {
    return -1;
  }

  player_position_cmd_t cmd;

  // we send a no movement pos command first so that 
  // the real pos command will look new.  sort of a hack FIX
  cmd.xspeed = 0;
  cmd.yawspeed = 0;
  cmd.yaw = 0;

  client->Write(PLAYER_POSITION_CODE, index, 
		(const char *)&cmd, sizeof(cmd));

  // now we send the real pos command

  short t = (short) trans;
  cmd.xspeed = htons(t);

  return client->Write(PLAYER_POSITION_CODE, index, 
			 (const char *)&cmd, sizeof(cmd));
}

/* if in position mode, this will cause a turn in place rotation of
 * rot degrees.  
 * undefined effect in velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int
PositionProxy::DoRotation(int rot)
{
  if (!client) {
    return -1;
  }

  player_position_cmd_t cmd;

  // as before, send a fake pos command first so the
  // real one will be flagged as new

  cmd.xspeed = 0;
  cmd.yawspeed = 0;
  cmd.yaw = 0;

  client->Write(PLAYER_POSITION_CODE, index,
		(const char *)&cmd, sizeof(cmd));

  short r = (short) rot;
  cmd.yawspeed = htons(r);

  return client->Write(PLAYER_POSITION_CODE, index, 
			 (const char *)&cmd, sizeof(cmd));
}

// enable/disable the motors
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PositionProxy::SetMotorState(unsigned char state)
{
  if(!client)
    return(-1);

  player_position_power_config_t config;

  config.request = PLAYER_POSITION_MOTOR_POWER_REQ;
  config.value = state;


  return(client->Request(PLAYER_POSITION_CODE,index,(const char*)&config,
                         sizeof(config)));
}

// select velocity control mode for the Pioneer 2
//   0 = direct wheel velocity control (default)
//   1 = separate translational and rotational control
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PositionProxy::SelectVelocityControl(unsigned char mode)
{
  if(!client)
    return(-1);

  player_position_velocitymode_config_t config;

  config.request = PLAYER_POSITION_VELOCITY_MODE_REQ;
  config.value = mode;

  return(client->Request(PLAYER_POSITION_CODE,index,(const char*)&config,
                         sizeof(config)));
}

// reset odometry to (0,0,0)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PositionProxy::ResetOdometry()
{
  if(!client)
    return(-1);

  player_position_resetodom_config_t config;

  config.request = PLAYER_POSITION_RESET_ODOM_REQ;

  return(client->Request(PLAYER_POSITION_CODE,index,(const char*)&config,
                         sizeof(config)));
}

/* select the kind of velocity control to perform
 * 1 for position mode
 * 0 for velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int
PositionProxy::SelectPositionMode(unsigned char mode)
{
  if (!client) {
    return -1;
  }

  player_position_position_mode_req_t req;

  req.subtype = PLAYER_POSITION_POSITION_MODE_REQ;
  req.state = mode;

  return client->Request(PLAYER_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* Set odometry to the gicen pose.  
 *
 * returns: 0 if ok, -1 else
 */
int
PositionProxy::SetOdometry(long x, long y,int t)
{
  if (!client) {
    return -1;
  }
  player_position_set_odom_req_t req;

  req.subtype = PLAYER_POSITION_SET_ODOM_REQ;
  req.x = htonl( x);
  req.y = htonl( y);
  t %= 360;
  t=(uint16_t) (t < 0 ? t+360 : t);
  req.theta = htons(t);
  return client->Request(PLAYER_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* set the PID for the speed controller
 *
 * returns: 0 if ok, -1 else
 */
int
PositionProxy::SetSpeedPID(int kp, int ki, int kd)
{
  if (!client) {
    return -1;
  }

  player_position_speed_pid_req_t req;

  req.subtype = PLAYER_POSITION_SPEED_PID_REQ;
  req.kp = htonl((unsigned int)kp);
  req.ki = htonl((unsigned int)ki);
  req.kd = htonl((unsigned int)kd);

  return client->Request(PLAYER_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* set the constants for the position PID
 *
 * returns: 0 if ok, -1 else
 */
int
PositionProxy::SetPositionPID(short kp, short ki, short kd)
{
  if (!client) {
    return -1;
  }

  player_position_position_pid_req_t req;

  req.subtype = PLAYER_POSITION_POSITION_PID_REQ;
  req.kp = htonl(kp);
  req.ki = htonl(ki);
  req.kd = htonl(kd);

  return client->Request(PLAYER_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* set the speed profile values used during position mode
 * spd is max speed in mm/s
 * acc is acceleration to use in mm/s^2
 *
 * returns: 0 if ok, -1 else
 */
int
PositionProxy::SetPositionSpeedProfile(short spd, short acc)
{
  if (!client) {
    return -1;
  }

  player_position_speed_prof_req_t req;

  req.subtype = PLAYER_POSITION_SPEED_PROF_REQ;
  req.speed = htons(spd);
  req.acc = htons(acc);

  return client->Request(PLAYER_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}


void PositionProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_position_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of position data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_position_data_t),hdr.size);
  }

  xpos = (int)ntohl(((player_position_data_t*)buffer)->xpos);
  ypos = (int)ntohl(((player_position_data_t*)buffer)->ypos);
  theta = ntohl(((player_position_data_t*)buffer)->yaw);
  speed = (short)ntohl(((player_position_data_t*)buffer)->xspeed);
  turnrate = (short)ntohl(((player_position_data_t*)buffer)->yawspeed);
  //compass = ntohs(((player_position_data_t*)buffer)->compass);
  stall = ((player_position_data_t*)buffer)->stall;
}

// interface that all proxies SHOULD provide
void PositionProxy::Print()
{
  printf("#Position(%d:%d) - %c\n", device, index, access);
  puts("#xpos\typos\ttheta\tspeed\tturn\tstall");
  printf("%d\t%d\t%u\t%d\t%d\t%d\n", 
         xpos,ypos,theta,speed,turnrate,stall);
}

