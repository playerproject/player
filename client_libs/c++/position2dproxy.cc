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
 * client-side position2D device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>
    
// send a motor command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)

int32_t 
Position2DProxy::SetSpeed(double xspeed, double yspeed, double yawspeed)
{
  if(!client)
    return(-1);

  player_position2d_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  cmd.xspeed   = htonl(static_cast<int32_t>(rint(xspeed*1e3)));
  cmd.yspeed   = htonl(static_cast<int32_t>(rint(yspeed*1e3)));
  cmd.yawspeed = htonl(static_cast<int32_t>(rint(yawspeed*1e3)));

  return(client->Write(m_device_id,
                       reinterpret_cast<const char*>(&cmd),
                       sizeof(cmd)));
}

/* sets the desired heading to theta, with the translational
 * and rotational velocity contraints xspeed and yawspeed, respectively
 *
 * returns: 0 if ok, -1 else
 */
int32_t
Position2DProxy::DoDesiredHeading(double yaw, double xspeed, double yawspeed)
{
  if (!client) {
    return -1;
  }

  player_position2d_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  // the desired heading is the yaw member
  cmd.yaw = htonl(static_cast<int32_t>(rint(yaw*1e3)));
  
  // set velocity constraints
  cmd.xspeed = htonl(static_cast<int32_t>(rint(xspeed*1e3)));
  cmd.yawspeed = htonl(static_cast<int32_t>(rint(yawspeed*1e3)));

  return client->Write(m_device_id,
                       reinterpret_cast<const char*>(&cmd),
                       sizeof(cmd));
}


/* if the robot is in position2d mode, this will make it perform
 * a straightline translation by trans mm. (negative values will be backwards)
 * undefined effect if in velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int32_t
Position2DProxy::DoStraightLine(double m)
{
  if (!client) {
    return -1;
  }

  player_position2d_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  // we send a no movement pos command first so that
  // the real pos command will look new.  sort of a hack FIX
  cmd.xspeed = 0;
  cmd.yawspeed = 0;
  cmd.yaw = 0;

  client->Write(m_device_id,
                reinterpret_cast<const char*>(&cmd),
                sizeof(cmd));

  // now we send the real pos command
  cmd.xspeed = htons(static_cast<int32_t>(rint(m*1e3)));

  return client->Write(m_device_id,
                       reinterpret_cast<const char*>(&cmd),
                       sizeof(cmd));
}

/* if in position2d mode, this will cause a turn in place rotation of
 * rot degrees.
 * undefined effect in velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int32_t
Position2DProxy::DoRotation(double yawspeed)
{
  if (!client) {
    return -1;
  }

  player_position2d_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  // as before, send a fake pos command first so the
  // real one will be flagged as new

  cmd.xspeed = 0;
  cmd.yawspeed = 0;
  cmd.yaw = 0;

  client->Write(m_device_id,
                reinterpret_cast<const char*>(&cmd),
                sizeof(cmd));

  cmd.yawspeed = htonl(static_cast<int32_t>(rint(yawspeed*1e3)));


  return client->Write(m_device_id,
                       reinterpret_cast<const char*>(&cmd),
                       sizeof(cmd));
}

// enable/disable the motors
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int32_t Position2DProxy::SetMotorState(unsigned char state)
{
  if(!client)
    return(-1);

  player_position2d_power_config_t config;
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_POSITION2D_MOTOR_POWER_REQ;
  config.value = state;


  return(client->Request(m_device_id,
                         reinterpret_cast<const char*>(&config),
                         sizeof(config)));
}

// select velocity control mode.  driver dependent
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int32_t Position2DProxy::SelectVelocityControl(unsigned char mode)
{
  if(!client)
    return(-1);

  player_position2d_velocitymode_config_t config;
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_POSITION2D_VELOCITY_MODE_REQ;
  config.value = mode;

  return(client->Request(m_device_id,
                         reinterpret_cast<const char*>(&config),
                         sizeof(config)));
}

// reset odometry to (0,0,0)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int32_t Position2DProxy::ResetOdometry()
{
  if(!client)
    return(-1);

  player_position2d_resetodom_config_t config;
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_POSITION2D_RESET_ODOM_REQ;

  return(client->Request(m_device_id,
                         reinterpret_cast<const char*>(&config),
                         sizeof(config)));
}

// set odometry to (x,y,a)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int32_t Position2DProxy::SetOdometry( double x, double y, double yaw)
{
  if(!client)
    return(-1);

  player_position2d_set_odom_req_t config;
  memset( &config, 0, sizeof(config) );

  config.subtype = PLAYER_POSITION2D_SET_ODOM_REQ;
  config.x = htonl(static_cast<int32_t>(rint(x*1e3)));
  config.y = htonl(static_cast<int32_t>(rint(y*1e3)));
  config.theta = htonl(static_cast<int32_t>(rint(yaw*1e3)));

  return(client->Request(m_device_id,
                         reinterpret_cast<const char*>(&config),
                         sizeof(config)));
}

/* select the kind of velocity control to perform
 * 1 for position2d mode
 * 0 for velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int32_t
Position2DProxy::SelectPositionMode(unsigned char mode)
{
  if (!client) {
    return -1;
  }

  player_position2d_position_mode_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION2D_POSITION_MODE_REQ;
  req.state = mode;

  return client->Request(m_device_id,
                         reinterpret_cast<const char*>(&req),
                         sizeof(req));
}

/* goto the specified location (m, m, radians)
 * this only works if the robot supports position2d control.
 *
 * returns: 0 if ok, -1 else
 */
