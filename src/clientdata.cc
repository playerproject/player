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
#include <errno.h>
#include <netinet/in.h>
#include <sys/time.h>

#include <devicetable.h>
#include <clientdata.h>
#include <clientmanager.h>
#include <packet.h>

#include <iostream.h> //some debug output it easier using stream IO

#ifdef PLAYER_SOLARIS
  #include <strings.h>  /* for bzero */
#endif

extern CDeviceTable* deviceTable;
extern CClientData* clients[];
extern pthread_mutex_t clients_mutex;
extern ClientManager* clientmanager;
extern char playerversion[];

extern int global_playerport; // used to generate useful output & debug

CClientData::CClientData(char* key) 
{
  requested = NULL;
  numsubs = 0;
  socket = 0;
  mode = CONTINUOUS;
  frequency = 10;

  readbuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE];
  writebuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE];
  replybuffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE];

  last_write = 0.0;

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

  pthread_mutex_init( &access, NULL ); 
  pthread_mutex_init( &socketwrite, NULL ); 

  datarequested = false;
}

bool CClientData::CheckAuth(player_msghdr_t hdr, unsigned char* payload,
                            unsigned int payload_size)
{
  player_device_ioctl_t player_ioctl;
  player_device_auth_req_t tmpreq;
  unsigned int real_payloadsize;

  if(hdr.device != PLAYER_PLAYER_CODE)
    return(false);

  // ignore the device_index.  can we have more than one player?
  // is the payload big enough?
  if(payload_size < sizeof(player_device_ioctl_t))
  {
    printf("CheckAuth(): Player device got small ioctl: %d\n",
           payload_size);
    return(false);
  }

  // what sort of ioctl is it?
  memcpy(&player_ioctl,payload,sizeof(player_device_ioctl_t));
  real_payloadsize = payload_size - sizeof(player_device_ioctl_t);
  player_ioctl.subtype = ntohs(player_ioctl.subtype);

  if(player_ioctl.subtype != PLAYER_PLAYER_AUTH_REQ)
    return(false);

  if(real_payloadsize > sizeof(player_device_auth_req_t))
  {
    printf("HandleRequests(): got big "
           "arg for auth change: %d\n",real_payloadsize);
    return(false);
  }

  bzero(&tmpreq,sizeof(tmpreq));
  memcpy(&tmpreq,payload+sizeof(player_device_ioctl_t),
         real_payloadsize);
  tmpreq.auth_key[sizeof(tmpreq.auth_key)-1] = '\0';

  if(!strcmp(auth_key,tmpreq.auth_key))
    return(true);
  else
    return(false);
}

