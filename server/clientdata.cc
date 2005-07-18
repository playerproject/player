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
#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdlib.h>

#include <error.h>
#include <device.h>
#include <devicetable.h>
#include <deviceregistry.h>
#include <clientdata.h>
#include <clientmanager.h>

#include <playertime.h>
extern PlayerTime* GlobalTime;

extern DeviceTable* deviceTable;
extern ClientData* clients[];
extern ClientManager* clientmanager;
extern char playerversion[];

extern int global_playerport; // used to generate useful output & debug
// true if we're connecting to Stage instead of a real robot
extern bool use_stage;

ClientData::ClientData(char* key, int myport)
{
  requested = NULL;
  numsubs = 0;
  socket = 0;
  mode = PLAYER_DATAMODE_PUSH_NEW;
  frequency = 10;

  port = myport;

  assert(readbuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE]);
  assert(replybuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE]);
  assert(writebuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE]);

  leftover_size=0;
  totalwritebuffersize = PLAYER_MAX_MESSAGE_SIZE;
  // totalwritebuffer is being malloc()ed, instead of new[]ed, so that it can 
  // be realloc()ed later on (for some reason, C++ does not provide a builtin 
  // equivalent of realloc()).  for consistency, we should probably pick either
  // new[] or malloc() throughout this file.
  assert(totalwritebuffer = (unsigned char*)malloc(totalwritebuffersize));
  memset(totalwritebuffer,0,totalwritebuffersize);

  memset((char*)readbuffer, 0, PLAYER_MAX_MESSAGE_SIZE);
  memset((char*)writebuffer, 0, PLAYER_MAX_MESSAGE_SIZE);
  memset((char*)replybuffer, 0, PLAYER_MAX_MESSAGE_SIZE);
  memset((char*)totalwritebuffer, 0, totalwritebuffersize);
  memset((char*)&hdrbuffer, 0, sizeof(player_msghdr_t));

  readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
  readcnt = 0;

  last_write = 0.0;

  markedfordeletion = false;

  if(strlen(key))
  {
    strncpy(auth_key,key,sizeof(auth_key));
    auth_key[sizeof(auth_key)-1] = '\0';
    auth_pending = true;
  }
  else
  {
    auth_pending = false;
  }

  datarequested = false;
}

