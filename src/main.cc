/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
 * the main code for the Player robot server.  here we instantiate
 * the device objects, do socket connection control and spawn
 * client reader/writer threads.
 */

//#define VERBOSE

#include <stdio.h>
#include <errno.h>
#include <string.h> // for bzero()
#include <stdlib.h>  /* atoi(3) */
#include <signal.h>
#include <pthread.h>  /* for pthread stuff */
#include <netinet/in.h> /* for struct sockaddr_in, SOCK_STREAM */
#include <unistd.h>  /* for close(2) */
#include <sys/types.h>  /* for accept(2) */
#include <sys/socket.h>  /* for accept(2) */

#include <pubsub_util.h> /* for create_and_bind_socket() */

#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

#ifdef INCLUDE_LASER
#include <laserdevice.h>
#endif
#ifdef INCLUDE_SONAR
#include <sonardevice.h>
#endif
#ifdef INCLUDE_VISION
#include <visiondevice.h>
#endif
#ifdef INCLUDE_POSITION
#include <positiondevice.h>
#endif
#ifdef INCLUDE_GRIPPER
#include <gripperdevice.h>
#endif
#ifdef INCLUDE_MISC
#include <miscdevice.h>
#endif
#ifdef INCLUDE_PTZ
#include <ptzdevice.h>
#endif
#ifdef INCLUDE_AUDIO
#include <audiodevice.h>
#endif
#ifdef INCLUDE_LASERBEACON
#include <laserbeacondevice.hh>
#endif
#ifdef INCLUDE_BROADCAST
#include <broadcastdevice.hh>
#endif
#ifdef INCLUDE_SPEECH
#include <speechdevice.h>
#endif

#include <clientdata.h>
#include <devicetable.h>
#include <counter.h>
#include <nodevice.h>

#include <sys/mman.h> // for mmap
#include <fcntl.h>

#ifdef INCLUDE_STAGE
#include <stagedevice.h>
#endif

caddr_t arenaIO; // the address for memory mapped IO to arena




CCounter num_threads;


CDeviceTable* deviceTable = new CDeviceTable();

// this number divided by two (two threads per client) is
// the maximum number of clients that the server will support
// i don't know what a good number would be
#define MAXNUMTHREADS 401
#define MAXNUMCLIENTS MAXNUMTHREADS/2

// keep track of the pointers to our various clients.
// that way we can cancel them at Shutdown
CClientData* clients[MAXNUMCLIENTS];
pthread_mutex_t clients_mutex;
bool SHUTTING_DOWN;

bool experimental = false;
bool debug = false;
int playerport = PLAYER_PORTNUM;

char* sane_spec[] = { "-misc:0",
                      "-gripper:0",
                      "-position:0",
                      "-sonar:0",
                      "-laser:0",
                      "-vision:0",
                      "-ptz:0",
                      "-audio:0",
                      "-laserbeacon:0",
                      "-broadcast:0",
                      "-speech:0" };


/* Usage statement */
void
Usage()
{
  puts("");
  fprintf(stderr, "USAGE: player [-port <port>] [-stage <path>] [-sane] "
          "[DEVICE]...\n");
  fprintf(stderr, "  -port <port>  : TCP port where Player will listen. "
          "Default: %d\n", PLAYER_PORTNUM);
  fprintf(stderr, "  -stage <path> : use memory-mapped IO with Stage "
          "through this file.\n");
  fprintf(stderr, "  -sane         : use the compiled-in device defaults:\n");
  for(int i=0;i<ARRAYSIZE(sane_spec);i++)
    fprintf(stderr, "                      %s\n",sane_spec[i]);
  fprintf(stderr, "\n  Each [DEVICE] should be of the form:\n");
  fprintf(stderr, "    -<name>:<index> [options]\n");
  fprintf(stderr, "  If omitted, <index> is assumed to be 0.\n");
  fprintf(stderr, "  [options] is a string of device-specific options.\n");
  fprintf(stderr, "  For example:\n");
  fprintf(stderr, "    -laser:0 \"port /dev/ttyS0\"\n");
}