int CClientData::HandleRequests(player_msghdr_t hdr, unsigned char *payload,  
                                 unsigned int payload_size) 
{
  bool request=false;
  bool unlock_pending=false;
  bool devicerequest=false;
  unsigned int i,j;
  CDevice* devicep;
  player_device_ioctl_t player_ioctl;
  player_device_req_t req;
  player_device_datamode_req_t datamode;
  player_device_datafreq_req_t datafreq;
  player_msghdr_t reply_hdr;
  struct timeval curr;
  unsigned int real_payloadsize;

  // clean the buffer every time for all-day freshness
  bzero(replybuffer, PLAYER_MAX_MESSAGE_SIZE);

  // debug output; leave it here
#if 0
  printf("type:%u device:%u index:%u\n", 
         hdr.type,hdr.device,hdr.device_index);
  printf("Request(%d):",payload_size);
  for(unsigned int i=0;i<payload_size;i++)
    printf("%c",payload[i]);
  printf("\t");
  for(unsigned int i=0;i<payload_size;i++)
    printf("%x ",payload[i]);
  puts("");
#endif

  
  if(auth_pending)
  {
    if(CheckAuth(hdr,payload,payload_size))
    {
      pthread_mutex_lock( &access );
      auth_pending = false;
      request = true;
      pthread_mutex_unlock( &access );
    }
    else
    {
      fputs("Warning: failed authentication; closing connection.\n", stderr);
      return(-1);
    }
  }
  else
  {
    switch(hdr.type)
    {
      case PLAYER_MSGTYPE_REQ:
        /* request message */
        request = true;
        // if it's for us, handle it here
        if(hdr.device == PLAYER_PLAYER_CODE)
        {
          // ignore the device_index.  can we have more than one player?
          // is the payload big enough?
          if(payload_size < sizeof(player_device_ioctl_t))
          {
            printf("HandleRequests(): Player device got small ioctl: %d\n",
                   payload_size);
            //pthread_mutex_unlock( &requesthandling );
            return(0);
          }

          // what sort of ioctl is it?
          memcpy(&player_ioctl,payload,sizeof(player_device_ioctl_t));
          real_payloadsize = payload_size - sizeof(player_device_ioctl_t);
          player_ioctl.subtype = ntohs(player_ioctl.subtype);

          switch(player_ioctl.subtype)
          {
            case PLAYER_PLAYER_DEV_REQ:
              devicerequest = true;
              if(real_payloadsize < sizeof(player_device_req_t))
              {
                printf("HandleRequests(): got small player_device_req_t: %d\n",
                       real_payloadsize);
                break;
              }
              for(j=sizeof(player_device_ioctl_t);
                  j<payload_size-(sizeof(player_device_req_t)-1);
                  j+=sizeof(player_device_req_t))
              {
                memcpy(&req,payload+j,sizeof(player_device_req_t));
                req.code = ntohs(req.code);
                req.index = ntohs(req.index);
                UpdateRequested(req);
              }
              //puts("done with requests");
              if(j != payload_size)
                puts("HandleRequests(): garbage following player DR ioctl");
              break;
            case PLAYER_PLAYER_DATAMODE_REQ:
              if(real_payloadsize != sizeof(player_device_datamode_req_t))
              {
                printf("HandleRequests(): got wrong size "
                       "player_device_datamode_req_t: %d\n",real_payloadsize);
                break;
              }
              memcpy(&datamode,payload+sizeof(player_device_ioctl_t),
                     sizeof(player_device_datamode_req_t));
              // lock before changing mode
              pthread_mutex_lock(&access);
              switch(datamode.mode)
              {
                case REQUESTREPLY:
                  /* changet to request/reply */
                  //puts("changing to REQUESTREPLY");
                  datarequested=false;
                  mode = REQUESTREPLY;
                  break;
                case CONTINUOUS:
                  /* change to continuous mode */
                  //puts("changing to CONTINUOUS");
                  mode = CONTINUOUS;
                  break;
                case UPDATE:
                  /* change to continuous mode */
                  //puts("changing to UPDATE");
                  mode = UPDATE;
                  break;
                default:
                  printf("Player warning: unknown I/O mode requested (%d)."
                         "Ignoring request\n",
                         datamode.mode );
                  break;
              } // end datamode switch
              pthread_mutex_unlock(&access);
              break;
            case PLAYER_PLAYER_DATA_REQ:
              // this ioctl takes no args
              if(real_payloadsize != 0)
              {
                printf("HandleRequests(): got wrong size "
                       "arg for player_data_req: %d\n",real_payloadsize);
                break;
              }
              if(mode != REQUESTREPLY)
                puts("WARNING: got request for data when not in "
                     "request/reply mode");
              else
                unlock_pending=true;
              break;
            case PLAYER_PLAYER_DATAFREQ_REQ:
              if(real_payloadsize != sizeof(player_device_datafreq_req_t))
              {
                printf("HandleRequests(): got wrong size "
                       "arg for update frequency change: %d\n",real_payloadsize);
                break;
              }
              memcpy(&datafreq,payload+sizeof(player_device_ioctl_t),
                     sizeof(player_device_datafreq_req_t));
              pthread_mutex_lock(&access);
              frequency = ntohs(datafreq.frequency);
              pthread_mutex_unlock(&access);
              break;
            case PLAYER_PLAYER_AUTH_REQ:
              fputs("Warning: unnecessary authentication request.\n",stderr);
              break;
            default:
              printf("Unknown server ioctl %x\n", player_ioctl.subtype);
              break;
          }
        }
        else
        {
          // it's for another device.  hand it off.

          // pass the config request on the proper device
          // make sure we've got a non-NULL pointer
          if((devicep = deviceTable->GetDevice(hdr.device,hdr.device_index)))
          {
            devicep->GetLock()->PutConfig(devicep,payload,payload_size);
          }
          else
            printf("HandleRequests(): got REQ for unkown device: %x:%x\n",
                   hdr.device,hdr.device_index);
        }
        break;
      case PLAYER_MSGTYPE_CMD:
        /* command message */
        if(CheckPermissions(hdr.device,hdr.device_index))
        {
          //puts( "got permissions" );

          // check if we can write to this device
          if((deviceTable->GetDeviceAccess(hdr.device,hdr.device_index) == 'w') ||
             (deviceTable->GetDeviceAccess(hdr.device,hdr.device_index) == 'a'))

          {
            //puts( "got access" );
            // make sure we've got a non-NULL pointer
            if((devicep = deviceTable->GetDevice(hdr.device,hdr.device_index)))
            {
              //puts( "got device" );
              devicep->GetLock()->PutCommand(devicep,payload,payload_size);
            }
            else
            {
              printf("HandleRequests(): found NULL pointer for device %x:%x\n",
                     hdr.device,hdr.device_index);
            }
          }
          else
            printf("You can't send commands to %x:%x\n",
                   hdr.device,hdr.device_index);
        }
        else 
        {
          printf("No permissions to command %x:%x\n",
                 hdr.device,hdr.device_index);
        }
        break;
      default:
        printf("HandleRequests(): Unknown message type %x\n", hdr.type);
        break;
    }
  }

  /* if it's a request, then we must generate a reply */
  if(request)
  {
    reply_hdr.stx = htons(PLAYER_STXX);
    reply_hdr.type = htons(PLAYER_MSGTYPE_RESP);
    reply_hdr.device = htons(hdr.device);
    reply_hdr.device_index = htons(hdr.device_index);
    reply_hdr.reserved = (uint32_t)0;
    reply_hdr.size = htonl(payload_size);

    /* if it was a player device request, then the reply should
     * reflect what permissions were granted for the indicated devices */
    if(devicerequest)
    {
      memcpy(replybuffer+sizeof(player_msghdr_t),payload,
             sizeof(player_device_ioctl_t));
      for(i=sizeof(player_msghdr_t)+sizeof(player_device_ioctl_t),
          j=sizeof(player_device_ioctl_t);
          j<payload_size-(sizeof(player_device_req_t)-1);
          i+=sizeof(player_device_req_t),j+=sizeof(player_device_req_t))
      {
        memcpy(&req,payload+j,sizeof(player_device_req_t));
        req.access = FindPermission(ntohs(req.code),ntohs(req.index));
        memcpy(replybuffer+i,&req,sizeof(player_device_req_t));
      }
    }
    /* otherwise, just copy back the request, since we can't get result
     * codes here (yet)
     */
    else
    {
      memcpy(replybuffer+sizeof(player_msghdr_t),payload,payload_size);
    }

    gettimeofday(&curr,NULL);
    reply_hdr.time_sec = htonl(curr.tv_sec);
    reply_hdr.time_usec = htonl(curr.tv_usec);

    reply_hdr.timestamp_sec = reply_hdr.time_sec;
    reply_hdr.timestamp_usec = reply_hdr.time_usec;
    memcpy(replybuffer,&reply_hdr,sizeof(player_msghdr_t));

    pthread_mutex_lock(&socketwrite);
    if(write(socket, replybuffer, payload_size+sizeof(player_msghdr_t)) < 0) 
    {
      if(errno != EAGAIN)
      {
        perror("HandleRequests: write()");
        pthread_mutex_unlock(&socketwrite);
        return(-1);
      }
    }
    pthread_mutex_unlock(&socketwrite);
  }

  if(unlock_pending)
  {
    pthread_mutex_lock(&access);
    datarequested = true;
    pthread_mutex_unlock(&access);
  }
  
  return(0);
}

