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
 *   c++ player client utils
 */

#include <stdio.h>
#include <netdb.h> /* for gethostbyname(3) */
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */
#include <sys/types.h>  /* for socket(2) */
#include <unistd.h> /* for close(2), fcntl(2), getpid(2) */
#include <sys/socket.h>  /* for socket(2) */
#include <fcntl.h>  /* for fcntl(2) */
#include <errno.h>
#include <string.h>
#include <stdlib.h> /* for system(3) */
#include <signal.h> /* for signal(2) */

#include <playerclient.h>

#define PORTNUM 6665

CRobot::~CRobot() 
{
  if(position) delete position;
  if(laser) delete laser;
  if(sonar) delete sonar;
  if(vision) delete vision;
  if(gripper) delete gripper;
  if(misc) delete misc;
  if(sock) close(sock);
}

CRobot::CRobot() 
{
  /* set defaults */
  signal(SIGPIPE,SIG_IGN);
  strcpy(host,"localhost");
  port = -1;  // means use default Player port

  /* set null commands */
  newturnrate = 0;
  newspeed = 0;
  newpan = 0;
  newtilt = 0;
  newzoom = 0; 
  GripperCommand( GRIPstore );

  /* set pointers to NULL */
  position = NULL;
  sonar=NULL;
  laser=NULL;
  vision=NULL;
  gripper=NULL;
  misc=NULL;
  ptz=NULL;

  /* set all access to none */
  laseraccess = NONE;
  positionaccess = NONE;
  sonaraccess = NONE;
  visionaccess = NONE;
  miscaccess = NONE;
  ptzaccess = NONE;
  gripperaccess = NONE;

  minfrontsonar = 1000;
  minbacksonar = 1000;
  minlaser = 1000;
  minlaser_index = 181;
}

void CRobot::GripperCommand( unsigned char command ) 
{
  if ( command == GRIPpress || command == LIFTcarry ) {
    puts("Gripper commands GRIPpress and LIFTcarry needs an argument.");
    puts("Use GripperCommand( unsigned char command, unsigned char value insted");
    return;
  }

  newgrip = command << 8;
}

void CRobot::GripperCommand( unsigned char command, unsigned char value ) 
{
  newgrip = (short) (command << 8) + (short) value;
}

/* 
 * Connect to the server running on host 'this.host' at port 'this.port'
 *
 * returns 0 on success, non-zero otherwise
 */
int CRobot::Connect()
{
  return(Connect(host));
}

/* 
 * Connect to the server running on host 'desthost' at default port
 *
 * returns 0 on success, non-zero otherwise
 */
int CRobot::Connect(const char* desthost)
{
  if(port == -1)
    return(Connect(desthost,PORTNUM));
  else
    return(Connect(desthost,port));
}

/* 
 * Connect to the server running on host 'desthost' at port 'port'
 *
 * returns 0 on success, non-zero otherwise
 */
int CRobot::Connect(const char* desthost, int port) 
{
  static struct sockaddr_in server;
  char host[32] = "127.0.0.1";
  struct hostent* entp;

  if(desthost)
    strcpy(host,desthost);

  /* fill in bcast structure */
  server.sin_family = PF_INET;
  /* 
   * this is okay to do, because gethostbyname(3) does no lookup if the 
   * 'host' * arg is already an IP addr
   */
  if((entp = gethostbyname(host)) == NULL)
  {
    fprintf(stderr, "CRobot::Connect() \"%s\" is unknown host\n", host);
    return(1);
  }

  memcpy(&server.sin_addr, entp->h_addr_list[0], entp->h_length);
  server.sin_port = htons(port);

  /* make our socket */
  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("CRobot::Connect():socket() failed");
    return(1);
  }

  /* 
   * hook it up
   */
  if(connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1)
  {
    perror("CRobot::Connect():connect() failed");
    return(1);
  }

  return(0);
}

/*
 * Request device access.
 *   'request' is of the form "lrsrpa" and MUST be NULL-terminated
 *
 * Returns 0 on success, non-zero otherwise
 */
