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
 *   the P2 vision device.  it takes pan tilt zoom commands for the
 *   sony PTZ camera (if equipped), and returns color blob data gathered
 *   from ACTS, which this device spawns and then talks to.
 */

#include <visiondevice.h>

#include <stdio.h>
#include <unistd.h> /* close(2),fcntl(2),getpid(2),usleep(3),execvp(3),fork(2)*/
#include <netdb.h> /* for gethostbyname(3) */
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */
#include <sys/types.h>  /* for socket(2) */
#include <sys/socket.h>  /* for socket(2) */
#include <signal.h>  /* for kill(2) */
#include <fcntl.h>  /* for fcntl(2) */
#include <string.h>  /* for strncpy(3),memcpy(3) */
#include <stdlib.h>  /* for atexit(3),atoi(3) */
#include <pthread.h>  /* for pthread stuff */
#include <signal.h>  /* for sigblock */
#include <pubsub_util.h>

#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

#define ACTS_REQUEST_QUIT '1'
#define ACTS_REQUEST_PACKET '0'

/* the following setting mean that we first try to connect after 1 seconds,
 * then try every 100ms for 6 more seconds before giving up */
#define ACTS_STARTUP_USEC 1000000 /* wait before first connection attempt */
#define ACTS_STARTUP_INTERVAL_USEC 100000 /* wait between connection attempts */
#define ACTS_STARTUP_CONN_LIMIT 60 /* number of attempts to make */


void* RunVisionThread(void* visiondevice);
void QuitACTS(void* visiondevice);


CVisionDevice::CVisionDevice(int argc, char** argv)
  : CDevice(sizeof(player_internal_vision_data_t),0,1,1)
{
  char tmpstr[MAX_FILENAME_SIZE];
  sock = -1;
  //data = new player_internal_vision_data_t;

  strncpy(configfilepath,DEFAULT_ACTS_CONFIGFILE,sizeof(configfilepath));
  strncpy(binarypath,DEFAULT_ACTS_PATH,sizeof(binarypath));
  acts_version = DEFAULT_ACTS_VERSION;
  portnum=DEFAULT_ACTS_PORT;

  for(int i=0;i<argc;i++)
  {
    if(!strcmp(argv[i],"port"))
    {
      if(++i<argc)
        portnum = atoi(argv[i]);
      else
        fprintf(stderr, "CVisionDevice: missing port; using default: %d\n",
                portnum);
    }
    else if(!strcmp(argv[i],"configfile"))
    {
      if(++i<argc)
      {
        strncpy(configfilepath,argv[i],sizeof(configfilepath));
        configfilepath[sizeof(configfilepath)-1] = '\0';
      }
      else
        fprintf(stderr, "CVisionDevice: missing configfile; "
                "using default: \"%s\"\n", configfilepath);
    }
    else if(!strcmp(argv[i],"path"))
    {
      if(++i<argc)
      {
        strncpy(binarypath,argv[i],sizeof(binarypath));
        binarypath[sizeof(binarypath)-1] = '\0';
      }
      else
        fputs("CVisionDevice: missing path to executable; "
                "will look for 'acts' in your PATH.\n",stderr);
    }
    else if(!strcmp(argv[i],"version"))
    {
      version_enum_to_string(acts_version,tmpstr,sizeof(tmpstr));

      if(++i<argc)
      {
        if(version_string_to_enum(argv[i]) == ACTS_VERSION_UNKNOWN)
          fprintf(stderr, "CVisionDevice: unknown ACTS version given; "
                          "using default: \"%s\"\n", tmpstr);
        else
          acts_version = version_string_to_enum(argv[i]);
      }
      else
        fprintf(stderr, "CVisionDevice: missing version string; "
                "using default: \"%s\"\n", tmpstr);
    }
    else
      fprintf(stderr, "CVisionDevice: ignoring unknown parameter \"%s\"\n",
              argv[i]);
  }

  // set up some version-specific parameters
  switch(acts_version)
  {
    case ACTS_VERSION_1_0:
      header_len = ACTS_HEADER_SIZE_1_0;
      blob_size = ACTS_BLOB_SIZE_1_0;
      portnum = htons(portnum);
      break;
    case ACTS_VERSION_1_2:
    default:
      header_len = ACTS_HEADER_SIZE_1_2;
      blob_size = ACTS_BLOB_SIZE_1_2;
      break;
  }
  header_elt_len = header_len / VISION_NUM_CHANNELS;
}
    
// returns the enum representation of the given version string, or
// -1 on failure to match.
acts_version_t CVisionDevice::version_string_to_enum(char* versionstr)
{
  if(!strcmp(versionstr,ACTS_VERSION_1_0_STRING))
    return(ACTS_VERSION_1_0);
  else if(!strcmp(versionstr,ACTS_VERSION_1_2_STRING))
    return(ACTS_VERSION_1_2);
  else
    return(ACTS_VERSION_UNKNOWN);
}

