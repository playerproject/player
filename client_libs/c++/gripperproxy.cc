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

#include <gripperproxy.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif
    
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

  return(client->Write(PLAYER_GRIPPER_CODE,index,
                       (const char*)&command,sizeof(cmd)));
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
}

// interface that all proxies SHOULD provide
void GripperProxy::Print()
{
  printf("#Gripper(%d:%d) - %c\n", device, index, access);
  puts("#state\tbeams");
  printf("%d\t%d\n", state,beams);
}

