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
 * client-side sonar device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define DEBUG

int IDARProxy::SendMessage( idartx_t* tx )
{
  assert( client );
  assert( tx );

  player_idar_config_t cfg;

  cfg.instruction = IDAR_TRANSMIT; 
  memcpy( &cfg.tx, tx, sizeof(cfg.tx) ); // paste in the message
 
  // send a request, don't wait for reply
  // returns -1 on error
  return(client->Request(m_device_id, (const char*)(&cfg),sizeof(cfg)));
}


int IDARProxy::SendGetMessage( idartx_t* tx, idarrx_t* rx )
{
  assert( client );
  assert( rx );
  assert( tx );
  
  player_idar_config_t cfg;
  player_msghdr_t hdr;
  
  cfg.instruction = IDAR_TRANSMIT_RECEIVE;
 
  memcpy( &cfg.tx, tx, sizeof(cfg.tx) ); // paste in the message
    
  // sends request, waits for reply, returns -1 on failure
  return(client->Request(m_device_id,
			 (const char*)&cfg,sizeof(cfg),
			 &hdr, (char*)rx, sizeof(idarrx_t) ) );
}

// interface that all proxies SHOULD provide

// this fetches the latest message and prints it out
void IDARProxy::Print()
{
  idarrx_t msg;
  
  printf("#IDAR(%d:%d) - %c ", m_device_id.code,
         m_device_id.index, access);

  switch( GetMessageNoFlush( &msg ) )
    {
    case -1:
      puts( "failed to get message" );
      break;
      
    case 0:
      PrintMessage( &msg );
      break;

    default:
      puts( "IDAR GetMessage() returned wierd value" );
      break;
    }
}


void IDARProxy::PrintMessage( idarrx_t* msg )
{
  if( msg->len == 0 )
    printf( "[ <none> ]\n" );
  else
    {
      printf( "[ " );
      
      for( int c=0; c<msg->len; c++ )
	printf( "%2X ", msg->mesg[c] );
      
      printf( "] (%d)\n", msg->intensity );
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
  return(client->Request(m_device_id,
			 (const char*)&cfg,sizeof(cfg),
			 &hdr, (char*)rx, sizeof(idarrx_t) ) );
}

int IDARProxy::GetMessageNoFlush( idarrx_t* rx )
{
  assert( client );
  assert( rx );
  
  player_idar_config_t cfg;
  player_msghdr_t hdr;

  cfg.instruction = IDAR_RECEIVE_NOFLUSH;
  // cfg.tx field is not used for receive messages
  
  // sends request, waits for reply, returns -1 on failure
  return(client->Request(m_device_id,
			 (const char*)&cfg,sizeof(cfg),
			 &hdr, (char*)rx, sizeof(idarrx_t) ) );
}

