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
 * client-side position3d device 
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
int Position3DProxy::SetSpeed(double xspeed, double yspeed, double zspeed,
                  double rollspeed, double pitchspeed,
                  double yawspeed)
{
  if(!client)
    return(-1);

  player_position3d_cmd_t cmd;
  memset(&cmd, 0, sizeof(cmd));

  cmd.xspeed     = htonl(static_cast<int32_t>(rint(xspeed*1e3)));
  cmd.yspeed     = htonl(static_cast<int32_t>(rint(yspeed*1e3)));
  cmd.zspeed     = htonl(static_cast<int32_t>(rint(zspeed*1e3)));
  cmd.rollspeed  = htonl(static_cast<int32_t>(rint(rollspeed*1e3)));
  cmd.pitchspeed = htonl(static_cast<int32_t>(rint(pitchspeed*1e3)));
  cmd.yawspeed   = htonl(static_cast<int32_t>(rint(yawspeed*1e3)));
  cmd.state = 1;

  return(client->Write(m_device_id,
                       reinterpret_cast<char*>(&cmd),
                       sizeof(cmd)));
}

/* goto the specified location (m, m, m, rad, rad,rad)
 * this only works if the robot supports position control.
 *
 * returns: 0 if ok, -1 else
 */
int
Position3DProxy::GoTo(double x, double y, double z,
            double roll, double pitch, double yaw)
{
  if (!client) {
    return -1;
  }

  player_position3d_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  cmd.xpos   = htonl(static_cast<int32_t>(rint(x*1e3)));
  cmd.ypos   = htonl(static_cast<int32_t>(rint(y*1e3)));
  cmd.zpos   = htonl(static_cast<int32_t>(rint(z*1e3)));

  cmd.roll   = htonl(static_cast<int32_t>(rint(roll*1e3)));
  cmd.pitch  = htonl(static_cast<int32_t>(rint(pitch*1e3)));
  cmd.yaw    = htonl(static_cast<int32_t>(rint(yaw*1e3)));
  cmd.state = 1;

  return(client->Write(m_device_id,
                       reinterpret_cast<char*>(&cmd),
                       sizeof(cmd)));
}

void Position3DProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  player_position3d_data_t* buf = (player_position3d_data_t*)buffer;

  if(hdr.size != sizeof(player_position3d_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of position3d data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_position3d_data_t),hdr.size);
  }

  xpos = static_cast<int32_t>(ntohl(buf->xpos)) / 1e3;
  ypos = static_cast<int32_t>(ntohl(buf->ypos)) / 1e3;
  zpos = static_cast<int32_t>(ntohl(buf->zpos)) / 1e3;

  roll  = static_cast<int32_t>(ntohl(buf->roll))  / 1e3;
  pitch = static_cast<int32_t>(ntohl(buf->pitch)) / 1e3;
  yaw   = static_cast<int32_t>(ntohl(buf->yaw))   / 1e3;

  xspeed = static_cast<int32_t>(ntohl(buf->xspeed)) / 1e3;
  yspeed = static_cast<int32_t>(ntohl(buf->yspeed)) / 1e3;
  zspeed = static_cast<int32_t>(ntohl(buf->zspeed)) / 1e3;

  rollspeed  = static_cast<int32_t>(ntohl(buf->rollspeed))  / 1e3;
  pitchspeed = static_cast<int32_t>(ntohl(buf->pitchspeed)) / 1e3;
  yawspeed   = static_cast<int32_t>(ntohl(buf->yawspeed))   / 1e3;

  stall = buf->stall;
}