int CRobot::Request( const char* request)
{
  return(Request(request,strlen(request)));
}

/*
 * Request device access.
 *   'request' is of the form "lrsrpa"
 *   'size' is the length of 'request'
 *
 * Returns 0 on success, non-zero otherwise
 */
int CRobot::Request( const char* request, int size ) 
{
  /* Manage memory */
  int response;
  unsigned char *reply;
  unsigned char packaged_request[MAX_REQUEST_SIZE];
  int packaged_size;
  unsigned short reply_size = 0;
  char isdevicerequest = 0;
  char xhost_str[64];

  reply = new unsigned char[MAX_REPLY_SIZE];
   
  /* if it is a configuration command (starts with 'x') then it's
   * already been packaged.  otherwise, prefix it with 'dr' and size */
  if(request[0] == 'x')
  {
    memcpy(packaged_request,request,size);
    packaged_size = size;
  }
  else
  {
    isdevicerequest = 1;

    /* if it's vision, then xhost the robot */
    if(strchr(request,'v') && (visionaccess == NONE))
    {
      printf("Since you want vision, i'm xhost'ing \"%s\". Hope that's okay.\n",
                      host);
      sprintf(xhost_str, "/usr/bin/X11/xhost %s", host);

      if(system(xhost_str) == -1)
      {
        puts("Hmm...system() returned -1. Bad.");
        return(1);
      }
    }

    packaged_request[0] = 'd';
    packaged_request[1] = 'r';
    *(unsigned short*)(packaged_request+2) = htons(size);
    memcpy(packaged_request+2+sizeof(unsigned short),request,size);
    packaged_size = 2+sizeof(unsigned short)+size;
  }

  /* Request sensor data */
  printf("Requesting \"%s\" ", request);
  if(write(sock, packaged_request, packaged_size) < 0) {
    perror("CRobot::Request(1)");
  }

  /* Check that the server gave us what we asked for */
  if(isdevicerequest)
  {
    while ( 1 ) 
    {
      //puts("reading header");
      if(ExactRead(reply, 1+sizeof(unsigned short)))
        return(1);

      reply_size =  ntohs(*(unsigned short *)&reply[1]);

      if (reply[0]=='r') break;

      /* throw away this information */
      //puts("reading trash");
      if(ExactRead( reply, reply_size))
        return(1);
    }

    //puts("reading reply");
    if(ExactRead(reply, reply_size))
      return(1);

    if (strncmp( request, (char *)reply, reply_size) != 0) 
    {
      printf("but got \"%s\"\n", reply);
      response=1;
    }
    else 
    {
      printf("- ok\n");
      response=0;
    } 

    UpdateAccess((char*)reply,reply_size);
  }
  else
    response = 0;

  delete reply;

  return(response);
}
    
/*
 * Write commands to all devices currently opened for writing.
 */
int CRobot::Write() 
{
  unsigned char data[20];
  int size;

  if(positionaccess == WRITE || positionaccess == ALL)
  {
    data[0]='c';
    data[1]='p';
    *(unsigned short*)&data[2]= htons( 2 * sizeof(short) );
    *(unsigned short*)(&data[2+sizeof(short)]) = htons( newspeed );
    *(unsigned short*)(&data[2+2*sizeof(short)]) = htons( newturnrate );
    size = 2+3*sizeof(short);

    if ( write(sock, data , size ) < 0 ) {
      perror("CRobot::Write");
      close(sock);
      return(-1);
    }
  }
  if(ptzaccess == WRITE || ptzaccess == ALL)
  {
    data[0]='c';
    data[1]='z';
    *(unsigned short*)&data[2]= htons( 3 * sizeof(short) );
    *(unsigned short*)(&data[2+sizeof(short)]) = htons( newpan );
    *(unsigned short*)(&data[2+2*sizeof(short)]) = htons( newtilt );
    *(unsigned short*)(&data[2+3*sizeof(short)]) = htons( newzoom );
    size = 2+4*sizeof(short);

    if ( write(sock, data , size ) < 0 ) {
      perror("CRobot::Write");
      close(sock);
      return(-1);
    }
  }
  if(gripperaccess == WRITE || gripperaccess == ALL)
  {
    data[0]='c';
    data[1]='g';
    *(unsigned short*)&data[2]= htons( 1 * sizeof(short) );
    *(unsigned short*)(&data[2+sizeof(short)]) = htons( newgrip );
    size = 2+2*sizeof(short);

    if ( write(sock, data , size ) < 0 ) {
      perror("CRobot::Write");
      close(sock);
      return(-1);
    }
  }
  return(0);
}

