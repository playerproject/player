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

#include <assert.h>
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
#include <socket_util.h>

#include <device.h>
#include <messages.h>

// note: acts_version_t is declared in defaults.h

#define ACTS_VERSION_1_0_STRING "1.0"
#define ACTS_VERSION_1_2_STRING "1.2"

class Acts:public CDevice 
{
  private:
    int debuglevel;             // debuglevel 0=none, 1=basic, 2=everything
    int pid;      // ACTS's pid so we can kill it later

    
    // returns the enum representation of the given version string, or
    // ACTS_VERSION_UNKNOWN on failure to match.
    acts_version_t version_string_to_enum(char* versionstr);
    
    // writes the string representation of the given version number into
    // versionstr, up to len.
    // returns  0 on success
    //         -1 on failure to match.
    int version_enum_to_string(acts_version_t versionnum, char* versionstr, 
                               int len);

    int portnum;  // port number where we'll connect to ACTS
    char configfilepath[MAX_FILENAME_SIZE];  // path to configfile
    char binarypath[MAX_FILENAME_SIZE];  // path to executable
    acts_version_t acts_version;  // the ACTS version, as an enum
    int header_len; // length of incoming packet header (varies by version)
    int header_elt_len; // length of each header element (varies by version)
    int blob_size;  // size of each incoming blob (varies by version)
    int width, height;  // the image dimensions.

  public:
    int sock;               // socket to ACTS

    // constructor 
    //
    //    takes argc,argv from command-line args
    //
    Acts(int argc, char** argv);

    virtual void Main();

    void KillACTS();

    int Setup();
    int Shutdown();
};

// a factory creation function
CDevice* Acts_Init(int argc, char** argv)
{
  return((CDevice*)(new Acts(argc,argv)));
}

#define ACTS_REQUEST_QUIT '1'
#define ACTS_REQUEST_PACKET '0'

/* the following setting mean that we first try to connect after 1 seconds,
 * then try every 100ms for 6 more seconds before giving up */
#define ACTS_STARTUP_USEC 1000000 /* wait before first connection attempt */
#define ACTS_STARTUP_INTERVAL_USEC 100000 /* wait between connection attempts */
#define ACTS_STARTUP_CONN_LIMIT 60 /* number of attempts to make */


void QuitACTS(void* visiondevice);


