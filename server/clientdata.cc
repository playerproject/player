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
 *  various methods for managing data pertaining to clients, like
 *  reader and writer threads, permission lists, etc.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdlib.h>
#include <assert.h>

#include "error.h"
#include "device.h"
#include "devicetable.h"
#include "deviceregistry.h"
#include "clientdata.h"
#include "clientmanager.h"
#include "playertime.h"
#include "message.h"

extern PlayerTime* GlobalTime;

//extern DeviceTable* deviceTable;
extern ClientData* clients[];
extern ClientManager* clientmanager;
extern char playerversion[];

void PrintHeader(player_msghdr_t hdr);

ClientData::ClientData(char* key, int myport)
{
  this->requested = NULL;
  this->numsubs = 0;
  this->mode = PLAYER_DATAMODE_PUSH_NEW;
  this->frequency = 10;

  this->port = myport;
  this->socket = -1;
  
  this->readbuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE];
  assert(readbuffer);
  this->replybuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE];
  assert(replybuffer);

  memset((char*)this->readbuffer, 0, PLAYER_MAX_MESSAGE_SIZE);
  memset((char*)this->replybuffer, 0, PLAYER_MAX_MESSAGE_SIZE);
  memset((char*)&this->hdrbuffer, 0, sizeof(player_msghdr_t));

  this->last_write = 0.0;

  this->markedfordeletion = false;

  if(strlen(key))
  {
    strncpy(this->auth_key,key,sizeof(auth_key));
    this->auth_key[sizeof(this->auth_key)-1] = '\0';
    this->auth_pending = true;
  }
  else
  {
    this->auth_pending = false;
  }

  this->datarequested = false;
  hasrequest = false;

  this->OutQueue = new MessageQueue(true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN);
  assert(OutQueue);
}

bool ClientData::CheckAuth(player_msghdr_t hdr, unsigned char* payload,
                            unsigned int payload_size)
{
  player_device_auth_req_t tmpreq;

  if(hdr.device != PLAYER_PLAYER_CODE)
    return(false);

  // ignore the device_index.  can we have more than one player?
  // is the payload in the right size range?
  if((payload_size < 0) || 
     (payload_size > sizeof(player_device_auth_req_t)))
  {
    PLAYER_WARN1("got wrong size ioctl: %d", payload_size);
    return(false);
  }

  memset((char*)&tmpreq,0,sizeof(player_device_auth_req_t));
  tmpreq = *((player_device_auth_req_t*)payload);

  // is it the right kind of ioctl?
  if((hdr.subtype) != PLAYER_PLAYER_AUTH)
    return(false);

  tmpreq.auth_key[sizeof(tmpreq.auth_key)-1] = '\0';

  if(!strcmp(auth_key,(const char*)tmpreq.auth_key))
    return(true);
  else
    return(false);
}