int32_t
Position2DProxy::GoTo(double x, double y, double yaw)
{
  if (!client) {
    return -1;
  }

  player_position2d_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  cmd.xpos = htonl(static_cast<int32_t>(rint(x*1e3)));
  cmd.ypos = htonl(static_cast<int32_t>(rint(y*1e3)));
  cmd.yaw  = htonl(static_cast<int32_t>(rint(yaw*1e3)));
  cmd.state = 1;
  cmd.type  = 1;

  return(client->Write(m_device_id,
                       reinterpret_cast<const char*>(&cmd),
                       sizeof(cmd)));
}

/* set the PID for the speed controller
 *
 * returns: 0 if ok, -1 else
 */
int32_t
Position2DProxy::SetSpeedPID(double kp, double ki, double kd)
{
  if (!client) {
    return -1;
  }

  player_position2d_speed_pid_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION2D_SPEED_PID_REQ;
  req.kp = htonl(static_cast<int32_t>(rint(kp*1e3)));
  req.ki = htonl(static_cast<int32_t>(rint(ki*1e3)));
  req.kd = htonl(static_cast<int32_t>(rint(kd*1e3)));

  return client->Request(m_device_id,
                         reinterpret_cast<const char*>(&req),
                         sizeof(req));
}

/* set the constants for the position PID
 *
 * returns: 0 if ok, -1 else
 */
int32_t
Position2DProxy::SetPositionPID(double kp, double ki, double kd)
{
  if (!client) {
    return -1;
  }

  player_position2d_position_pid_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION2D_POSITION_PID_REQ;
  req.kp = htonl(static_cast<int32_t>(rint(kp*1e3)));
  req.ki = htonl(static_cast<int32_t>(rint(ki*1e3)));
  req.kd = htonl(static_cast<int32_t>(rint(kd*1e3)));

  return client->Request(m_device_id,
                         reinterpret_cast<const char*>(&req),
                         sizeof(req));
}

/* set the speed profile values used during position mode
 * spd is max speed in mm/s
 * acc is acceleration to use in mm/s^2
 *
 * returns: 0 if ok, -1 else
 */
int32_t
Position2DProxy::SetPositionSpeedProfile(double spd, double acc)
{
  if (!client) {
    return -1;
  }

  player_position2d_speed_prof_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION2D_SPEED_PROF_REQ;
  req.speed   = htonl(static_cast<int32_t>(rint(spd*1e3))); //mrad/s
  req.acc     = htonl(static_cast<int32_t>(rint(acc*1e3))); //mrad/s/s

  return client->Request(m_device_id,
                         reinterpret_cast<const char*>(&req),
                         sizeof(req));
}

void Position2DProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_position_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of position data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_position_data_t),hdr.size);
  }
  const player_position_data_t* lclBuffer =
    reinterpret_cast<const player_position_data_t*>(buffer);

  xpos     = static_cast<int32_t>(ntohl(lclBuffer->xpos))     / 1e3;
  ypos     = static_cast<int32_t>(ntohl(lclBuffer->ypos))     / 1e3;
  yaw      = static_cast<int32_t>(ntohl(lclBuffer->yaw))      / 1e3;
  xspeed   = static_cast<int32_t>(ntohl(lclBuffer->xspeed))   / 1e3;
  yspeed   = static_cast<int32_t>(ntohl(lclBuffer->yspeed))   / 1e3;
  yawspeed = static_cast<int32_t>(ntohl(lclBuffer->yawspeed)) / 1e3;
  stall    = lclBuffer->stall;
}

// interface that all proxies SHOULD provide
void Position2DProxy::Print()
{
  printf("#Position(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  puts("#xpos\typos\ttheta\tspeed\tsidespeed\tturn\tstall");
  printf("%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%5u\n", 
         xpos,ypos,RTOD(yaw),xspeed,yspeed,RTOD(yawspeed),stall);
}

