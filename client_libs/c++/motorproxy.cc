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
 * client-side motor device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// send a motor command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int32_t MotorProxy::SetSpeed(double speed)
{
  if(!client)
    return(-1);

  player_motor_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  cmd.thetaspeed = htonl(static_cast<int32_t>(rint(speed*1e3)));

  return(client->Write(m_device_id,
                       reinterpret_cast<char*>(&cmd),sizeof(cmd)));
}


// enable/disable the motors
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int32_t MotorProxy::SetMotorState(unsigned char state)
{
  if(!client)
    return(-1);

  player_motor_power_config_t config;
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_MOTOR_POWER_REQ;
  config.value = state;


  return(client->Request(m_device_id,reinterpret_cast<char*>(&config),
                         sizeof(config)));
}

// Select velocity control mdoe, driver specific
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int32_t MotorProxy::SelectVelocityControl(unsigned char mode)
{
  if(!client)
    return(-1);

  player_motor_velocitymode_config_t config;
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_MOTOR_VELOCITY_MODE_REQ;
  config.value = mode;

  return(client->Request(m_device_id,reinterpret_cast<char*>(&config),
                         sizeof(config)));
}

// reset odometry to (0,0,0)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int32_t MotorProxy::ResetOdometry()
{
  if(!client)
    return(-1);

  player_motor_resetodom_config_t config;
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_MOTOR_RESET_ODOM_REQ;

  return(client->Request(m_device_id,reinterpret_cast<char*>(&config),
                         sizeof(config)));
}

// set odometry to (x,y,a)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int32_t MotorProxy::SetOdometry( double theta)
{
  if(!client)
    return(-1);

  player_motor_set_odom_req_t config;
  memset( &config, 0, sizeof(config) );

  config.subtype = PLAYER_MOTOR_SET_ODOM_REQ;
  config.theta = htonl(static_cast<int32_t>(rint(theta*1e3)));
  
  return(client->Request(m_device_id,reinterpret_cast<char*>(&config),
                         sizeof(config)));
}

/* select the kind of velocity control to perform
 * 1 for motor mode
 * 0 for velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int32_t
MotorProxy::SelectPositionMode(unsigned char mode)
{
  if (!client) {
    return -1;
  }

  player_motor_position_mode_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_MOTOR_POSITION_MODE_REQ;
  req.state = mode;

  return client->Request(m_device_id,
                         reinterpret_cast<char*>(&req), sizeof(req));
}

/* goto the specified location (m, m, radians)
 * this only works if the robot supports motor control.
 *
 * returns: 0 if ok, -1 else
 */
int32_t
MotorProxy::GoTo(double angle)
{
  if (!client) {
    return -1;
  }  

  player_motor_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  cmd.theta = htonl(static_cast<int32_t>(rint(angle*1e3)));
  cmd.state = 1;
  cmd.type = 1;

  return(client->Write(m_device_id,
                       reinterpret_cast<char*>(&cmd),sizeof(cmd)));
}

/* set the PID for the speed controller
 *
 * returns: 0 if ok, -1 else
 */
int32_t
MotorProxy::SetSpeedPID(double kp, double ki, double kd)
{
  if (!client) {
    return -1;
  }

  player_motor_speed_pid_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_MOTOR_SPEED_PID_REQ;
  // * 1e3 to give more precision over the wire
  req.kp = htonl(static_cast<int32_t>(rint(kp*1e3)));
  req.ki = htonl(static_cast<int32_t>(rint(ki*1e3)));
  req.kd = htonl(static_cast<int32_t>(rint(kd*1e3)));

  return client->Request(m_device_id,
             reinterpret_cast<char*>(&req), sizeof(req));
}

/* set the constants for the motor PID
 *
 * returns: 0 if ok, -1 else
 */
int32_t
MotorProxy::SetPositionPID(double kp, double ki, double kd)
{
  if (!client) {
    return -1;
  }

  player_motor_speed_pid_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_MOTOR_POSITION_PID_REQ;
  // * 1000 to give more precision over the wire
  req.kp = htonl(static_cast<int32_t>(rint(kp*1e3)));
  req.ki = htonl(static_cast<int32_t>(rint(ki*1e3)));
  req.kd = htonl(static_cast<int32_t>(rint(kd*1e3)));

  return client->Request(m_device_id,
             reinterpret_cast<char*>(&req), sizeof(req));
}

/* set the speed profile values used during motor mode
 * spd is max speed in mm/s
 * acc is acceleration to use in mm/s^2
 *
 * returns: 0 if ok, -1 else
 */
int32_t
MotorProxy::SetPositionSpeedProfile(double spd, double acc)
{
  if (!client) {
    return -1;
  }

  player_motor_speed_prof_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_MOTOR_SPEED_PROF_REQ;
  // * 1000 to switch the speed into mrad/s
  req.speed = htonl(static_cast<int32_t>(rint(spd*1e3)));
  // * 1000 to switch the speed into mrad/s/s
  req.acc = htonl(static_cast<int32_t>(rint(acc*1e3)));

  return client->Request(m_device_id,
             reinterpret_cast<char*>(&req), sizeof(req));
}

void MotorProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_motor_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of motor data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_motor_data_t),hdr.size);
  }
  const player_motor_data_t* lclBuffer = reinterpret_cast<const player_motor_data_t*>(buffer);

  theta = static_cast<int32_t>(ntohl(lclBuffer->theta)) / 1e3;
  thetaspeed = static_cast<int32_t>(ntohl(lclBuffer->thetaspeed)) / 1e3;
  stall = lclBuffer->stall;
}

// interface that all proxies SHOULD provide
void MotorProxy::Print()
{
  printf("#Motor(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  printf("#\ttheta\tthetaspeed\tstall");
  printf("%.3f\t%.3f\t%i\t\n", 
         theta, thetaspeed, stall);
}

