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

// suck in the version string from the file in the top level directory
#include "../VERSION"


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

#define DEFAULT_ACTS_PORT 5001
#define DEFAULT_ACTS_CONFIGFILE "/usr/local/acts/actsconfig"

#define DEFAULT_P2OS_PORT "/dev/ttyS0"
#define DEFAULT_LASER_PORT "/dev/ttyS1"

#define DEFAULT_PTZ_PORT "/dev/ttyS2"

#define USAGE \
  "USAGE: player [-gp <port>] [-pp <port>] [-lp <port>] [-zp <port>] \n" \
  "             [-vp <port>] [-vc <path>] [-stage <path>] [-exp] [-debug]\n" \
  "  -gp <port>   : TCP port where Player will listen. Default: 6665\n" \
  "  -pp <port>   : serial port where P2OS device is connected. Default: /dev/ttyS0\n" \
  "  -lp <port>   : serial port where laser is connected. Default: /dev/ttyS1\n" \
  "  -zp <port>   : serial port where PTZ camera is connected.  Default: /dev/ttyS2\n" \
  "  -vp <port>   : TCP port where ACTS should run. Default: 5001\n" \
  "  -vc <path>   : config file for ACTS. Default: /usr/local/acts/actsconfig\n" \
  "  -stage <path>: use memory-mapped IO with Stage through this name in the filesystem\n" \
  "  -exp         : use experimental features\n" \
  "  -debug       : turn on debugging info"

CCounter num_threads;


CDeviceTable* deviceTable = new CDeviceTable();

// this number divided by two (two threads per client) is
// the maximum number of clients that the server will support
// i don't know what a good number would be
#define MAXNUMTHREADS 16
#define MAXNUMCLIENTS MAXNUMTHREADS/2

// keep track of the pointers to our various clients.
// that way we can cancel them at Shutdown
CClientData* clients[MAXNUMCLIENTS];
pthread_mutex_t clients_mutex;
bool SHUTTING_DOWN;

bool experimental = false;
bool debug = false;

void Interrupt( int dummy ) {
  // setting this will suppress print statements from the client
  // deaths
  SHUTTING_DOWN = true;
  // first, delete the clients
  for(unsigned int i=0;i<MAXNUMCLIENTS;i++)
  {
    if(clients[i])
      delete clients[i];
  }

  //delete laserDevice;
  //delete sonarDevice;
  //delete visionDevice;
  //delete positionDevice;
  //delete gripperDevice;
  //delete miscDevice;
  //delete ptzDevice;
  delete deviceTable;
  //pthread_kill_other_threads_np();
  puts("Player quitting");
  exit(0);
}

void *client_reader(void* arg)
{
  CClientData *cd = (CClientData *) arg;
  unsigned char *buffer;
  unsigned int readcnt;
  player_msghdr_t hdr;

  pthread_detach(pthread_self());

  pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
  sigblock(SIGINT);
  sigblock(SIGALRM);

  buffer = new unsigned char[PLAYER_MAX_MESSAGE_SIZE];

  //printf("client_reader() with id %ld and socket %d - created\n", 
	 //cd->readThread, cd->socket);

  while(1) 
  {
    hdr.stx = 0;
    /* wait for the STX */
    while(hdr.stx != PLAYER_STX)
    {
      //printf("looking for STX:%x:\n",PLAYER_STX);
      if((readcnt = read(cd->socket,&(hdr.stx),sizeof(hdr.stx))) <= 0)
      {
        // client must be gone. fuck 'em
        //perror("client_reader(): read() while waiting for STX");
        delete cd;
      }
      //printf("got:%x:\n",hdr.stx);
    }
    //puts("got STX");

    /* get the rest of the header */
    if((readcnt += read(cd->socket,
                        &(hdr.type),
                        sizeof(player_msghdr_t)-sizeof(hdr.stx))) != 
                    sizeof(player_msghdr_t))
    {
      //perror("client_reader(): read() while reading header");
      delete cd;
    }

    // byte-swap as necessary
    hdr.type = ntohs(hdr.type);
    hdr.device = ntohs(hdr.device);
    hdr.device_index = ntohs(hdr.device_index);
    hdr.time = ntohl(hdr.time);
    hdr.timestamp = ntohl(hdr.timestamp);
    hdr.size = ntohl(hdr.size);
    //puts("got HDR");

    /* get the payload */
    if(hdr.size > PLAYER_MAX_MESSAGE_SIZE-sizeof(player_msghdr_t))
      printf("WARNING: client's message is too big (%d bytes). "
                      "Buffer overflow likely.", hdr.size);

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
}

void *client_writer(void* arg)
{
  CClientData *clientData = (CClientData *) arg;
  unsigned char *data;
  int size;
  
  sigblock(SIGINT);
  sigblock(SIGALRM);
  pthread_detach(pthread_self());
  pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
  
  //printf("client_writer() with id %ld and socket %d - created\n", 
	 //clientData->writeThread, clientData->socket);

  int data_buffer_size = PLAYER_MAX_MESSAGE_SIZE;
  data = new unsigned char[data_buffer_size];

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
      usleep((unsigned long)(1000000.0 / clientData->frequency));
    }
    else if(clientData->mode == REQUESTREPLY)
    {
      pthread_mutex_lock(&(clientData->datarequested));
    }
  }

  delete data;
}