int ClientData::HandleRequests(player_msghdr_t hdr, unsigned char *payload,  
                                size_t payload_size) 
{
  unsigned short requesttype = 0;
  Driver* driver;
  player_device_req_t req;
  player_device_resp_t resp;
  player_device_datamode_req_t datamode;
  player_device_datafreq_req_t datafreq;
  size_t replysize = 0;
  struct timeval curr;

  if(GlobalTime->GetTime(&curr) == -1)
    PLAYER_ERROR("GetTime() failed!!!!");

  // clean the buffer every time for all-day freshness
  memset((char*)this->replybuffer, 0, PLAYER_MAX_MESSAGE_SIZE);

  if(auth_pending)
  {
    if(CheckAuth(hdr,payload,payload_size))
    {
      auth_pending = false;
      requesttype = PLAYER_MSGTYPE_RESP_ACK;
    }
    else
    {
      PLAYER_WARN("failed authentication; closing connection");
      return(-1);
    }
  }
  else
  {
    player_device_id_t id;
    id.port = port;
    id.code = hdr.device;
    id.index = hdr.device_index;

    // if it's for us, handle it here
    if(hdr.device == PLAYER_PLAYER_CODE && hdr.type == PLAYER_MSGTYPE_REQ)
    {
      // ignore the device_index.  can we have more than one player?
      // is the payload big enough?
      switch(hdr.subtype)
      {
        // Process device list requests.
        case PLAYER_PLAYER_DEVLIST:
          requesttype = PLAYER_MSGTYPE_RESP_ACK;
          HandleListRequest((player_device_devlist_t*) payload,
                            (player_device_devlist_t*)this->replybuffer);
          replysize = sizeof(player_device_devlist_t);
          break;

        // Process driver info requests.
        case PLAYER_PLAYER_DRIVERINFO:
          HandleDriverInfoRequest((player_device_driverinfo_t*) payload,
                                  (player_device_driverinfo_t*) replybuffer);
          requesttype = PLAYER_MSGTYPE_RESP_ACK;
          replysize = sizeof(player_device_driverinfo_t);
          break;

        case PLAYER_PLAYER_DEV:
          if(payload_size < sizeof(player_device_req_t))
          {
            PLAYER_WARN1("got small player_device_req_t: %d", payload_size);
            requesttype = PLAYER_MSGTYPE_RESP_NACK;
            break;
          }
          req = *((player_device_req_t*)payload);
          req.code = ntohs(req.code);
          req.index = ntohs(req.index);
          UpdateRequested(req);
          resp.code = htons(req.code);
          resp.index = htons(req.index);

          player_device_id_t id;
          id.port = port; 
          id.code = (req.code);
          id.index = (req.index);
          resp.access = FindPermission(id);

          char* drivername;
          if((drivername = deviceTable->GetDriverName(id)))
            strncpy((char*)resp.driver_name, drivername, sizeof(resp.driver_name));
          else
            resp.driver_name[0]='\0';
          memcpy(replybuffer, &resp, sizeof(resp));
          
          requesttype = PLAYER_MSGTYPE_RESP_ACK;
          replysize = sizeof(resp);
          break;

        case PLAYER_PLAYER_DATAMODE:
          if(payload_size != sizeof(player_device_datamode_req_t))
          {
            PLAYER_WARN1("got wrong size player_device_datamode_req_t: %d",
                         payload_size);
            requesttype = PLAYER_MSGTYPE_RESP_NACK;
            break;
          }
          datamode = *((player_device_datamode_req_t*)payload);
          switch(datamode.mode)
          {
            case PLAYER_DATAMODE_PULL_NEW:
              this->datarequested=false;
              this->mode = PLAYER_DATAMODE_PULL_NEW;
              this->OutQueue->SetReplace(true);
              requesttype = PLAYER_MSGTYPE_RESP_ACK;
              break;
            case PLAYER_DATAMODE_PULL_ALL:
              this->datarequested=false;
              this->mode = PLAYER_DATAMODE_PULL_ALL;
              this->OutQueue->SetReplace(true);
              requesttype = PLAYER_MSGTYPE_RESP_ACK;
              break;
            case PLAYER_DATAMODE_PUSH_ALL:
              this->mode = PLAYER_DATAMODE_PUSH_ALL;
              this->OutQueue->SetReplace(true);
              requesttype = PLAYER_MSGTYPE_RESP_ACK;
              break;
            case PLAYER_DATAMODE_PUSH_NEW:
              this->mode = PLAYER_DATAMODE_PUSH_NEW;
              this->OutQueue->SetReplace(true);
              requesttype = PLAYER_MSGTYPE_RESP_ACK;
              break;
            case PLAYER_DATAMODE_PUSH_ASYNC:
              this->mode = PLAYER_DATAMODE_PUSH_ASYNC;
              this->OutQueue->SetReplace(false);
              requesttype = PLAYER_MSGTYPE_RESP_ACK;
              break;
            default:
              PLAYER_WARN1("unknown I/O mode requested (%d)."
                           "Ignoring request", datamode.mode);
              requesttype = PLAYER_MSGTYPE_RESP_NACK;
              break;
          }
          break;
        case PLAYER_PLAYER_DATA:
          // this ioctl takes no args
          if(payload_size != sizeof(player_device_data_req_t))
          {
            PLAYER_WARN1("got wrong size arg for "
                         "player_data_req: %d",payload_size);
            requesttype = PLAYER_MSGTYPE_RESP_NACK;
            break;
          }
          if((mode != PLAYER_DATAMODE_PULL_ALL) &&
             (mode != PLAYER_DATAMODE_PULL_NEW))
          {
            PLAYER_WARN("got request for data when not in "
                        "request/reply mode");
            requesttype = PLAYER_MSGTYPE_RESP_NACK;
          }
          else
          {
            datarequested = true;
            requesttype = PLAYER_MSGTYPE_RESP_ACK;
          }
          break;
        case PLAYER_PLAYER_DATAFREQ:
          if(payload_size != sizeof(player_device_datafreq_req_t))
          {
            PLAYER_WARN1("got wrong size arg for update frequency "
                         "change: %d",payload_size);
            requesttype = PLAYER_MSGTYPE_RESP_NACK;
            break;
          }
          datafreq = *((player_device_datafreq_req_t*)payload);
          frequency = ntohs(datafreq.frequency);
          requesttype = PLAYER_MSGTYPE_RESP_ACK;
          break;
        case PLAYER_PLAYER_AUTH:
          PLAYER_WARN("unnecessary authentication request");
          requesttype = PLAYER_MSGTYPE_RESP_NACK;
          break;
        case PLAYER_PLAYER_NAMESERVICE:
          HandleNameserviceRequest((player_device_nameservice_req_t*)payload,
                                   (player_device_nameservice_req_t*)replybuffer);
          requesttype = PLAYER_MSGTYPE_RESP_ACK;
          replysize = sizeof(player_device_nameservice_req_t);
          break;
        case PLAYER_PLAYER_IDENT:
          // we dont need to do anything here?
          break;
        default:
          PLAYER_WARN1("Unknown server ioctl %x", hdr.subtype);
          requesttype = PLAYER_MSGTYPE_RESP_NACK;
          break;
      }
    }
    else
    {
      // it's for another device.  hand it off.
      // make sure we've opened this one, in any mode
      if((CheckOpenPermissions(id) && 
          !(hdr.type == PLAYER_MSGTYPE_CMD)) || 
         CheckWritePermissions(id))
      {
        // pass the config request on the proper device
        // make sure we've got a non-NULL pointer
        if((driver = deviceTable->GetDriver(id)))
        {
          // create a message and enqueue it to driver
          Message New(hdr,payload,hdr.size, this);
          driver->InQueue->Push(New);
          requesttype = 0;
        }
        else
        {
          PLAYER_WARN2("got request for unknown device: %x:%x",
                       id.code,id.index);
          requesttype = PLAYER_MSGTYPE_RESP_ERR;
        }
      }
      else
      {
        PLAYER_WARN2("No permissions to configure %x:%x",
                     id.code,id.index);
        requesttype = PLAYER_MSGTYPE_RESP_ERR;
      }
    }
  }

  /* if it's a request, then we must generate a reply */
  if(requesttype)
  {
    GlobalTime->GetTime(&curr);

    PutMsg(requesttype, hdr.subtype, hdr.device, hdr.device_index, 
           &curr,replysize,replybuffer);

    // write data to the client immediately
    if(this->Write() < 0)
      return(-1);
  }
  return(0);
}

