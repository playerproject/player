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

#include <device.h>
#include <devicetable.h>
#include <clientdata.h>
#include <clientmanager.h>

#if HAVE_STRINGS_H
  #include <strings.h>
#endif

//#include <iostream> //some debug output it easier using stream IO

#include <playertime.h>
extern PlayerTime* GlobalTime;

extern CDeviceTable* deviceTable;
extern CClientData* clients[];
extern ClientManager* clientmanager;
extern char playerversion[];

extern int global_playerport; // used to generate useful output & debug

CClientData::CClientData(char* key, int myport) 
{
  requested = NULL;
  numsubs = 0;
  socket = 0;
  mode = PLAYER_DATAMODE_PUSH_NEW;
  frequency = 10;

  port = myport;

  readbuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE];
  replybuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE];
  writebuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE];

  totalwritebuffersize = PLAYER_MAX_MESSAGE_SIZE;
  totalwritebuffer = new unsigned char[totalwritebuffersize];

  bzero((char*)readbuffer, PLAYER_MAX_MESSAGE_SIZE);
  bzero((char*)writebuffer, PLAYER_MAX_MESSAGE_SIZE);
  bzero((char*)replybuffer, PLAYER_MAX_MESSAGE_SIZE);
  bzero((char*)totalwritebuffer, totalwritebuffersize);
  bzero((char*)&hdrbuffer, sizeof(player_msghdr_t));

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

bool CClientData::CheckAuth(player_msghdr_t hdr, unsigned char* payload,
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
    printf("CheckAuth(): Player device got wrong size ioctl: %d\n",
           payload_size);
    return(false);
  }

  bzero((char*)&tmpreq,sizeof(player_device_auth_req_t));
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

