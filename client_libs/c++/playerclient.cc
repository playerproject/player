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
 * The C++ client
 */

#include <playerclient.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

// just make a client, and connect, if instructed
PlayerClient::PlayerClient(const char* hostname = NULL, int port=PLAYER_PORTNUM)
{
  destroyed = false;
  // so we know we're not connected
  conn.sock = -1;
  bzero(conn.banner,sizeof(conn.banner));
  proxies = (ClientProxyNode*)NULL;
  num_proxies = 0;
  fresh = false;

  memset( this->hostname, 0, 256 );
  this->port = 0;

  this->timestamp.tv_sec = 0;
  this->timestamp.tv_usec = 0;
  
  reserved = 0;

  if(hostname)
  {
    if(Connect(hostname, port))
    {
      if(player_debug_level(-1) >= 2)
        fprintf(stderr, "WARNING: unable to connect to \"%s\" on port %d\n",
                hostname, port);
    }
  }
}

// destructor
PlayerClient::~PlayerClient()
{
  destroyed=true;
  Disconnect();
}

int PlayerClient::Connect(const char* hostname, int port)
{
  // store the hostname and port
  strncpy( this->hostname, hostname, 255 );
  this->port = port;

  Disconnect();
  return(player_connect(&conn, hostname, port));
}

int PlayerClient::Disconnect()
{
  if(!destroyed)
  {
    for(ClientProxyNode* thisnode=proxies;thisnode;thisnode=thisnode->next)
    {
      if(thisnode->proxy)
        thisnode->proxy->access = 'c';
    }
  }

  if(Connected())
    return(player_disconnect(&conn));
  else
    return(0);
}

int PlayerClient::Read()
{
  player_msghdr_t hdr;
  char buffer[PLAYER_MAX_MESSAGE_SIZE];
  struct timeval curr;
  ClientProxy* thisproxy;
  
  if(!Connected())
    return(-1);
  // read as many times as necessary
  for(int numreads = CountReadProxies();numreads;numreads--)
  {
    if(player_read(&conn, &hdr, buffer, sizeof(buffer)))
    {
      // mark this client as having fresh data
      fresh = true;

      if(player_debug_level(-1) >= 2)
        fputs("WARNING: player_read() errored\n", stderr);
      return(-1);
    }
    gettimeofday(&curr,NULL);

    // update timestamp
    if ((int) hdr.timestamp_usec > this->timestamp.tv_usec ||
        (int) hdr.timestamp_sec > this->timestamp.tv_sec)
    {
        this->timestamp.tv_sec = hdr.timestamp_sec;
        this->timestamp.tv_usec = hdr.timestamp_usec;
    }

    //printf( "PlayerClient::Read() %d reads: (X,%d,%d)\n",
    //    numreads, hdr.device, hdr.device_index );

    if(!(thisproxy = GetProxy(hdr.device,hdr.device_index)))
    {
      if(player_debug_level(-1) >= 3)
        fprintf(stderr,"WARNING: read unexpected data for device %d:%d\n",
                hdr.device, hdr.device_index);
      continue;
    }

    // put the data in the object
    if(hdr.size)
      thisproxy->FillData(hdr,buffer);
    
    // fill in the timestamps
    thisproxy->timestamp.tv_sec = hdr.timestamp_sec;
    thisproxy->timestamp.tv_usec = hdr.timestamp_usec;
    thisproxy->senttime.tv_sec = hdr.time_sec;
    thisproxy->senttime.tv_usec = hdr.time_usec;
    //printf("setting receivedtime: %ld %ld\n", curr.tv_sec,curr.tv_usec);
    thisproxy->receivedtime.tv_sec = curr.tv_sec;
    thisproxy->receivedtime.tv_usec = curr.tv_usec;
  }

  return(0);
}
    
//
// write a message to our connection.  
//
// Returns:
//    0 if everything went OK
//   -1 if something went wrong (you should probably close the connection!)
//
int PlayerClient::Write(unsigned short device, unsigned short index,
          const char* command, size_t commandlen)
{
  if(!Connected())
    return(-1);

  if(!GetReserved())
    return(player_write(&conn,device,index,command,commandlen));
  else
    return(_player_write(&conn,device,index,command,commandlen,GetReserved()));
}

int PlayerClient::Request(unsigned short device,
                          unsigned short index,
                          const char* payload,
                          size_t payloadlen,
                          player_msghdr_t* replyhdr,
                          char* reply, size_t replylen)
{
  if(!Connected())
    return(-1);
  return(player_request(&conn, device, index, payload, payloadlen,
                        replyhdr, reply, replylen));
}

// request access to a device, meant for use by client-side device
// proxy constructors
//
// req_access is requested access
// grant_access, if non-NULL, will be filled with the granted access
//
// Returns:
//   0 if everything went OK
//   -1 if something went wrong (you should probably close the connection!)
int PlayerClient::RequestDeviceAccess(unsigned short device,
                                      unsigned short index,
                                      unsigned char req_access,
                                      unsigned char* grant_access)
{
  if(!Connected())
    return(-1);
  
  return(player_request_device_access(&conn, device, index, 
                                      req_access, grant_access));
}
    
