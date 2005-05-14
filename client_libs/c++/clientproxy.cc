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
 * parent class for client-side device proxies
 */

#include <playerclient.h>
#include <string.h> // for memset
#include <stdio.h>

// constructor will try to get access to the device
ClientProxy::ClientProxy(PlayerClient* pc, 
            unsigned short req_device,
            unsigned short req_index,
            unsigned char req_access)
{
#ifdef PLAYERCLIENT_THREAD
	pthread_mutex_create(&update_lock,NULL);
#endif


  m_device_id.port = pc->port;
  m_device_id.code = req_device;
  m_device_id.index = req_index;

  client = pc;

  timestamp.tv_sec = 0;
  timestamp.tv_usec = 0;
  senttime.tv_sec = 0;
  senttime.tv_usec = 0;
  receivedtime.tv_sec = 0;
  receivedtime.tv_usec = 0;
  valid = false;
  fresh = false;

  /*
  memset(&last_data,0,sizeof(last_data));
  memset(&last_header,0,sizeof(last_header));
  */

  // start out with no access
  unsigned char grant_access = 'e';
   
  // add it to our client's list to manage
  if(client)
    client->AddProxy(this);

  //printf( "requesting access to device %d:%d:%d\n",
  //  m_device_id.port, m_device_id.code, m_device_id.index );

  if(client && req_access!='c')
  {
    client->RequestDeviceAccess(m_device_id,
                                req_access, &grant_access,
                                driver_name, sizeof(driver_name));

    if((req_access != grant_access) && (player_debug_level(-1) >= 1))
      printf("WARNING: tried to get '%c' access to device %d:%d:%d but got "
             "'%c' access.\n", 
	     req_access,
             m_device_id.port, m_device_id.code, m_device_id.index, 
	     grant_access);
  }
  else
  {
    /*
    if(player_debug_level(-1) >= 4)
      printf("WARNING: couldn't open device %d:%d:%d because no client "
 	     "object is available\n", m_device_id.robot,m_device_id.code,
             m_device_id.index);
     */
  }

  access = grant_access;
}

// destructor will try to close access to the device
ClientProxy::~ClientProxy()
{
#ifdef PLAYERCLIENT_THREAD
	pthread_mutex_destroy(&update_lock,NULL);
#endif

  if(client)
  {
    if((access != 'c') && (access != 'e'))
      client->RequestDeviceAccess(m_device_id,'c',NULL);
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
    if(client->RequestDeviceAccess(m_device_id, req_access, 
                                   &our_grant_access,
                                   driver_name, sizeof(driver_name)))
    {
      if(player_debug_level(-1) >= 1)
        puts("WARNING: RequestDeviceAccess() errored");
      return(-1);
    }

    if((req_access != our_grant_access) && (player_debug_level(-1) >= 1))
      printf("WARNING: tried to get '%c' access to device %d:%d but got "
 	     "'%c' access.\n", req_access, m_device_id.code,
             m_device_id.index, our_grant_access);
  }
  else
  {
    puts("no client");
    return(-1);
  }

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
  // (the data is actually copied in by StoreData(), which is called before
  // this method).

  //if(player_debug_level(-1) >= 1)
  //fputs("WARNING: virtual FillData() was called.\n",stderr);
}

// functionality currently disabled, until it is fixed to handle big messages.
#if 0
// This method is used internally to keep a copy of the last message from
// the device
void ClientProxy::StoreData(player_msghdr_t hdr, const char* buffer)
{
  // copy in the data in a generic sort of way - don't attempt to parse it.
  memcpy(last_data, buffer, hdr.size);

  // store the header, too.
  memcpy(&last_header, &hdr, sizeof(hdr));
}
#endif

// interface that all proxies SHOULD provide
void ClientProxy::FillGeom(player_msghdr_t hdr, const char* buffer)
{
  if(player_debug_level(-1) >= 1)
    fputs("WARNING: Proxy Doesnt support FillGeom\n",stderr);
}

// interface that all proxies SHOULD provide
void ClientProxy::FillConfig(player_msghdr_t hdr, const char* buffer)
{
  if(player_debug_level(-1) >= 1)
    fputs("WARNING: Proxy Doesnt support FillConfig\n",stderr);
}

// interface that all proxies SHOULD provide
void ClientProxy::Print()
{
  puts("Don't know how to print this device.");
}

// thread lock and unlock functions
#ifdef PLAYERCLIENT_THREAD
int ClientProxy::Lock()
{
	return pthread_mutex_lock(&update_lock);
}

int ClientProxy::Unlock()
{
	return pthread_mutex_unlock(&update_lock);
}
#else
int ClientProxy::Lock()
{
	return 0;
}

int ClientProxy::Unlock()
{
	return 0;
}
#endif