int main( int argc, char *argv[] )
{
  unsigned int j;
  struct sockaddr_in listener;
  struct sockaddr_in sender;
  socklen_t sender_len;
  CClientData *clientData;
  int player_sock = 0;

  // devices
  CDevice* laserDevice = NULL;
  CDevice* sonarDevice = NULL;
  CDevice* visionDevice = NULL;
  CDevice* positionDevice = NULL;
  CDevice* gripperDevice = NULL;
  CDevice* miscDevice = NULL;
  CDevice* ptzDevice = NULL;
  CDevice* audioDevice = NULL;
  CDevice* laserbeaconDevice = NULL;
  CDevice* broadcastDevice = NULL;

  /* use these to temporarily store command-line args */
  int playerport = PLAYER_PORTNUM;
  char p2osport[MAX_FILENAME_SIZE] = DEFAULT_P2OS_PORT;
  char laserserialport[MAX_FILENAME_SIZE] = DEFAULT_LASER_PORT;
  char ptzserialport[MAX_FILENAME_SIZE] = DEFAULT_PTZ_PORT;
  int  visionport = DEFAULT_ACTS_PORT;
  char visionconfigfile[MAX_FILENAME_SIZE] = DEFAULT_ACTS_CONFIGFILE;
  bool useoldacts = false;

  int useArena = false;
  char arenaFile[128]; // filename for mapped memory

  pthread_mutex_init(&clients_mutex,NULL);

  // printf("Player Robot Server v%s\n", PLAYER_VERSION);
  
  // new look to match Stage - revert if you don't like it! - RTV
  printf("** Player v%s ** ", PLAYER_VERSION);

  for( int i = 1; i < argc; i++ ) 
  {
    //printf( "ARG: %s\n", argv[i] ); fflush( stdout );

    if ( strcmp( argv[i], "-lp") == 0 ) {
      i++;
      if (i<argc) { 
	//strcpy( laserDevice->LASER_SERIAL_PORT, argv[i] );
	strcpy( laserserialport, argv[i] );
	printf("Assuming laser is connected to serial port \"%s\"\n",
	       argv[i]);
      }
      else {
	puts(USAGE);
	exit(0);
      }
    }
    else if ( strcmp( argv[i], "-zp") == 0 ) {
      i++;
      if (i<argc) { 
	strcpy(ptzserialport,argv[i]);
	printf("Assuming PTZ camera is connected to serial port \"%s\"\n",
                        ptzserialport);
      }
      else {
	puts(USAGE);
	exit(0);
      }
    }
    else if ( strcmp( argv[i], "-pp") == 0 ) {
      i++;
      if (i<argc) { 
	strcpy(p2osport,argv[i]);
	printf("Assuming P2OS device (the robot) is connected to serial port \"%s\"\n",
                        p2osport);
      }
      else {
	puts(USAGE);
	exit(0);
      }
    }
    else if ( strcmp( argv[i], "-gp") == 0 ) {
      i++;
      if (i<argc) { 
	playerport = atoi(argv[i]);
	//printf("Will run Player on port %d\n", playerport);
	// new look to match Stage - what do you think?
	// just chuck this out if you don't like it - RTV
	printf("[Port %d]", playerport);
      }
      else {
	puts(USAGE);
	exit(0);
      }
    }
    else if ( strcmp( argv[i], "-debug") == 0 ) {
      puts("Debug info comin' at ya'");
      debug = true;
    }
    else if ( strcmp( argv[i], "-vp") == 0 ) {
      i++;
      if (i<argc) { 
	//visionDevice->portnum = atoi(argv[i]);
	visionport = atoi(argv[i]);
	printf("Will run ACTS on port %d\n", visionport);
      }
      else {
	puts(USAGE);
	exit(0);
      }
    }
    else if ( strcmp( argv[i], "-vc") == 0 ) {
      i++;
      if (i<argc) { 
        strcpy(visionconfigfile, argv[i]);
	printf("ACTS will use config file \"%s\"\n", argv[i]);
      }
      else {
	puts(USAGE);
	exit(0);
      }
    }
    else if ( strcmp( argv[i], "-stage") == 0 ) {
      i++;
      if( i<argc ) {
	strcpy( arenaFile, argv[i] );
	useArena = true;
	//printf( "Taking the Stage\n" );
	// new look to match Stage - what do you think?
	// just chuck this out if you don't like it - RTV
	printf("[Stage]");
      }
      else {
	puts( USAGE );
	exit( 0 );
      }
    }
    else if ( strcmp( argv[i], "-exp") == 0 ) {
      experimental = true;
      puts("Experimental");
    }
    else {
      puts(USAGE);
      exit(0);
    }
  }

  puts( "" ); // newline, flush

  // create the devices dynamically 
  if( useArena )
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

#ifdef INCLUDE_MISC
    miscDevice = new CStageDevice( arenaIO + MISC_DATA_START,
                                   MISC_DATA_BUFFER_SIZE,
                                   MISC_COMMAND_BUFFER_SIZE,
                                   MISC_CONFIG_BUFFER_SIZE);
#endif
    
    positionDevice = new CStageDevice( arenaIO + POSITION_DATA_START,
                                       POSITION_DATA_BUFFER_SIZE,
                                       POSITION_COMMAND_BUFFER_SIZE,
                                       POSITION_CONFIG_BUFFER_SIZE);

    sonarDevice =    new CStageDevice( arenaIO + SONAR_DATA_START,
                                       SONAR_DATA_BUFFER_SIZE,
                                       SONAR_COMMAND_BUFFER_SIZE,
                                       SONAR_CONFIG_BUFFER_SIZE); 
    
    laserDevice = new CStageDevice(arenaIO + LASER_DATA_START,
                                   LASER_DATA_BUFFER_SIZE,
                                   LASER_COMMAND_BUFFER_SIZE,
                                   LASER_CONFIG_BUFFER_SIZE); 
 
    visionDevice = new CStageDevice(arenaIO + ACTS_DATA_START,
                                    ACTS_DATA_BUFFER_SIZE,
                                    ACTS_COMMAND_BUFFER_SIZE,
                                    ACTS_CONFIG_BUFFER_SIZE);

    ptzDevice = new CStageDevice(arenaIO + PTZ_DATA_START,
                                 PTZ_DATA_BUFFER_SIZE,
                                 PTZ_COMMAND_BUFFER_SIZE,
                                 PTZ_CONFIG_BUFFER_SIZE);
#ifdef INCLUDE_LASERBEACON
    laserbeaconDevice = new CStageDevice(arenaIO + LASERBEACON_DATA_START,
                                         LASERBEACON_DATA_BUFFER_SIZE,
                                         LASERBEACON_COMMAND_BUFFER_SIZE,
                                         LASERBEACON_CONFIG_BUFFER_SIZE);
#endif

#ifdef INCLUDE_BROADCAST
    broadcastDevice = new CStageDevice(arenaIO + BROADCAST_DATA_START,
                                       BROADCAST_DATA_BUFFER_SIZE,
                                       BROADCAST_COMMAND_BUFFER_SIZE,
                                       BROADCAST_CONFIG_BUFFER_SIZE); 
#endif    

    
    // unsupported devices - CNoDevice::Setup() fails
#ifdef INCLUDE_GRIPPER
    gripperDevice = (CGripperDevice*)new CNoDevice();
#endif
#ifdef INCLUDE_AUDIO
    audioDevice =  (CAudioDevice*)new CNoDevice();
#endif
#endif

  }
  else 
  { // use the real robot devices
#ifdef INCLUDE_LASER
    laserDevice =    new CLaserDevice(laserserialport);
#endif
#ifdef INCLUDE_SONAR
    sonarDevice =    new CSonarDevice(p2osport);
#endif
#ifdef INCLUDE_VISION
    visionDevice =  
      new CVisionDevice(visionport,visionconfigfile,useoldacts);
#endif
#ifdef INCLUDE_POSITION
    positionDevice = new CPositionDevice(p2osport);
#endif
#ifdef INCLUDE_GRIPPER
    gripperDevice =  new CGripperDevice(p2osport);
#endif
#ifdef INCLUDE_MISC
    miscDevice =     new CMiscDevice(p2osport);
#endif
#ifdef INCLUDE_PTZ
    ptzDevice =     new CPtzDevice(ptzserialport);
#endif
#ifdef INCLUDE_AUDIO
    audioDevice =     new CAudioDevice();
#endif
#ifdef INCLUDE_LASERBEACON
    laserbeaconDevice = new CLaserBeaconDevice(laserDevice);
#endif
#ifdef INCLUDE_BROADCAST
    broadcastDevice = new CBroadcastDevice;
#endif
  }

  // add the devices to the global table