// Player device configurations

// change continuous data rate (freq is in Hz)
int PlayerClient::SetFrequency(unsigned short freq)
{
  player_device_ioctl_t this_ioctl;
  player_device_datafreq_req_t this_req;
  char payload[sizeof(this_ioctl)+sizeof(this_req)];

  this_ioctl.subtype = htons(PLAYER_PLAYER_DATAFREQ_REQ);
  this_req.frequency = htons(freq);

  memcpy(payload,&this_ioctl,sizeof(this_ioctl));
  memcpy(payload+sizeof(this_ioctl),&this_req,sizeof(this_req));

  return(Request(PLAYER_PLAYER_CODE, 0, payload, sizeof(payload),NULL,0,0));
}

// change data delivery mode
//   1 = request/reply
//   0 = continuous (default)
int PlayerClient::SetDataMode(unsigned char mode)
{
  player_device_ioctl_t this_ioctl;
  player_device_datamode_req_t this_req;
  char payload[sizeof(this_ioctl)+sizeof(this_req)];

  this_ioctl.subtype = htons(PLAYER_PLAYER_DATAMODE_REQ);
  this_req.mode = mode;

  memcpy(payload,&this_ioctl,sizeof(this_ioctl));
  memcpy(payload+sizeof(this_ioctl),&this_req,sizeof(this_req));

  return(Request(PLAYER_PLAYER_CODE, 0, payload, sizeof(payload),NULL,0,0));
}

// request a round of data (only valid when in request/reply mode)
int PlayerClient::RequestData()
{
  player_device_ioctl_t this_ioctl;

  this_ioctl.subtype = htons(PLAYER_PLAYER_DATA_REQ);

  return(Request(PLAYER_PLAYER_CODE, 0, (char*)&this_ioctl, sizeof(this_ioctl)));
}

// authenticate
int PlayerClient::Authenticate(char* key)
{
  player_device_ioctl_t this_ioctl;
  player_device_auth_req_t this_req;
  char payload[sizeof(this_ioctl)+sizeof(this_req)];

  this_ioctl.subtype = htons(PLAYER_PLAYER_AUTH_REQ);
  strncpy(this_req.auth_key,key,sizeof(this_req.auth_key));

  memcpy(payload,&this_ioctl,sizeof(this_ioctl));
  memcpy(payload+sizeof(this_ioctl),&this_req,sizeof(this_req));

  return(Request(PLAYER_PLAYER_CODE, 0, payload, sizeof(payload),NULL,0,0));
}

// proxy list management methods

// add a proxy to the list
void PlayerClient::AddProxy(ClientProxy* proxy)
{
  if(proxy)
  {
    // if nothing in the list, make a head
    if(!proxies)
    {
      if(!(proxies = new ClientProxyNode))
      {
        if(player_debug_level(-1)>=0)
          fputs("PlayerClient::AddProxy(): new failed. Out of memory?",
                stderr);
        return;
      }
      proxies->proxy = proxy;
      proxies->next = (ClientProxyNode*)NULL;
    }
    else
    {
      ClientProxyNode* thisnode;
      for(thisnode = proxies; thisnode->next; thisnode = thisnode->next);

      if(!(thisnode->next = new ClientProxyNode))
      {
        if(player_debug_level(-1)>=0)
          fputs("PlayerClient::AddProxy(): new failed. Out of memory?",
                stderr);
        return;
      }
      thisnode->next->proxy = proxy;
      thisnode->next->next = (ClientProxyNode*)NULL;

    }
    num_proxies++;
  }
}

// remove a proxy from the list
void PlayerClient::RemoveProxy(ClientProxy* proxy)
{
  if(proxy)
  {
    ClientProxyNode* thisnode;
    ClientProxyNode* prevnode;
    for(thisnode = proxies, prevnode = (ClientProxyNode*)NULL; 
        thisnode; 
        prevnode = thisnode, thisnode=thisnode->next)
    {
      if(thisnode->proxy == proxy)
      {
        if(prevnode)
          prevnode->next = thisnode->next;
        else
          proxies=(ClientProxyNode*)NULL;
        delete thisnode;
        num_proxies--;
        break;
      }
    }
  }
}

// count devices with access 'r' or 'a'
int PlayerClient::CountReadProxies()
{
  int numreads = 0;

  for(ClientProxyNode* thisnode=proxies;thisnode;thisnode=thisnode->next)
  {
    if(thisnode->proxy && 
       (thisnode->proxy->access == 'a' || thisnode->proxy->access == 'r'))
      numreads++;
  }

  return(numreads);
}

// get the pointer to the proxy for the given device and index
//
// returns NULL if we can't find it.
//
ClientProxy* PlayerClient::GetProxy(unsigned short device, unsigned short index)
{
  for(ClientProxyNode* thisnode=proxies;thisnode;thisnode=thisnode->next)
  {
    if(thisnode->proxy && 
       thisnode->proxy->device == device &&
       thisnode->proxy->index == index)
      return(thisnode->proxy);
  }

  return((ClientProxy*)NULL);
}


