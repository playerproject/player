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
 * The C++ client
 */

#include <playerclient.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h> // for gethostbyname(3)

// just make a client, and connect, if instructed
PlayerClient::PlayerClient(const char* hostname,
                           const int port,
                           const int protocol)
{
  conn.protocol = protocol;
  destroyed = false;
  // so we know we're not connected
  conn.sock = -1;
  memset(conn.banner,0,sizeof(conn.banner));
  proxies = (ClientProxyNode*)NULL;
  num_proxies = 0;
  fresh = false;

  memset( this->hostname, 0, 256 );
  memset( &this->hostaddr, 0, sizeof(struct in_addr) );
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

// alternate constructor using a binary IP instead of a hostname
// just make a client, and connect, if instructed
PlayerClient::PlayerClient(const struct in_addr* hostaddr, 
                           const int port, 
                           const int protocol)
{
  conn.protocol = protocol;
  destroyed = false;
  // so we know we're not connected
  conn.sock = -1;
  memset(conn.banner,0,sizeof(conn.banner));
  proxies = (ClientProxyNode*)NULL;
  num_proxies = 0;
  fresh = false;

  memset( this->hostname, 0, 256 );
  memset( &this->hostaddr, 0, sizeof(struct in_addr) );
  this->port = 0;

  this->timestamp.tv_sec = 0;
  this->timestamp.tv_usec = 0;
  
  reserved = 0;

  if(hostaddr)
  {
    if(Connect(hostaddr, port))
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
  // look up the IP address from the hostname
  struct hostent *entp = gethostbyname( hostname );
  
  // check for valid address
  if(entp && entp->h_addr_list[0] && (entp->h_length > 0))
  {
    this->hostaddr = *((in_addr*)entp->h_addr_list[0]);

    if(Connect(&this->hostaddr, port) == 0)
    {
      // printf( "connected  to Player on port %d\n", port );
      // connect good - store the hostname and returm sucess
      strncpy(this->hostname, hostname, 255);     
      return(0);
    }
  }
  return(1); // fail - forget this hostname
}

int PlayerClient::Connect(const struct in_addr* addr, int port)
{
  // if we connect this way, we don't bother doing a reverse DNS lookup.
  // we make the hostname blank.
  // this would mess up clients that check the hostname, but they should
  // connect using the hostname and all would be well.
 
  Disconnect(); // make sure we're cleaned up first

  // attempt a connect
  if(player_connect_ip(&conn, addr, port) == 0)
  {
    // connect good - store the address and port
    memcpy(&this->hostaddr, addr, sizeof(struct in_addr));
    this->port = port;
    this->hostname[0] = 0; // nullify the string

    return(0);
  }
  else
    return(1);
}

int 
PlayerClient::ConnectRNS(const char* robotname, const char* hostname, int port)
{
  PlayerClient tmppc;
  int robotport;

  if(tmppc.Connect(hostname, port))
  {
    if(player_debug_level(-1) >= 2)
      fprintf(stderr, 
              "WARNING: couldn't get name service from %s:%d\n",
              hostname, port);
    return(-1);
  }

  if((robotport = tmppc.LookupPort(robotname)) < 0)
  {
    if(player_debug_level(-1) >= 2)
      fprintf(stderr, 
              "WARNING: name service failed at %s:%d\n",
              hostname, port);
    return(-1);
  }
  if(!robotport)
  {
    if(player_debug_level(-1) >= 2)
      fprintf(stderr, 
              "WARNING: name service at %s:%d couldn't find robot \"%s\"\n",
              hostname, port, robotname);
    return(-1);
  }

  tmppc.Disconnect();

  return(Connect(hostname,robotport));
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
  {
    fprintf(stderr,"ERROR PlayerClient not connected\n" );
    return(-1);
  }
  
  // read until we get a SYNCH packet
  for(;;)
  {
    if(player_read(&conn, &hdr, buffer, sizeof(buffer)))
    {
      if(player_debug_level(-1) >= 2)
        fputs("WARNING: player_read() errored\n", stderr);
      return(-1);
    }
    gettimeofday(&curr,NULL);
    // is this the SYNCH packet?
    if(hdr.type == PLAYER_MSGTYPE_SYNCH)
    {
      //printf("%ld:%ld GOT SYNCH PACKET\n",
           //curr.tv_sec,curr.tv_usec);
      break;
    }
    else if(hdr.type == PLAYER_MSGTYPE_DATA)
    {
      // mark this client as having fresh data
      fresh = true;

      // update timestamp
      if ((int) hdr.timestamp_usec > this->timestamp.tv_usec ||
          (int) hdr.timestamp_sec > this->timestamp.tv_sec)
      {
        this->timestamp.tv_sec = hdr.timestamp_sec;
        this->timestamp.tv_usec = hdr.timestamp_usec;
      }

      player_device_id_t id;
      id.code=hdr.device;
      id.index=hdr.device_index;
      if(!(thisproxy = GetProxy(id)))
      {
        if(player_debug_level(-1) >= 3)
          fprintf(stderr,"WARNING: read unexpected data for device %d:%d\n",
                  hdr.device,hdr.device_index);
        continue;
      }

      //printf("%ld:%ld:got new data for %d:%d\n",
             //curr.tv_sec, curr.tv_usec, hdr.device, hdr.device_index);
      thisproxy->Lock();

      // put the data in the object
      if(hdr.size)
      {
        // store an opaque copy
        thisproxy->StoreData(hdr,buffer);
        // also let the device-specific proxy parse it
        thisproxy->FillData(hdr,buffer);
        // let the user know that data has arrived.  set it every time, in
        // case the user wants to use it to determine freshness, in
        // addition to initial validity.
        thisproxy->valid = true;
      }

      // fill in the timestamps
      thisproxy->timestamp.tv_sec = hdr.timestamp_sec;
      thisproxy->timestamp.tv_usec = hdr.timestamp_usec;
      thisproxy->senttime.tv_sec = hdr.time_sec;
      thisproxy->senttime.tv_usec = hdr.time_usec;
      //printf("setting receivedtime: %ld %ld\n", curr.tv_sec,curr.tv_usec);
      thisproxy->receivedtime.tv_sec = curr.tv_sec;
      thisproxy->receivedtime.tv_usec = curr.tv_usec;
      thisproxy->Unlock();

    }
    else
    {
      printf("else\n");
      if(player_debug_level(-1)>=3)
      {
        fprintf(stderr,"PlayerClient::Read(): received unexpected message"
                "type: %d\n", hdr.type);
      }
    }
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
int PlayerClient::Write(player_device_id_t device_id,
          const char* command, size_t commandlen)
{
  if(!Connected())
    return(-1);

  return(player_write(&conn,device_id.code,device_id.index,
                      command,commandlen));
}

int PlayerClient::Request(player_device_id_t device_id,
                          const char* payload,
                          size_t payloadlen,
                          player_msghdr_t* replyhdr,
                          char* reply, size_t replylen)
{
  if(!Connected())
    return(-1);
  return(player_request(&conn, device_id.code,device_id.index, payload, payloadlen,
                        replyhdr, reply, replylen));
}
    
// use this one if you don't want the reply. it will return -1 if 
// the request failed outright or if the response type is not ACK
int PlayerClient::Request(player_device_id_t device_id,
                          const char* payload,
                          size_t payloadlen)
{ 
  int retval;
  player_msghdr_t hdr;
  retval = Request(device_id,payload,payloadlen,&hdr,NULL,0);

  if((retval < 0) || 
     (hdr.type != PLAYER_MSGTYPE_RESP_ACK) || 
     (hdr.device != device_id.code) || 
     (hdr.device_index != device_id.index))
    return(-1);
  else
    return(retval);
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
int PlayerClient::RequestDeviceAccess(player_device_id_t device_id,
                                      unsigned char req_access,
                                      unsigned char* grant_access,
                                      char* driver_name,
                                      int driver_name_len)
{
  int retval;

  if(!Connected())
    return(-1);
  
  retval = player_request_device_access(&conn, device_id.code,device_id.index,
                                        req_access, grant_access, 
                                        driver_name, driver_name_len);

  
  /* if(driver_name)
    printf("%d:%d:%d with driver %s\n", 
	   this->port, device_id.code, device_id.index, driver_name);
  */

  return(retval);
}
    
// Player device configurations

// change continuous data rate (freq is in Hz)
int PlayerClient::SetFrequency(unsigned short freq)
{
  player_device_datafreq_req_t this_req;
  char payload[sizeof(this_req)];
  player_device_id_t id;

  this_req.subtype = htons(PLAYER_PLAYER_DATAFREQ_REQ);
  this_req.frequency = htons(freq);

  memcpy(payload,&this_req,sizeof(this_req));

  id.code=PLAYER_PLAYER_CODE;
  id.index=0;
  return(Request(id, payload, sizeof(payload),NULL,0,0));
}

// change data delivery mode
// valid modes are given in include/messages.h
int PlayerClient::SetDataMode(unsigned char mode)
{
  player_device_datamode_req_t this_req;
  char payload[sizeof(this_req)];
  player_device_id_t id;

  this_req.subtype = htons(PLAYER_PLAYER_DATAMODE_REQ);
  this_req.mode = mode;

  memcpy(payload,&this_req,sizeof(this_req));

  id.code=PLAYER_PLAYER_CODE;
  id.index=0;
  return(Request(id, payload, sizeof(payload),NULL,0,0));
}

// request a round of data (only valid when in request/reply mode)
int PlayerClient::RequestData()
{
  player_device_data_req_t this_req;
  player_device_id_t id;

  this_req.subtype = htons(PLAYER_PLAYER_DATA_REQ);

  id.code=PLAYER_PLAYER_CODE;
  id.index=0;
  return(Request(id,(char*)&this_req, sizeof(this_req)));
}

// authenticate
int PlayerClient::Authenticate(char* key)
{
  player_device_auth_req_t this_req;
  char payload[sizeof(this_req)];
  player_device_id_t id;

  this_req.subtype = htons(PLAYER_PLAYER_AUTH_REQ);
  strncpy((char*)this_req.auth_key,key,sizeof(this_req.auth_key));

  memcpy(payload,&this_req,sizeof(this_req));

  id.code=PLAYER_PLAYER_CODE;
  id.index=0;
  return(Request(id, payload, sizeof(payload),NULL,0,0));
}

// use nameservice to get corresponding port for a robot name (only with
// Stage)
int
PlayerClient::LookupPort(const char* name)
{
  player_device_id_t id;
  player_device_nameservice_req_t req, rep;
  player_msghdr_t hdr;
  memset(&req,0,sizeof(req));
  memset(&rep,0,sizeof(rep));

  req.subtype = htons(PLAYER_PLAYER_NAMESERVICE_REQ);
  strncpy((char*)req.name,name,sizeof(req.name));

  id.code=PLAYER_PLAYER_CODE;
  id.index=0;

  if(Request(id,(char*)&req,sizeof(req),&hdr,(char*)&rep,sizeof(rep)) < 0)
    return(-1);
  else
    return(ntohs(rep.port));
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

// get the pointer to the proxy for the given device and index
//
// returns NULL if we can't find it.
//
ClientProxy* PlayerClient::GetProxy(unsigned short device, unsigned short index)
{
  player_device_id_t id;

  id.code = device;
  id.index = index;
  return(GetProxy(id));
}

ClientProxy* PlayerClient::GetProxy(player_device_id_t id)
{
  for(ClientProxyNode* thisnode=proxies;thisnode;thisnode=thisnode->next)
  {
    if(thisnode->proxy && 
       (thisnode->proxy->m_device_id.code == id.code) &&
       (thisnode->proxy->m_device_id.index == id.index))
      return(thisnode->proxy);
  }

  return((ClientProxy*)NULL);
}


// Get the list of available device ids. The data is written into the
// proxy structure rather than retured to the caller.
int PlayerClient::GetDeviceList()
{
  int i;
 
  player_device_id_t id;
  player_msghdr_t hdr;
  player_device_devlist_t config;

  player_device_driverinfo_t req;
  player_device_driverinfo_t rep;


  config.subtype = htons(PLAYER_PLAYER_DEVLIST_REQ);
  id.code=PLAYER_PLAYER_CODE;
  id.index=0;

  // Request the list of devices
  this->Request(id, (const char*)&config, sizeof(config), &hdr,
      (char *)&config, sizeof(config));

  if (hdr.size <0)             
    return -1;

  if (hdr.size != sizeof(config))
  {
    PLAYER_ERROR2("Device list reply has incorrect length (%d!=%d)", hdr.size, sizeof(config)); 
    return -1;
  }

  // Do some byte swapping
  config.device_count = ntohs(config.device_count);
  for (i = 0; i < config.device_count; i++)
  {
    this->ids[i].code = ntohs(config.devices[i].code);
    this->ids[i].index = ntohs(config.devices[i].index);
    this->ids[i].port = ntohs(config.devices[i].port);
  }
  this->id_count = config.device_count;


   // Get the driver info for all devices. 
  for (i=0; i < this->id_count; i++)
  {
    req.subtype = htons(PLAYER_PLAYER_DRIVERINFO_REQ);
    req.id.code = htons(this->ids[i].code);
    req.id.index = htons(this->ids[i].index);
    req.id.port = htons(this->ids[i].port);

    // Get the device's info
    this->Request(id, (const char*)&req, sizeof(req), &hdr,
        (char *)&rep, sizeof(rep));

    if (hdr.size < 0)
      return -1;

    if (hdr.size != sizeof(rep))
    {
      PLAYER_ERROR2("reply to driverinfo request has incorrect length (%d != %d)",
          hdr.size, sizeof(rep));
      return -1;
    }

    strcpy(this->drivernames[i], rep.driver_name);
  }
 
  return 0;
}