// writes the string representation of the given version number into
// versionstr, up to len.
// returns  0 on success
//         -1 on failure to match.
int CVisionDevice::version_enum_to_string(acts_version_t versionnum, 
                                          char* versionstr, int len)
{
  switch(versionnum)
  {
    case ACTS_VERSION_1_0:
      strncpy(versionstr, ACTS_VERSION_1_0_STRING, len);
      return(0);
    case ACTS_VERSION_1_2:
      strncpy(versionstr, ACTS_VERSION_1_2_STRING, len);
      return(0);
    default:
      return(-1);
  }
}

CVisionDevice::~CVisionDevice()
{
  Shutdown();
  if(sock != -1)
    QuitACTS(this);
}

int
CVisionDevice::Setup()
{
  int i = 0;
  int j;

  char acts_bin_name[] = "acts";
  char acts_configfile_flag[] = "-t";
  char acts_port_flag[] = "-s";

  char acts_port_num[MAX_FILENAME_SIZE];
  char* acts_args[8];

  static struct sockaddr_in server;
  char host[] = "localhost";
  struct hostent* entp;

  printf("ACTS vision server connection initializing (%s,%d)...",
         configfilepath,portnum);
  fflush(stdout);

  sprintf(acts_port_num,"%d",portnum);

  acts_args[i++] = acts_bin_name;
  if(strlen(configfilepath))
  {
    acts_args[i++] = acts_configfile_flag;
    acts_args[i++] = configfilepath;
  }
  acts_args[i++] = acts_port_flag;
  acts_args[i++] = acts_port_num;
  acts_args[i] = (char*)NULL;

  if(!(pid = fork()))
  {
    // make sure we don't get that "ACTS: Packet" bullshit on the console
    int dummy_fd = open("/dev/null",O_RDWR);
    dup2(dummy_fd,0);
    dup2(dummy_fd,1);
    dup2(dummy_fd,2);

    /* detach from controlling tty, so we don't get pesky SIGINTs and such */
    if(setpgrp() == -1)
    {
      perror("CVisionDevice:Setup(): error while setpgrp()");
      exit(1);
    }

    // if no path to the binary was given, search the user's PATH
    if(!strlen(binarypath))
    {
      if(execvp(acts_bin_name,acts_args) == -1)
      {
        /* 
        * some error.  print it here.  it will really be detected
        * later when the parent tries to connect(2) to it
         */
        perror("CVisionDevice:Setup(): error while execvp()ing ACTS");
        exit(1);
      }
    }
    else
    {
      if(execv(binarypath,acts_args) == -1)
      {
        /* 
        * some error.  print it here.  it will really be detected
        * later when the parent tries to connect(2) to it
         */
        perror("CVisionDevice:Setup(): error while execv()ing ACTS");
        exit(1);
      }
    }
  }
  else
  {
    /* in parent */
    /* fill in addr structure */
    server.sin_family = PF_INET;
    /* 
     * this is okay to do, because gethostbyname(3) does no lookup if the 
     * 'host' * arg is already an IP addr
     */
    if((entp = gethostbyname(host)) == NULL)
    {
      fprintf(stderr, "CVisionDevice::Setup(): \"%s\" is unknown host; "
                      "can't connect to ACTS\n", host);
      /* try to kill ACTS just in case it's running */
      KillACTS();
      return(1);
    }

    memcpy(&server.sin_addr, entp->h_addr_list[0], entp->h_length);


    server.sin_port = htons(portnum);

    /* ok, we'll make this a bit smarter.  first, we wait a baseline amount
     * of time, then try to connect periodically for some predefined number
     * of times
     */
    usleep(ACTS_STARTUP_USEC);

    for(j = 0;j<ACTS_STARTUP_CONN_LIMIT;j++)
    {
      /* 
       * hook it up
       */
      
      // make a new socket, because connect() screws with the old one somehow
      if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
      {
        perror("CVisionDevice::Setup(): socket(2) failed");
        KillACTS();
        return(1);
      }
      if(connect(sock,(struct sockaddr*)&server, sizeof(server)) == 0)
        break;
      usleep(ACTS_STARTUP_INTERVAL_USEC);
    }
    if(j == ACTS_STARTUP_CONN_LIMIT)
    {
      perror("CVisionDevice::Setup(): connect(2) failed");
      KillACTS();
      return(1);
    }
    puts("Done.");

    /* now spawn reading thread */
    if(pthread_create(&thread, NULL, &RunVisionThread, this))
    {
      fputs("CVisionDevice::Setup(): pthread creation messed up\n",stderr);
      KillACTS();
      return(1);
    }

    return(0);
  }

  // shut up compiler!
  return(0);
}