// interface that all proxies SHOULD provide
void Position3DProxy::Print()
{
  printf("#Position(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  puts("#xpos\typos\tzpos\troll\tpitch\tyaw");
  printf("%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n",
         xpos,ypos,zpos,roll,pitch,yaw);
  puts("#xspeed\tyspeed\tzspeed\trollspeed\tpitchspeed\tyawspeed");
  printf("%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n",
         xspeed,yspeed,zspeed,rollspeed,pitchspeed,yawspeed);
}

int Position3DProxy::SetMotorState(unsigned char state)
{
  if(!client)
    return(-1);

  player_position_power_config_t config;
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_POSITION_MOTOR_POWER_REQ;
  config.value = state;


  return(client->Request(m_device_id,
                         reinterpret_cast<char*>(&config),
                         sizeof(config)));
}


// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int Position3DProxy::SelectVelocityControl(unsigned char mode)
{
  if(!client)
    return(-1);

  player_position3d_velocitymode_config_t config;
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_POSITION3D_VELOCITY_MODE_REQ;
  config.value = mode;

  return(client->Request(m_device_id,
                         reinterpret_cast<char*>(&config),
                         sizeof(config)));
}

// reset odometry to (0,0,0)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int Position3DProxy::ResetOdometry()
{
  if(!client)
    return(-1);

  player_position3d_resetodom_config_t config;
  memset( &config, 0, sizeof(config) );

  config.request = PLAYER_POSITION3D_RESET_ODOM_REQ;

  return(client->Request(m_device_id,
                         reinterpret_cast<char*>(&config),
                         sizeof(config)));

}

// set odometry to (x,y,z,roll,pitch,theta)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int Position3DProxy::SetOdometry( double x, double y, double z,
                                      double roll, double pitch, double yaw )
{
  if(!client)
    return(-1);

  player_position3d_set_odom_req_t config;
  memset( &config, 0, sizeof(config) );

  config.subtype = PLAYER_POSITION3D_SET_ODOM_REQ;
  config.x = htonl(static_cast<int32_t>(rint(x*1e3)));
  config.y = htonl(static_cast<int32_t>(rint(y*1e3)));
  config.z = htonl(static_cast<int32_t>(rint(z*1e3)));

  config.roll  = htonl(static_cast<int32_t>(rint(roll*1e3)));
  config.pitch = htonl(static_cast<int32_t>(rint(pitch*1e3)));
  config.yaw   = htonl(static_cast<int32_t>(rint(yaw*1e3)));

  return(client->Request(m_device_id,
                         reinterpret_cast<char*>(&config),
                         sizeof(config)));
}

/* set the PID for the speed controller
 *
 * returns: 0 if ok, -1 else
 */
int
Position3DProxy::SetSpeedPID(double kp, double ki, double kd)
{
  if (!client) {
    return -1;
  }

  player_position3d_speed_pid_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION3D_SPEED_PID_REQ;
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
int
Position3DProxy::SetPositionPID(double kp, double ki, double kd)
{
  if (!client) {
    return -1;
  }

  player_position3d_position_pid_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION3D_POSITION_PID_REQ;
  req.kp = htonl(static_cast<int32_t>(rint(kp*1e3)));
  req.ki = htonl(static_cast<int32_t>(rint(ki*1e3)));
  req.kd = htonl(static_cast<int32_t>(rint(kd*1e3)));

  return client->Request(m_device_id,
                         reinterpret_cast<const char*>(&req),
                         sizeof(req));
}

/* set the speed profile values used during position mode
 * returns: 0 if ok, -1 else
 */
int
Position3DProxy::SetPositionSpeedProfile(double spd, double acc)
{
  if (!client) {
    return -1;
  }

  player_position3d_speed_prof_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION3D_SPEED_PROF_REQ;
  req.speed   = htonl(static_cast<int32_t>(rint(spd*1e3))); //rad/s
  req.acc     = htonl(static_cast<int32_t>(rint(acc*1e3))); //rad/s/s

  return client->Request(m_device_id,
                         reinterpret_cast<char*>(&req),
                         sizeof(req));
}

/* select the kind of velocity control to perform
 * 1 for position mode
 * 0 for velocity mode
 *
 * returns: 0 if ok, -1 else
 */
int
Position3DProxy::SelectPositionMode(unsigned char mode)
{
  if (!client) {
    return -1;
  }

  player_position3d_position_mode_req_t req;
  memset( &req, 0, sizeof(req) );

  req.subtype = PLAYER_POSITION3D_POSITION_MODE_REQ;
  req.state = mode;

  return client->Request(m_device_id,
                         reinterpret_cast<char*>(&req),
                         sizeof(req));
}

