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
    
// send a motor command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int Position3DProxy::SetSpeed(int xspeed, int yspeed, int yawspeed)
{
  if(!client)
    return(-1);

  player_position3d_cmd_t cmd;
  memset(&cmd, 0, sizeof(cmd));

  cmd.xpos = cmd.ypos = cmd.zpos = 0;
  cmd.roll = cmd.pitch = cmd.yaw = 0;
  cmd.xspeed = (int32_t)htonl(xspeed);
  cmd.yspeed = (int32_t)htonl(yspeed);
  cmd.zspeed = 0;
  cmd.rollspeed = 0;
  cmd.pitchspeed = 0;
  cmd.yawspeed = (int32_t)htonl(yawspeed);

  return(client->Write(m_device_id,
                       (const char*)&cmd,sizeof(cmd)));
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

  xpos = (int)ntohl(buf->xpos);
  ypos = (int)ntohl(buf->ypos);
  zpos = (int)ntohl(buf->zpos);

  roll = (unsigned int)ntohl(buf->roll);
  pitch = (unsigned int)ntohl(buf->pitch);
  yaw = (unsigned int)ntohl(buf->yaw);

  xspeed = (int)ntohl(buf->xspeed);
  yspeed = (int)ntohl(buf->yspeed);
  zspeed = (int)ntohl(buf->zspeed);

  rollspeed = (int)ntohl(buf->rollspeed);
  pitchspeed = (int)ntohl(buf->pitchspeed);
  yawspeed = (int)ntohl(buf->yawspeed);

  stall = buf->stall;
}

// interface that all proxies SHOULD provide
void Position3DProxy::Print()
{
  printf("#Position(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  puts("#xpos\typos\tzpos\troll\tpitch\tyaw");
  printf("%4d\t%4d\t%4d\t%4u\t%5u\t%3u\n",
         xpos,ypos,zpos,roll,pitch,yaw);
  puts("#xspeed\tyspeed\tzspeed\trollspeed\tpitchspeed\tyawspeed");
  printf("%5d\t%5d\t%5d\t%9d\t%10d\t%8d\n",
         xspeed,yspeed,zspeed,rollspeed,pitchspeed,yawspeed);
}