ClientData::~ClientData() 
{
  this->RemoveRequests();

  if(this->socket >= 0) 
  {
    close(this->socket);
    printf("** Player [port %d] killing client on socket %d **\n", 
           this->port, this->socket);
  }

  if(this->readbuffer)
    delete[] this->readbuffer;

  if(this->replybuffer)
    delete[] this->replybuffer;
}

void ClientData::RemoveRequests() 
{
  DeviceSubscription* thissub = requested;
  DeviceSubscription* tmpsub;

  while(thissub)
  {
    switch(thissub->access) 
    {
      case PLAYER_ALL_MODE:
      case PLAYER_READ_MODE:
      case PLAYER_WRITE_MODE:
        Unsubscribe(thissub->id);
        break;
      default:
        break;
    }
    tmpsub = thissub->next;
    delete thissub;
    thissub = tmpsub;
  }
  requested = NULL;
}


// Handle device list requests.
void
ClientData::HandleListRequest(player_device_devlist_t *req,
                              player_device_devlist_t *rep)
{
  Device *device;

  rep->device_count = 0;

  // Get all the device entries that have the right port number.
  // TODO: test this with Stage.
  for (device = deviceTable->GetFirstDevice(); device != NULL;
       device = deviceTable->GetNextDevice(device))
  {
    assert(rep->device_count < ARRAYSIZE(rep->devices));
    if (device->id.port == port)
    {
      rep->devices[rep->device_count].code = htons(device->id.code);
      rep->devices[rep->device_count].index = htons(device->id.index);
      rep->devices[rep->device_count].port = htons(device->id.port);
      rep->device_count++;
    }
  }

  // Do some byte swapping.
  rep->device_count = htons(rep->device_count);
}