CClientData::~CClientData() 
{
  RemoveRequests();

  usleep(100000);

  if (socket) close(socket);
  printf("** Player [port %d] killing client on socket %d **\n", 
	 global_playerport, socket);

  if(readbuffer)
    delete readbuffer;

  if(writebuffer)
    delete writebuffer;

  if(replybuffer)
    delete replybuffer;
}

void CClientData::RemoveRequests() 
{
  CDeviceSubscription* thissub = requested;
  CDeviceSubscription* tmpsub;

  while(thissub)
  {
    switch(thissub->access) 
    {
      case 'a':
        Unsubscribe(thissub->code, thissub->index);
        Unsubscribe(thissub->code, thissub->index);
        break;
      case 'r':
      case 'w':
        Unsubscribe(thissub->code, thissub->index);
        break;
      default:
        break;
    }
    if(thissub->code == PLAYER_POSITION_CODE)
      MotorStop();
    tmpsub = thissub->next;
    delete thissub;
    thissub = tmpsub;
  }
  requested = NULL;
}

void CClientData::MotorStop() 
{
  unsigned char command[4];
  CDevice* devicep;

  *( short *)&command[0]=0;
  *( short *)&command[sizeof(short)]=0;

  if((devicep = deviceTable->GetDevice(PLAYER_POSITION_CODE,0)))
    devicep->GetLock()->PutCommand(devicep, command, 4);
}

