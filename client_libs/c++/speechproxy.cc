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
 * client-side speech device 
 */

#include <speechproxy.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif
    
// send a speech command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int SpeechProxy::Say(char* str)
{
  if(!client)
    return(-1);

  player_speech_cmd_t cmd;

  bzero(cmd.string,sizeof(cmd.string));
  if(str)
    strncpy((char*)(cmd.string),str,SPEECH_MAX_STRING_LEN);

  return(client->Write(PLAYER_SPEECH_CODE,index,
                       (const char*)&cmd,sizeof(cmd)));
}