int CClientData::HandleRequests(player_msghdr_t hdr, unsigned char *payload,  
                                size_t payload_size) 
{
  unsigned short requesttype = 0;
  bool unlock_pending=false;
  bool devlistrequest=false;
  bool devicerequest=false;
  CDevice* devicep;
  player_device_req_t req;
  player_device_resp_t resp;
  player_device_datamode_req_t datamode;
  player_device_datafreq_req_t datafreq;
  player_msghdr_t reply_hdr;
  size_t replysize;
  struct timeval curr;

  // clean the buffer every time for all-day freshness
  bzero((char*)replybuffer, PLAYER_MAX_MESSAGE_SIZE);

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
           (short)ntohs(cmd->xspeed), (short)ntohs(cmd->yawspeed));
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
      fputs("Warning: failed authentication; closing connection.\n", stderr);
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
            printf("HandleRequests(): Player device got small ioctl: %d\n",
                   payload_size);

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
              
            case PLAYER_PLAYER_DEV_REQ:
              devicerequest = true;
              if(payload_size < sizeof(player_device_req_t))
              {
                printf("HandleRequests(): got small player_device_req_t: %d\n",
                       payload_size);
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
                printf("HandleRequests(): got wrong size "
                       "player_device_datamode_req_t: %d\n",payload_size);
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
                default:
                  printf("Player warning: unknown I/O mode requested (%d)."
                         "Ignoring request\n",
                         datamode.mode);
                  requesttype = PLAYER_MSGTYPE_RESP_NACK;
                  break;
              }
              break;
            case PLAYER_PLAYER_DATA_REQ:
              // this ioctl takes no args
              if(payload_size != sizeof(player_device_data_req_t))
              {
                printf("HandleRequests(): got wrong size "
                       "arg for player_data_req: %d\n",payload_size);
                requesttype = PLAYER_MSGTYPE_RESP_NACK;
                break;
              }
              if((mode != PLAYER_DATAMODE_PULL_ALL) &&
                 (mode != PLAYER_DATAMODE_PULL_NEW))
              {
                puts("WARNING: got request for data when not in "
                     "request/reply mode");
                requesttype = PLAYER_MSGTYPE_RESP_NACK;
              }
              else
              {
                unlock_pending=true;
                requesttype = PLAYER_MSGTYPE_RESP_ACK;
              }
              break;
            case PLAYER_PLAYER_DATAFREQ_REQ:
              if(payload_size != sizeof(player_device_datafreq_req_t))
              {
                printf("HandleRequests(): got wrong size "
                       "arg for update frequency change: %d\n",payload_size);
                requesttype = PLAYER_MSGTYPE_RESP_NACK;
                break;
              }
              datafreq = *((player_device_datafreq_req_t*)payload);
              frequency = ntohs(datafreq.frequency);
              requesttype = PLAYER_MSGTYPE_RESP_ACK;
              break;
            case PLAYER_PLAYER_AUTH_REQ:
              fputs("Warning: unnecessary authentication request.\n",stderr);
              requesttype = PLAYER_MSGTYPE_RESP_NACK;
              break;
            default:
              printf("Unknown server ioctl %x\n", subtype);
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
            if((devicep = deviceTable->GetDevice(id)))
            {
              // try to push it on the request queue
              if(devicep->PutConfig(&id,this,payload,payload_size))
              {
                // queue was full
                requesttype = PLAYER_MSGTYPE_RESP_ERR;
              }
              else
                requesttype = 0;
            }
            else
            {
              printf("HandleRequests(): got REQ for unkown device: %x:%x\n",
                     id.code,id.index);
              requesttype = PLAYER_MSGTYPE_RESP_ERR;
            }
          }
          else
          {
            printf("No permissions to configure %x:%x\n",
                   id.code,id.index);
            requesttype = PLAYER_MSGTYPE_RESP_ERR;
          }
        }
        break;
      case PLAYER_MSGTYPE_CMD:
        /* command message */
        if(CheckWritePermissions(id))
        {
          // check if we can write to this device
          if((deviceTable->GetDeviceAccess(id) == PLAYER_WRITE_MODE) ||
             (deviceTable->GetDeviceAccess(id) == PLAYER_ALL_MODE))

          {
            // make sure we've got a non-NULL pointer
            if((devicep = deviceTable->GetDevice(id)))
              devicep->PutCommand(this,payload,payload_size);
            else
              printf("HandleRequests(): found NULL pointer for device %x:%x\n",
                     id.code,id.index);
          }
          else
            printf("You can't send commands to %x:%x\n",
                   id.code,id.index);
        }
        else 
          printf("No permissions to command %x:%x\n",
                 id.code,id.index);
        break;
      default:
        printf("HandleRequests(): Unknown message type %x\n", hdr.type);
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

      bzero(resp.driver_name, sizeof(resp.driver_name));
      char* drivername;
      if((drivername = deviceTable->GetDriver(id)))
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
      fputs("CClientData::HandleRequests(): GetTime() failed!!!!\n", stderr);
    reply_hdr.time_sec = htonl(curr.tv_sec);
    reply_hdr.time_usec = htonl(curr.tv_usec);

    reply_hdr.timestamp_sec = reply_hdr.time_sec;
    reply_hdr.timestamp_usec = reply_hdr.time_usec;
    memcpy(replybuffer,&reply_hdr,sizeof(player_msghdr_t));

    if(write(socket, replybuffer, replysize+sizeof(player_msghdr_t)) < 0) 
    {
      if(errno != EAGAIN)
      {
        perror("HandleRequests: write()");
        return(-1);
      }
    }
  }

  // FIXME: should this still be here?
  if(unlock_pending)
  {
    datarequested = true;
  }
  
  return(0);
}

CClientData::~CClientData() 
{
  RemoveRequests();

  if (socket) close(socket);

  printf("** Player [port %d] killing client on socket %d **\n", 
	 global_playerport, socket);

  if(readbuffer)
    delete readbuffer;

  if(writebuffer)
    delete writebuffer;

  if(totalwritebuffer)
    delete totalwritebuffer;

  if(replybuffer)
    delete replybuffer;
}