void CClientData::UpdateRequested(player_device_req_t req)
{
  CDeviceSubscription* thisub;
  CDeviceSubscription* prevsub;

  pthread_mutex_lock( &access );

  // find place to place the update
  for(thisub=requested,prevsub=NULL;thisub;
      prevsub=thisub,thisub=thisub->next)
  {
    if((thisub->code == req.code) && (thisub->index == req.index))
      break;
  }

  if(!thisub)
  {
    thisub = new CDeviceSubscription;
    thisub->code = req.code;
    thisub->index = req.index;

    thisub->last_sec = 0; // init the freshness timer
    thisub->last_usec = 0;

    if(prevsub)
      prevsub->next = thisub;
    else
      requested = thisub;
    numsubs++;
  }

  /* UPDATE */
  
  // go from either 'r' or 'w' to 'a'
  if(((thisub->access=='w') || (thisub->access=='r')) && (req.access=='a'))
  {
    // subscribe once more
    if(Subscribe(req.code,req.index) == 0)
      thisub->access = 'a';
    else 
      thisub->access='e';
  }
  // go from 'a' to either 'r' or 'w'
  else if(thisub->access=='a' && (req.access=='r' || req.access=='w')) 
  {
    // unsubscribe once
    Unsubscribe(req.code,req.index);
    thisub->access=req.access;
  }
  // go from either 'r' to 'w' or 'w' to 'r'
  else if(((thisub->access=='r') && (req.access=='w')) ||
          ((thisub->access=='w') && (req.access=='r')))
  {
    // no subscription change necessary
    thisub->access=req.access;
  }

  /* CLOSE */
  else if(req.access=='c') 
  {
    // close 
    switch(thisub->access)
    {
      case 'a':
        Unsubscribe(req.code,req.index);   // we want to unsubscribe two times
      case 'w':
      case 'r':
        Unsubscribe(req.code,req.index);
        thisub->access = 'c';
        break;
      case 'c':
      case 'e':
        printf("Device \"%x:%x\" already closed\n", req.code,req.index);
        break;
      default:
        printf("Unknown access permission \"%c\"\n", req.access);
        break;
    }
  }
  /* OPEN */
  else if((thisub->access == 'e') || (thisub->access=='c'))
  {
    switch(req.access) 
    {
      case 'a':
        if((Subscribe(req.code,req.index)==0) && 
           (Subscribe(req.code,req.index) == 0))
          thisub->access='a';
        else
          thisub->access='e';
        break;
      case 'w':
        if(Subscribe(req.code,req.index)==0)
          thisub->access='w';
        else 
          thisub->access='e';
        break;
      case 'r':
        if(Subscribe(req.code,req.index) == 0)
          thisub->access='r';
        else
          thisub->access='e';
        break;
      default:
        printf("Unknown access \"%c\"\n", req.access);
    }
  }
  /* IGNORE */
  else 
  {
    printf("The current access is \"%x:%x:%c\". ",
                    thisub->code, thisub->index, thisub->access);
    printf("Unknown unused request \"%x:%x:%c\".\n",
                    req.code, req.index, req.access);
  }
  pthread_mutex_unlock( &access );
}

unsigned char 
CClientData::FindPermission(unsigned short code, unsigned short index)
{
  unsigned char tmpaccess;
  pthread_mutex_lock(&access);
  for(CDeviceSubscription* thisub=requested;thisub;thisub=thisub->next)
  {
    if((thisub->code == code) && (thisub->index == index))
    {
      tmpaccess = thisub->access;
      pthread_mutex_unlock(&access);
      return(tmpaccess);
    }
  }
  pthread_mutex_unlock(&access);
  return('e');
}

bool CClientData::CheckPermissions(unsigned short code, unsigned short index)
{
  bool permission = false;
  unsigned char letter;

  letter = FindPermission(code,index);
  if((letter=='a') || ('w'==letter)) 
    permission = true;

  return(permission);
}