#ifdef INCLUDE_LASER
  deviceTable->AddDevice(PLAYER_LASER_CODE, 0, PLAYER_READ_MODE, laserDevice);
#endif
#ifdef INCLUDE_SONAR
  deviceTable->AddDevice(PLAYER_SONAR_CODE, 0, PLAYER_READ_MODE, sonarDevice);
#endif
#ifdef INCLUDE_VISION
  deviceTable->AddDevice(PLAYER_VISION_CODE, 0, PLAYER_READ_MODE, visionDevice);
#endif
#ifdef INCLUDE_POSITION
  deviceTable->AddDevice(PLAYER_POSITION_CODE, 0, PLAYER_ALL_MODE, positionDevice);
#endif
#ifdef INCLUDE_GRIPPER
  deviceTable->AddDevice(PLAYER_GRIPPER_CODE, 0, PLAYER_ALL_MODE, gripperDevice);
#endif
#ifdef INCLUDE_MISC
  deviceTable->AddDevice(PLAYER_MISC_CODE, 0, PLAYER_READ_MODE, miscDevice);
#endif
#ifdef INCLUDE_PTZ
  deviceTable->AddDevice(PLAYER_PTZ_CODE, 0, PLAYER_ALL_MODE, ptzDevice);
#endif
#ifdef INCLUDE_AUDIO
  deviceTable->AddDevice(PLAYER_AUDIO_CODE, 0, PLAYER_ALL_MODE, audioDevice);