Acts::Acts(int argc, char** argv)
  : CDevice(sizeof(player_vision_data_t),0,0,0)
{
  char tmpstr[MAX_FILENAME_SIZE];
  sock = -1;

  strncpy(configfilepath,DEFAULT_ACTS_CONFIGFILE,sizeof(configfilepath));
  strncpy(binarypath,DEFAULT_ACTS_PATH,sizeof(binarypath));
  acts_version = DEFAULT_ACTS_VERSION;
  portnum=DEFAULT_ACTS_PORT;

  this->width = 160;
  this->height = 120;

  for(int i=0;i<argc;i++)
  {
    if(!strcmp(argv[i],"port"))
    {
      if(++i<argc)
        portnum = atoi(argv[i]);
      else
        fprintf(stderr, "Acts: missing port; using default: %d\n",
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
        fprintf(stderr, "Acts: missing configfile; "
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
        fputs("Acts: missing path to executable; "
                "will look for 'acts' in your PATH.\n",stderr);
    }
    else if(!strcmp(argv[i],"version"))
    {
      version_enum_to_string(acts_version,tmpstr,sizeof(tmpstr));

      if(++i<argc)
      {
        if(version_string_to_enum(argv[i]) == ACTS_VERSION_UNKNOWN)
          fprintf(stderr, "Acts: unknown ACTS version given; "
                          "using default: \"%s\"\n", tmpstr);
        else
          acts_version = version_string_to_enum(argv[i]);
      }
      else
        fprintf(stderr, "Acts: missing version string; "
                "using default: \"%s\"\n", tmpstr);
    }
    else
      fprintf(stderr, "Acts: ignoring unknown parameter \"%s\"\n",
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
acts_version_t Acts::version_string_to_enum(char* versionstr)
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
int Acts::version_enum_to_string(acts_version_t versionnum, 
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

int
Acts::Setup()
{
  int i = 0;
  int j;

  char acts_bin_name[] = "acts";
  char acts_configfile_flag[] = "-t";
  char acts_port_flag[] = "-s";

  char acts_port_num[MAX_FILENAME_SIZE];
  char* acts_args[32];

  static struct sockaddr_in server;
  char host[] = "localhost";
  struct hostent* entp;

  printf("ACTS vision server connection initializing (%s,%d)...",
         configfilepath,portnum);
  fflush(stdout);

  player_vision_data_t dummy;
  bzero(&dummy,sizeof(dummy));
  // zero the data buffer
  PutData((unsigned char*)&dummy,
          sizeof(dummy.width)+sizeof(dummy.height)+sizeof(dummy.header),0,0);

  sprintf(acts_port_num,"%d",portnum);

  /* HACK - I've customized for ACTS2.0.  ahoward
  acts_args[i++] = acts_bin_name;
  if(strlen(configfilepath))
  {
    acts_args[i++] = acts_configfile_flag;
    acts_args[i++] = configfilepath;
  }
  acts_args[i++] = acts_port_flag;
  acts_args[i++] = acts_port_num;
  acts_args[i] = (char*)NULL;
  */

  // HACK - custom for ACTS2.0
  acts_args[i++] = acts_bin_name;
  if(strlen(configfilepath))
  {
    acts_args[i++] = "-t";
    acts_args[i++] = configfilepath;
  }
  acts_args[i++] = "-p";
  acts_args[i++] = acts_port_num;
  acts_args[i++] = "-G";
  acts_args[i++] = "bttv";
  acts_args[i++] = "-d";
  acts_args[i++] = "/dev/video0";
  acts_args[i++] = "-n";
  acts_args[i++] = "0";
  acts_args[i++] = "-x";
  acts_args[i] = (char*)NULL;
  assert(i <= sizeof(acts_args) / sizeof(acts_args[0]));
  
  if(!(pid = fork()))
  {
    // make sure we don't get that "ACTS: Packet" bullshit on the console
    //int dummy_fd = open("/dev/null",O_RDWR);
    //dup2(dummy_fd,0);
    //dup2(dummy_fd,1);
    //dup2(dummy_fd,2);

    /* detach from controlling tty, so we don't get pesky SIGINTs and such */
    if(setpgrp() == -1)
    {
      perror("Acts:Setup(): error while setpgrp()");
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
        perror("Acts:Setup(): error while execvp()ing ACTS");
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
        perror("Acts:Setup(): error while execv()ing ACTS");
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
      fprintf(stderr, "Acts::Setup(): \"%s\" is unknown host; "
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
        perror("Acts::Setup(): socket(2) failed");
        KillACTS();
        return(1);
      }
      if(connect(sock,(struct sockaddr*)&server, sizeof(server)) == 0)
        break;
      usleep(ACTS_STARTUP_INTERVAL_USEC);
    }
    if(j == ACTS_STARTUP_CONN_LIMIT)
    {
      perror("Acts::Setup(): connect(2) failed");
      KillACTS();
      return(1);
    }
    puts("Done.");

    /* now spawn reading thread */
    StartThread();

    return(0);
  }

  // shut up compiler!
  return(0);
}

int
Acts::Shutdown()
{
  /* if Setup() was never called, don't do anything */
  if(sock == -1)
    return(0);

  StopThread();

  sock = -1;
  puts("ACTS vision server has been shutdown");
  return(0);
}

void 
Acts::KillACTS()
{
  if(kill(pid,SIGKILL) == -1)
    perror("Acts::KillACTS(): some error while killing ACTS");
}

void
Acts::Main()
{
  int numread;
  int num_blobs;
  int i;

  // we'll transform the data into this structured buffer
  player_vision_data_t local_data;

  // first, we'll read into these two temporary buffers
  uint8_t acts_hdr_buf[sizeof(local_data.header)];
  uint8_t acts_blob_buf[sizeof(local_data.blobs)];

  char acts_request_packet = ACTS_REQUEST_PACKET;

  // TODO: put a descriptive color in here (I'm not sure where
  // to get it from).  Some default colors...
  int colors[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00};
  
  /* make sure we aren't canceled at a bad time */
  if(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL))
  {
    fputs("RunVisionThread: pthread_setcanceltype() failed. exiting\n",
                    stderr);
    pthread_exit(NULL);
  }

  /* make sure we kill ACTS on exiting */
  pthread_cleanup_push(QuitACTS,this);

  /* loop and read */
  for(;;)
  {
    // clean our buffers
    bzero(&local_data,sizeof(local_data));
    
    // put in some stuff that doesnt change
    local_data.width = htons(this->width);
    local_data.height = htons(this->height);
    
    /* test if we are supposed to cancel */
    pthread_testcancel();

    /* request a packet from ACTS */
    if(write(sock,&acts_request_packet,sizeof(acts_request_packet)) == -1)
    {
      perror("RunVisionThread: write() failed sending ACTS_REQUEST_PACKET;"
                      "exiting.");
      break;
    }

    /* get the header first */
    if((numread = read(sock,acts_hdr_buf,header_len)) == -1)
    {
      perror("RunVisionThread: read() failed for header: exiting");
      break;
    }
    else if(numread != header_len)
    {
      fprintf(stderr,"RunVisionThread: something went wrong\n"
             "              expected %d bytes of header, but only got %d\n", 
             header_len,numread);
      break;
    }

    /* convert the header, if necessary */
    if(acts_version == ACTS_VERSION_1_0)
    {
      for(i=0;i<VISION_NUM_CHANNELS;i++)
      {
        // convert 2-byte ACTS 1.0 encoded entries to byte-swapped integers
        // in a structured array
        local_data.header[i].index = 
                htons(acts_hdr_buf[header_elt_len*i]-1);
        local_data.header[i].num = 
                htons(acts_hdr_buf[header_elt_len*i+1]-1);
      }
    }
    else
    {
      for(i=0;i<VISION_NUM_CHANNELS;i++)
      {
        // convert 4-byte ACTS 1.2 encoded entries to byte-swapped integers
        // in a structured array
        local_data.header[i].index = acts_hdr_buf[header_elt_len*i]-1;
        local_data.header[i].index = 
                local_data.header[i].index << 6;
        local_data.header[i].index |= 
                acts_hdr_buf[header_elt_len*i+1]-1;
        local_data.header[i].index = 
                htons(local_data.header[i].index);

        local_data.header[i].num = acts_hdr_buf[header_elt_len*i+2]-1;
        local_data.header[i].num = 
                local_data.header[i].num << 6;
        local_data.header[i].num |= 
                acts_hdr_buf[header_elt_len*i+3]-1;
        local_data.header[i].num = 
                htons(local_data.header[i].num);
      }
    }

    /* sum up the data we expect */
    num_blobs=0;
    for(i=0;i<VISION_NUM_CHANNELS;i++)
      num_blobs += ntohs(local_data.header[i].num);

    /* read in the blob data */
    if((numread = read(sock,acts_blob_buf,num_blobs*blob_size)) == -1)
    {
      perror("RunVisionThread: read() failed on blob data; exiting.");
      break;
    }
    else if(numread != num_blobs*blob_size)
    {
      fprintf(stderr,"RunVisionThread: something went wrong\n"
             "              expected %d bytes of blob data, but only got %d\n", 
             num_blobs*blob_size,numread);
      break;
    }

    if(acts_version == ACTS_VERSION_1_0)
    {
      // convert 10-byte ACTS 1.0 blobs to new byte-swapped structured array
      for(i=0;i<num_blobs;i++)
      {
        int tmpptr = blob_size*i;

        // TODO: put a descriptive color in here (I'm not sure where
        // to get it from).
        local_data.blobs[i].color = htonl(0xFF0000);
        
        // stage puts the range in here to simulate stereo vision. we
        // can't do that (yet?) so set the range to zero - rtv
        local_data.blobs[i].range = 0;

        // get the 4-byte area first
        local_data.blobs[i].area = 0;
        for(int j=0;j<4;j++)
        {
          local_data.blobs[i].area = local_data.blobs[i].area << 6;
          local_data.blobs[i].area |= acts_blob_buf[tmpptr++] - 1;
        }
        local_data.blobs[i].area = htonl(local_data.blobs[i].area);

        // convert the other 6 one-byte entries to byte-swapped shorts
        local_data.blobs[i].x = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].x = htons(local_data.blobs[i].x);

        local_data.blobs[i].y = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].y = htons(local_data.blobs[i].y);

        local_data.blobs[i].left = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].left = htons(local_data.blobs[i].left);

        local_data.blobs[i].right = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].right = htons(local_data.blobs[i].right);

        local_data.blobs[i].top = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].top = htons(local_data.blobs[i].top);

        local_data.blobs[i].bottom = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].bottom = htons(local_data.blobs[i].bottom);
      }
    }
    else
    {
      // convert 16-byte ACTS 1.2 blobs to new byte-swapped structured array
      for(i=0;i<num_blobs;i++)
      {
        int tmpptr = blob_size*i;

        // Figure out the blob channel number
        int ch = 0;
        for (int j = 0; j < VISION_NUM_CHANNELS; j++)
        {
          if (i >= ntohs(local_data.header[j].index) &&
              i < ntohs(local_data.header[j].index) + ntohs(local_data.header[j].num))
          {
            ch = j;
            break;
          }
        }

        // Put in a descriptive color.
        if (ch < (int) (sizeof(colors) / sizeof(colors[0])))
          local_data.blobs[i].color = htonl(colors[ch]);            
        else
          local_data.blobs[i].color = htonl(0xFF0000);

        // stage puts the range in here to simulate stereo vision. we
        // can't do that (yet?) so set the range to zero - rtv
        local_data.blobs[i].range = 0;

        // get the 4-byte area first
        local_data.blobs[i].area = 0;
        for(int j=0;j<4;j++)
        {
          local_data.blobs[i].area = local_data.blobs[i].area << 6;
          local_data.blobs[i].area |= acts_blob_buf[tmpptr++] - 1;
        }
        local_data.blobs[i].area = htonl(local_data.blobs[i].area);
        
        // convert the other 6 two-byte entries to byte-swapped shorts
        local_data.blobs[i].x = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].x = local_data.blobs[i].x << 6;
        local_data.blobs[i].x |= acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].x = htons(local_data.blobs[i].x);

        local_data.blobs[i].y = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].y = local_data.blobs[i].y << 6;
        local_data.blobs[i].y |= acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].y = htons(local_data.blobs[i].y);

        local_data.blobs[i].left = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].left = local_data.blobs[i].left << 6;
        local_data.blobs[i].left |= acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].left = htons(local_data.blobs[i].left);

        local_data.blobs[i].right = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].right = local_data.blobs[i].right << 6;
        local_data.blobs[i].right |= acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].right = htons(local_data.blobs[i].right);

        local_data.blobs[i].top = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].top = local_data.blobs[i].top << 6;
        local_data.blobs[i].top |= acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].top = htons(local_data.blobs[i].top);

        local_data.blobs[i].bottom = acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].bottom = local_data.blobs[i].bottom << 6;
        local_data.blobs[i].bottom |= acts_blob_buf[tmpptr++] - 1;
        local_data.blobs[i].bottom = htons(local_data.blobs[i].bottom);
      }
    }
    
    /* test if we are supposed to cancel */
    pthread_testcancel();

    /* got the data. now fill it in */
    PutData((unsigned char*)&local_data, 
                (VISION_HEADER_SIZE + num_blobs*VISION_BLOB_SIZE),0,0);
  }

  pthread_cleanup_pop(1);
  pthread_exit(NULL);
}

void 
QuitACTS(void* visiondevice)
{
  char acts_request_quit = ACTS_REQUEST_QUIT;
  Acts* vd = (Acts*)visiondevice;


  if((fcntl(vd->sock, F_SETFL, O_NONBLOCK) == -1) ||
     (write(vd->sock,&acts_request_quit,sizeof(acts_request_quit)) == -1))
  {
    perror("Acts::QuitACTS(): WARNING: either fcntl() or write() "
            "failed while sending QUIT command; killing ACTS by hand");
    vd->KillACTS();
  }
  vd->sock = -1;
}