int CClientData::BuildMsg( unsigned char *data, size_t maxsize) 
{
  unsigned short size, totalsize=0;
  CDevice* devicep;
  player_msghdr_t hdr;
  struct timeval curr;
  
  // don't lock this here; it's already locked by the caller
  //pthread_mutex_lock( &access );

  hdr.stx = htons(PLAYER_STXX);
  hdr.type = htons(PLAYER_MSGTYPE_DATA);
  for(CDeviceSubscription* thisub=requested;thisub;thisub=thisub->next)
  {
    if(thisub->access=='a' || thisub->access=='r') 
    {
      if((deviceTable->GetDeviceAccess(thisub->code,thisub->index) == 'a') ||
         (deviceTable->GetDeviceAccess(thisub->code,thisub->index) == 'r'))
      {
        if((devicep = deviceTable->GetDevice(thisub->code,thisub->index)))
        {
          hdr.device = htons(thisub->code);
          hdr.device_index = htons(thisub->index);
          hdr.reserved = 0;

          size = devicep->GetLock()->GetData(devicep, 
                                             data+totalsize+sizeof(hdr),
                                             maxsize-totalsize-sizeof(hdr),
                                             &(hdr.timestamp_sec), 
					     &(hdr.timestamp_usec));
	  
	  // if we're in UPDATE mode, we only want this data if it is new
	  if( mode == UPDATE )
          {
            //printf( "last_sec: %u\tlast_usec: %u\n", 
            //      thisub->last_sec, thisub->last_usec );

            // if the data has the same timestamp as last time then
            // we don;t want it - just send back an empty
            // packet. (Byte order doesn't matter for the equality
            // check)
            if( hdr.timestamp_sec == thisub->last_sec && 
                hdr.timestamp_usec == thisub->last_usec )  
            {
              size = 0; // this prevents us copying in the data
              //puts( "stale data" );
            }
            //else
            //printf( "fresh data at %u sec.\n", ntohl(hdr.timestamp_sec) );

            // record the time we got data for this device
            // keep 'em in network byte order - it doesn't matter
            // as long as we remember to swap them for printing
            thisub->last_sec = hdr.timestamp_sec;
            thisub->last_usec = hdr.timestamp_usec;
          }

          hdr.size = htonl(size);

          gettimeofday(&curr,NULL);
          hdr.time_sec = htonl(curr.tv_sec);
          hdr.time_usec = htonl(curr.tv_usec);

          memcpy(data+totalsize,&hdr,sizeof(hdr));
          totalsize += sizeof(hdr) + size; 

        }
        else
        {
          printf("BuildMsg(): found NULL pointer for device \"%x:%x\"\n",
                          thisub->code, thisub->index);
        }
      }
      else
      {
        printf("BuildMsg(): Unknown device \"%x:%x\"\n",
                        thisub->code,thisub->index);
      }
    }
  }
  //pthread_mutex_unlock( &access );

  return(totalsize);
}

int CClientData::Subscribe( unsigned short code, unsigned short index )
{
  CDevice* devicep;

  if((devicep = deviceTable->GetDevice(code,index)))
  {
    return(devicep->GetLock()->Subscribe(devicep));
  }
  else
  {
    printf("Subscribe(): Unknown device \"%x:%x\" - subscribe cancelled\n", 
                    code,index);
    return(1);
  }
}


void CClientData::Unsubscribe( unsigned short code, unsigned short index )
{
  CDevice* devicep;

  if((devicep = deviceTable->GetDevice(code,index)))
  {
    devicep->GetLock()->Unsubscribe(devicep);
  }
  else
  {
    printf("Unsubscribe(): Unknown device \"%x:%x\" - unsubscribe cancelled\n", 
                    code,index);
  }
}

void
CClientData::PrintRequested(char* str)
{
  printf("%s:requested: ",str);
  pthread_mutex_lock(&access);
  for(CDeviceSubscription* thissub=requested;thissub;thissub=thissub->next)
    printf("%x:%x:%d ", thissub->code,thissub->index,thissub->access);
  pthread_mutex_unlock(&access);
  puts("");
}

