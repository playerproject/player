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
 * parent class for client-side device proxies
 */

#include <clientproxy.h>
#include <playerclient.h>

// constructor will try to get access to the device
ClientProxy::ClientProxy(PlayerClient* pc, 
            unsigned short req_device,
            unsigned short req_index,
            unsigned char req_access)
{
  device = req_device;
  index = req_index;
  client = pc;

  timestamp.tv_sec = 0;
  timestamp.tv_usec = 0;
  senttime.tv_sec = 0;
  senttime.tv_usec = 0;
  receivedtime.tv_sec = 0;
  receivedtime.tv_usec = 0;

  // start out with no access
  unsigned char grant_access = 'e';
   
  // add it to our client's list to manage
  if(client)
    client->AddProxy(this);

  if(client && req_access!='c')
  {
    client->RequestDeviceAccess(req_device, req_index, 
                                req_access, &grant_access);

    if((req_access != grant_access) && (player_debug_level(-1) >= 1))
      printf("WARNING: tried to get '%c' access to device %d:%d but got "
 	     "'%c' access.\n", req_access, device, index, grant_access);
  }
  else
  {
    //if(player_debug_level(-1) >= 4)
      //printf("WARNING: couldn't open device %d:%d because no client "
 	     //"object is available\n", device,index);
  }

  access = grant_access;
}

// destructor will try to close access to the device
ClientProxy::~ClientProxy()
{
  if(client)
  {
    if((access != 'c') && (access != 'e'))
      client->RequestDeviceAccess(device,index,'c',NULL);
    // remove it to our client's list to manage
    client->RemoveProxy(this);
  }
}

// methods for changing access mode
int ClientProxy::ChangeAccess(unsigned char req_access, 
                              unsigned char* grant_access )
			      
{
  unsigned char our_grant_access = access;

  if(client)
  {
    if(client->RequestDeviceAccess(device, index, req_access, 
                                   &our_grant_access ) )
    {
      if(player_debug_level(-1) >= 1)
        puts("WARNING: RequestDeviceAccess() errored");
      return(-1);
    }

    if((req_access != our_grant_access) && (player_debug_level(-1) >= 1))
      printf("WARNING: tried to get '%c' access to device %d:%d but got "
 	     "'%c' access.\n", req_access, device, index, our_grant_access);
  }
  else
    return(-1);

  access = our_grant_access;

  if(grant_access)
    *grant_access = access;

  return(0);
}

// interface that all proxies must provide
void ClientProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  // we can use this base class as a generic device
  // to just pull data out of player

  // copy in the data in a generic sort of way - don't attempt to parse it.
  memcpy( last_data, buffer, hdr.size );

  // store the header, too.
  memcpy( &last_header, &hdr, sizeof( hdr ) );

  //if(player_debug_level(-1) >= 1)
  //fputs("WARNING: virtual FillData() was called.\n",stderr);
}

// interface that all proxies SHOULD provide
void ClientProxy::Print()
{
  puts("Don't know how to print this device.");
}

