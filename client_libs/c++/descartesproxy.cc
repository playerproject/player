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
    
// send a motor command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int DescartesProxy::Move(short speed, short heading, short distance )
{
  if(!client)
    return(-1);


  // normalize heading
  while( heading < 0 )
    heading += (short)360;

  heading %= (short)360;

  player_descartes_config_t cfg;
  
  cfg.speed = htons(speed);
  cfg.heading = htons(heading);
  cfg.distance = htons(distance);
  
  return(client->Request(PLAYER_DESCARTES_CODE,index,
			 (const char*)&cfg,sizeof(cfg)));
} 
  //return(client->Write(PLAYER_DESCARTES_CODE,index,
  //                   (const char*)&cmd,sizeof(cmd)));

void DescartesProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_descartes_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of descartes data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_descartes_data_t),hdr.size);
  }

  xpos = (int)ntohl(((player_descartes_data_t*)buffer)->xpos);
  ypos = (int)ntohl(((player_descartes_data_t*)buffer)->ypos);
  theta = (int)ntohs(((player_descartes_data_t*)buffer)->theta);
  bumpers[0] = ((player_descartes_data_t*)buffer)->bumpers[0];
  bumpers[1] = ((player_descartes_data_t*)buffer)->bumpers[1];
}

// interface that all proxies SHOULD provide
void DescartesProxy::Print()
{
  printf("#Descartes(%d:%d) - %c\n", device, index, access);
  puts("#xpos\typos\ttheta\tbumpers");
  printf("%d\t%d\t%u\t%u-%u\n", 
         xpos,ypos,theta,bumpers[0],bumpers[1]);
}

void DescartesProxy::GetPos( double* x, double* y, double* th )
{
  *x = xpos;
  *y = ypos;
  *th = theta;
}


