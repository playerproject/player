/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * $Id$
 *
 * client-side position device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
    
// send a motor command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PositionProxy::SetSpeed(double speed, double sidespeed, double turnrate)
{
  if(!client)
    return(-1);

  player_position_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  cmd.xspeed = (int32_t)htonl((int)rint(speed*1e3));
  cmd.yspeed = (int32_t)htonl((int)rint(sidespeed*1e3));
  cmd.yawspeed = (int32_t)htonl((int)rint(RTOD(turnrate)));
  cmd.state = 1;

  return(client->Write(m_device_id,
                       (const char*)&cmd,sizeof(cmd)));
}

/* sets the desired heading to theta, with the translational
 * and rotational velocity contraints xspeed and yawspeed, respectively
 *
 * returns: 0 if ok, -1 else
 */
int
PositionProxy::DoDesiredHeading(int theta, int xspeed, int yawspeed)
{
  if (!client) {
    return -1;
  }

  player_position_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  // the desired heading is the yaw member
  cmd.yaw = htonl(theta);
  
  // set velocity constraints
  cmd.xspeed = htonl(xspeed);
  cmd.yawspeed = htonl(yawspeed);
  cmd.state = 1;

  return client->Write(m_device_id,
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
  memset( &cmd, 0, sizeof(cmd) );

  // we send a no movement pos command first so that 
  // the real pos command will look new.  sort of a hack FIX
  cmd.xspeed = 0;
  cmd.yawspeed = 0;
  cmd.yaw = 0;
  cmd.state = 1;

  client->Write(m_device_id,
		(const char *)&cmd, sizeof(cmd));

  // now we send the real pos command

  short t = (short) trans;
  cmd.xspeed = htons(t);

  return client->Write(m_device_id,
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
  memset( &cmd, 0, sizeof(cmd) );

  // as before, send a fake pos command first so the
  // real one will be flagged as new

  cmd.xspeed = 0;
  cmd.yawspeed = 0;
  cmd.yaw = 0;
  cmd.state = 1;

  client->Write(m_device_id,
		(const char *)&cmd, sizeof(cmd));

  short r = (short) rot;
  cmd.yawspeed = htons(r);

  return client->Write(m_device_id,
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
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_POSITION_MOTOR_POWER_REQ;
  config.value = state;


  return(client->Request(m_device_id,(const char*)&config,
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
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_POSITION_VELOCITY_MODE_REQ;
  config.value = mode;

  return(client->Request(m_device_id,(const char*)&config,
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
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_POSITION_RESET_ODOM_REQ;

  return(client->Request(m_device_id,(const char*)&config,
                         sizeof(config)));
}

// set odometry to (x,y,a)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PositionProxy::SetOdometry( double x, double y, double theta)
{
  if(!client)
    return(-1);

  player_position_set_odom_req_t config;
  memset( &config, 0, sizeof(config) );

  config.subtype = PLAYER_POSITION_SET_ODOM_REQ;
  config.x = (int32_t)htonl((int)rint(x*1e3));
  config.y = (int32_t)htonl((int)rint(y*1e3));
  config.theta = (int32_t)htonl((int)rint(RTOD(theta)));
  //printf("theta: %d\n", (int)ntohl(config.theta));
  
  return(client->Request(m_device_id,(const char*)&config,
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
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION_POSITION_MODE_REQ;
  req.state = mode;

  return client->Request(m_device_id,
			 (const char *)&req, sizeof(req));
}

/* goto the specified location (m, m, radians)
 * this only works if the robot supports position control.
 *
 * returns: 0 if ok, -1 else
 */
int
PositionProxy::GoTo(double x, double y, double t)
{
  if (!client) {
    return -1;
  }  

  player_position_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  cmd.xpos = (int32_t)htonl((int)rint(x*1e3));
  cmd.ypos = (int32_t)htonl((int)rint(y*1e3));
  cmd.yaw  = (int32_t)htonl((int)rint(RTOD(t)));
  cmd.state = 1;
  cmd.type = 1;

  return(client->Write(m_device_id,
                       (const char*)&cmd,sizeof(cmd)));
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
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION_SPEED_PID_REQ;
  req.kp = htonl((unsigned int)kp);
  req.ki = htonl((unsigned int)ki);
  req.kd = htonl((unsigned int)kd);

  return client->Request(m_device_id,
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
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION_POSITION_PID_REQ;
  req.kp = htonl(kp);
  req.ki = htonl(ki);
  req.kd = htonl(kd);

  return client->Request(m_device_id,
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
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION_SPEED_PROF_REQ;
  req.speed = htons(spd);
  req.acc = htons(acc);

  return client->Request(m_device_id,
			 (const char *)&req, sizeof(req));
}

/* For the segwayrmp driver, this allows us to send status commands
 * to the RMP.  The cmd is one of PLAYER_POSITION_RMP_* and the values
 * for each are detailed in the manual.  The driver just passes
 * this straight through to the RMP
 *
 * returns: 0 on success, negative else
 */
int
PositionProxy::SetStatus(uint8_t cmd, uint16_t value)
{
  if (!client) {
    return -1;
  }

  if (cmd != PLAYER_POSITION_RMP_VELOCITY_SCALE &&
      cmd != PLAYER_POSITION_RMP_ACCEL_SCALE &&
      cmd != PLAYER_POSITION_RMP_TURN_SCALE &&
      cmd != PLAYER_POSITION_RMP_GAIN_SCHEDULE &&
      cmd != PLAYER_POSITION_RMP_CURRENT_LIMIT &&
      cmd != PLAYER_POSITION_RMP_RST_INTEGRATORS) {
    // not a valid config for this command
    return -1;
  }

  player_rmp_config_t rmp_cfg;

  rmp_cfg.subtype = cmd;

  rmp_cfg.value = htons(value);

  return client->Request(m_device_id,
			 (const char *)&rmp_cfg, sizeof(rmp_cfg));
}

int
PositionProxy::PlatformShutdown()
{
  if (!client) {
    return -1;
  }

  player_rmp_config_t rmp;
  rmp.subtype = PLAYER_POSITION_RMP_SHUTDOWN;

  return client->Request(m_device_id,
			 (const char *)&rmp, sizeof(rmp));
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

  xpos = ((int)ntohl(((player_position_data_t*)buffer)->xpos)) / 1e3;
  ypos = ((int)ntohl(((player_position_data_t*)buffer)->ypos)) / 1e3;
  theta = DTOR((double)(int)ntohl(((player_position_data_t*)buffer)->yaw));
  speed = ((int)ntohl(((player_position_data_t*)buffer)->xspeed)) / 1e3;
  sidespeed = ((int)ntohl(((player_position_data_t*)buffer)->yspeed)) / 1e3;
  turnrate = DTOR((double)(int)ntohl(((player_position_data_t*)buffer)->yawspeed));
  stall = ((player_position_data_t*)buffer)->stall;
}

// interface that all proxies SHOULD provide
void PositionProxy::Print()
{
  printf("#Position(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  puts("#xpos\typos\ttheta\tspeed\tsidespeed\tturn\tstall");
  printf("%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%5u\n", 
         xpos,ypos,RTOD(theta),speed,sidespeed,RTOD(turnrate),stall);
}