/* sighandler to shut everything down properly */
void Interrupt( int dummy ) 
{
  // setting this will suppress print statements from the client deaths
  SHUTTING_DOWN = true;
  
  // first, delete the clients
  for(unsigned int i=0;i<MAXNUMCLIENTS;i++)
  {
    if(clients[i])
      delete clients[i];
  }

  // next, delete the deviceTable; it will delete all the devices internally
  delete deviceTable;

  // we're done
  puts("Player quitting");
  exit(0);
}

/* for debugging */
void PrintHeader(player_msghdr_t hdr)
{
  printf("stx: %u\n", hdr.stx);
  printf("type: %u\n", hdr.type);
  printf("device: %u\n", hdr.device);
  printf("index: %u\n", hdr.device_index);
  printf("time: %u:%u\n", hdr.time_sec,hdr.time_usec);
  printf("times: %u:%u\n", hdr.timestamp_sec,hdr.timestamp_usec);
  printf("reserved: %u\n", hdr.reserved);
  printf("size:%u\n", hdr.size);
}

/* simple func to call delete on a char* */
void
delete_func(void* ptr)
{
  delete (char*)ptr;
}

void *client_reader(void* arg)
{
  CClientData *cd = (CClientData *) arg;
  unsigned char *buffer;
  unsigned int readcnt;
  player_msghdr_t hdr;

  pthread_detach(pthread_self());

  pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
#ifdef PLAYER_LINUX
  sigblock(SIGINT);
  sigblock(SIGALRM);
#endif

  buffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE];

  //printf("client_reader() with id %ld and socket %d - created\n", 
	 //cd->readThread, cd->socket);

  pthread_cleanup_push(delete_func,buffer);
  while(1) 
  {
    char c;
    hdr.stx = 0;

    /* wait for the STX */
    // Fixed to support big-endian machines. ahoward
    while(hdr.stx != PLAYER_STXX)
    {
      //puts("looking for STX");

      // make sure we don't get garbage
      c = 0;

      //printf("read %d bytes; reading now\n", readcnt);
      if(read(cd->socket,&c,1) <= 0)
      {
        // client must be gone. fuck 'em
        //perror("client_reader(): read() while waiting for STX");
        delete cd;
      }
      //printf("c:%x\n", c);

      // This should be the high byte
      hdr.stx = ((short) c) << 8;

      if(read(cd->socket,&c,1) <= 0)
      {
        // client must be gone. fuck 'em
        //perror("client_reader(): read() while waiting for STX");
        delete cd;
      }
      //printf("c:%x\n", c);

      // This should be the low byte
      hdr.stx |= ((short) c);

      //printf("got:%x:\n",ntohs(hdr.stx));
      readcnt = sizeof(hdr.stx);
    }
    //puts("got STX");

    int thisreadcnt;
    /* get the rest of the header */
    while(readcnt < sizeof(player_msghdr_t))
    {
      if((thisreadcnt = read(cd->socket, &(hdr.type), 
                          sizeof(player_msghdr_t)-readcnt)) <= 0)
      {
        //perror("client_reader(): read() while reading header");
        delete cd;
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

    /* get the payload */
    if(hdr.size > PLAYER_MAX_MESSAGE_SIZE-sizeof(player_msghdr_t))
    {
      printf("WARNING: client's message is too big (%d bytes). Ignoring\n",
             hdr.size);
      continue;
    }

    for(readcnt  = read(cd->socket,buffer,hdr.size);
        readcnt != hdr.size;
        readcnt += read(cd->socket,buffer+readcnt,hdr.size-readcnt));

    //if((readcnt = read(cd->socket,buffer,hdr.size)) != hdr.size)
    //{
      //printf("client_reader: tried to read client-specified %d bytes, but "
                      //"only got %d\n", hdr.size, readcnt);
      //delete cd;
    //}
    //puts("got payload");

    cd->HandleRequests(hdr,buffer, hdr.size);
  } 
  pthread_cleanup_pop(1);
}

void *client_writer(void* arg)
{
  CClientData *clientData = (CClientData *) arg;
  unsigned char *data;
  int size;
  unsigned long realsleep;
  
#ifdef PLAYER_LINUX
  sigblock(SIGINT);
  sigblock(SIGALRM);
#endif
  pthread_detach(pthread_self());
  pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
  
  //printf("client_writer() with id %ld and socket %d - created\n", 
	 //clientData->writeThread, clientData->socket);

  int data_buffer_size = PLAYER_MAX_MESSAGE_SIZE;
  data = new unsigned char[data_buffer_size];
  //unsigned char data[data_buffer_size];

  // write back an identifier string
  sprintf((char*)data, "%s%s", PLAYER_IDENT_STRING, PLAYER_VERSION);
  bzero(((char*)data)+strlen((char*)data),
                  PLAYER_IDENT_STRLEN-strlen((char*)data));

  pthread_mutex_lock(&(clientData->socketwrite));
  if(write(clientData->socket, data, PLAYER_IDENT_STRLEN) < 0 ) {
    perror("client_writer");
    delete clientData;
  }
  pthread_mutex_unlock(&(clientData->socketwrite));

  pthread_cleanup_push(delete_func,data);
  for(;;)
  {
    size = clientData->BuildMsg( data, data_buffer_size );
          
    pthread_mutex_lock(&(clientData->socketwrite));
    if( size>0 && write(clientData->socket, data, size) < 0 ) {
      perror("client_writer");
      delete clientData;
    }
    pthread_mutex_unlock(&(clientData->socketwrite));
    
    if(clientData->mode == CONTINUOUS)
    {
      //printf("req. sleep for: %lu\n", 
                      //(unsigned long)(1000000.0 / clientData->frequency));

      // account for 10ms delay
      realsleep = (unsigned long)(1000000.0 / clientData->frequency)-10000;
      realsleep = max(realsleep,0);
      //usleep((unsigned long)(1000000.0 / clientData->frequency));
      usleep(realsleep);
    }
    else if(clientData->mode == REQUESTREPLY)
    {
      pthread_mutex_lock(&(clientData->datarequested));
    }
  }
  pthread_cleanup_pop(1);

  //delete data;
}

/*
 * parses strings that look like "-laser:2"
 *   <str1> is the device string; <str2> is the argument string for the device
 *
 * if the string can be parsed, then the appropriate device will be created, 
 *    and 0 will be returned.
 * otherwise, -1 is retured
 */
int
parse_device_string(char* str1, char* str2)
{
  char* colon;
  uint16_t index = 0;
  CDevice* tmpdevice;

  if(!str1 || (str1[0] != '-') || (strlen(str1) < 2))
    return(-1);

  /* get the index */
  if((colon = strchr(str1,':')))
  {
    if(strlen(colon) < 2)
      return(-1);
    index = atoi(colon+1);
  }

  /* parse the config string into argc/argv format */
  int argc = 0;
  char* argv[32];
  char* tmpptr;
  if(str2 && strlen(str2))
  {
    argv[argc++] = strtok(str2, " ");
    while(argc < (int)sizeof(argv))
    {
      tmpptr = strtok(NULL, " ");
      if(tmpptr)
        argv[argc++] = tmpptr;
      else
        break;
    }
  }

  /* get the device name */
  if(!strncmp(PLAYER_MISC_STRING,str1+1,strlen(PLAYER_MISC_STRING)))
  {
#ifdef INCLUDE_MISC
    tmpdevice = new CMiscDevice(argc,argv);
    deviceTable->AddDevice(PLAYER_MISC_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for misc device was not included at compile time.\n",stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_GRIPPER_STRING,str1+1,strlen(PLAYER_GRIPPER_STRING)))
  {
#ifdef INCLUDE_GRIPPER
    tmpdevice = new CGripperDevice(argc,argv);
    deviceTable->AddDevice(PLAYER_GRIPPER_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for gripper device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_POSITION_STRING,str1+1,
                   strlen(PLAYER_POSITION_STRING)))
  {
#ifdef INCLUDE_POSITION
    tmpdevice = new CPositionDevice(argc,argv);
    deviceTable->AddDevice(PLAYER_POSITION_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for position device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_SONAR_STRING,str1+1,strlen(PLAYER_SONAR_STRING)))
  {
#ifdef INCLUDE_SONAR
    tmpdevice = new CSonarDevice(argc,argv);
    deviceTable->AddDevice(PLAYER_SONAR_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for sonar device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_LASER_STRING,str1+1,strlen(PLAYER_LASER_STRING)))
  {
#ifdef INCLUDE_LASER
    tmpdevice = new CLaserDevice(argc,argv);
    deviceTable->AddDevice(PLAYER_LASER_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for laser device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_VISION_STRING,str1+1,strlen(PLAYER_VISION_STRING)))
  {
#ifdef INCLUDE_VISION
    tmpdevice = new CVisionDevice(argc,argv);
    deviceTable->AddDevice(PLAYER_VISION_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for vision device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_PTZ_STRING,str1+1,strlen(PLAYER_PTZ_STRING)))
  {
#ifdef INCLUDE_PTZ
    tmpdevice = new CPtzDevice(argc,argv);
    deviceTable->AddDevice(PLAYER_PTZ_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for ptz device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_AUDIO_STRING,str1+1,strlen(PLAYER_AUDIO_STRING)))
  {
#ifdef INCLUDE_AUDIO
    tmpdevice = new CAudioDevice(argc,argv);
    deviceTable->AddDevice(PLAYER_AUDIO_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for audio device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_LASERBEACON_STRING,str1+1,
                   strlen(PLAYER_LASERBEACON_STRING)))
  {
#ifdef INCLUDE_LASERBEACON
    tmpdevice = new CLaserBeaconDevice(argc,argv); 
    deviceTable->AddDevice(PLAYER_LASERBEACON_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for laserbeacon device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_BROADCAST_STRING,str1+1,
                   strlen(PLAYER_BROADCAST_STRING)))
  {
#ifdef INCLUDE_BROADCAST
    tmpdevice = new CBroadcastDevice(argc,argv);
    deviceTable->AddDevice(PLAYER_BROADCAST_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for broadcast device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_SPEECH_STRING,str1+1,strlen(PLAYER_SPEECH_STRING)))
  {
#ifdef INCLUDE_SPEECH
    tmpdevice = new CSpeechDevice(argc,argv);
    deviceTable->AddDevice(PLAYER_SPEECH_CODE, index, 
                           PLAYER_WRITE_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for speech device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_GPS_STRING,str1+1,strlen(PLAYER_GPS_STRING)))
  {
    fputs("GPS device is not yet implemented in Player.\n",stderr);
    return(-1);
  }
  else
  {
    printf("Error: unknown device \"%s\"\n", str1);
    return(-1);
  }
  return(0);
}

int main( int argc, char *argv[] )
{
  bool special_config = false;
  bool already_sane = false;
  bool use_stage = false;
  unsigned int j;
  struct sockaddr_in listener;
  //struct sockaddr_in sender;
#ifdef PLAYER_LINUX
  socklen_t sender_len;
#else
  int sender_len;
#endif
  CClientData *clientData;
  int player_sock = 0;

  // devices
  CDevice* tmpdevice = NULL;

  char arenaFile[MAX_FILENAME_SIZE]; // filename for mapped memory

  pthread_mutex_init(&clients_mutex,NULL);

  printf("** Player v%s ** ", PLAYER_VERSION);
  fflush(stdout);

  for( int i = 1; i < argc; i++ ) 
  {
    if(!strcmp(argv[i],"-stage"))
    {
      if(++i<argc) 
      {
	strncpy(arenaFile, argv[i], sizeof(arenaFile));
	use_stage = true;
	printf("[Stage]");
      }
      else 
      {
	Usage();
	exit(-1);
      }
    }
    else if(!strcmp(argv[i], "-port"))
    {
      if(++i<argc) 
      { 
	playerport = atoi(argv[i]);
	printf("[Port %d]", playerport);
      }
      else 
      {
	Usage();
	exit(-1);
      }
    }
    else if(!strcmp(argv[i], "-sane"))
    {
      for(int i=0;i<ARRAYSIZE(sane_spec);i++)
      {
        if(parse_device_string(sane_spec[i],NULL) < 0)
        {
          fprintf(stderr, "Warning: got error while creating sane "
                  "device \"%s\"\n", sane_spec[i]);
        }
      }
      already_sane = true;
    }
    else if((i+1)<argc && argv[i+1][0] != '-')
    {
      if(parse_device_string(argv[i],argv[i+1]) < 0)
      {
        Usage();
        exit(-1);
      }
      i++;
      special_config = true;
    }
    else
    {
      if(parse_device_string(argv[i],NULL) < 0)
      {
        Usage();
        exit(-1);
      }
      special_config = true;
    }
  }

  if(!already_sane && !special_config && !use_stage)
  {
    for(int i=0;i<ARRAYSIZE(sane_spec);i++)
    {
      if(parse_device_string(sane_spec[i],NULL) < 0)
      {
        fprintf(stderr, "Warning: got error while creating sane "
                "device \"%s\"\n", sane_spec[i]);
      }
    }
  }

  puts( "" ); // newline, flush

  // create the devices dynamically 
  if( use_stage )
  { 
#ifdef INCLUDE_STAGE
    // create and test the shared memory connection to arena

#ifdef VERBOSE
    printf( "Mapping shared memory through %s\n", arenaFile );
#endif

    int tfd = 0;
    if( (tfd = open( arenaFile, O_RDWR )) < 0 )
    {
      perror( "Failed to open file" );
      exit( -1 );
    }
    
    size_t areaSize = TOTAL_SHARED_MEMORY_BUFFER_SIZE;
    
    if( (arenaIO = (caddr_t)mmap( NULL, areaSize, PROT_READ | PROT_WRITE, 
			     MAP_SHARED, tfd, (off_t)0 ))  == MAP_FAILED )
    {
      perror( "Failed to map memory" );
      exit( -1 );
    }
      
    close( tfd ); // can close fd once mapped

#ifdef VERBOSE
    puts( "Testing memory map...");
    fflush( stdout );
#endif

    for(int c = 0; c < TEST_TOTAL_BUFFER_SIZE; c++ )
    {
        if( c != ((unsigned char*)arenaIO)[TEST_DATA_START + c] )
        {
            perror( "Warning: memory check failed" );
            break;
        } 
    }

#ifdef VERBOSE
    puts( "ok.\n" );
    fflush( stdout );
#endif

    /* this will all be obviated by Richard's new stuff... */
    int index = 0;
    tmpdevice = new CStageDevice( arenaIO + MISC_DATA_START,
                                   MISC_DATA_BUFFER_SIZE,
                                   MISC_COMMAND_BUFFER_SIZE,
                                   MISC_CONFIG_BUFFER_SIZE);
    deviceTable->AddDevice(PLAYER_MISC_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
    
    tmpdevice = new CStageDevice( arenaIO + POSITION_DATA_START,
                                       POSITION_DATA_BUFFER_SIZE,
                                       POSITION_COMMAND_BUFFER_SIZE,
                                       POSITION_CONFIG_BUFFER_SIZE);
    deviceTable->AddDevice(PLAYER_POSITION_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);

    tmpdevice =    new CStageDevice( arenaIO + SONAR_DATA_START,
                                       SONAR_DATA_BUFFER_SIZE,
                                       SONAR_COMMAND_BUFFER_SIZE,
                                       SONAR_CONFIG_BUFFER_SIZE); 
    deviceTable->AddDevice(PLAYER_SONAR_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
    
    tmpdevice = new CStageDevice(arenaIO + LASER_DATA_START,
                                   LASER_DATA_BUFFER_SIZE,
                                   LASER_COMMAND_BUFFER_SIZE,
                                   LASER_CONFIG_BUFFER_SIZE); 
    deviceTable->AddDevice(PLAYER_LASER_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
 
    tmpdevice = new CStageDevice(arenaIO + ACTS_DATA_START,
                                    ACTS_DATA_BUFFER_SIZE,
                                    ACTS_COMMAND_BUFFER_SIZE,
                                    ACTS_CONFIG_BUFFER_SIZE);
    deviceTable->AddDevice(PLAYER_VISION_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);

    tmpdevice = new CStageDevice(arenaIO + PTZ_DATA_START,
                                 PTZ_DATA_BUFFER_SIZE,
                                 PTZ_COMMAND_BUFFER_SIZE,
                                 PTZ_CONFIG_BUFFER_SIZE);
    deviceTable->AddDevice(PLAYER_PTZ_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);

    tmpdevice = new CStageDevice(arenaIO + LASERBEACON_DATA_START,
                                         LASERBEACON_DATA_BUFFER_SIZE,
                                         LASERBEACON_COMMAND_BUFFER_SIZE,
                                         LASERBEACON_CONFIG_BUFFER_SIZE);
    deviceTable->AddDevice(PLAYER_LASERBEACON_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);

    tmpdevice = new CStageDevice(arenaIO + BROADCAST_DATA_START,
                                       BROADCAST_DATA_BUFFER_SIZE,
                                       BROADCAST_COMMAND_BUFFER_SIZE,
                                       BROADCAST_CONFIG_BUFFER_SIZE); 
    deviceTable->AddDevice(PLAYER_BROADCAST_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);

    tmpdevice = new CStageDevice(arenaIO + GRIPPER_DATA_START,
                                     GRIPPER_DATA_BUFFER_SIZE,
                                     GRIPPER_COMMAND_BUFFER_SIZE,
                                     GRIPPER_CONFIG_BUFFER_SIZE);
    deviceTable->AddDevice(PLAYER_GRIPPER_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);

    tmpdevice = new CStageDevice(arenaIO + GPS_DATA_START,
                                     GPS_DATA_BUFFER_SIZE,
                                     GPS_COMMAND_BUFFER_SIZE,
                                     GPS_CONFIG_BUFFER_SIZE);
    deviceTable->AddDevice(PLAYER_GPS_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
    
    // unsupported devices - CNoDevice::Setup() fails
    tmpdevice =  (CDevice*)new CNoDevice();
    deviceTable->AddDevice(PLAYER_AUDIO_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
    tmpdevice =  (CDevice*)new CNoDevice();
    deviceTable->AddDevice(PLAYER_SPEECH_CODE, index, 
                           PLAYER_WRITE_MODE, tmpdevice);
#endif

  }

  /* set up to handle SIGPIPE (happens when the client dies) */
  if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGPIPE");
    exit(1);
  }

  if(signal(SIGINT, &Interrupt ) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGINT");
    exit(1);
  }

 if(signal(SIGHUP, &Interrupt ) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGHUP");
    exit(1);
  }

  if((player_sock = create_and_bind_socket(&listener,1,playerport,SOCK_STREAM,200)) 
                  == -1)
  {
    fputs("create_and_bind_socket() failed; quitting", stderr);
    exit(-1);
  }

  while(1) 
  {
    clientData = new CClientData;

    /* block here */
    if((clientData->socket = accept(player_sock,(struct sockaddr *)NULL, 
                                    &sender_len)) == -1)
    {
      perror("accept(2) failed: ");
      exit(-1);
    }

    /* got conn */
    if(num_threads.Value() < MAXNUMTHREADS)
    {
      printf("** New client accepted on socket %d ** [Port %d]\n", 
             clientData->socket,playerport);
      if(pthread_create(&clientData->writeThread, NULL, client_writer, 
                        clientData))
      {
        perror("pthread_create(3) failed");
        exit(-1);
      }
      num_threads+=1;

      if(pthread_create(&clientData->readThread, NULL, client_reader, 
                        clientData))
      {
        perror("pthread_create(3) failed");
        exit(-1);
      }
      num_threads+=1;
      
      pthread_mutex_lock(&clients_mutex);
      // find a place to put it in our array
      for(j=0;j<MAXNUMCLIENTS;j++)
      {
        if(!clients[j])
          break;
      }
      if(j == MAXNUMCLIENTS)
        puts("VERY bad. no free room to store new CClientData pointer");
      else
      {
        clients[j] = clientData;
        clientData->client_index = j;
      }
      pthread_mutex_unlock(&clients_mutex);
    }
    else
    {
      puts("No more connections accepted");
      /* this cleans up nicely */     
      delete clientData;
    }
  }

  /* don't get here */
  return(0);
}

