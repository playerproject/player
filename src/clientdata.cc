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
#include <netinet/in.h>
#include <sys/time.h>

#include <devicetable.h>
#include <clientdata.h>
#include <counter.h>
#include <packet.h>

#ifdef PLAYER_SOLARIS
  #include <strings.h>  /* for bzero */
#endif

extern CDeviceTable* deviceTable;
extern CCounter num_threads;
extern CClientData* clients[];
extern pthread_mutex_t clients_mutex;
extern bool SHUTTING_DOWN;


CClientData::CClientData() 
{
  requested = NULL;
  numsubs = 0;
  readThread = 0;
  writeThread = 0;
  socket = 0;
  mode = CONTINUOUS;
  frequency = 10;
  pthread_mutex_init( &access, NULL ); 
  pthread_mutex_init( &datarequested, NULL ); 
  pthread_mutex_init( &requesthandling, NULL ); 
  pthread_mutex_init( &socketwrite, NULL ); 
}

void CClientData::HandleRequests(player_msghdr_t hdr, unsigned char *payload,  
                                 unsigned int payload_size) 
{
  bool request=false;
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

  static unsigned char reply[PLAYER_MAX_MESSAGE_SIZE];

  bzero(reply,sizeof(reply));

  if(0)
  {
    printf("type:%u device:%u index:%u\n", 
       hdr.type,hdr.device,hdr.device_index);
    printf("Request(%d):",payload_size);
    for(unsigned int i=0;i<payload_size;i++)
      printf("%c",payload[i]);
    printf("\t");
    for(unsigned int i=0;i<payload_size;i++)
      printf("%x ",payload[i]);
    puts("");
  }

  pthread_mutex_lock( &requesthandling );
  
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
          return;
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
              //return;
            }
            for(j=sizeof(player_device_ioctl_t);
                j<payload_size-(sizeof(player_device_req_t)-1);
                j+=sizeof(player_device_req_t))
            {
              //puts("memcpying request");
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
            if(datamode.mode)
            {
              /* changet to request/reply */
              //puts("changing to REQUESTREPLY");
              mode = REQUESTREPLY;
              pthread_mutex_unlock(&datarequested);
              pthread_mutex_lock(&datarequested);
            }
            else
            {
              /* change to continuous mode */
              //puts("changing to CONTINUOUS");
              mode = CONTINUOUS;
              pthread_mutex_unlock(&datarequested);
            }
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
              pthread_mutex_unlock( &datarequested);
            break;
          case PLAYER_PLAYER_DATAFREQ_REQ:
            if(real_payloadsize != sizeof(unsigned short))
            {
              printf("HandleRequests(): got wrong size "
                     "arg for update frequency change: %d\n",real_payloadsize);
              break;
            }
            memcpy(&datafreq,payload+sizeof(player_device_ioctl_t),
                            sizeof(player_device_datafreq_req_t));
            frequency = ntohs(datafreq.frequency);
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
        // check if we can write to this device
        if((deviceTable->GetDeviceAccess(hdr.device,hdr.device_index) == 'w') ||
           (deviceTable->GetDeviceAccess(hdr.device,hdr.device_index) == 'a'))
        
        {
          // make sure we've got a non-NULL pointer
          if((devicep = deviceTable->GetDevice(hdr.device,hdr.device_index)))
          {
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
      memcpy(reply+sizeof(player_msghdr_t),payload,
                      sizeof(player_device_ioctl_t));
      for(i=sizeof(player_msghdr_t)+sizeof(player_device_ioctl_t),
          j=sizeof(player_device_ioctl_t);
          j<payload_size-(sizeof(player_device_req_t)-1);
          i+=sizeof(player_device_req_t),j+=sizeof(player_device_req_t))
      {
        memcpy(&req,payload+j,sizeof(player_device_req_t));
        req.access = FindPermission(ntohs(req.code),ntohs(req.index));
        memcpy(reply+i,&req,sizeof(player_device_req_t));
      }
    }
    /* otherwise, just copy back the request, since we can't get result
     * codes here
     */
    else
    {
      memcpy(reply+sizeof(player_msghdr_t),payload,payload_size);
    }

    gettimeofday(&curr,NULL);
    reply_hdr.time_sec = htonl(curr.tv_sec);
    reply_hdr.time_usec = htonl(curr.tv_usec);

    reply_hdr.timestamp_sec = reply_hdr.time_sec;
    reply_hdr.timestamp_usec = reply_hdr.time_usec;
    memcpy(reply,&reply_hdr,sizeof(player_msghdr_t));

    pthread_mutex_lock(&socketwrite);
    if(write(socket, reply, payload_size+sizeof(player_msghdr_t)) < 0) 
    {
      perror("HandleRequests");
      delete this;
    }
    pthread_mutex_unlock(&socketwrite);
  }

  pthread_mutex_unlock( &requesthandling );
}

