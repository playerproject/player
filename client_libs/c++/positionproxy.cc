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
int PositionProxy::SetSpeed(short speed, short turnrate)
{
  if(!client)
    return(-1);

  player_position_cmd_t cmd;

  cmd.xspeed = htons(speed);
  cmd.yawspeed = htons(turnrate);

  return(client->Write(PLAYER_POSITION_CODE,index,
                       (const char*)&cmd,sizeof(cmd)));
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

  char buffer[2];

  buffer[0] = PLAYER_P2OS_POSITION_MOTOR_POWER_REQ;
  buffer[1] = state;


  return(client->Request(PLAYER_POSITION_CODE,index,(const char*)buffer,
                         sizeof(buffer)));
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

  char buffer[2];

  buffer[0] = PLAYER_P2OS_POSITION_VELOCITY_CONTROL_REQ;
  buffer[1] = mode;

  return(client->Request(PLAYER_POSITION_CODE,index,(const char*)buffer,
                         sizeof(buffer)));
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

  char buffer[1];

  buffer[0] = PLAYER_P2OS_POSITION_RESET_ODOM_REQ;

  return(client->Request(PLAYER_POSITION_CODE,index,(const char*)buffer,
                         sizeof(buffer)));
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
  theta = ntohs(((player_position_data_t*)buffer)->yaw);
  speed = (short)ntohs(((player_position_data_t*)buffer)->xspeed);
  turnrate = (short)ntohs(((player_position_data_t*)buffer)->yawspeed);
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