void CClientData::RemoveRequests() 
{
  CDeviceSubscription* thissub = requested;
  CDeviceSubscription* tmpsub;

  while(thissub)
  {
    if(thissub->id.code == PLAYER_POSITION_CODE)
      MotorStop();
    switch(thissub->access) 
    {
      case PLAYER_ALL_MODE:
        Unsubscribe(thissub->id);
        Unsubscribe(thissub->id);
        break;
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

void CClientData::MotorStop() 
{
  player_position_cmd_t command;
  CDevice* devicep;
  player_device_id_t id;

  id.port = port;
  id.code = PLAYER_POSITION_CODE;
  id.index = 0;

  command.xspeed = command.yspeed = command.yawspeed = 0;

  if((devicep = deviceTable->GetDevice(id)))
  {
    devicep->PutCommand(this,(unsigned char*)&command, sizeof(command));
  }
}


// Handle device list requests.
void CClientData::HandleListRequest(player_device_devlist_t *req,
                                    player_device_devlist_t *rep)
{
  CDeviceEntry *entry;

  rep->subtype = PLAYER_PLAYER_DEVLIST_REQ;
  rep->device_count = 0;

  // Get all the device entries that have the right port number.
  // TODO: test this with Stage.
  for (entry = deviceTable->GetFirstEntry(); entry != NULL;
       entry = deviceTable->GetNextEntry(entry))
  {
    assert(rep->device_count < ARRAYSIZE(rep->devices));
    if (entry->id.port == port)
    {
      rep->devices[rep->device_count].code = htons(entry->id.code);
      rep->devices[rep->device_count].index = htons(entry->id.index);
      rep->devices[rep->device_count].port = htons(entry->id.port);
      rep->device_count++;
    }
  }

  // Do some byte swapping.
  rep->subtype = htons(rep->subtype);
  rep->device_count = htons(rep->device_count);

  return;
}


void CClientData::UpdateRequested(player_device_req_t req)
{
  CDeviceSubscription* thisub;
  CDeviceSubscription* prevsub;

  // find place to place the update
  for(thisub=requested,prevsub=NULL;thisub;
      prevsub=thisub,thisub=thisub->next)
  {
    if((thisub->id.code == req.code) && (thisub->id.index == req.index))
      break;
  }

  if(!thisub)
  {
    thisub = new CDeviceSubscription;
    thisub->id.code = req.code;
    thisub->id.index = req.index;
    thisub->id.port = port;
    thisub->devicep = deviceTable->GetDevice(thisub->id);

    thisub->last_sec = 0; // init the freshness timer
    thisub->last_usec = 0;

    if(prevsub)
      prevsub->next = thisub;
    else
      requested = thisub;
    numsubs++;
  }

  /* UPDATE */
  
  // go from either PLAYER_READ_MODE or PLAYER_WRITE_MODE to PLAYER_ALL_MODE
  if(((thisub->access==PLAYER_WRITE_MODE) || 
      (thisub->access==PLAYER_READ_MODE)) && 
     (req.access==PLAYER_ALL_MODE))
  {
    // subscribe once more
    if(Subscribe(thisub->id) == 0)
      thisub->access = PLAYER_ALL_MODE;
    else 
      thisub->access='e';
  }
  // go from PLAYER_ALL_MODE to either PLAYER_READ_MODE or PLAYER_WRITE_MODE
  else if(thisub->access==PLAYER_ALL_MODE && 
          (req.access==PLAYER_READ_MODE || 
           req.access==PLAYER_WRITE_MODE)) 
  {
    // unsubscribe once
    Unsubscribe(thisub->id);
    thisub->access=req.access;
  }
  // go from either PLAYER_READ_MODE to PLAYER_WRITE_MODE or PLAYER_WRITE_MODE to PLAYER_READ_MODE
  else if(((thisub->access==PLAYER_READ_MODE) && 
           (req.access==PLAYER_WRITE_MODE)) ||
          ((thisub->access==PLAYER_WRITE_MODE) && 
           (req.access==PLAYER_READ_MODE)))
  {
    // no subscription change necessary
    thisub->access=req.access;
  }

  /* CLOSE */
  else if(req.access==PLAYER_CLOSE_MODE) 
  {
    // close 
    switch(thisub->access)
    {
      case PLAYER_ALL_MODE:
        Unsubscribe(thisub->id);   // we want to unsubscribe two times
      case PLAYER_WRITE_MODE:
      case PLAYER_READ_MODE:
        Unsubscribe(thisub->id);
        thisub->access = PLAYER_CLOSE_MODE;
        break;
      case PLAYER_CLOSE_MODE:
      case PLAYER_ERROR_MODE:
        printf("Device \"%x:%x\" already closed\n", req.code,req.index);
        break;
      default:
        printf("Unknown access permission \"%c\"\n", req.access);
        break;
    }
  }
  /* OPEN */
  else if((thisub->access == PLAYER_ERROR_MODE) || 
          (thisub->access==PLAYER_CLOSE_MODE))
  {
    switch(req.access) 
    {
      case PLAYER_ALL_MODE:
        if((Subscribe(thisub->id)==0) && 
           (Subscribe(thisub->id) == 0))
          thisub->access=PLAYER_ALL_MODE;
        else
          thisub->access=PLAYER_ERROR_MODE;
        break;
      case PLAYER_WRITE_MODE:
        if(Subscribe(thisub->id)==0)
          thisub->access=PLAYER_WRITE_MODE;
        else 
          thisub->access=PLAYER_ERROR_MODE;
        break;
      case PLAYER_READ_MODE:
        if(Subscribe(thisub->id) == 0)
          thisub->access=PLAYER_READ_MODE;
        else
          thisub->access=PLAYER_ERROR_MODE;
        break;
      default:
        printf("Unknown access \"%c\"\n", req.access);
    }
  }
  /* IGNORE */
  else 
  {
    printf("The current access is \"%x:%x:%c\". ",
                    thisub->id.code, thisub->id.index, thisub->access);
    printf("Unknown unused request \"%x:%x:%c\".\n",
                    req.code, req.index, req.access);
  }
}

unsigned char 
CClientData::FindPermission(player_device_id_t id)
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

bool CClientData::CheckOpenPermissions(player_device_id_t id)
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

bool CClientData::CheckWritePermissions(player_device_id_t id)
{
  bool permission = false;
  unsigned char letter;

  letter = FindPermission(id);

  if((letter==PLAYER_ALL_MODE) || (PLAYER_WRITE_MODE==letter)) 
    permission = true;

  return(permission);
}

int CClientData::BuildMsg()
{
  unsigned short size, totalsize=0;
  CDevice* devicep;
  player_msghdr_t hdr;
  struct timeval curr;
  size_t numdata;
  
  hdr.stx = htons(PLAYER_STXX);
  hdr.type = htons(PLAYER_MSGTYPE_DATA);
  for(CDeviceSubscription* thisub=requested;thisub;thisub=thisub->next)
  {
    if(thisub->access==PLAYER_ALL_MODE || thisub->access==PLAYER_READ_MODE) 
    {
      char access = deviceTable->GetDeviceAccess(thisub->id);

      if((access == PLAYER_ALL_MODE) || (access == PLAYER_READ_MODE)) 
      {
        // make sure we've got a good pointer
        if(!(devicep = deviceTable->GetDevice(thisub->id)))
        {
          printf("BuildMsg(): found NULL pointer for device \"%x:%x\"\n",
                          thisub->id.code, thisub->id.index);
          continue;
        }

        // how many packets are available for this client?
        numdata = devicep->GetNumData(this);
        while(numdata > 0)
        {
          numdata--;

          hdr.device = htons(thisub->id.code);
          hdr.device_index = htons(thisub->id.index);
          hdr.reserved = 0;

          size = devicep->GetData(this, writebuffer+sizeof(hdr),
                                  PLAYER_MAX_MESSAGE_SIZE-sizeof(hdr),
                                  &(hdr.timestamp_sec), 
                                  &(hdr.timestamp_usec));

          hdr.timestamp_sec = htonl(hdr.timestamp_sec);
          hdr.timestamp_usec = htonl(hdr.timestamp_usec);

          // if we're in an UPDATE mode, we only want this data if it is new
          if((mode == PLAYER_DATAMODE_PUSH_NEW) || 
             (mode == PLAYER_DATAMODE_PULL_NEW))
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
            fputs("CLock::PutData(): GetTime() failed!!!!\n", stderr);
          hdr.time_sec = htonl(curr.tv_sec);
          hdr.time_usec = htonl(curr.tv_usec);

          memcpy(writebuffer,&hdr,sizeof(hdr));

          while((totalsize + sizeof(hdr) + size) > totalwritebuffersize)
          {
            // need more memory
            totalwritebuffersize *= 2;
            assert(totalwritebuffer = 
                   (unsigned char*)realloc(totalwritebuffer, 
                                           totalwritebuffersize));
          }

          memcpy(totalwritebuffer + totalsize,
                 writebuffer,
                 sizeof(hdr) + size);
          totalsize += sizeof(hdr) + size;
        }
      }
      else
      {
        printf("BuildMsg(): Unknown device \"%x:%x\"\n",
                        thisub->id.code,thisub->id.index);
      }
    }
  }

  // now add a zero-length SYNCH packet to the end of the buffer
  hdr.stx = htons(PLAYER_STXX);
  hdr.type = htons(PLAYER_MSGTYPE_SYNCH);
  hdr.device = htons(PLAYER_PLAYER_CODE);
  hdr.device_index = htons(0);
  hdr.reserved = 0;
  hdr.size = 0;

  if(GlobalTime->GetTime(&curr) == -1)
    fputs("CLock::PutData(): GetTime() failed!!!!\n", stderr);
  hdr.time_sec = hdr.timestamp_sec = htonl(curr.tv_sec);
  hdr.time_usec = hdr.timestamp_usec = htonl(curr.tv_usec);


  while((totalsize + sizeof(hdr)) > totalwritebuffersize)
  {
    // need more memory
    totalwritebuffersize *= 2;
    assert(totalwritebuffer = 
           (unsigned char*)realloc(totalwritebuffer, totalwritebuffersize));
  }

  memcpy(totalwritebuffer + totalsize, &hdr, sizeof(hdr));
  totalsize += sizeof(hdr);

  return(totalsize);
}


int CClientData::Subscribe(player_device_id_t id)
{
  CDevice* devicep;
  int subscribe_result;

  if((devicep = deviceTable->GetDevice(id)))
  {
    subscribe_result = devicep->Subscribe(this);
    return(subscribe_result);
  }
  else
  {
    printf("Subscribe(): Unknown device \"%x:%x\" - subscribe cancelled\n", 
                    id.code,id.index);
    return(1);
  }
}


void CClientData::Unsubscribe(player_device_id_t id)
{
  CDevice* devicep;

  if((devicep = deviceTable->GetDevice(id)))
  {
    devicep->Unsubscribe(this);
  }
  else
  {
    printf("Unsubscribe(): Unknown device \"%x:%x\" - unsubscribe cancelled\n", 
                    id.code,id.index);
  }
}

void
CClientData::PrintRequested(char* str)
{
  printf("%s:requested: ",str);
  for(CDeviceSubscription* thissub=requested;thissub;thissub=thissub->next)
    printf("%x:%x:%d ", thissub->id.code,thissub->id.index,thissub->access);
  puts("");
}

int CClientData::Read()
{
  int thisreadcnt;
  bool msgready = false;

  char c;

  switch(readstate)
  {
    case PLAYER_AWAITING_FIRST_BYTE_STX:
      //puts("PLAYER_AWAITING_FIRST_BYTE_STX");
      readcnt = 0;

      if(read(socket,&c,1) <= 0)
      {
        if(errno == EAGAIN)
          break;
        else
        {
          // client must be gone. fuck 'em
          //perror("client_reader(): read() while waiting for first "
                 //"byte of STX");
          return(-1);
        }
      }

      // This should be the high byte (we're in network byte order)
      if(c == PLAYER_STXX >> 8)
      {
        readcnt = 1;
        readstate = PLAYER_AWAITING_SECOND_BYTE_STX;
      }
      break;
    case PLAYER_AWAITING_SECOND_BYTE_STX:
      //puts("PLAYER_AWAITING_SECOND_BYTE_STX");
      
      if(read(socket,&c,1) <= 0)
      {
        if(errno == EAGAIN)
          break;
        else
        {
          // client must be gone. fuck 'em
          //perror("client_reader(): read() while waiting for second "
                 //"byte of STX");
          return(-1);
        }
      }
      if(c == (char)(PLAYER_STXX & 0x00FF))
      {
        hdrbuffer.stx = PLAYER_STXX;
        readcnt += 1;
        readstate = PLAYER_AWAITING_REST_OF_HEADER;
      }
      else
      {
        readcnt = 0;
        readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
      }
      break;
    case PLAYER_AWAITING_REST_OF_HEADER:
      //printf("PLAYER_AWAITING_REST_OF_HEADER: %d/%d\n",
             //readcnt,sizeof(player_msghdr_t));
      /* get the rest of the header */
      if((thisreadcnt = read(socket, (char*)(&hdrbuffer)+readcnt, 
                             sizeof(player_msghdr_t)-readcnt)) <= 0)
      {
        if(errno == EAGAIN)
          break;
        else
        {
          //perror("client_reader(): read() while reading header");
          return(-1);
        }
      }
      readcnt += thisreadcnt;
      if(readcnt == sizeof(player_msghdr_t))
      {
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

        // make sure it's not too big
        if(hdrbuffer.size > PLAYER_MAX_MESSAGE_SIZE-sizeof(player_msghdr_t))
        {
          printf("WARNING: client's message is too big (%d bytes). Ignoring\n",
                 hdrbuffer.size);
          readcnt = 0;
          readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
        }
        // ...or too small
        else if(!hdrbuffer.size)
        {
          puts("WARNING: client sent zero-length message.\n");
          readcnt = 0;
          readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
        }
        else
        {
          readcnt = 0;
          readstate = PLAYER_AWAITING_REST_OF_BODY;
        }
      }
      break;
    case PLAYER_AWAITING_REST_OF_BODY:
      //printf("PLAYER_AWAITING_REST_OF_BODY: %d bytes read so far\n",readcnt);
      /* get the payload */
      if((thisreadcnt = read(socket,readbuffer+readcnt,
                             hdrbuffer.size-readcnt))<=0)
      {
        if(!errno || errno == EAGAIN)
          break;
        else
        {
          //perror("CClientData::Read(): read() errored");
          return(-1);
        }
      }
      readcnt += thisreadcnt;
      if(readcnt == hdrbuffer.size)
      {
        readcnt = 0;
        readstate = PLAYER_AWAITING_FIRST_BYTE_STX;
        msgready = true;
      }
      break;
    case PLAYER_READ_ERROR:
      fputs("CClientData:Read(): i'm in an error read state!\n",stderr);
      break;
    default:
      fputs("CClientData:Read(): i'm in an unknown read state!\n",stderr);
      break;
  }
 

  if(msgready)
    return(HandleRequests(hdrbuffer,readbuffer, hdrbuffer.size));
  else
    return(0);
}

int
CClientData::WriteIdentString()
{
  unsigned char data[PLAYER_IDENT_STRLEN];
  // write back an identifier string
  sprintf((char*)data, "%s%s", PLAYER_IDENT_STRING, playerversion);
  bzero(((char*)data)+strlen((char*)data),
        PLAYER_IDENT_STRLEN-strlen((char*)data));

  if(write(socket, data, PLAYER_IDENT_STRLEN) < 0 ) 
  {
    if(errno != EAGAIN)
    {
      perror("ClientManager::Write():write()");
      return(-1);
    }
  }
  return(0);
}

int
CClientData::Write()
{
  unsigned int size;

  size = BuildMsg();

  if(size>0 && write(socket, totalwritebuffer, size) < 0 ) 
  {
    if(errno != EAGAIN)
      return(-1);
  }
  return(0);
}