/*
 * Read data from all devices currently openend for reading
 *
 * Returns 0 on success; non-zero otherwise
 */
int CRobot::Read()
{
  static unsigned char buf[4096];
  unsigned char prefix[3];
  unsigned short size;
  
  for(int devicecount = CountReads();devicecount > 0;devicecount--)
  {
    /* read first byte and size */
    if(ExactRead(prefix, 3))
      return(1);

    size = ntohs( *(unsigned short *) &prefix[1] );

    // Make sure buffer is big enough
    //
    if (size > sizeof(buf))
    {
        puts("WARNING: client buffer is too small to contain packet");
        return 1;
    }

    /* now read the other data */
    if(ExactRead( buf, size))
      return(1);
    
    /* see what device it is */
    switch( prefix[0] ) 
    {
      case 'm':
        if(miscaccess != READ && miscaccess != ALL)
          puts("WARNING: received misc data without asking");
        FillMiscData(buf,size);
        break;
      case 'z':
        if(ptzaccess != READ && ptzaccess != ALL)
          puts("WARNING: received ptz data without asking");
        FillPtzData(buf,size);
        break;
      case 'g':
        if(gripperaccess != READ && gripperaccess != ALL)
          puts("WARNING: received gripper data without asking");
        FillGripperData(buf,size);
        break;
      case 'p':
        if(positionaccess != READ && positionaccess != ALL)
          puts("WARNING: received position data without asking");
        FillPositionData(buf,size);
        break;
      case 's':
        if(sonaraccess != READ && sonaraccess != ALL)
          puts("WARNING: received sonar data without asking");
        FillSonarData(buf,size);
        break;
      case 'l':
        if(laseraccess != READ && laseraccess != ALL)
          puts("WARNING: received laser data without asking");
        FillLaserData(buf,size);
        break;
      case 'v':
        if(visionaccess != READ && visionaccess != ALL)
          puts("WARNING: received vision data without asking");
        FillVisionData(buf,size);
        break;
      default:
        printf("Unknown data code \"%c\" - ignoring %u bytes\n", prefix[0],
                        size);
        printf("Ignoring:\"%s\"\n", buf);
        break;
    }
  }

  return(0);
}

int CRobot::ExactRead( unsigned char buf[], unsigned short size ) 
{
    // Read the exact number of bytes we need
    // ahoward -- ACTS packets may exceed the TCP packet length,
    // so multiple reads may be requred.
    //
    // *** WARNING - should probably check for errors on read
    //
    int numread = 0;
    while (numread < size)
        numread += read(sock, buf + numread, size - numread);
    return 0; 

  /*
  int numread;
    
  if( (numread = read(sock, buf, size)) != size ) 
  {
    if (errno != 0 ) 
      perror("CRobot::ExactRead failed - you should probably exit");
    else 
      printf("Expected to read %u bytes only %u available - you should "
                      "probably exit\n", size, numread);
    return(1);
  }
  return(0);
  */
}

void CRobot::FillPtzData(unsigned char* buf, int size)
{
  int cnt;

  if (size != PTZ_DATA_BUFFER_SIZE) {
    puts("Size of ptz data changed - ignoring");
    return;
  }

  cnt = 0;
  ptz->pan = (short) ntohs( *(unsigned short*)&buf[cnt]);
  cnt+=sizeof(unsigned short);
  ptz->tilt = (short) ntohs( *(unsigned short*)&buf[cnt]);
  cnt+=sizeof(unsigned short);
  ptz->zoom = (short) ntohs( *(unsigned short*)&buf[cnt]);
}
void CRobot::FillMiscData(unsigned char* buf, int size)
{
  int cnt;

  if (size != MISC_DATA_BUFFER_SIZE) {
    puts("Size of misc data changed - ignoring");
    return;
  }

  cnt = 0;
  misc->frontbumpers = buf[cnt++];
  misc->rearbumpers = buf[cnt++];
  misc->voltage = buf[cnt];
}

