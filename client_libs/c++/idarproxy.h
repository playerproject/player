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

#ifndef IDARPROXY_H
#define IDARPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

class IDARProxy : public ClientProxy
{
  public:
  
  // the client calls this method to make a new proxy
  //   leave access empty to start unconnected
  IDARProxy(PlayerClient* pc, unsigned short index, 
	    unsigned char access = 'c');
  
  // interface that all proxies must provide
  // reads the receive buffers from player
  //void FillData(player_msghdr_t hdr, const char* buffer);
  
  // interface that all proxies SHOULD provide
  void Print();
  
  // tx parameter is optional; defaults to 0
  int SendMessage( idartx_t* tx );
  
  // get message and transmission details 
  int GetMessage( idarrx_t* rx );  

  // pretty print a message
  void PrintMessage(idarrx_t* rx); 
};

#endif