// Handle driver info requests.
void ClientData::HandleDriverInfoRequest(player_device_driverinfo_t *req,
                                          player_device_driverinfo_t *rep)
{
  char *driver_name;
  player_device_id_t id;

  id.code = ntohs(req->id.code);
  id.index = ntohs(req->id.index);
  id.port = ntohs(req->id.port);

  driver_name = deviceTable->GetDriverName(id);
  if (driver_name)
    strcpy(rep->driver_name, driver_name);
  else
    strcpy(rep->driver_name, "unknown");

//  rep->subtype = htons(PLAYER_PLAYER_DRIVERINFO_REQ);
  req->id.code = req->id.code;
  req->id.index = req->id.index;
  req->id.port = req->id.port;
  return;
}

void ClientData::HandleNameserviceRequest(player_device_nameservice_req_t *req,
                                           player_device_nameservice_req_t *rep)
{
  Device *device;

//  rep->subtype = PLAYER_PLAYER_NAMESERVICE_REQ;
  strncpy((char*)rep->name,(char*)req->name,sizeof(rep->name));
  rep->name[sizeof(rep->name)-1]='\0';
  rep->port=0;

  for(device = deviceTable->GetFirstDevice();
      device;
      device = deviceTable->GetNextDevice(device))
  {
    if(!strcmp((char*)req->name,(char*)device->robotname))
    {
      rep->port = htons(device->id.port);
      break;
    }
  }

  return;
}

// TODO: check permissions as registered by the underlying driver
unsigned char 
ClientData::UpdateRequested(player_device_req_t req)
{
  DeviceSubscription* thissub;
  DeviceSubscription* prevsub;

  // find place to put the update
  for(thissub=requested,prevsub=NULL;thissub;
      prevsub=thissub,thissub=thissub->next)
  {
    if((thissub->id.code == req.code) && (thissub->id.index == req.index))
      break;
  }

  if(!thissub)
  {
    thissub = new DeviceSubscription;
    thissub->id.code = req.code;
    thissub->id.index = req.index;
    thissub->id.port = port;
    thissub->driver = deviceTable->GetDriver(thissub->id);

    thissub->access = PLAYER_ERROR_MODE;

    if(prevsub)
      prevsub->next = thissub;
    else
      requested = thissub;
    numsubs++;
  }

  unsigned char allowed_access = deviceTable->GetDeviceAccess(thissub->id);

  if(allowed_access == PLAYER_ERROR_MODE)
  {
    PLAYER_WARN3("not allowing subscription to unknown device \"%d:%s:%d\"",
                       thissub->id.port,
                       ::lookup_interface_name(0, thissub->id.code),
                       thissub->id.index);
    return(PLAYER_ERROR_MODE);
  }


  if(req.access != thissub->access)
  {
    switch(req.access)
    {
      case PLAYER_CLOSE_MODE:
        // client wants to close
        if((thissub->access != PLAYER_READ_MODE) ||
           (thissub->access != PLAYER_WRITE_MODE) ||
           (thissub->access != PLAYER_ALL_MODE))
        {
          // it was open, so Unsubscribe
          this->Unsubscribe(thissub->id);
        }
        // regardless, now mark it as closed
        thissub->access = PLAYER_CLOSE_MODE;
        break;
      case PLAYER_READ_MODE:
      case PLAYER_WRITE_MODE:
      case PLAYER_ALL_MODE:
        // client wants to open it in some fashion

        // make sure that the requested access is allowed
        if((allowed_access != PLAYER_ALL_MODE) &&
           (allowed_access != req.access))
        {
          PLAYER_WARN4("not granting unallowed access '%c' to device \"%d:%s:%d\"",
                       req.access,thissub->id.port,
                       ::lookup_interface_name(0, thissub->id.code),
                       thissub->id.index);
        }
        else
        {
          if((thissub->access == PLAYER_CLOSE_MODE) ||
             (thissub->access == PLAYER_ERROR_MODE))
          {
            // it wasn't already open, so Subscribe
            if(this->Subscribe(thissub->id) == 0)
            {
              // Subscribe succeeded, so grant requested access
              thissub->access = req.access;
            }
            else
            {
              // Subscribe failes, so mark it as ERROR
              thissub->access = PLAYER_ERROR_MODE;
            }
          }
          else
          {
            // it was already open, so merely grant the new access
            thissub->access = req.access;
          }
        }
        break;
      default:
        PLAYER_WARN1("received subscription request for unknown mode %c",
                     req.access);
        break;
    }
  }

  return(thissub->access);
}