void CRobot::FillGripperData(unsigned char* buf, int size)
{
  if ( size != GRIPPER_DATA_BUFFER_SIZE) 
  {
    puts("Size of gripper data changed - ignoring");
    return;
  }

  gripper->paddlesOpen = buf[0] & 0x01;
  gripper->paddlesClose = ( buf[0] >> 1) & 0x01;
  gripper->paddlesMoving = ( buf[0] >> 2) & 0x01;
  gripper->paddlesError = ( buf[0] >> 3) & 0x01;
  gripper->liftUp = ( buf[0] >> 4) & 0x01;
  gripper->liftDown = ( buf[0] >> 5) & 0x01;
  gripper->liftMoving = ( buf[0] >> 6) & 0x01;
  gripper->liftError = ( buf[0] >> 7) & 0x01;

  gripper->gripLimitReached = buf[1] & 0x01;
  gripper->liftLimitReached = (buf[1] >> 1 ) & 0x01;
  gripper->outerBeamObstructed = (buf[1] >> 2 ) & 0x01;
  gripper->innerBeamObstructed = (buf[1] >> 3 ) & 0x01;
  gripper->leftPaddleOpen = (buf[1] >> 4 ) & 0x01;
  gripper->rightPaddleOpen = (buf[1] >> 5 ) & 0x01;
}

void CRobot::FillPositionData(unsigned char* buf, int size)
{
  int cnt;

  if ( size != POSITION_DATA_BUFFER_SIZE)
  {
    puts("Size of position data changed - ignoring");
    return;
  }

  cnt = 0;

  position->time = ntohl(*(unsigned int*)&buf[cnt]);
  cnt+=sizeof(unsigned int);

  position->xpos = (int) ntohl( *(unsigned int*)&buf[cnt]);
  cnt+=sizeof(unsigned int);

  position->ypos = (int) ntohl( *(unsigned int*)&buf[cnt]);
  cnt+=sizeof(unsigned int);

  position->heading = ntohs( *(unsigned short*)&buf[cnt]);
  cnt+=sizeof(unsigned short);

  position->speed = (short) ntohs( *(unsigned short*)&buf[cnt]);
  cnt+=sizeof(unsigned short);

  position->turnrate = (short) ntohs( *(unsigned short*)&buf[cnt]);
  cnt+=sizeof(unsigned short);

  position->compass = (short)(ntohs( *(unsigned short*)&buf[cnt]));
  cnt+=sizeof(unsigned short);

  position->stalls = buf[cnt++];
}

void CRobot::FillSonarData(unsigned char* buf, int size)
{
  int cnt;

  if ( size != SONAR_DATA_BUFFER_SIZE)
  {
    puts("Size of sonar data changed - ignoring");

    printf( "size is %d, expecting %d\n", size, SONAR_DATA_BUFFER_SIZE );
    return;
  }

  minfrontsonar = 5000;
  minbacksonar = 5000;
  for( cnt=0 ; cnt< 32 ; cnt+=2 ) 
  {
    sonar[cnt/2] = ntohs(*(unsigned short*)&buf[cnt]);

    /* figure out minimum sonar values */
    if(cnt/2 > 1 && cnt/2 < 6 && sonar[cnt/2] < minfrontsonar)
      minfrontsonar = sonar[cnt/2];
    else if(cnt/2 > 9 && cnt/2 < 14 && sonar[cnt/2] < minbacksonar)
      minbacksonar = sonar[cnt/2];
  }
}