CClientData::~CClientData() 
{
  RemoveRequests();

  usleep(100000);

  pthread_mutex_lock( &access );
  if (readThread) num_threads-=1;
  if (writeThread) num_threads-=1;

  if (socket) close(socket);
  printf("** Killing client on socket %d **\n", socket);

  if (readThread && writeThread) 
  {
    // kill whichever threads that are not me
    if(!pthread_equal(pthread_self(), readThread)) {
      pthread_cancel(readThread);
    }
    if(!pthread_equal(pthread_self(), writeThread))
    {
      pthread_cancel(writeThread);
    }
    
    if(!SHUTTING_DOWN)
    {
      //printf("client_writer() with id %ld - killed\n", 
                      //writeThread);
      //printf("client_reader() with id %ld - killed\n", 
                      //readThread);
    }
    pthread_mutex_lock(&clients_mutex);
    clients[client_index] = NULL;
    pthread_mutex_unlock(&clients_mutex);
  }

  pthread_mutex_destroy( &access );
  pthread_mutex_destroy( &requesthandling );
  pthread_mutex_destroy( &socketwrite );
  pthread_mutex_destroy( &datarequested );
     
  if(pthread_equal(pthread_self(),readThread) ||  
     pthread_equal(pthread_self(),writeThread)) 
    pthread_exit(0);
}

void CClientData::RemoveRequests() 
{
  CDeviceSubscription* thissub = requested;
  CDeviceSubscription* tmpsub;

  pthread_mutex_lock( &access );
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
  pthread_mutex_unlock( &access );
}

void CClientData::MotorStop() 
{
  unsigned char command[4];
  CDevice* devicep;

  *( short *)&command[0]=0;
  *( short *)&command[sizeof(short)]=0;

  if((devicep = deviceTable->GetDevice(PLAYER_POSITION_CODE,0)))
    devicep->GetLock()->PutCommand(devicep, command, 4);
  else
    puts("MotorStop(): got NULL for the position device");
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
  for(CDeviceSubscription* thisub=requested;thisub;thisub=thisub->next)
  {
    if((thisub->code == code) && (thisub->index == index))
      return(thisub->access);
  }
  return('e');
}

bool CClientData::CheckPermissions(unsigned short code, unsigned short index)
{
  bool permission = false;
  unsigned char letter;

  pthread_mutex_lock( &access );
 
  letter = FindPermission(code,index);
  if((letter=='a') || ('w'==letter)) 
    permission = true;

  pthread_mutex_unlock( &access );

  return(permission);
}

int CClientData::BuildMsg( unsigned char *data, size_t maxsize) 
{
  unsigned short size, totalsize=0;
  CDevice* devicep;
  player_msghdr_t hdr;
  struct timeval curr;
  
  /* make sure that we are not changing format */
  pthread_mutex_lock( &requesthandling );

  pthread_mutex_lock( &access );

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
          
          //puts("CClientData::BuildMsg() calling GetData");
          size = devicep->GetLock()->GetData(devicep, 
                                             data+totalsize+sizeof(hdr),
                                             maxsize-totalsize-sizeof(hdr),
                                             &(hdr.timestamp_sec), &(hdr.timestamp_usec));
          //puts("CClientData::BuildMsg() called GetData");

          // *** HACK -- ahoward
          // Skip this data if it is zero length
          //
          if(size == 0)
          {
              puts("BuldMsg(): got zero length data; ignoring");
              continue;
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
  pthread_mutex_unlock( &access );
  pthread_mutex_unlock( &requesthandling );

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
  for(CDeviceSubscription* thissub=requested;thissub;thissub=thissub->next)
    printf("%x:%x:%d ", thissub->code,thissub->index,thissub->access);
  puts("");
}


