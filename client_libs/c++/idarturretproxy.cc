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
 * interface with IDAR Turret 
 */


#include <netinet/in.h>
#include <string.h>
#include <math.h>
#include "idarturretproxy.h"

#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

#define DEBUG

IDARTurretProxy::IDARTurretProxy(PlayerClient* pc, unsigned short index, 
		     unsigned char access ) :
  ClientProxy(pc,PLAYER_IDARTURRET_CODE,index,access) 
{

}


int IDARTurretProxy::SendMessages( player_idarturret_config_t* conf )
{
  assert( client );
  assert( conf );

  conf->instruction = IDAR_TRANSMIT; 

  // send a request, don't wait for reply
  // returns -1 on error
  return(client->Request(PLAYER_IDARTURRET_CODE,index,
			 (const char*)(conf),sizeof(*conf)));
}

// interface that all proxies SHOULD provide

// this fetches the latest message and prints it out
void IDARTurretProxy::Print()
{
  
  player_idarturret_reply_t reply;

  printf("#IDAR(%d:%d) - %c ", device, index, access);

  switch( GetMessages( &reply ) )
    {
    case -1:
      puts( "failed to get message" );
      break;
      
    case 0:
      PrintMessages( &reply );
      break;

    default:
      puts( "IDARTurret - GetMessage() returned wierd value" );
      break;
    }
}


void IDARTurretProxy::PrintMessages( player_idarturret_reply_t* reply )
{
  for( int i=0; i<PLAYER_IDARTURRET_IDAR_COUNT; i++ )
    PrintMessage( &reply->rx[i] );
}


void IDARTurretProxy::PrintMessage( idarrx_t* msg )
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

int IDARTurretProxy::GetMessages( player_idarturret_reply_t* reply )
{
  assert( client );
  assert( reply );

  player_idarturret_config_t cfg;
  player_msghdr_t hdr;

  cfg.instruction = IDAR_RECEIVE;
  // cfg.tx field is not used for receive messages
  
  // sends request, waits for reply, returns -1 on failure
  return(client->Request(PLAYER_IDARTURRET_CODE,index,
			 (const char*)&cfg,sizeof(cfg),
			 &hdr, (char*)reply, sizeof(*reply) ) );
}

int IDARTurretProxy::SendGetMessages( player_idarturret_config_t* conf,
				      player_idarturret_reply_t* reply )
{
  assert( client );
  assert( conf );
  assert( reply );

  player_msghdr_t hdr;

  conf->instruction = IDAR_TRANSMIT_RECEIVE; 

  
  // sends request, waits for reply, returns -1 on failure
  return(client->Request(PLAYER_IDARTURRET_CODE,index,
			 (const char*)conf,sizeof(player_idarturret_config_t),
			 &hdr, (char*)reply, sizeof(player_idarturret_reply_t) ) );
}