unsigned char 
ClientData::FindPermission(player_device_id_t id)
{
  unsigned char tmpaccess;
  for(DeviceSubscription* thisub=requested;
      thisub;
      thisub=thisub->next)
  {
    if((thisub->id.code == id.code) && (thisub->id.index == id.index))
    {
      tmpaccess = thisub->access;
      return(tmpaccess);
    }
  }
  return(PLAYER_ERROR_MODE);
}

bool ClientData::CheckOpenPermissions(player_device_id_t id)
{
  bool permission = false;
  unsigned char letter;

  letter = FindPermission(id);

  if((letter==PLAYER_ALL_MODE) || 
     (letter==PLAYER_READ_MODE) || 
     (letter==PLAYER_WRITE_MODE))
    permission = true;

  return(permission);
}

bool ClientData::CheckWritePermissions(player_device_id_t id)
{
  bool permission = false;
  unsigned char letter;

  letter = FindPermission(id);

  if((letter==PLAYER_ALL_MODE) || (PLAYER_WRITE_MODE==letter)) 
    permission = true;

  return(permission);
}

int ClientData::Subscribe(player_device_id_t id)
{
  Driver* driver;
  int subscribe_result;

  if((driver = deviceTable->GetDriver(id)))
  {
    subscribe_result = driver->Subscribe(id);
    return(subscribe_result);
  }
  else
  {
    PLAYER_WARN3("Unknown device \"%d:%s:%d\" - subscribe cancelled", 
                 id.port,::lookup_interface_name(0, id.code),id.index);
    return(1);
  }
}


void ClientData::Unsubscribe(player_device_id_t id)
{
  Driver* driver;

  if((driver = deviceTable->GetDriver(id)))
  {
    driver->Unsubscribe(id);
  }
  else
  {
    PLAYER_WARN3("Unknown device \"%d:%s:%d\" - unsubscribe cancelled", 
                 id.port,::lookup_interface_name(0, id.code),id.index);
  }
}

void 
ClientData::PutMsg(uint8_t type, 
		   uint8_t subtype, 
		   uint16_t device, 
		   uint16_t device_index, 
		   struct timeval* timestamp,
		   uint32_t size, 
		   unsigned char * data)
{
  player_msghdr_t hdr;
  memset(&hdr,0,sizeof(player_msghdr_t));
  hdr.stx = htons(PLAYER_STXX);
  hdr.type=type;
  hdr.subtype=subtype;
  hdr.device=htons(device);
  hdr.device_index=htons(device_index);
  hdr.timestamp_sec=htonl(timestamp->tv_sec);
  hdr.timestamp_usec=htonl(timestamp->tv_usec);
  hdr.size=htonl(size); // size of message data

  if (type == PLAYER_MSGTYPE_REQ || type == PLAYER_MSGTYPE_RESP_ACK
    || type == PLAYER_MSGTYPE_RESP_NACK || type == PLAYER_MSGTYPE_RESP_ERR)
    hasrequest = true;

  Message New(hdr,data,size);
  OutQueue->Push(New);
}

/*************************************************************************
 * ClientDataTCP
 *************************************************************************/
ClientDataTCP::ClientDataTCP(char* key, int port) : ClientData(key, port) 
{
  this->totalwritebuffersize = PLAYER_MAX_MESSAGE_SIZE;
  // totalwritebuffer is being malloc()ed, instead of new[]ed, so that it can 
  // be realloc()ed later on (for some reason, C++ does not provide a builtin 
  // equivalent of realloc()).  for consistency, we should probably pick either
  // new[] or malloc() throughout this file.
  this->totalwritebuffer = (uint8_t*)calloc(1,this->totalwritebuffersize);
  assert(totalwritebuffer);
  this->usedwritebuffersize = 0;
  this->warned = false;
  this->readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
  this->readcnt = 0;
  this->leftover_size=0;
};

ClientDataTCP::~ClientDataTCP()
{
  if(this->totalwritebuffer)
    free(this->totalwritebuffer);
}