void CRobot::FillLaserData(unsigned char* buf, int size)
{
  int cnt;
  //bool overflow = false;

  if ( size != LASER_DATA_BUFFER_SIZE) 
  {
    puts("Size of laser data changed - ignoring");
    return;
  }

  minlaser = 8000;
  for( cnt=0; cnt<size;cnt+=2) 
  {
    laser[cnt/2] = ntohs(*(unsigned short*)&buf[cnt]);
    if(laser[cnt/2] < minlaser)
    {
      minlaser = laser[cnt/2];
      minlaser_index = cnt/2;
    }
  }
}

void CRobot::FillVisionData(unsigned char* buf, int size)
{
  int i,j,k;
  int area;
  int bufptr;

  bufptr = ACTS_HEADER_SIZE;
  for(i=0;i<ACTS_NUM_CHANNELS;i++)
  {
    //printf("%d blobs starting at %d on %d\n", buf[2*i+1]-1,buf[2*i]-1,i+1);
    if(!(buf[2*i+1]-1))
    {
      vision->NumBlobs[i] = 0;
    }
    else
    {
      /* check to see if we need more room */
      if(buf[2*i+1]-1 > vision->NumBlobs[i])
      {
        delete vision->Blobs[i];
        vision->Blobs[i] = new CBlob[buf[2*i+1]-1];
      }
      for(j=0;j<buf[2*i+1]-1;j++)
      {
        /* first compute the area */
        area=0;
        for(k=0;k<4;k++)
        {
          area = area << 6;
          area |= buf[bufptr + k] - 1;
        }
        vision->Blobs[i][j].area = area;
        vision->Blobs[i][j].x = buf[bufptr + 4] - 1;
        vision->Blobs[i][j].y = buf[bufptr + 5] - 1;
        vision->Blobs[i][j].left = buf[bufptr + 6] - 1;
        vision->Blobs[i][j].right = buf[bufptr + 7] - 1;
        vision->Blobs[i][j].top = buf[bufptr + 8] - 1;
        vision->Blobs[i][j].bottom = buf[bufptr + 9] - 1;

        bufptr += ACTS_BLOB_SIZE;
      }
      vision->NumBlobs[i] = buf[2*i+1]-1;
    }
  }
}

/*
 * Print data from all devices currently opened for reading
 */
void CRobot::Print() 
{
  int i,j;

  if (miscaccess == READ || miscaccess == ALL) 
  {
    printf("BatteryVoltage: %f Bumpers: front: ",
	   (float)misc->voltage / 10.0);
    for(i=0;i<5;i++) {
      printf("%d", (misc->frontbumpers >> i ) & 0x01);
    }
    
    printf(" rear: ");

    for(i=0;i<5;i++) {
      printf("%d", (misc->rearbumpers >> i ) & 0x01);
    }
    puts("");
  }
  if (ptzaccess == READ || ptzaccess == ALL) 
  {
    printf("Pan: %d Tilt: %d Zoom: %d\n",
	   ptz->pan, ptz->tilt, ptz->zoom);
  }
  if (positionaccess == READ || positionaccess == ALL) 
  {
    printf("Time: %u Pos: (%d,%d) Heading: %hu Speed: %d Turnrate: %d\n"
           "Compass: %d Stall: %d\n",
	   position->time, 
           position->xpos, 
           position->ypos, 
           position->heading, 
	   position->speed, 
           position->turnrate, 
           position->compass,
	   position->stalls);
  }

  if (gripperaccess == READ || gripperaccess == ALL) 
  {
    printf("Gripper: ");

    /* paddles */
    if (gripper->paddlesMoving)
      printf("Paddles (moving): ");
    else
      printf("Paddles: ");

    if (gripper->paddlesClose)  
      printf("closed ");
    else if (gripper->paddlesOpen) 
      printf("open ");
    else 
      printf("unknown ");

    if (gripper->leftPaddleOpen) 
      printf("left opened ");
    else
      printf("left closed ");

    if (gripper->rightPaddleOpen) 
      printf("right opened ");
    else
      printf("right closed ");

    if ( !gripper->gripLimitReached ) printf("limit reached ");

    if (gripper->paddlesError) printf("paddles error ");

    /* lift */
    if (gripper->liftMoving)
      printf("Lift (moving): ");
    else
      printf("Lift: ");

    if (gripper->liftUp)  
      printf("up ");
    else if (gripper->liftDown) 
      printf("down ");
    else 
      printf("unknown ");

    if ( !gripper->gripLimitReached ) printf("limit reached ");

    if (gripper->liftError) printf("lift error ");

    /* beams */
    printf("beams: ");

    if ( gripper->outerBeamObstructed ) 
      printf("1");
    else
      printf("0");

    if ( gripper->innerBeamObstructed )
       printf("1");
    else
      printf("0");
   

    puts("");
  }

  if (sonaraccess == READ || sonaraccess == ALL) 
  {
    printf("Sonars: ");
    for(int i=0;i<16;i++) printf("%hu ", sonar[i]);
    puts("");
    }

  if (laser) {
    printf("Laser: ");
    for(int i=0;i<361;i++) printf("%hu ", laser[i]);
    puts("");
  }

  if (visionaccess == READ || visionaccess == ALL) 
  {
    printf("Vision: ");
    for(i=0;i<ACTS_NUM_CHANNELS;i++)
    {
      if(vision->NumBlobs[i])
      {
        printf("Channel %d:\n", i);
        for(j=0;j<vision->NumBlobs[i];j++)
        {
          printf("  blob %d:\n"
                 "             area: %d\n"
                 "                X: %d\n"
                 "                Y: %d\n"
                 "             Left: %d\n"
                 "            Right: %d\n"
                 "              Top: %d\n"
                 "           Bottom: %d\n",
                 j+1,
                 vision->Blobs[i][j].area,
                 vision->Blobs[i][j].x,
                 vision->Blobs[i][j].y,
                 vision->Blobs[i][j].left,
                 vision->Blobs[i][j].right,
                 vision->Blobs[i][j].top,
                 vision->Blobs[i][j].bottom);
        }
      }
    }
    puts("");
  }
}