int
CVisionDevice::Shutdown()
{
  void* dummy;
  /* if Setup() was never called, don't do anything */
  if(sock == -1)
    return(0);

  pthread_cancel(thread);
  if(pthread_join(thread,&dummy))
    perror("CVisionDevice::Shutdown::pthread_join()");
  // just to be sure...
  //KillACTS();

  sock = -1;
  puts("ACTS vision server has been shutdown");
  return(0);
}

void 
CVisionDevice::KillACTS()
{
  if(kill(pid,SIGKILL) == -1)
    perror("CVisionDevice::KillACTS(): some error while killing ACTS");
}

size_t
CVisionDevice::GetData(unsigned char* dest, size_t maxsize,
                       uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  int size;
  Lock();
  /* size is stored at first two bytes */
  *((player_vision_data_t*)dest) = 
          ((player_internal_vision_data_t*)device_data)->data;
  size=((player_internal_vision_data_t*)device_data)->size;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();
  return(size);
}

void* 
RunVisionThread(void* visiondevice)
{
  int numread;
  int num_blobs;
  int i;

  CVisionDevice* vd = (CVisionDevice*)visiondevice;

  // we'll transform the data into this structured buffer
  player_internal_vision_data_t local_data;
  
  // first, we'll read into these two temporary buffers
  uint8_t acts_hdr_buf[sizeof(local_data.data.header)];
  uint8_t acts_blob_buf[sizeof(local_data.data.blobs)];

  char acts_request_packet = ACTS_REQUEST_PACKET;


  /* make sure we aren't canceled at a bad time */
  if(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL))
  {
    fputs("RunVisionThread: pthread_setcanceltype() failed. exiting\n",
                    stderr);
    pthread_exit(NULL);
  }

#ifdef PLAYER_LINUX
  sigblock(SIGINT);
  sigblock(SIGALRM);