int 
ClientDataTCP::Read()
{
  int thisreadcnt;
  bool msgready = false;
  char c;

  switch(this->readstate)
  {
    case PLAYER_AWAITING_FIRST_BYTE_STX:
      readcnt = 0;

      if(read(socket,&c,1) <= 0)
      {
        if(errno == EAGAIN)
          break;
        else
          return(-1);
      }
      // This should be the high byte (we're in network byte order)
      if(c == PLAYER_STXX >> 8)
      {
        readcnt = 1;
        this->readstate = PLAYER_AWAITING_SECOND_BYTE_STX;
      }
      break;
    case PLAYER_AWAITING_SECOND_BYTE_STX:
      if(read(socket,&c,1) <= 0)
      {
        if(errno == EAGAIN)
          break;
        else
          return(-1);
      }
      if(c == (char)(PLAYER_STXX & 0x00FF))
      {
        hdrbuffer.stx = PLAYER_STXX;
        readcnt += 1;
        this->readstate = PLAYER_AWAITING_REST_OF_HEADER;
      }
      else
      {
        readcnt = 0;
        this->readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
      }
      break;
    case PLAYER_AWAITING_REST_OF_HEADER:
      /* get the rest of the header */
      if((thisreadcnt = read(socket, (char*)(&hdrbuffer)+readcnt, 
                             sizeof(player_msghdr_t)-readcnt)) <= 0)
      {
        if(errno == EAGAIN)
          break;
        else
          return(-1);
      }
      readcnt += thisreadcnt;
      if(readcnt == sizeof(player_msghdr_t))
      {
        // byte-swap as necessary
        hdrbuffer.type = (hdrbuffer.type);
        hdrbuffer.subtype = (hdrbuffer.subtype);
        hdrbuffer.device = ntohs(hdrbuffer.device);
        hdrbuffer.device_index = ntohs(hdrbuffer.device_index);
        hdrbuffer.time_sec = ntohl(hdrbuffer.time_sec);
        hdrbuffer.time_usec = ntohl(hdrbuffer.time_usec);
        hdrbuffer.timestamp_sec = ntohl(hdrbuffer.timestamp_sec);
        hdrbuffer.timestamp_usec = ntohl(hdrbuffer.timestamp_usec);
        hdrbuffer.seq = ntohs(hdrbuffer.seq);
        hdrbuffer.conid = ntohs(hdrbuffer.conid);
        hdrbuffer.size = ntohl(hdrbuffer.size);
	
        // make sure it's not too big
        if(hdrbuffer.size > PLAYER_MAX_MESSAGE_SIZE-sizeof(player_msghdr_t))
        {
          PLAYER_WARN1("client's message is too big (%d bytes). Ignoring",
                 hdrbuffer.size);
          readcnt = 0;
          this->readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
        }
        else if(hdrbuffer.size == 0)
        {
          readcnt = 0;
          this->readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
          msgready = true;
        }
        else
        {
          readcnt = 0;
          this->readstate = PLAYER_AWAITING_REST_OF_BODY;
        }
      }
      break;
    case PLAYER_AWAITING_REST_OF_BODY:
      /* get the payload */
      if((thisreadcnt = read(socket,readbuffer+readcnt,
                             hdrbuffer.size-readcnt))<=0)
      {
        if(!errno || errno == EAGAIN)
          break;
        else
          return(-1);
      }
      readcnt += thisreadcnt;
      if(readcnt == hdrbuffer.size)
      {
        readcnt = 0;
        this->readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
        msgready = true;
      }
      break;
    default:
      PLAYER_ERROR("in an unknown read state!");
      break;
  }

  if(msgready)
    return(HandleRequests(hdrbuffer,readbuffer,hdrbuffer.size));
  else
    return(0);
}

// Try to write() len bytes from totalwritebuffer, which should have been
// filled with FillWriteBuffer().  If fewer than len bytes are written, 
// the remaining bytes are moved up to the front of totalwritebuffer and 
// their length is recorded in leftover_size.  Returns 0 on success (which
// includes writing fewer than len bytes) and -1 on error (e.g., if the 
// other end of the socket was closed).
int 
ClientDataTCP::Write(bool requst_only)
{
  struct timeval curr;
  int byteswritten;

  GlobalTime->GetTime(&curr);

  if(this->usedwritebuffersize==0)
  {
    // Loop through waiting messages and write them to buffer
    MessageQueueElement * el;
    while((el=this->OutQueue->Pop()))
    {
      size_t totalsize = this->usedwritebuffersize + el->msg.GetSize();

      while(totalsize > this->totalwritebuffersize)
      {
        // need more memory
        this->totalwritebuffersize *= 2;
        totalwritebuffer = (unsigned char*)realloc(totalwritebuffer, totalwritebuffersize);
        assert(totalwritebuffer);
      }

      // Fill in latest server time
      el->msg.GetHeader()->time_sec = htonl(curr.tv_sec);
      el->msg.GetHeader()->time_usec = htonl(curr.tv_usec);
      memcpy(this->totalwritebuffer + this->usedwritebuffersize, 
	     el->msg.GetData(), el->msg.GetSize());	
      this->usedwritebuffersize += el->msg.GetSize();

      delete el;
    }
  }

  // if we have any data then write it
  if(this->usedwritebuffersize > 0)
  {
    if((byteswritten = ::write(this->socket, 
			       this->totalwritebuffer, 
			       this->usedwritebuffersize)) < 0) 
    {
      // EAGAIN is ok; just means the send/receive buffers are filling up
      if(errno == EAGAIN)
	byteswritten = 0;
      else
	return(-1);
    }
    this->usedwritebuffersize -= byteswritten;	
  }

  // data remaining from previous write
  if(this->usedwritebuffersize)
  {
    // didn't get all the data out; move the remaining data to the front of
    // the buffer and record how much is left.
    memmove(this->totalwritebuffer, 
	    this->totalwritebuffer+byteswritten, 
     	    this->usedwritebuffersize);

    if(!this->warned)
    {
      PLAYER_WARN1("%d bytes leftover on write() to client", 
                   usedwritebuffersize);
      this->warned = true;
    }
  }
  else
    this->warned = false;

  return(0);
}