/*
 * change velocity control mode
 *   'mode' should be either DIRECTWHEELVELOCITY or SEPARATETRANSROT
 *
 * Returns 0 on success; non-zero otherwise
 */
int CRobot::ChangeVelocityControl(velocity_mode_t mode)
{
  unsigned char req[2+sizeof(unsigned short)+sizeof(unsigned char)];
  req[0] = 'x';
  req[1] = 'v';
  *(unsigned short*)&req[2] = htons(sizeof(unsigned char));
  if(mode == DIRECTWHEELVELOCITY)
    req[2+sizeof(unsigned short)] = 0;
  else if(mode == SEPARATETRANSROT)
    req[2+sizeof(unsigned short)] = 1;
  else
  {
    puts("CRobot::ChangeVelocityControl(): received unknown velocity mode "
                    "as argument");
    return(false);
  }
  return(Request((char*)req,sizeof(req)));
}

/*
 * Enable/disable the motors
 *   if 'state' is non-zero, then enable motors
 *   else disable motors
 *
 * Returns 0 on success; non-zero otherwise
 */
int CRobot::ChangeMotorState(unsigned char state)
{
  unsigned char req[4+sizeof(unsigned short)];
  req[0] = 'x';
  req[1] = 'p';
  *(unsigned short*)&req[2] = htons(2*sizeof(unsigned char));
  req[2+sizeof(unsigned short)] = 'm';
  req[2+sizeof(unsigned short)+sizeof(char)] = state;
  return(Request((char*)req,sizeof(req)));
}

/*
 * Set continous data frequency
 *
 * Returns 0 on success; non-zero otherwise
 */
int CRobot::SetFrequency(unsigned short freq)
{
  unsigned char req[3+sizeof(unsigned short)+sizeof(unsigned short)];
  req[0] = 'x';
  req[1] = 'y';
  *(unsigned short*)&req[2] = htons(1+sizeof(unsigned short));
  req[2+sizeof(unsigned short)] = 'f';
  *(unsigned short*)&req[3+sizeof(unsigned short)] = htons(freq);
  return(Request((char*)req,sizeof(req)));
}

