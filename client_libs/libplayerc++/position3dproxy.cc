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

#define PTOHL(x) static_cast<int32_t>(ntohl(x))/1e3
#define HTOPL(x) htonl(static_cast<int32_t>(rint(x*1e3)))

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

  cmd.xspeed     = HTOPL(xspeed);
  cmd.yspeed     = HTOPL(yspeed);
  cmd.zspeed     = HTOPL(zspeed);
  cmd.rollspeed  = HTOPL(rollspeed);
  cmd.pitchspeed = HTOPL(pitchspeed);
  cmd.yawspeed   = HTOPL(yawspeed);
  cmd.state      = 1;
  cmd.type       = 0;

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

  cmd.xpos   = HTOPL(x);
  cmd.ypos   = HTOPL(y);
  cmd.zpos   = HTOPL(z);

  cmd.roll   = HTOPL(roll);
  cmd.pitch  = HTOPL(pitch);
  cmd.yaw    = HTOPL(yaw);
  cmd.state  = 1;
  cmd.type   = 1;

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

  xpos = PTOHL(buf->xpos);
  ypos = PTOHL(buf->ypos);
  zpos = PTOHL(buf->zpos);

  roll  = PTOHL(buf->roll);
  pitch = PTOHL(buf->pitch);
  yaw   = PTOHL(buf->yaw);

  xspeed = PTOHL(buf->xspeed);
  yspeed = PTOHL(buf->yspeed);
  zspeed = PTOHL(buf->zspeed);

  rollspeed  = PTOHL(buf->rollspeed);
  pitchspeed = PTOHL(buf->pitchspeed);
  yawspeed   = PTOHL(buf->yawspeed);

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

//  config.request = PLAYER_POSITION_MOTOR_POWER_REQ;
  config.value = state;


  return(client->Request(m_device_id,PLAYER_POSITION_MOTOR_POWER,
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

//  config.request = PLAYER_POSITION3D_VELOCITY_MODE_REQ;
  config.value = mode;

  return(client->Request(m_device_id,PLAYER_POSITION3D_VELOCITY_MODE,
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

//  config.request = PLAYER_POSITION3D_RESET_ODOM_REQ;

  return(client->Request(m_device_id,PLAYER_POSITION3D_RESET_ODOM,
                         reinterpret_cast<char*>(&config),
                         sizeof(config)));

}

// set odometry to (x,y,z,roll,pitch,theta)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int Position3DProxy::SetOdometry(double x, double y, double z,
                                 double roll, double pitch, double yaw )
{
  if(!client)
    return(-1);

  player_position3d_set_odom_req_t config;
  memset( &config, 0, sizeof(config) );

  config.x = HTOPL(x);
  config.y = HTOPL(y);
  config.z = HTOPL(z);

  config.roll  = HTOPL(roll);
  config.pitch = HTOPL(pitch);
  config.yaw   = HTOPL(yaw);

  return(client->Request(m_device_id,PLAYER_POSITION3D_SET_ODOM,
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

  req.kp = HTOPL(kp);
  req.ki = HTOPL(ki);
  req.kd = HTOPL(kd);

  return client->Request(m_device_id,PLAYER_POSITION3D_SPEED_PID,
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

  req.kp = HTOPL(kp);
  req.ki = HTOPL(ki);
  req.kd = HTOPL(kd);

  return client->Request(m_device_id,PLAYER_POSITION3D_POSITION_PID,
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

  req.speed   = HTOPL(spd); //rad/s
  req.acc     = HTOPL(acc); //rad/s/s

  return client->Request(m_device_id,PLAYER_POSITION3D_SPEED_PROF,
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

//  req.subtype = PLAYER_POSITION3D_POSITION_MODE_REQ;
  req.state = mode;

  return client->Request(m_device_id,PLAYER_POSITION3D_POSITION_MODE,
                         reinterpret_cast<char*>(&req),
                         sizeof(req));
}