/*************************************************************************
 * ClientDataUDP
 *************************************************************************/
int 
ClientDataUDP::Read()
{
  int numread;

  // Assume for now that we get an entire Player packet within one UDP
  // packet.   Might need to later add support for Player packet
  // fragmentation and re-assembly.
  if((numread = recvfrom(socket,readbuffer,PLAYER_MAX_MESSAGE_SIZE,
                         0,NULL,NULL)) < 0)
  {
    PLAYER_ERROR1("%s",strerror(errno));
    return(-1);
  }

  if(numread < (int)sizeof(player_msghdr_t))
  {
    PLAYER_WARN1("Message too short (%d bytes)", numread);
    return(0);
  }
  memcpy((void*)&hdrbuffer,readbuffer,sizeof(player_msghdr_t));
        
  // byte-swap as necessary
  hdrbuffer.type = (hdrbuffer.type);
  hdrbuffer.subtype = (hdrbuffer.subtype);
  hdrbuffer.device = ntohs(hdrbuffer.device);
  hdrbuffer.device_index = ntohs(hdrbuffer.device_index);
  hdrbuffer.time_sec = ntohl(hdrbuffer.time_sec);
  hdrbuffer.time_usec = ntohl(hdrbuffer.time_usec);
  hdrbuffer.timestamp_sec = ntohl(hdrbuffer.timestamp_sec);
  hdrbuffer.timestamp_usec = ntohl(hdrbuffer.timestamp_usec);
  hdrbuffer.seq = ntohs(hdrbuffer.seq);
  hdrbuffer.conid = ntohs(hdrbuffer.conid);
  hdrbuffer.size = ntohl(hdrbuffer.size);

  memmove(readbuffer,readbuffer+sizeof(player_msghdr_t),
          numread-sizeof(player_msghdr_t));

  return(HandleRequests(hdrbuffer,readbuffer,hdrbuffer.size));

  return(0);
}

int 
ClientDataUDP::Write(bool request_only)
{
  // Loop through waiting messages and write them to buffer
  MessageQueueElement * el;
  while ((el=OutQueue->Pop()))
  {
    int byteswritten;

    // Assume that all data can be written in a single datagram.  Need to
    // make this smarter later.
    if((byteswritten = sendto(socket, el->msg.GetData(), el->msg.GetSize(), 0, 
                              (struct sockaddr*)&clientaddr, 
                              (socklen_t)clientaddr_len)) < 0)
    {
      PLAYER_ERROR1("%s", strerror(errno));
      return(-1);
    }
    delete el;
  }

  return(0);
}

/*************************************************************************
 * ClientDataInternal
 *************************************************************************/
ClientDataInternal::ClientDataInternal(Driver * _driver, 
                                       char * key, 
                                       int port) : ClientData(key, port) 
{
  assert(_driver); 
  driver=_driver;
  this->InQueue = new MessageQueue(true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN);
  assert(InQueue);
}

ClientDataInternal::~ClientDataInternal()
{
}

int 
ClientDataInternal::Read()
{
  // this is a 'psuedo read' basically we take messages from the InQueue and
  // Dispatch them to the drivers
  MessageQueueElement * el;
  while ((el=InQueue->Pop()))
    driver->InQueue->Push(el->msg);

  return(0);
}

