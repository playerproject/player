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
 * client-side sonar device 
 */

#include <idarproxy.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>

#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

#define DEBUG

IDARProxy::IDARProxy(PlayerClient* pc, unsigned short index, 
		     unsigned char access ) :
  ClientProxy(pc,PLAYER_IDAR_CODE,index,access) 
{

}


int IDARProxy::SendMessage( idartx_t* tx )
{
  assert( client );
  assert( tx );

  player_idar_config_t cfg;

  cfg.instruction = IDAR_TRANSMIT; 
  memcpy( &cfg.tx, tx, sizeof(cfg.tx) ); // paste in the message
 
  // send a request, don't wait for reply
  // returns -1 on error
  return(client->Request(PLAYER_IDAR_CODE,index,
			 (const char*)(&cfg),sizeof(cfg)));
}

// interface that all proxies SHOULD provide
void IDARProxy::Print()
{
  //puts( "IDAR Proxy: no data" );
  
  idarrx_t msg;
  
  if( GetMessage( &msg ) < 0 )
    puts( "failed to get message for printing" );
  else
    {
      printf( "Idar: recv %d bytes, intensity: %d \n",
	      msg.len, msg.intensity );
    }
}

int IDARProxy::GetMessage( idarrx_t* rx )
{
  assert( client );
  assert( rx );

  player_idar_config_t cfg;
  player_msghdr_t hdr;

  cfg.instruction = IDAR_RECEIVE;
  // cfg.tx field is not used for receive messages
  
  // sends request, waits for reply, returns -1 on failure
  return(client->Request(PLAYER_IDAR_CODE,index,
			 (const char*)(&cfg),sizeof(cfg),
			 &hdr, (char*)&rx, sizeof(idarrx_t) ) );
}

