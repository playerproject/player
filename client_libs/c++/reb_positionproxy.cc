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
 *   John Sweeney & Bryan Thibodeau,
 *       UMASS, Amherst, Laboratory for Perceptual Robotics
 *
 * $Id$
 * 
 * the implementation for REBPositionProxy
 *
 */
#include <netinet/in.h>
#include <playerclient.h>

/* set the motor state
 * non-zero for enabled
 * 0 for disabled
 *
 * returns: 0 if ok, -1 else
 */
int
REBPositionProxy::SetMotorState(unsigned char state)
{
  if (!client) {
    return -1;
  }

  player_position_power_config_t req;

  req.request = PLAYER_POSITION_MOTOR_POWER_REQ;
  req.value = state;

  return client->Request(PLAYER_REB_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* set the movement mode of the robot.
 * 1 for velocity-based heading PD controller
 * 0 for direct velocity control
 *
 * returns: 0 is ok, -1 else
 */
int
REBPositionProxy::SelectVelocityControl(unsigned char mode)
{
  if (!client) {
    return -1;
  }

  player_position_velocitymode_config_t req;

  req.request = PLAYER_POSITION_VELOCITY_MODE_REQ;
  req.value = mode;

  return client->Request(PLAYER_REB_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* select the kind of velocity control to perform
 * 1 for position mode
 * 0 for velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int
REBPositionProxy::SelectPositionMode(unsigned char mode)
{
  if (!client) {
    return -1;
  }

  player_reb_pos_mode_req_t req;

  req.subtype = PLAYER_REB_POSITION_POSITION_MODE_REQ;
  req.state = mode;

  return client->Request(PLAYER_REB_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}


/* reset the odometry to 0,0,0.
 *
 * returns: 0 on ok, -1 else
 */
int
REBPositionProxy::ResetOdometry()
{
  if (!client)
    return(-1);
  
  player_position_resetodom_config_t config;
  
  config.request = PLAYER_POSITION_RESET_ODOM_REQ;
  
  return(client->Request(PLAYER_POSITION_CODE,index,
			 (const char*)&config, sizeof(config)));
}

/* Set odometry to the gicen pose.  
 *
 * returns: 0 if ok, -1 else
 */
int
REBPositionProxy::SetOdometry(long x, long y,int t)
{
  if (!client) {
    return -1;
  }
  player_reb_set_odom_req_t req;

  req.subtype = PLAYER_REB_POSITION_SET_ODOM_REQ;
  req.x = htonl( x);
  req.y = htonl( y);
  t %= 360;
  t=(uint16_t) (t < 0 ? t+360 : t);
  req.theta = htons(t);
  return client->Request(PLAYER_REB_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* set the PID for the speed controller
 *
 * returns: 0 if ok, -1 else
 */
int
REBPositionProxy::SetSpeedPID(int kp, int ki, int kd)
{
  if (!client) {
    return -1;
  }

  player_reb_speed_pid_req_t req;

  req.subtype = PLAYER_REB_POSITION_SPEED_PID_REQ;
  req.kp = htonl((unsigned int)kp);
  req.ki = htonl((unsigned int)ki);
  req.kd = htonl((unsigned int)kd);

  return client->Request(PLAYER_REB_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* set the constants for the position PID
 *
 * returns: 0 if ok, -1 else
 */
int
REBPositionProxy::SetPositionPID(short kp, short ki, short kd)
{
  if (!client) {
    return -1;
  }

  player_reb_pos_pid_req_t req;

  req.subtype = PLAYER_REB_POSITION_POSITION_PID_REQ;
  req.kp = htonl(kp);
  req.ki = htonl(ki);
  req.kd = htonl(kd);

  return client->Request(PLAYER_REB_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* set the speed profile values used during position mode
 * spd is max speed in mm/s
 * acc is acceleration to use in mm/s^2
 *
 * returns: 0 if ok, -1 else
 */
int
REBPositionProxy::SetPositionSpeedProfile(short spd, short acc)
{
  if (!client) {
    return -1;
  }

  player_reb_speed_prof_req_t req;

  req.subtype = PLAYER_REB_POSITION_SPEED_PROF_REQ;
  req.speed = htons(spd);
  req.acc = htons(acc);

  return client->Request(PLAYER_REB_POSITION_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* this will give a velocity mode command
 * different outcomes depending on type of velocity control
 * DIRECT:
 *   trans - translational velocity in mm/s to perform
 *   rot - rotational velocity in deg/s to perform
 *   heading - not used (defaults to 0, so no need to give a value)
 *
 * PD CONTROL:
 *   trans - max translational velocity allowed
 *   rot - max rotational velocity to use
 *   heading - desired heading in deg to achieve
 *
 * NOTE: if the robot is in position mode, this will cause it to move
 * forward by trans mm (ie like DoStraightLine)
 * returns: 0 if ok, -1 else
 */
int
REBPositionProxy::SetSpeed(short trans, short rot, short heading)
{
  if (!client) {
    return -1;
  }

  player_position_cmd_t cmd;

  cmd.xspeed = htons(trans);
  cmd.yawspeed = htons(rot);
  cmd.yaw = htons(heading);

  return client->Write(PLAYER_REB_POSITION_CODE, index, 
			 (const char *)&cmd, sizeof(cmd));
}

/* if the robot is in position mode, this will make it perform
 * a straightline translation by trans mm. (negative values will be backwards)
 * undefined effect if in velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int
REBPositionProxy::DoStraightLine(int trans)
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

  client->Write(PLAYER_REB_POSITION_CODE, index, 
		(const char *)&cmd, sizeof(cmd));

  // now we send the real pos command

  short t = (short) trans;
  cmd.xspeed = htons(t);

  return client->Write(PLAYER_REB_POSITION_CODE, index, 
			 (const char *)&cmd, sizeof(cmd));
}

/* if in position mode, this will cause a turn in place rotation of
 * rot degrees.  
 * undefined effect in velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int
REBPositionProxy::DoRotation(int rot)
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

  client->Write(PLAYER_REB_POSITION_CODE, index,
		(const char *)&cmd, sizeof(cmd));

  short r = (short) rot;
  cmd.yawspeed = htons(r);

  return client->Write(PLAYER_REB_POSITION_CODE, index, 
			 (const char *)&cmd, sizeof(cmd));
}
 
/* this will get the data being sent by the device
 *
 * returns: 
 */
void
REBPositionProxy::FillData(player_msghdr_t hdr, const char * buffer)
{
  if (hdr.size != sizeof(player_position_data_t)) {
    fprintf(stderr, "REBPOSPROXY: expected %d bytes but got %d\n",
	    sizeof(player_position_data_t), hdr.size);
  }

  x = (int32_t) ntohl(((player_position_data_t *)buffer)->xpos);
  y = (int32_t) ntohl(((player_position_data_t *)buffer)->ypos);
  theta = (uint16_t) ntohs(((player_position_data_t *)buffer)->yaw);

  //  desired_heading = (int16_t) ntohs( ((player_reb_position_data_t*)buffer)->heading);

  translational = (int16_t) ntohs(((player_position_data_t *)buffer)->xspeed);
  rotational = (int16_t) ntohs(((player_position_data_t *)buffer)->yawspeed);

  on_target = ((player_position_data_t *)buffer)->stall;
}

/* print out all the good stuff
 *
 * returns:
 */
void
REBPositionProxy::Print()
{
  printf("#REB Position(%d:%d) - %c\n", device, index, access);
  printf("\tx\ty\ttheta\ttrans\trot\ttarget\tdh\n");
  printf("\t%d\t%d\t%d\t%d\t%d\t%02x\t%d\n", int(x), int(y), theta, translational,
	 rotational, on_target, desired_heading);
}

 