int 
ClientDataInternal::Write(bool requst_only)
{
  // Take messages off the queue and give them to the player server for processing
  MessageQueueElement * el;
  while ((el=OutQueue->Pop())) 
  {
    player_msghdr * hdr = el->msg.GetHeader();
    uint8_t * data = el->msg.GetPayload();
    if(HandleRequests(*hdr, data, hdr->size) < 0)
      return -1;
  }	
  return(0);	
}


// Called by client manager for messages heading for client
void 
ClientDataInternal::PutMsg(uint8_t type, 
                           uint8_t subtype, 
                           uint16_t device, 
                           uint16_t device_index, 
                           struct timeval* timestamp,
                           uint32_t size, 
                           unsigned char * data)
{
  player_msghdr hdr;
  hdr.stx = PLAYER_STXX;
  hdr.type = type;
  hdr.subtype = subtype;
  hdr.device = device;
  hdr.device_index = device_index;
  hdr.timestamp_sec = timestamp->tv_sec;
  hdr.timestamp_usec = timestamp->tv_usec;
  hdr.size = size; // size of message data

  Message New(hdr,data,size,this);	
  assert(*New.RefCount);
  InQueue->Push(New);
}

// Called by client driver to send a message to another driver
int 
ClientDataInternal::SendMsg(player_device_id_t device, 
                            uint8_t type, 
                            uint8_t subtype, 
                            uint8_t * data, 
                            size_t size,
                            struct timeval* timestamp)
{
  struct timeval ts;
  if (timestamp == NULL)
    GlobalTime->GetTime(&ts);
  else
    ts = *timestamp;

  player_msghdr hdr;
  hdr.stx = PLAYER_STXX;
  hdr.type = type;
  hdr.subtype = subtype;
  hdr.device = device.code;
  hdr.device_index = device.index;
  hdr.timestamp_sec = ts.tv_sec;
  hdr.timestamp_usec = ts.tv_usec;
  hdr.size = size; // size of message data

  Message New(hdr,data,size,this);
  OutQueue->Push(New);

  return 0;
}

int 
ClientDataInternal::Subscribe(player_device_id_t device, char access)
{
  player_device_req_t req;

  player_msghdr hdr;
  hdr.stx = PLAYER_STXX;
  hdr.type = PLAYER_MSGTYPE_REQ;
  hdr.subtype = PLAYER_PLAYER_DEV;
  hdr.device = PLAYER_PLAYER_CODE;
  hdr.device_index = 0;
  hdr.timestamp_sec = 0;
  hdr.timestamp_usec = 0;
  hdr.size = sizeof(req); // size of message data

  //	req.subtype = htons(PLAYER_PLAYER_DEV_REQ);
  req.code = htons(device.code);
  req.index = htons(device.index);
  req.access = access;

  if (HandleRequests(hdr, (unsigned char *) &req, hdr.size) < 0)
    return -1;
  return 0;
}

int
ClientDataInternal::Unsubscribe(player_device_id_t device)
{
  player_device_req_t req;

  player_msghdr hdr;
  hdr.stx = PLAYER_STXX;
  hdr.type = PLAYER_MSGTYPE_REQ;
  hdr.subtype = PLAYER_PLAYER_DEV;
  hdr.device = PLAYER_PLAYER_CODE;
  hdr.device_index = 0;
  hdr.timestamp_sec = 0;
  hdr.timestamp_usec = 0;
  hdr.size = sizeof(req); // size of message data

  //	req.subtype = htons(PLAYER_PLAYER_DEV_REQ);
  req.code = htons(device.code);
  req.index = htons(device.index);
  req.access = 'c';

  if (HandleRequests(hdr, (unsigned char *) &req, hdr.size) < 0)
    return -1;
  return 0;
}

int
ClientDataInternal::SetDataMode(uint8_t datamode)
{
  player_device_datamode_req req;

  player_msghdr hdr;
  hdr.stx = PLAYER_STXX;
  hdr.type = PLAYER_MSGTYPE_REQ;
  hdr.subtype = PLAYER_PLAYER_DATAMODE;
  hdr.device = PLAYER_PLAYER_CODE;
  hdr.device_index = 0;
  hdr.timestamp_sec = 0;
  hdr.timestamp_usec = 0;
  hdr.size = sizeof(req); // size of message data

  //	req.subtype = htons(PLAYER_PLAYER_DATAMODE_REQ);
  req.mode = datamode;

  if (HandleRequests(hdr, (unsigned char *) &req, hdr.size) < 0)
    return -1;
  return 0;
}