int CClientData::Read()
{
  unsigned int readcnt;
  unsigned int thisreadcnt;
  player_msghdr_t hdr;

  char c;
  hdr.stx = 0;

  /* wait for the STX */
  while(hdr.stx != PLAYER_STXX)
  {
    //puts("looking for STX");

    // make sure we don't get garbage
    c = 0;

    //printf("read %d bytes; reading now\n", readcnt);
    if(read(socket,&c,1) <= 0)
    {
      if(errno == EAGAIN)
        return(0);
      else
      {
        // client must be gone. fuck 'em
        //perror("client_reader(): read() while waiting for first byte of STX");
        return(-1);
      }
    }
    //printf("c:%x\n", c);

    // This should be the high byte
    hdr.stx = ((short) c) << 8;

    if(read(socket,&c,1) <= 0)
    {
      if(errno == EAGAIN)
        return(0);
      else
      {
        // client must be gone. fuck 'em
        //perror("client_reader(): read() while waiting for second byte of STX");
        return(-1);
      }
    }
    //printf("c:%x\n", c);

    // This should be the low byte
    hdr.stx |= ((short) c);

    //printf("got:%x:\n",ntohs(hdr.stx));
    readcnt = sizeof(hdr.stx);
  }
  //puts("got STX");

  /* get the rest of the header */
  while(readcnt < sizeof(player_msghdr_t))
  {
    if((thisreadcnt = read(socket, &(hdr.type), 
                           sizeof(player_msghdr_t)-readcnt)) <= 0)
    {
      if(errno == EAGAIN)
        return(0);
      else
      {
        perror("client_reader(): read() while reading header");
        return(-1);
      }
    }
    readcnt += thisreadcnt;
  }

  // byte-swap as necessary
  hdr.type = ntohs(hdr.type);
  hdr.device = ntohs(hdr.device);
  hdr.device_index = ntohs(hdr.device_index);
  hdr.time_sec = ntohl(hdr.time_sec);
  hdr.time_usec = ntohl(hdr.time_usec);
  hdr.timestamp_sec = ntohl(hdr.timestamp_sec);
  hdr.timestamp_usec = ntohl(hdr.timestamp_usec);
  hdr.size = ntohl(hdr.size);

  //puts("got HDR");

  // make sure it's not too big
  if(hdr.size > PLAYER_MAX_MESSAGE_SIZE-sizeof(player_msghdr_t))
  {
    printf("WARNING: client's message is too big (%d bytes). Ignoring\n",
           hdr.size);
    return(0);
  }

  /* get the payload */
  readcnt = 0;
  while(readcnt != hdr.size)
  {
    if((thisreadcnt = read(socket,readbuffer+readcnt,hdr.size-readcnt)) <= 0)
    {
      if(thisreadcnt < 0 && errno != EAGAIN)
      {
        perror("CClientData::Read(): read() errored");
        return(-1);
      }
      break;
    }
    readcnt += thisreadcnt;
  }

  if(readcnt != hdr.size)
  {
    printf("CClientData::Read(): tried to read client-specified %d bytes, but "
           "only got %d\n", hdr.size, readcnt);
    return(0);
  }
  else
    return(HandleRequests(hdr,readbuffer, hdr.size));
}

int
CClientData::WriteIdentString()
{
  unsigned char data[PLAYER_IDENT_STRLEN];
  // write back an identifier string
  sprintf((char*)data, "%s%s", PLAYER_IDENT_STRING, playerversion);
  bzero(((char*)data)+strlen((char*)data),
        PLAYER_IDENT_STRLEN-strlen((char*)data));

  pthread_mutex_lock(&socketwrite);
  if(write(socket, data, PLAYER_IDENT_STRLEN) < 0 ) 
  {
    if(errno != EAGAIN)
    {
      perror("ClientManager::Write():write()");
      pthread_mutex_unlock(&socketwrite);
      return(-1);
    }
  }
  pthread_mutex_unlock(&socketwrite);
  return(0);
}

int
CClientData::Write()
{
  unsigned int size;

  size = BuildMsg(writebuffer,PLAYER_MAX_MESSAGE_SIZE);

  pthread_mutex_lock(&socketwrite);
  if(size>0 && write(socket, writebuffer, size) < 0 ) 
  {
    if(errno != EAGAIN)
    {
      perror("CClientData::Write: write()");
      pthread_mutex_unlock(&socketwrite);
      return(-1);
    }
  }
  pthread_mutex_unlock(&socketwrite);
  return(0);
}