#endif

  /* make sure we kill ACTS on exiting */
  pthread_cleanup_push(QuitACTS,vd);

  /* loop and read */
  for(;;)
  {
    /* test if we are supposed to cancel */
    pthread_testcancel();

    /* request a packet from ACTS */
    if(write(vd->sock,&acts_request_packet,sizeof(acts_request_packet)) == -1)
    {
      perror("RunVisionThread: write() failed sending ACTS_REQUEST_PACKET;"
                      "exiting.");
      break;
    }

    /* get the header first */
    if((numread = read(vd->sock,acts_hdr_buf,vd->header_len)) == -1)
    {
      perror("RunVisionThread: read() failed for header: exiting");
      break;
    }
    else if(numread != vd->header_len)
    {
      fprintf(stderr,"RunVisionThread: something went wrong\n"
             "              expected %d bytes of header, but only got %d\n", 
             vd->header_len,numread);
      break;
    }

    /* convert the header, if necessary */
    if(vd->acts_version == ACTS_VERSION_1_0)
    {
      for(i=0;i<VISION_NUM_CHANNELS;i++)
      {
        // convert 2-byte ACTS 1.0 encoded entries to byte-swapped integers
        // in a structured array
        local_data.data.header[i].index = 
                htons(acts_hdr_buf[vd->header_elt_len*i]-1);
        local_data.data.header[i].num = 
                htons(acts_hdr_buf[vd->header_elt_len*i+1]-1);
      }
    }
    else
    {
      for(i=0;i<VISION_NUM_CHANNELS;i++)
      {
        // convert 4-byte ACTS 1.2 encoded entries to byte-swapped integers
        // in a structured array
        local_data.data.header[i].index = acts_hdr_buf[vd->header_elt_len*i]-1;
        local_data.data.header[i].index = 
                local_data.data.header[i].index << 6;
        local_data.data.header[i].index |= 
                acts_hdr_buf[vd->header_elt_len*i+1]-1;
        local_data.data.header[i].index = 
                htons(local_data.data.header[i].index);

        local_data.data.header[i].num = acts_hdr_buf[vd->header_elt_len*i+2]-1;
        local_data.data.header[i].num = 
                local_data.data.header[i].num << 6;
        local_data.data.header[i].num |= 
                acts_hdr_buf[vd->header_elt_len*i+3]-1;
        local_data.data.header[i].num = 
                htons(local_data.data.header[i].num);
      }
    }

    /* sum up the data we expect */
    num_blobs=0;
    for(i=0;i<VISION_NUM_CHANNELS;i++)
      num_blobs += ntohs(local_data.data.header[i].num);

    /* read in the blob data */
    if((numread = read(vd->sock,acts_blob_buf,num_blobs*vd->blob_size)) == -1)
    {
      perror("RunVisionThread: read() failed on blob data; exiting.");
      break;
    }
    else if(numread != num_blobs*vd->blob_size)
    {
      fprintf(stderr,"RunVisionThread: something went wrong\n"
             "              expected %d bytes of blob data, but only got %d\n", 
             num_blobs*vd->blob_size,numread);
      break;
    }

    if(vd->acts_version == ACTS_VERSION_1_0)
    {
      // convert 10-byte ACTS 1.0 blobs to new byte-swapped structured array
      for(i=0;i<VISION_NUM_CHANNELS;i++)
      {
        int tmpptr = vd->blob_size*i;
        // get the 4-byte area first
        local_data.data.blobs[i].area = 0;
        for(int j=0;j<4;j++)
        {
          local_data.data.blobs[i].area = 
                  local_data.data.blobs[i].area << 6;
          local_data.data.blobs[i].area |= acts_blob_buf[tmpptr++] - 1;
        }
        local_data.data.blobs[i].area = 
                htonl(local_data.data.blobs[i].area);

        // convert the other 6 one-byte entries to byte-swapped shorts
        local_data.data.blobs[i].x = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].x = 
                htons(local_data.data.blobs[i].x);

        local_data.data.blobs[i].y = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].y = 
                htons(local_data.data.blobs[i].y);

        local_data.data.blobs[i].left = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].left = 
                htons(local_data.data.blobs[i].left);

        local_data.data.blobs[i].right = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].right = 
                htons(local_data.data.blobs[i].right);

        local_data.data.blobs[i].top = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].top = 
                htons(local_data.data.blobs[i].top);

        local_data.data.blobs[i].bottom = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].bottom = 
                htons(local_data.data.blobs[i].bottom);
      }
    }
    else
    {
      // convert 16-byte ACTS 1.2 blobs to new byte-swapped structured array
      for(i=0;i<VISION_NUM_CHANNELS;i++)
      {
        int tmpptr = vd->blob_size*i;
        
        // get the 4-byte area first
        local_data.data.blobs[i].area = 0;
        for(int j=0;j<4;j++)
        {
          local_data.data.blobs[i].area = 
                  local_data.data.blobs[i].area << 6;
          local_data.data.blobs[i].area |= acts_blob_buf[tmpptr++] - 1;
        }
        local_data.data.blobs[i].area = 
                htonl(local_data.data.blobs[i].area);
        
        // convert the other 6 two-byte entries to byte-swapped shorts
        local_data.data.blobs[i].x = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].x = local_data.data.blobs[i].x << 6;
        local_data.data.blobs[i].x |= acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].x = 
                htons(local_data.data.blobs[i].x);

        local_data.data.blobs[i].y = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].y = local_data.data.blobs[i].y << 6;
        local_data.data.blobs[i].y |= acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].y = 
                htons(local_data.data.blobs[i].y);

        local_data.data.blobs[i].left = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].left = 
                local_data.data.blobs[i].left << 6;
        local_data.data.blobs[i].left |= acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].left = 
                htons(local_data.data.blobs[i].left);

        local_data.data.blobs[i].right = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].right = 
                local_data.data.blobs[i].right << 6;
        local_data.data.blobs[i].right |= acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].right = 
                htons(local_data.data.blobs[i].right);

        local_data.data.blobs[i].top = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].top = 
                local_data.data.blobs[i].top << 6;
        local_data.data.blobs[i].top |= acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].top = 
                htons(local_data.data.blobs[i].top);

        local_data.data.blobs[i].bottom = acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].bottom = 
                local_data.data.blobs[i].bottom << 6;
        local_data.data.blobs[i].bottom |= acts_blob_buf[tmpptr++] - 1;
        local_data.data.blobs[i].bottom = 
                htons(local_data.data.blobs[i].bottom);
      }
    }
    
    /* don't byte-swap anything; the ACTS encoding is sufficient */

    /* store total size */
    local_data.size = (uint16_t)(VISION_HEADER_SIZE + 
                                 num_blobs*VISION_BLOB_SIZE);

    /* test if we are supposed to cancel */
    pthread_testcancel();

    /* got the data. now fill it in */
    vd->PutData((unsigned char*)&local_data, sizeof(local_data),0,0);
  }

  pthread_cleanup_pop(1);
  pthread_exit(NULL);

  // shut up, compiler
  return((void*)NULL);
}

void 
QuitACTS(void* visiondevice)
{
  char acts_request_quit = ACTS_REQUEST_QUIT;
  CVisionDevice* vd = (CVisionDevice*)visiondevice;


  if((fcntl(vd->sock, F_SETFL, O_NONBLOCK) == -1) ||
     (write(vd->sock,&acts_request_quit,sizeof(acts_request_quit)) == -1))
  {
    perror("CVisionDevice::QuitACTS(): WARNING: either fcntl() or write() "
            "failed while sending QUIT command; killing ACTS by hand");
    vd->KillACTS();
  }
  vd->sock = -1;
}
