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
  readThread = 0;
  writeThread = 0;
  socket = 0;
  mode = CONTINUOUS;
  frequency = 10;
  memset(requested, 0, 20);
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
        UpdateRequested(buffer+2+sizeof(unsigned short)+j);
      break;
    case 'c':
      /* command message */
      if(CheckPermissions(buffer)) 
      {
        // check if we can write to this device
        if((deviceTable->GetDeviceAccess(buffer[1]) == 'w') ||
           (deviceTable->GetDeviceAccess(buffer[1]) == 'a'))
        
        {
          // make sure we've got a non-NULL pointer
          if((devicep = deviceTable->GetDevice(buffer[1])))
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
        if((devicep = deviceTable->GetDevice(buffer[1])))
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
              FindPermission( buffer[2+sizeof(unsigned short)+j] );
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
  //puts("~CClientData():calling RemoveReadRequests");
  RemoveReadRequests();
  //puts("~CClientData():calling RemoveWriteRequests");
  RemoveWriteRequests();

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

void CClientData::RemoveBlanks() 
{
  int freespot, takenspot;

  //PrintRequested("RemoveBlanks()");
  for( freespot=0; freespot<18; freespot+=2) {
    if ( requested[freespot]==0) {
      for( takenspot = freespot; takenspot<20; takenspot+=2) {
	if ( requested[takenspot]!=0) {
	  /* no zero entry found */
	  requested[freespot]=requested[takenspot];
	  requested[freespot+1]=requested[takenspot+1];
	  requested[takenspot] = 0;
	  requested[takenspot+1] = 0;
          break;
	}
      }
      
      /* no more elements */
      if (takenspot == 20 ) break;
    }
  }
  //PrintRequested("RemoveBlanks()");
}
  
void CClientData::RemoveReadRequests() 
{
  int i;

  pthread_mutex_lock( &access );
  //PrintRequested("RemoveReadRequests()");
  for(i=1;requested[i]!=0 ;i+=2) {
    switch(requested[i]) {
    case 'a':
      requested[i]='w';
      //printf("RemoveReadRequests(): Unsubscribe(%c)\n",requested[i-1]);
      Unsubscribe( requested[i-1] );
      break;
    case 'r':
      //printf("RemoveReadRequests(): Unsubscribe(%c)\n",requested[i-1]);
      Unsubscribe( requested[i-1] );
      requested[i]=0;
      requested[i-1]=0;
      break;
    default:
      break;
    }
  }
  //PrintRequested("RemoveReadRequests()");
  RemoveBlanks();
  pthread_mutex_unlock( &access );
}

void CClientData::MotorStop() 
{
  unsigned char command[4];
  CDevice* devicep;

  *( short *)&command[0]=0;
  *( short *)&command[sizeof(short)]=0;

  if((devicep = deviceTable->GetDevice('p')))
    devicep->GetLock()->PutCommand(devicep, command, 4);
  else
    puts("MotorStop(): got NULL for the 'p' device");
}

void CClientData::RemoveWriteRequests() 
{
  int i;

  pthread_mutex_lock( &access );
  //PrintRequested("RemoveWriteRequests()");
  for(i=1;requested[i]!=0;i+=2) {
    switch(requested[i]) {
    case 'a':
      //printf("RemoveWriteRequests(): Unsubscribe(%c)\n",requested[i-1]);
      Unsubscribe( requested[i-1] );
      if (requested[i-1]=='p') 
      {
	/* stop motors for safety */
	MotorStop();
      }
      requested[i]='r';
      break;
    case 'w':
      //printf("RemoveWriteRequests(): Unsubscribe(%c)\n",requested[i-1]);
      Unsubscribe( requested[i-1] );
      if (requested[i-1]=='p') {
	/* stop motors for safety */
	MotorStop();
      }
      requested[i]=0;
      requested[i-1]=0;
      break;
    default:
      break;
    }
  }
  //PrintRequested("RemoveWriteRequests()");
  RemoveBlanks();
  pthread_mutex_unlock( &access );
}

void CClientData::UpdateRequested( unsigned char *request ) 
{
  int i;

  pthread_mutex_lock( &access );

  //printf("UpdateRequested():request:%s:\n",request);
  //PrintRequested("UpdateRequested():");
  // find place to place the update
  for(i=0; requested[i]!=0; i+=2) {
    if (request[0]==requested[i]) break;
  }

  /* UPDATE */
  if ( (requested[i+1]=='w' && (request[1]=='r' || request[1]=='a')) ||
       (requested[i+1]=='r' && (request[1]=='w' || request[1]=='a')) ||
       (requested[i+1]=='e' && 
          (request[1]=='w' || request[1]=='a' || request[1]=='r'))) { 
    if ( Subscribe( request[0] ) == 0 )
      requested[i+1]='a';
    else 
      requested[i+1]='e';
  }
  else if (requested[i+1]=='a' && ( request[1]=='r' || request[1]=='w') ) {
    requested[i+1]=request[1];
    Unsubscribe( request[0] );
  }

  /* CLOSE */
  else if (request[1]=='c') {
    // close 
    switch(requested[i+1]) {
    case 'a':
      //printf("UpdateRequested(): Unsubscribe(%c)\n",requested[i]);
      Unsubscribe(requested[i]);      // we want to unsubscribe two times
    case 'w':
    case 'r':
      //printf("UpdateRequested(): Unsubscribe(%c)\n",requested[i]);
      Unsubscribe(requested[i]);
      //requested[i]=0;
      //requested[i+1]=0;
      requested[i+1]='c';
      RemoveBlanks();
      break;
    case 'c':
    case 0:
       printf("Device \"%c\" already closed\n", request[0]);
       break;
    default:
      printf("Unknown access permission \"%c\"\n", requested[i+1]);
      break;
    }

    /* check if it is time to shutdown */
    if (requested[0]==0 || requested[1]=='c') {
      pthread_mutex_unlock( &access );
      /* nothing to do closing */
      /* should we really close here? */
      //delete this;
    }
  }

  /* OPEN */
  else if ( requested[i+1]==0 || requested[i+1]=='c') {
    requested[i] = request[0];
    switch( request[1] ) {
    case 'a':
      if ( Subscribe( request[0] )==0 && Subscribe( request[0] ) == 0)
	requested[i+1]='a';
      else
	requested[i+1]='e';
     break;
    case 'w':
      if (Subscribe( request[0] )==0) 
	requested[i+1]='w';
      else 
	requested[i+1]='e';
      break;
    case 'r':
      if (Subscribe( request[0] )==0) 
	requested[i+1]='r';
      else
	requested[i+1]='e';
      break;
    default:
      printf("Unknown request \"%c%c\"\n", request[0], request[1]);
    }
  }

  /* IGNORE */
  else {
    printf("The current access is \"%c%c\". ",requested[i], requested[i+1]);
    printf("Unknown unused request \"%c%c\"\n", request[0], request[1]);
  }

  //PrintRequested("UpdateRequested():");
  pthread_mutex_unlock( &access );
}

unsigned char CClientData::FindPermission( unsigned char device ) {
  for(int i=0;i<18;i+=2) {
    if (requested[i]==device) return( requested[i+1] );
  }
			
  return( 'e' );
}

bool CClientData::CheckPermissions( unsigned char *command ) {
  bool permission = false;
  unsigned char letter;

  pthread_mutex_lock( &access );
 
  switch( command[0] ) 
  {
    case 'l':
    case 's':
    case 'p':
    case 'v':
    case 'g':
    case 'm':
    case 'z':
      letter = FindPermission( command[0] );
      if ((letter=='a') || (command[0]==letter)) permission = true;
      break;
    case 'c':
      letter = FindPermission( command[1] );
      if ((letter=='a') || ('w'==letter)) permission = true;
      break;
    default:
      printf("Expected device or command but got %c\n", command[0]);
      break;
  }
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

  for(int i=0;requested[i]!=0 && requested[i]!='c';i+=2) 
  {
    if(requested[i+1]=='a' || requested[i+1]=='r') 
    {
      if((deviceTable->GetDeviceAccess(requested[i]) == 'a') ||
         (deviceTable->GetDeviceAccess(requested[i]) == 'r'))
      {
        if((devicep = deviceTable->GetDevice(requested[i])))
        {
          data[totalsize] = requested[i];
          size = devicep->GetLock()->GetData(devicep, &data[totalsize+3], maxsize-totalsize-3);

          // *** HACK -- ahoward
          // Skip this data if it is zero length
          //
          if (size == 0)
          {
              puts("BuldMsg(): got zero length data; ignoring");
              continue;
          }
          
          *(unsigned short *)&data[totalsize+1] = htons ( size );
          totalsize+=size+3;
        }
        else
        {
          printf("BuildMsg(): found NULL pointer for device '%c'\n",
                          (char)requested[i]);
        }
      }
      else
      {
        printf("BuildMsg(): Unknown device \"%c\"\n", (char)requested[i]);
      }
    }
  }
  pthread_mutex_unlock( &access );
  pthread_mutex_unlock( &requesthandling );

  return(totalsize);
}

int CClientData::Subscribe( unsigned char device ) 
{
  CDevice* devicep;

  if((devicep = deviceTable->GetDevice(device)))
  {
    return(devicep->GetLock()->Subscribe(devicep));
  }
  else
  {
    printf("Subscribe(): Unknown device \"%c\" - subscribe cancelled\n", 
                    device);
    return( 1 );
  }
}


void CClientData::Unsubscribe( unsigned char device ) 
{
  CDevice* devicep;

  if((devicep = deviceTable->GetDevice(device)))
  {
    devicep->GetLock()->Unsubscribe(devicep);
  }
  else
  {
    printf("Unsubscribe(): Unknown device \"%c\" - unsubscribe cancelled\n", 
                    device);
  }
}

void
CClientData::PrintRequested(char* str)
{
  int i;
  printf("%s:requested: ",str);
  for(i=0;i<20;i++)
  {
    if(!requested[i])
      printf("0");
    else
      printf("%c",requested[i]);
  }
  puts("");
}