#endif
#ifdef INCLUDE_LASERBEACON
  deviceTable->AddDevice(PLAYER_LASERBEACON_CODE, 0, PLAYER_READ_MODE, laserbeaconDevice);
#endif
#ifdef INCLUDE_BROADCAST
  deviceTable->AddDevice(PLAYER_BROADCAST_CODE, 0, PLAYER_ALL_MODE, broadcastDevice);
#endif

  /* set up to handle SIGPIPE (happens when the client dies) */
  if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGPIPE");
    exit(1);
  }
  //if(signal(SIGALRM, EmptySigHanndler) == SIG_ERR)
  //{
    //perror("signal(2) failed while setting up for SIGALRM");
    //exit(1);
  //}

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

  if((player_sock = create_and_bind_socket(&listener,1,playerport,SOCK_STREAM,0)) 
                  == -1)
  {
    fputs("create_and_bind_socket() failed; quitting", stderr);
    exit(-1);
  }

  while(1) {
    clientData = new CClientData;

    /* block here */
    if((clientData->socket = accept(player_sock,(struct sockaddr *) &sender,&sender_len)) == -1)
    {
      perror("accept(2) failed: ");
      exit(-1);
    }

    /* got conn */
    if(num_threads.Value() < MAXNUMTHREADS)
    {
      printf("** New client accepted on socket %d **\n", clientData->socket);
      if(pthread_create(&clientData->writeThread, NULL, client_writer, clientData))
      {
        perror("pthread_create(3) failed");
        exit(-1);
      }
      num_threads+=1;

      if(pthread_create(&clientData->readThread, NULL, client_reader, clientData))
      {
        perror("pthread_create(3) failed");
        exit(-1);
      }
      num_threads+=1;
      
      //puts("main locking clients_mutex");
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
      //puts("main unlocking clients_mutex");
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