bool ClientData::CheckAuth(player_msghdr_t hdr, unsigned char* payload,
                            unsigned int payload_size)
{
  player_device_auth_req_t tmpreq;

  if(hdr.device != PLAYER_PLAYER_CODE)
    return(false);

  // ignore the device_index.  can we have more than one player?
  // is the payload in the right size range?
  if((payload_size < sizeof(tmpreq.subtype)) || 
     (payload_size > sizeof(player_device_auth_req_t)))
  {
    PLAYER_WARN1("got wrong size ioctl: %d", payload_size);
    return(false);
  }

  memset((char*)&tmpreq,0,sizeof(player_device_auth_req_t));
  tmpreq = *((player_device_auth_req_t*)payload);

  // is it the right kind of ioctl?
  if(ntohs(tmpreq.subtype) != PLAYER_PLAYER_AUTH_REQ)
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
  bool devlistrequest=false;
  bool driverinforequest=false;
  bool nameservicerequest=false;
  bool devicerequest=false;
  Driver* driver;
  player_device_req_t req;
  player_device_resp_t resp;
  player_device_datamode_req_t datamode;
  player_device_datafreq_req_t datafreq;
  player_msghdr_t reply_hdr;
  size_t replysize;
  struct timeval curr;

  if(GlobalTime->GetTime(&curr) == -1)
    PLAYER_ERROR("GetTime() failed!!!!");

  // clean the buffer every time for all-day freshness
  memset((char*)replybuffer, 0, PLAYER_MAX_MESSAGE_SIZE);

  // debug output; leave it here
#if 0
  printf("type:%u device:%u index:%u\n", 
         hdr.type,hdr.device,hdr.device_index);
  printf("Request(%d):",payload_size);
  /*
  for(unsigned int i=0;i<payload_size;i++)
    printf("%c",payload[i]);
  printf("\t");
  */
  for(unsigned int i=0;i<payload_size;i++)
    printf("%x ",payload[i]);
  puts("");
  if(hdr.device == PLAYER_POSITION_CODE)
  {
    player_position_cmd_t* cmd = (player_position_cmd_t*)payload;
    printf("speeds: %d %d\n", 
           (int)ntohl(cmd->xspeed), (int)ntohl(cmd->yawspeed));
  }
#endif

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

    switch(hdr.type)
    {
      case PLAYER_MSGTYPE_REQ:
        /* request message */
        //request = true;
        // if it's for us, handle it here
        if(hdr.device == PLAYER_PLAYER_CODE)
        {
          // ignore the device_index.  can we have more than one player?
          // is the payload big enough?
          if(payload_size < sizeof(req.subtype))
          {
            PLAYER_WARN1("got small ioctl: %d", payload_size);

            requesttype = PLAYER_MSGTYPE_RESP_NACK;
            break;
          }

          // what sort of ioctl is it?
          unsigned short subtype = 
                  ntohs(((player_device_req_t*)payload)->subtype);
          switch(subtype)
          {
            // Process device list requests.
            case PLAYER_PLAYER_DEVLIST_REQ:
              devlistrequest = true;
              HandleListRequest((player_device_devlist_t*) payload,
                                (player_device_devlist_t*) (replybuffer + sizeof(player_msghdr_t)));
              requesttype = PLAYER_MSGTYPE_RESP_ACK;
              break;

              // Process driver info requests.
            case PLAYER_PLAYER_DRIVERINFO_REQ:
              driverinforequest = true;
              HandleDriverInfoRequest((player_device_driverinfo_t*) payload,
                                      (player_device_driverinfo_t*) (replybuffer + sizeof(player_msghdr_t)));
              requesttype = PLAYER_MSGTYPE_RESP_ACK;
              break;

            case PLAYER_PLAYER_DEV_REQ:
              devicerequest = true;
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
              requesttype = PLAYER_MSGTYPE_RESP_ACK;
              break;
            case PLAYER_PLAYER_DATAMODE_REQ:
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
                  /* change to update request/reply */
                  datarequested=false;
                  mode = PLAYER_DATAMODE_PULL_NEW;
                  requesttype = PLAYER_MSGTYPE_RESP_ACK;
                  break;
                case PLAYER_DATAMODE_PULL_ALL:
                  /* change to request/reply */
                  //puts("changing to REQUESTREPLY");
                  datarequested=false;
                  mode = PLAYER_DATAMODE_PULL_ALL;
                  requesttype = PLAYER_MSGTYPE_RESP_ACK;
                  break;
                case PLAYER_DATAMODE_PUSH_ALL:
                  /* change to continuous mode */
                  //puts("changing to CONTINUOUS");
                  mode = PLAYER_DATAMODE_PUSH_ALL;
                  requesttype = PLAYER_MSGTYPE_RESP_ACK;
                  break;
                case PLAYER_DATAMODE_PUSH_NEW:
                  /* change to update mode (doesn't re-send old data)*/
                  //puts("changing to UPDATE");
                  mode = PLAYER_DATAMODE_PUSH_NEW;
                  requesttype = PLAYER_MSGTYPE_RESP_ACK;
                  break;
                case PLAYER_DATAMODE_PUSH_ASYNC:
                  mode = PLAYER_DATAMODE_PUSH_ASYNC;
                  requesttype = PLAYER_MSGTYPE_RESP_ACK;
                  break;
                default:
                  PLAYER_WARN1("unknown I/O mode requested (%d)."
                         "Ignoring request", datamode.mode);
                  requesttype = PLAYER_MSGTYPE_RESP_NACK;
                  break;
              }
              break;
            case PLAYER_PLAYER_DATA_REQ:
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
            case PLAYER_PLAYER_DATAFREQ_REQ:
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
            case PLAYER_PLAYER_AUTH_REQ:
              PLAYER_WARN("unnecessary authentication request");
              requesttype = PLAYER_MSGTYPE_RESP_NACK;
              break;
            case PLAYER_PLAYER_NAMESERVICE_REQ:
              nameservicerequest = true;
              HandleNameserviceRequest((player_device_nameservice_req_t*)payload,
                                       (player_device_nameservice_req_t*) (replybuffer + sizeof(player_msghdr_t)));
              requesttype = PLAYER_MSGTYPE_RESP_ACK;
              break;

            default:
              PLAYER_WARN1("Unknown server ioctl %x", subtype);
              requesttype = PLAYER_MSGTYPE_RESP_NACK;
              break;
          }
        }
        else
        {
          // it's for another device.  hand it off.
          //

          // make sure we've opened this one, in any mode
          if(CheckOpenPermissions(id))
          {
            // pass the config request on the proper device
            // make sure we've got a non-NULL pointer
            if((driver = deviceTable->GetDriver(id)))
            {
              // try to push it on the request queue
              if(driver->PutConfig(id,this,payload,payload_size,&curr))
              {
                // queue was full
                requesttype = PLAYER_MSGTYPE_RESP_ERR;
              }
              else
                requesttype = 0;
            }
            else
            {
              PLAYER_WARN2("got request for unkown device: %x:%x",
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
        break;
      case PLAYER_MSGTYPE_CMD:

        /* command message */
        if(CheckWritePermissions(id))
        {
          // make sure we've got a non-NULL pointer
          if((driver = deviceTable->GetDriver(id)))
          {
            driver->PutCommand(id,payload,payload_size,&curr);
          }
          else
            PLAYER_WARN2("found NULL pointer for device %x:%x",
                         id.code,id.index);
        }
        else 
          PLAYER_WARN2("No permissions to command %x:%x",
                       id.code,id.index);
        break;
      default:
        PLAYER_WARN1("Unknown message type %x", hdr.type);
        break;
    }
  }

  /* if it's a request, then we must generate a reply */
  if(requesttype)
  {
    reply_hdr.stx = htons(PLAYER_STXX);
    reply_hdr.type = htons(requesttype);
    reply_hdr.device = htons(hdr.device);
    reply_hdr.device_index = htons(hdr.device_index);
    reply_hdr.reserved = (uint32_t)0;

    /* if it was a player device list request... */
    if(devlistrequest)
    {
      replysize = sizeof(player_device_devlist_t);
    }
    /* if it was a player device list request... */
    else if(driverinforequest)
    {
      replysize = sizeof(player_device_driverinfo_t);
    }
    /* if it was a player nameservice request... */
    else if(nameservicerequest)
    {
      replysize = sizeof(player_device_nameservice_req_t);
    }
    /* if it was a player device request, then the reply should
     * reflect what permissions were granted for the indicated devices */
    else if(devicerequest)
    {
      resp.subtype = ((player_device_req_t*)payload)->subtype;
      resp.code = ((player_device_req_t*)payload)->code;
      resp.index = ((player_device_req_t*)payload)->index;

      player_device_id_t id;
      id.port = port; 
      id.code = ntohs(resp.code);
      id.index = ntohs(resp.index);
      resp.access = FindPermission(id);

      memset(resp.driver_name, 0, sizeof(resp.driver_name));
      char* drivername;
      if((drivername = deviceTable->GetDriverName(id)))
        strncpy((char*)resp.driver_name, drivername, sizeof(resp.driver_name));

      memcpy(replybuffer+sizeof(player_msghdr_t),&resp,
             sizeof(player_device_resp_t));

      replysize = sizeof(player_device_resp_t);
    }
    /* otherwise, leave it empty */
    else
    {
      //memcpy(replybuffer+sizeof(player_msghdr_t),payload,payload_size);
      replysize = 0;
    }

    reply_hdr.size = htonl(replysize);

    if(GlobalTime->GetTime(&curr) == -1)
      PLAYER_ERROR("GetTime() failed!!!!");
    reply_hdr.time_sec = htonl(curr.tv_sec);
    reply_hdr.time_usec = htonl(curr.tv_usec);

    reply_hdr.timestamp_sec = reply_hdr.time_sec;
    reply_hdr.timestamp_usec = reply_hdr.time_usec;
    memcpy(replybuffer,&reply_hdr,sizeof(player_msghdr_t));

    FillWriteBuffer(replybuffer,
                    leftover_size,
                    replysize+sizeof(player_msghdr_t));
    if(Write(leftover_size+replysize+sizeof(player_msghdr_t)) < 0)
      return(-1);
  }

  return(0);
}

ClientData::~ClientData() 
{
  RemoveRequests();

  if(socket >= 0) 
  {
    close(socket);
    printf("** Player [port %d] killing client on socket %d **\n", 
           port, socket);
  }

  if(readbuffer)
    delete[] readbuffer;

  if(writebuffer)
    delete[] writebuffer;

  if(totalwritebuffer)
    free(totalwritebuffer);

  if(replybuffer)
    delete[] replybuffer;
}

void ClientData::RemoveRequests() 
{
  CDeviceSubscription* thissub = requested;
  CDeviceSubscription* tmpsub;

  while(thissub)
  {
    // Stop position devices if we're the last one to use it (safety)
    if(((thissub->access != PLAYER_CLOSE_MODE) &&
        (thissub->access != PLAYER_ERROR_MODE)) && 
       (thissub->id.code == PLAYER_POSITION_CODE) &&
       (((thissub->access == PLAYER_ALL_MODE) &&
         (thissub->driver->subscriptions == 2)) ||
        (thissub->driver->subscriptions == 1)))
      MotorStop();
    
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

void ClientData::MotorStop() 
{
  player_position_cmd_t command;
  Driver* driver;
  player_device_id_t id;
  struct timeval curr;

  GlobalTime->GetTime(&curr);

  // TODO: fix this for single-port action
  id.port = port;
  id.code = PLAYER_POSITION_CODE;
  id.index = 0;

  command.xspeed = command.yspeed = command.yawspeed = 0;

  if((driver = deviceTable->GetDriver(id)))
  {
    driver->PutCommand(id, (void*)&command, sizeof(command), &curr);
  }
}


// Handle device list requests.
void ClientData::HandleListRequest(player_device_devlist_t *req,
                                    player_device_devlist_t *rep)
{
  Device *device;

  rep->subtype = PLAYER_PLAYER_DEVLIST_REQ;
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
  rep->subtype = htons(rep->subtype);
  rep->device_count = htons(rep->device_count);

  return;
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

  rep->subtype = htons(PLAYER_PLAYER_DRIVERINFO_REQ);
  req->id.code = req->id.code;
  req->id.index = req->id.index;
  req->id.port = req->id.port;
  return;
}

void ClientData::HandleNameserviceRequest(player_device_nameservice_req_t *req,
                                           player_device_nameservice_req_t *rep)
{
  Device *device;

  rep->subtype = PLAYER_PLAYER_NAMESERVICE_REQ;
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
  CDeviceSubscription* thissub;
  CDeviceSubscription* prevsub;

  // find place to put the update
  for(thissub=requested,prevsub=NULL;thissub;
      prevsub=thissub,thissub=thissub->next)
  {
    if((thissub->id.code == req.code) && (thissub->id.index == req.index))
      break;
  }

  if(!thissub)
  {
    thissub = new CDeviceSubscription;
    thissub->id.code = req.code;
    thissub->id.index = req.index;
    thissub->id.port = port;
    thissub->driver = deviceTable->GetDriver(thissub->id);

    thissub->last_sec = 0; // init the freshness timer
    thissub->last_usec = 0;

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
  for(CDeviceSubscription* thisub=requested;thisub;thisub=thisub->next)
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

// Loop through all devices currently open for this client, get data from
// them, and assemble the results into totalwritebuffer.  Returns the
// total size of the data that was written into that buffer.
size_t
ClientData::BuildMsg(bool include_sync)
{
  size_t size, totalsize=0;
  Driver* driver;
  player_msghdr_t hdr;
  struct timeval curr;
  struct timeval datatime;
  
  hdr.stx = htons(PLAYER_STXX);
  hdr.type = htons(PLAYER_MSGTYPE_DATA);
  for(CDeviceSubscription* thisub=requested;thisub;thisub=thisub->next)
  {
    // does the client want data from this device?
    if(thisub->access==PLAYER_ALL_MODE || thisub->access==PLAYER_READ_MODE) 
    {
      char access = deviceTable->GetDeviceAccess(thisub->id);

      if((access == PLAYER_ALL_MODE) || (access == PLAYER_READ_MODE)) 
      {
        // make sure we've got a good pointer
        if(!(driver = deviceTable->GetDriver(thisub->id)))
        {
          PLAYER_WARN2("found NULL pointer for device \"%x:%x\"",
                       thisub->id.code, thisub->id.index);
          continue;
        }

        // Prepare the header
        hdr.device = htons(thisub->id.code);
        hdr.device_index = htons(thisub->id.index);
        hdr.reserved = 0;

        // Get the data
        size = driver->GetData(thisub->id,
                               writebuffer+sizeof(hdr),
                               PLAYER_MAX_MESSAGE_SIZE-sizeof(hdr),
                               &datatime);

        hdr.timestamp_sec = htonl(datatime.tv_sec);
        hdr.timestamp_usec = htonl(datatime.tv_usec);

        // if we're in an UPDATE mode, we only want this data if it is new
        if((mode == PLAYER_DATAMODE_PUSH_NEW) || 
           (mode == PLAYER_DATAMODE_PULL_NEW) ||
           (mode == PLAYER_DATAMODE_PUSH_ASYNC))
        {
          // if the data has the same timestamp as last time then
          // we don't want it, so skip it 
          // (Byte order doesn't matter for the equality check)
          if(hdr.timestamp_sec == thisub->last_sec && 
             hdr.timestamp_usec == thisub->last_usec)  
          {
            continue;
          }

          // record the time we got data for this device
          // keep 'em in network byte order - it doesn't matter
          // as long as we remember to swap them for printing
          thisub->last_sec = hdr.timestamp_sec;
          thisub->last_usec = hdr.timestamp_usec;
        }

        hdr.size = htonl(size);

        if(GlobalTime->GetTime(&curr) == -1)
          PLAYER_ERROR("GetTime() failed!!!!");
        hdr.time_sec = htonl(curr.tv_sec);
        hdr.time_usec = htonl(curr.tv_usec);

        memcpy(writebuffer,&hdr,sizeof(hdr));

        FillWriteBuffer(writebuffer,totalsize,sizeof(hdr)+size);

        totalsize += sizeof(hdr) + size;
      }
    }
  }

  if(include_sync)
  {
    // now add a zero-length SYNCH packet to the end of the buffer
    hdr.stx = htons(PLAYER_STXX);
    hdr.type = htons(PLAYER_MSGTYPE_SYNCH);
    hdr.device = htons(PLAYER_PLAYER_CODE);
    hdr.device_index = htons(0);
    hdr.reserved = 0;
    hdr.size = 0;

    if(GlobalTime->GetTime(&curr) == -1)
      PLAYER_ERROR("GetTime() failed!!!!");
    hdr.time_sec = hdr.timestamp_sec = htonl(curr.tv_sec);
    hdr.time_usec = hdr.timestamp_usec = htonl(curr.tv_usec);


    FillWriteBuffer((unsigned char*)&hdr,totalsize,sizeof(hdr));

    totalsize += sizeof(hdr);
  }

  return(totalsize);
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
ClientData::PrintRequested(char* str)
{
  printf("%s:requested: ",str);
  for(CDeviceSubscription* thissub=requested;thissub;thissub=thissub->next)
    printf("%x:%x:%d ", thissub->id.code,thissub->id.index,thissub->access);
  puts("");
}

// Copy len bytes of src to totalwritebuffer+offset.  Will realloc()
// totalwritebuffer to make room, if necessary.
void 
ClientData::FillWriteBuffer(unsigned char* src, size_t offset, size_t len)
{
  size_t totalsize = offset + len;

  while(totalsize > totalwritebuffersize)
  {
    // need more memory
    totalwritebuffersize *= 2;
    assert(totalwritebuffer = 
           (unsigned char*)realloc(totalwritebuffer, 
                                   totalwritebuffersize));
  }

  memcpy(totalwritebuffer + offset, src, len);
}

/*************************************************************************
 * ClientDataTCP
 *************************************************************************/


void print_buffer( char* desc, unsigned char* buf, size_t buflen )
{
  printf( "Buffer %s at %p %d bytes \n", desc, buf, (int)buflen );
  
  for( size_t i=0; i<buflen; i++ )
    printf( "%X ", buf[i] );
  
  puts( "<end>" );
}


int 
ClientDataTCP::Read()
 {
   //int thisreadcnt;
   //bool msgready = false;

   //char c;
  
  // create a static input buffer that we'll use every time though
  // this function.
  const size_t inbuflen = 65535;
  static unsigned char* inbuf = NULL;
  if( ! inbuf )
    inbuf = (unsigned char*)calloc( inbuflen, 1 );
 
  int bytes_rcv = 0;
 
  // read a big chunk from the socket
  if( bytes_rcv = read(socket,inbuf,inbuflen) );
  
  //printf( "read %d bytes on socket %d\n", (int)bytes_rcv, socket );
  
  //print_buffer( "inbuf", inbuf, bytes_rcv );

  if( bytes_rcv < 1 )
    {
      if( errno == EAGAIN )
	return 0; // no data was available to parse. done.
      else
	return -1; // client gone or some other socket problem. error.
    }

  // now we parse the chunk that we read.

  // ptr will move through the buffer, starting at the head
  unsigned char *ptr = inbuf;
  
  // this is the last address in the buffer
  unsigned char *end = ptr + inbuflen;

  int message_count = 0;
  
  for( unsigned char* ptr = inbuf; ptr <= end; /*empty*/  )
    {
      //puts( "*STATE MACHINE START*" );

      switch(readstate)
	{
	case PLAYER_AWAITING_FIRST_BYTE_STX:
	  
	  //puts("PLAYER_AWAITING_FIRST_BYTE_STX");
	  readcnt = 0;
	  
	  // zip through the buffer looking for the first byte
	  // This should be the high byte (we're in network byte order)
	  while( *ptr != PLAYER_STXX >> 8  )
	    {
	      ptr++;
	      
	      if( ptr > end )
		break; // need more data
	    }
	  
	  //printf( "found first byte at buffer index %d\n", ptr - inbuf );

	  readcnt = 1;
	  readstate = PLAYER_AWAITING_SECOND_BYTE_STX;
	  ptr++;
	  
	  // deliberate no-break
	  
	case PLAYER_AWAITING_SECOND_BYTE_STX:
	  //puts("PLAYER_AWAITING_SECOND_BYTE_STX");
	  
	  if( *ptr == (char)(PLAYER_STXX & 0x00FF))
	    {
	      hdrbuffer.stx = PLAYER_STXX;
	      readcnt += 1;
	      readstate = PLAYER_AWAITING_REST_OF_HEADER;

	      //printf( "found first byte at buffer index %d\n", ptr - inbuf );

	      ptr++;
	      	  
	      if( ptr > end ) break; // need more data
	    }
	  else
	    {
	      readcnt = 0; // wrong second byte. start again
	      readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
	      break; // need more data
	    }
	  
	  // deliberate no-break
	  

	  //print_buffer( "ptr", ptr, sizeof(player_msghdr_t) );

	case PLAYER_AWAITING_REST_OF_HEADER:
	  //printf("PLAYER_AWAITING_REST_OF_HEADER: %d/%d\n",
	  // readcnt,sizeof(player_msghdr_t));
	  
	  // now we must fill a player_msghdr_t from the bytes in our
	  // buffer, but it might take more than one read to do it.
	  
	  // fill up our header structure
	  while( readcnt < sizeof(player_msghdr_t) )
	    {
	      ((unsigned char*)&hdrbuffer)[readcnt++] =  *ptr++;
	      if( ptr > end ) break; // need more data
	    }
	  
	  // now the header is full of data, we parse it
	  
	  // byte-swap as necessary
	  hdrbuffer.type = ntohs(hdrbuffer.type);
	  hdrbuffer.device = ntohs(hdrbuffer.device);
	  hdrbuffer.device_index = ntohs(hdrbuffer.device_index);
	  hdrbuffer.time_sec = ntohl(hdrbuffer.time_sec);
	  hdrbuffer.time_usec = ntohl(hdrbuffer.time_usec);
	  hdrbuffer.timestamp_sec = ntohl(hdrbuffer.timestamp_sec);
	  hdrbuffer.timestamp_usec = ntohl(hdrbuffer.timestamp_usec);
	  hdrbuffer.reserved = ntohl(hdrbuffer.reserved);
	  hdrbuffer.size = ntohl(hdrbuffer.size);
	  

	  //print_buffer( "hdrbuffer", (unsigned char*)&hdrbuffer, sizeof(hdrbuffer));
	
  
	  //printf( "header:\ntype %d\ndevice %d\ndevice_index %d\nsize %d\n",
	  //  hdrbuffer.type, hdrbuffer.device, hdrbuffer.device_index, hdrbuffer.size );
	  
	  // make sure it's not too big
	  if(hdrbuffer.size > PLAYER_MAX_MESSAGE_SIZE-sizeof(player_msghdr_t))
	    {
	      PLAYER_WARN1("client's message is too big (%d bytes). Ignoring",
			   hdrbuffer.size);
	      readcnt = 0;
	      readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
	      break;
	    }
	  // ...or too small
	  else if(!hdrbuffer.size)
	    {
	      PLAYER_WARN("client sent zero-length message.");
	      readcnt = 0;
	      readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
	      break;
	    }
	  else
	    {
	      readcnt = 0;
	      readstate = PLAYER_AWAITING_REST_OF_BODY;
	    }
	  
	  // deliberate no-break
	  
	case PLAYER_AWAITING_REST_OF_BODY:
	  //printf("PLAYER_AWAITING_REST_OF_BODY: %d/%d bytes\n",readcnt,hdrbuffer.size);
	  /* get the payload */
	  
	  //print_buffer( "ptr (body)", ptr, hdrbuffer.size );

	  //printf( "starting copy. readcnt: %d hdrbuffer.size: %d\n", 
	  //  (int)readcnt, (int)hdrbuffer.size );

	  while( readcnt < hdrbuffer.size )
	    {
	      ((unsigned char*)readbuffer)[readcnt++] =  *ptr++;
	      if( ptr > end ) break; // need more data
	    }

	  //printf( "ended copy. readcnt: %d hdrbuffer.size: %d\n", 
	  //  (int)readcnt, (int)hdrbuffer.size );
	  
	  
	  //puts( "MESSAGE COMPLETE" );
	  
	  //print_buffer( "readbuffer", (unsigned char*)readbuffer, readcnt );

	  // we have a whole message: handle it right here.
	  HandleRequests(hdrbuffer,readbuffer, hdrbuffer.size);

	  message_count++;
	  //puts( "MESSAGE HANDLED. Back to the start" );
	  readcnt = 0;
	  readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
	  ptr++;

	  break;
	  
	  // remove this case?
	case PLAYER_READ_ERROR:
	  PLAYER_ERROR("in an error read state!");
	  break;	
	  
	default:
	  PLAYER_ERROR("in an unknown read state!");
	  break;
	}
    }
  
  printf( "handled %d messages with 1 read\n", message_count );

  return(0);
 }

// Try to write() len bytes from totalwritebuffer, which should have been
// filled with FillWriteBuffer().  If fewer than len bytes are written, 
// the remaining bytes are moved up to the front of totalwritebuffer and 
// their length is recorded in leftover_size.  Returns 0 on success (which
// includes writing fewer than len bytes) and -1 on error (e.g., if the 
// other end of the socket was closed).
int 
ClientDataTCP::Write(size_t len)
{
  static bool warned=false;
  int byteswritten;

  //assert(socket > 0);

  if(len>0)
  {
    if((byteswritten = ::write(socket, totalwritebuffer, len)) < 0) 
    {
      if(errno != EAGAIN)
      {
        // some error, like maybe a closed socket
        return(-1);
      }
      else
        byteswritten=0;
    }

    if((size_t)byteswritten < len)
    {
      // didn't get all the data out; move the remaining data to the front of
      // the buffer and record how much is left.
      leftover_size = len - byteswritten;
      memmove(totalwritebuffer, 
              totalwritebuffer+byteswritten, 
              leftover_size);

      if(!warned)
      {
        PLAYER_WARN1("%d bytes leftover on write() to client", leftover_size);
        warned = true;
      }
    }
    else
    {
      leftover_size=0;
      warned = false;
    }
  }

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
  hdrbuffer.type = ntohs(hdrbuffer.type);
  hdrbuffer.device = ntohs(hdrbuffer.device);
  hdrbuffer.device_index = ntohs(hdrbuffer.device_index);
  hdrbuffer.time_sec = ntohl(hdrbuffer.time_sec);
  hdrbuffer.time_usec = ntohl(hdrbuffer.time_usec);
  hdrbuffer.timestamp_sec = ntohl(hdrbuffer.timestamp_sec);
  hdrbuffer.timestamp_usec = ntohl(hdrbuffer.timestamp_usec);
  hdrbuffer.reserved = ntohl(hdrbuffer.reserved);
  hdrbuffer.size = ntohl(hdrbuffer.size);

  memmove(readbuffer,readbuffer+sizeof(player_msghdr_t),
          numread-sizeof(player_msghdr_t));

  return(HandleRequests(hdrbuffer,readbuffer,hdrbuffer.size));

  return(0);
}

int 
ClientDataUDP::Write(size_t len)
{
  int byteswritten;
  
  // Assume that all data can be written in a single datagram.  Need to
  // make this smarter later.
  if((byteswritten = sendto(socket, totalwritebuffer, len, 0, 
                            (struct sockaddr*)&clientaddr, 
                            (socklen_t)clientaddr_len)) < 0)
  {
    PLAYER_ERROR1("%s", strerror(errno));
    return(-1);
  }
  leftover_size=0;

  return(0);
}
