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

#include <devicetable.h>
#include <clientdata.h>
#include <counter.h>
#include <packet.h>

// this is the biggest single incoming message that the server
// will take.
#define REQUEST_BUFFER_SIZE 1024

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

void CClientData::HandleRequests( unsigned char *buffer,  int readcnt ) 
{
  bool request=false;
  int size = ntohs(*(unsigned short*)(buffer+2));
  int j;
  CDevice* devicep;
  player_device_req_t req;

  static unsigned char reply[REQUEST_BUFFER_SIZE];

  bzero(reply,sizeof(reply));


  if(debug)
  {
    printf("request: %c%c:%d:",buffer[0],buffer[1],size);
    for(j=0;j<size;j++)
      printf("%.2x ",buffer[2+sizeof(unsigned short)+j]);
    puts("");
  }

  pthread_mutex_lock( &requesthandling );
  
  switch(buffer[0]) 
  {
    case 'd':
      /* request message */
      request = true;
      for(j=0;j<size;j+=2)
      {
        //UpdateRequested(buffer+2+sizeof(unsigned short)+j);
        req.code = buffer[2+sizeof(unsigned short)+j];
        req.index = 0;
        req.access = buffer[2+sizeof(unsigned short)+j+1];
        UpdateRequested(req);
      }
      break;
    case 'c':
      /* command message */
      if(CheckPermissions(buffer[1],0))
      {
        // check if we can write to this device
        if((deviceTable->GetDeviceAccess(buffer[1],0) == 'w') ||
           (deviceTable->GetDeviceAccess(buffer[1],0) == 'a'))
        
        {
          // make sure we've got a non-NULL pointer
          if((devicep = deviceTable->GetDevice(buffer[1],0)))
          {
            devicep->GetLock()->PutCommand(devicep,
                            &buffer[2+sizeof(unsigned short)],size);
          }
          else
          {
            printf("HandleRequests(): found NULL pointer for device '%c'\n",
                            (char)buffer[1]);
          }
        }
        else
	  printf("You can't send commands to %c\n", (char) buffer[1]);
      }
      else 
      {
	printf("No permissions to command %c\n", buffer[1]);
      }
      break;
    case 'x':
      /* expert command */
      
      //printf("GOT EXPERT COMMAND: %c%c\n",buffer[0],buffer[1]);

      // see if this is a configuration request for the server
      // itself
      if(buffer[1] == 'y')
      {
        /* lock here so the writer thread won't interfere while
         * we change these values in the ClientData object */
        pthread_mutex_lock( &access );
        // switch on the first char of the payload
        switch(buffer[2+sizeof(unsigned short)])
        {
          case 'd':
            /* send data packet: no args */
            if (size-1 != 0)
            {
              puts("Arg to data packet request is wrong size; ignoring");
              break;
            }
            if(mode != REQUESTREPLY)
              puts("WARNING: got request for data when not in "
                              "request/reply mode");
            else
              pthread_mutex_unlock( &datarequested);
            break;
          case 'r':
            /* data transfer mode change request:
             *   0 = continuous (default)
             *   1 = request/reply
             */
            if (size-1 != sizeof(char))
            {
              puts("Arg to data transfer mode change is wrong size; ignoring");
              break;
            }
            if(buffer[2+sizeof(unsigned short)+sizeof(char)])
            {
              /* changet to request/reply */
              mode = REQUESTREPLY;
              pthread_mutex_unlock(&datarequested);
              pthread_mutex_lock(&datarequested);
            }
            else
            {
              /* change to continuous mode */
              mode = CONTINUOUS;
              pthread_mutex_unlock(&datarequested);
            }
            break;
          case 'f':
            /* change frequency of data update */
            if (size-1 != sizeof(unsigned short))
            {
              puts("Arg to frequency change request is wrong size; ignoring");
              break;
            }
            frequency = 
                    ntohs(*(unsigned short*)&buffer[2+sizeof(unsigned short)
                                    +sizeof(char)]);
            break;
          default:
            printf("Unknown server expert command %c\n", 
                            buffer[2+sizeof(unsigned short)]);
            break;
        }
        pthread_mutex_unlock( &access );
      }
      else
      {
        // pass the config request on the proper device
        // make sure we've got a non-NULL pointer
        if((devicep = deviceTable->GetDevice(buffer[1],0)))
        {
          devicep->GetLock()->PutConfig(devicep,
                          &buffer[2+sizeof(unsigned short)],size);
        }
        else
          puts("HandleRequests(): Unknown config request");
      }
      break;
    default:
      printf("HandleRequests(): Unknown request %c\n", buffer[0]);
      break;
  }
  if (request)
  {
    reply[0]='r';
    *(unsigned short*)&reply[1] = htons( size );

    for(j=0;j<size;j+=2)
    {
      reply[1+sizeof(unsigned short)+j] = buffer[2+sizeof(unsigned short)+j];
      reply[2+sizeof(unsigned short)+j] =
              FindPermission(buffer[2+sizeof(unsigned short)+j],0);
              //FindPermission( buffer[2+sizeof(unsigned short)+j] );
    }

    pthread_mutex_lock(&socketwrite);
    if( write(socket, reply, size+1+sizeof(unsigned short)) < 0 ) {
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
  //int i;
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
  if((thisub->access=='w' && (req.access=='r' || req.access=='a')) ||
     (thisub->access=='r' && (req.access=='w' || req.access=='a')))
  { 
    if(Subscribe(req.code,req.access) == 0)
      thisub->access = 'a';
    else 
      thisub->access='e';
  }
  // go from 'a' to either 'r' or 'w'
  else if(thisub->access=='a' && (req.access=='r' || req.access=='w')) 
  {
    Unsubscribe(req.code,req.access);
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
        printf("Device \"%d:%d\" already closed\n", req.code,req.index);
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
        if(Subscribe(req.code,req.index)==0)
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
    printf("The current access is \"%d:%d:%c\". ",
                    thisub->code, thisub->index, thisub->access);
    printf("Unknown unused request \"%d:%d:%c\".\n",
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
  return( 'e' );
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
  
  /* make sure that we are not changing format */
  pthread_mutex_lock( &requesthandling );

  pthread_mutex_lock( &access );

  for(CDeviceSubscription* thisub=requested;thisub;thisub=thisub->next)
  {
    if(thisub->access=='a' || thisub->access=='r') 
    {
      if((deviceTable->GetDeviceAccess(thisub->code,thisub->index) == 'a') ||
         (deviceTable->GetDeviceAccess(thisub->code,thisub->index) == 'r'))
      {
        if((devicep = deviceTable->GetDevice(thisub->code,thisub->index)))
        {
          data[totalsize] = (unsigned char)(thisub->code);
          size = devicep->GetLock()->GetData(devicep, 
                                             &data[totalsize+3], 
                                             maxsize-totalsize-3);

          // *** HACK -- ahoward
          // Skip this data if it is zero length
          //
          if(size == 0)
          {
              puts("BuldMsg(): got zero length data; ignoring");
              continue;
          }
          
          *(unsigned short *)&data[totalsize+1] = htons ( size );
          totalsize+=size+3;
        }
        else
        {
          printf("BuildMsg(): found NULL pointer for device \"%d:%d\"\n",
                          thisub->code, thisub->index);
        }
      }
      else
      {
        printf("BuildMsg(): Unknown device \"%d:%d\"\n",
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
    printf("Subscribe(): Unknown device \"%d:%d\" - subscribe cancelled\n", 
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
    printf("Unsubscribe(): Unknown device \"%d:%d\" - unsubscribe cancelled\n", 
                    code,index);
  }
}

void
CClientData::PrintRequested(char* str)
{
  printf("%s:requested: ",str);
  for(CDeviceSubscription* thissub=requested;thissub;thissub=thissub->next)
    printf("%d:%d:%d ", thissub->code,thissub->index,thissub->access);
  puts("");
}