/*
 * Set data delivery mode to 'mode'
 *   'mode' should be either REQUESTREPLY or CONTINUOUS
 *
 * Returns 0 on success; non-zero otherwise
 */
int CRobot::SetDataMode(data_mode_t mode)
{
  unsigned char req[4+sizeof(unsigned short)];
  req[0] = 'x';
  req[1] = 'y';
  *(unsigned short*)&req[2] = htons(2*sizeof(unsigned char));
  req[2+sizeof(unsigned short)] = 'r';

  if(mode == CONTINUOUS)
    req[3+sizeof(unsigned short)] = 0;
  else if(mode == REQUESTREPLY)
    req[3+sizeof(unsigned short)] = 1;
  else
  {
    puts("CRobot::SetDataMode(): received unknown data mode as argument");
    return(false);
  }
  return(Request((char*)req,sizeof(req)));
}

/*
 * Reset the robot's odometry to 0,0,0
 *
 * Returns 0 on success; non-zero otherwise
 */
int CRobot::ResetPosition()
{
  unsigned char req[3+sizeof(unsigned short)];
  req[0] = 'x';
  req[1] = 'p';
  *(unsigned short*)&req[2] = htons(1);
  req[2+sizeof(unsigned short)] = 'R';

  return(Request((char*)req,sizeof(req)));
}


/*
 * Set the laser configuration
 *
 * Returns 0 on success; non-zero otherwise
 */
int CRobot::SetLaserConfig(int min_segment, int max_segment, bool intensity)
{
  unsigned char req[4 + 2 * 2 + 1];
  req[0] = 'x';
  req[1] = 'l';
  *((unsigned short*) &req[2]) = htons(2 * 2 + 1);
  *((unsigned short*) &req[4]) = htons(min_segment);
  *((unsigned short*) &req[6]) = htons(max_segment);
  req[8] = (unsigned char) intensity;

  return(Request((char*) req, sizeof(req)));   
}


/*
 * When in REQUESTREPLY mode, request a round of sensor data
 *
 * Returns 0 on success; non-zero otherwise
 */
int CRobot::RequestData()
{
  unsigned char req[3+sizeof(unsigned short)];
  req[0] = 'x';
  req[1] = 'y';
  *(unsigned short*)&req[2] = htons(1);
  req[2+sizeof(unsigned short)] = 'd';
  return(Request((char*)req,sizeof(req)));
}

