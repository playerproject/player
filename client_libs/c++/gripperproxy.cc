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
 * client-side gripper device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
    
// send a gripper command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int GripperProxy::SetGrip(unsigned char cmd, unsigned char arg)
{
  if(!client)
    return(-1);

  player_gripper_cmd_t command;

  command.cmd = cmd;
  command.arg = arg;

  return(client->Write(m_device_id, (const char*)&command,sizeof(command)));
}

void GripperProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_gripper_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of gripper data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_gripper_data_t),hdr.size);
  }

  state = ((player_gripper_data_t*)buffer)->state;
  beams = ((player_gripper_data_t*)buffer)->beams;

  outer_break_beam = (beams & 0x04) ? true : false;
  inner_break_beam = (beams & 0x08) ? true : false;

  paddles_open = (state & 0x01) ? true : false;
  paddles_closed = (state & 0x02) ? true : false;
  paddles_moving = (state & 0x04) ? true : false;
  gripper_error = (state & 0x08) ? true : false;
  lift_up = (state & 0x10) ? true : false;
  lift_down = (state & 0x20) ? true : false;
  lift_moving = (state & 0x40) ? true : false;
  lift_error = (state & 0x80) ? true : false;
}

// interface that all proxies SHOULD provide
void GripperProxy::Print()
{
  printf("#Gripper(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  puts("#paddles\tinner beams\touter beams");
  printf("%s\t\t%s\t\t%s\n",
         (paddles_open) ? "open" : "closed",
         (inner_break_beam) ? "broken" : "clear",
         (outer_break_beam) ? "broken" : "clear");
}