void CRobot::UpdateAccess(char* reply, unsigned short reply_size)
{
  //printf("UpdateAccess:%s:\n", reply);
  /* update our current device status */
  for ( int i=0;i<reply_size;i+=2) 
  {
    switch(reply[i]) 
    {
      case 'l':
        if(reply[i+1] == 'c' || reply[i+1] == 'e')
        {
          laseraccess = NONE;
          if(laser)
          {
            delete laser;
            laser = NULL;
          }
        }
        else
        {
          if(!laser)
          {
            laser = new unsigned short[361];
            for(int i=0;i<361;i++)
              laser[i] = 1000;
          }
          switch(reply[i+1])
          {
            case 'r':
              laseraccess = READ;
              break;
            case 'w':
              laseraccess = WRITE;
              break;
            case 'a':
              laseraccess = ALL;
              break;
            default:
              printf("CRobot::UpdateAccess() received unknown access type \"%c\"\n",
                              reply[i+1]);
          }
        }
        break;
      case 's':
        if(reply[i+1] == 'c' || reply[i+1] == 'e')
        {
          sonaraccess = NONE;
          if(sonar)
          {
            delete sonar;
            sonar = NULL;
          }
        }
        else
        {
          if(!sonar)
          {
            sonar = new unsigned short[16];
            for(int i=0;i<16;i++)
              sonar[i] = 1000;
          }
          switch(reply[i+1])
          {
            case 'r':
              sonaraccess = READ;
              break;
            case 'w':
              sonaraccess = WRITE;
              break;
            case 'a':
              sonaraccess = ALL;
              break;
            default:
              printf("CRobot::UpdateAccess() received unknown access type \"%c\"\n",
                              reply[i+1]);
          }
        }
        break;
      case 'p':
        if(reply[i+1] == 'c' || reply[i+1] == 'e')
        {
          positionaccess = NONE;
          if(position)
          {
            delete position;
            position = NULL;
          }
        }
        else
        {
          if(!position)
            position = new CPosition;
          switch(reply[i+1])
          {
            case 'r':
              positionaccess = READ;
              break;
            case 'w':
              positionaccess = WRITE;
              break;
            case 'a':
              positionaccess = ALL;
              break;
            default:
              printf("CRobot::UpdateAccess() received unknown access type \"%c\"\n",
                              reply[i+1]);
          }
        }
        break;
      case 'g':
        if(reply[i+1] == 'c' || reply[i+1] == 'e')
        {
          gripperaccess = NONE;
          if(gripper)
          {
            delete gripper;
            gripper = NULL;
          }
        }
        else
        {
          if(!gripper)
            gripper = new CGripper;
          switch(reply[i+1])
          {
            case 'r':
              gripperaccess = READ;
              break;
            case 'w':
              gripperaccess = WRITE;
              break;
            case 'a':
              gripperaccess = ALL;
              break;
            default:
              printf("CRobot::UpdateAccess() received unknown access type \"%c\"\n",
                              reply[i+1]);
          }
        }
        break;
      case 'm':
        if(reply[i+1] == 'c' || reply[i+1] == 'e')
        {
          miscaccess = NONE;
          if(misc)
          {
            delete misc;
            misc = NULL;
          }
        }
        else
        {
          if(!misc)
            misc = new CMisc;
          switch(reply[i+1])
          {
            case 'r':
              miscaccess = READ;
              break;
            case 'w':
              miscaccess = WRITE;
              break;
            case 'a':
              miscaccess = ALL;
              break;
            default:
              printf("CRobot::UpdateAccess() received unknown access type \"%c\"\n",
                              reply[i+1]);
          }
        }
        break;
      case 'z':
        if(reply[i+1] == 'c' || reply[i+1] == 'e')
        {
          ptzaccess = NONE;
          if(ptz)
          {
            delete ptz;
            ptz = NULL;
          }
        }
        else
        {
          if(!ptz)
            ptz = new CPtz;
          switch(reply[i+1])
          {
            case 'r':
              ptzaccess = READ;
              break;
            case 'w':
              ptzaccess = WRITE;
              break;
            case 'a':
              ptzaccess = ALL;
              break;
            default:
              printf("CRobot::UpdateAccess() received unknown access type \"%c\"\n",
                              reply[i+1]);
          }
        }
        break;
      case 'v':
        if(reply[i+1] == 'c' || reply[i+1] == 'e')
        {
          visionaccess = NONE;
          if(vision)
          {
            delete vision;
            vision = NULL;
          }
        }
        else
        {
          if(!vision)
          {
            vision = new CVision;
            bzero(vision->NumBlobs,sizeof(vision->NumBlobs));
          }
          switch(reply[i+1])
          {
            case 'r':
              visionaccess = READ;
              break;
            case 'w':
              visionaccess = WRITE;
              break;
            case 'a':
              visionaccess = ALL;
              break;
            default:
              printf("CRobot::UpdateAccess() received unknown access type \"%c\"\n",
                              reply[i+1]);
          }
        }
        break;
      default:
        printf("CRobot::UpdateAccess() received unknown device type \"%c\"\n",
                        reply[i]);
    }
  }
}

int CRobot::CountReads()
{
  int count = 0;
  if(laseraccess == READ || laseraccess == ALL)
    count++;
  if(sonaraccess == READ || sonaraccess == ALL)
    count++;
  if(positionaccess == READ || positionaccess == ALL)
    count++;
  if(visionaccess == READ || visionaccess == ALL)
    count++;
  if(miscaccess == READ || miscaccess == ALL)
    count++;
  if(gripperaccess == READ || gripperaccess == ALL)
    count++;
  if(ptzaccess == READ || ptzaccess == ALL)
    count++;

  return(count);
}
