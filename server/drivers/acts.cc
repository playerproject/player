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

#include <player.h>

#if HAVE_STRINGS_H
  #include <strings.h>
#endif

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

#include <drivertable.h>

#define ACTS_NUM_CHANNELS 32
#define ACTS_HEADER_SIZE_1_0 2*ACTS_NUM_CHANNELS  
#define ACTS_HEADER_SIZE_1_2 4*ACTS_NUM_CHANNELS  
#define ACTS_BLOB_SIZE_1_0 10
#define ACTS_BLOB_SIZE_1_2 16
#define ACTS_MAX_BLOBS_PER_CHANNEL 10

#define ACTS_VERSION_1_0_STRING "1.0"
#define ACTS_VERSION_1_2_STRING "1.2"
#define ACTS_VERSION_2_0_STRING "2.0"

#define DEFAULT_ACTS_PORT 5001

/* a variable of this type tells the vision device how to interact with ACTS */
typedef enum
{
  ACTS_VERSION_UNKNOWN = 0,
  ACTS_VERSION_1_0 = 1,
  ACTS_VERSION_1_2 = 2,
  ACTS_VERSION_2_0 = 3
} acts_version_t;
/* default is to use older ACTS (until we change our robots) */
#define DEFAULT_ACTS_VERSION ACTS_VERSION_1_0
#define DEFAULT_ACTS_CONFIGFILE "/usr/local/acts/actsconfig"
/* default is to give no path for the binary; in this case, use execvp() 
 * and user's PATH */
#define DEFAULT_ACTS_PATH ""
#define DEFAULT_ACTS_WIDTH 160
#define DEFAULT_ACTS_HEIGHT 120
/********************************************************************/

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

    // stuff that will be used on the cmdline to start ACTS
    acts_version_t acts_version;  // the ACTS version, as an enum
    char binarypath[MAX_FILENAME_SIZE];  // path to executable
    char configfilepath[MAX_FILENAME_SIZE];  // path to configfile (-t)
    char minarea[8];  // min num of pixels for tracking (-w)
    int portnum;  // port number where we'll connect to ACTS (-p)
    char fps[8]; // capture rate (-R)
    char drivertype[8]; // e.g., bttv, bt848 (-G)
    char invertp; // invert the image? (-i)
    char devicepath[MAX_FILENAME_SIZE]; // e.g., /dev/video0 (-d)
    char channel[8]; // channel to use (-n)
    char norm[8]; // PAL or NTSC (-V)
    char pxc200p; // using PXC200? (-x)
    char brightness[8]; // brightness of image (-B)
    char contrast[8]; // contrast of image (-C)
    int width, height;  // the image dimensions. (-W, -H)

    int header_len; // length of incoming packet header (varies by version)
    int header_elt_len; // length of each header element (varies by version)
    int blob_size;  // size of each incoming blob (varies by version)
    char portnumstring[128]; // string version of portnum
    char widthstring[128];
    char heightstring[128];

    // Descriptive colors for each channel.
    uint32_t colors[PLAYER_BLOBFINDER_MAX_CHANNELS];

  public:
    int sock;               // socket to ACTS

    // constructor 
    //
    Acts(char* interface, ConfigFile* cf, int section);

    virtual void Main();

    void KillACTS();

    int Setup();
    int Shutdown();
};

// a factory creation function
CDevice* Acts_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_BLOBFINDER_STRING))
  {
    PLAYER_ERROR1("driver \"acts\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new Acts(interface, cf, section)));
}

// a driver registration function
void 
Acts_Register(DriverTable* table)
{
  table->AddDriver("acts", PLAYER_READ_MODE, Acts_Init);
}

#define ACTS_REQUEST_QUIT '1'
#define ACTS_REQUEST_PACKET '0'

/* the following setting mean that we first try to connect after 1 seconds,
 * then try every 100ms for 6 more seconds before giving up */
#define ACTS_STARTUP_USEC 1000000 /* wait before first connection attempt */
#define ACTS_STARTUP_INTERVAL_USEC 100000 /* wait between connection attempts */
#define ACTS_STARTUP_CONN_LIMIT 60 /* number of attempts to make */


void QuitACTS(void* visiondevice);


Acts::Acts(char* interface, ConfigFile* cf, int section)
  : CDevice(sizeof(player_blobfinder_data_t),0,0,0)
{
  char tmpstr[MAX_FILENAME_SIZE];
  int tmpint;
  int ch;
  uint32_t color;

  sock = -1;

  // first, get the necessary args
  strncpy(binarypath,
          cf->ReadFilename(section, "path", DEFAULT_ACTS_PATH),
          sizeof(binarypath));
  strncpy(configfilepath, 
          cf->ReadFilename(section, "configfile", DEFAULT_ACTS_CONFIGFILE),
          sizeof(configfilepath));
  strncpy(tmpstr,
          cf->ReadString(section, "version", ACTS_VERSION_1_0_STRING),
          sizeof(tmpstr));
  if((acts_version = version_string_to_enum(tmpstr)) == ACTS_VERSION_UNKNOWN)
  {
    PLAYER_WARN2("unknown version \"%s\"; using default \"%s\"",
                 tmpstr, ACTS_VERSION_1_0_STRING);
    acts_version = version_string_to_enum(ACTS_VERSION_1_0_STRING);
  }
  width = cf->ReadInt(section, "width", DEFAULT_ACTS_WIDTH);
  height = cf->ReadInt(section, "height", DEFAULT_ACTS_HEIGHT);
  
  // now, get the optionals
  bzero(minarea,sizeof(minarea));
  if((tmpint = cf->ReadInt(section, "pixels", -1)) >= 0)
    sprintf(minarea,"%d",tmpint);
  portnum = cf->ReadInt(section, "port", DEFAULT_ACTS_PORT);
  bzero(fps,sizeof(fps));
  if((tmpint = cf->ReadInt(section, "fps", -1)) >= 0)
    sprintf(fps,"%d",tmpint);
  bzero(drivertype,sizeof(drivertype));
  if(cf->ReadString(section, "drivertype", NULL))
    strncpy(drivertype, cf->ReadString(section, "drivertype", NULL), 
            sizeof(drivertype)-1);
  invertp = cf->ReadInt(section, "invert", -1);
  bzero(devicepath,sizeof(devicepath));
  if(cf->ReadString(section, "devicepath", NULL))
    strncpy(devicepath, cf->ReadString(section, "devicepath", NULL), 
            sizeof(devicepath)-1);
  bzero(channel,sizeof(channel));
  if((tmpint = cf->ReadInt(section, "channel", -1)) >= 0)
    sprintf(channel,"%d",tmpint);
  bzero(norm,sizeof(norm));
  if(cf->ReadString(section, "norm", NULL))
    strncpy(norm, cf->ReadString(section, "norm", NULL), 
            sizeof(norm)-1);
  pxc200p = cf->ReadInt(section, "pxc200", -1);
  bzero(brightness,sizeof(brightness));
  if((tmpint = cf->ReadInt(section, "brightness", -1)) >= 0)
    sprintf(brightness,"%d",tmpint);
  bzero(contrast,sizeof(contrast));
  if((tmpint = cf->ReadInt(section, "contrast", -1)) >= 0)
    sprintf(contrast,"%d",tmpint);

  // set up some version-specific parameters
  switch(acts_version)
  {
    case ACTS_VERSION_1_0:
      header_len = ACTS_HEADER_SIZE_1_0;
      blob_size = ACTS_BLOB_SIZE_1_0;
      // extra byte-swap cause ACTS 1.0 got it wrong
      portnum = htons(portnum);
      break;
    case ACTS_VERSION_1_2:
    case ACTS_VERSION_2_0:
    default:
      header_len = ACTS_HEADER_SIZE_1_2;
      blob_size = ACTS_BLOB_SIZE_1_2;
      break;
  }
  header_elt_len = header_len / PLAYER_BLOBFINDER_MAX_CHANNELS;
  bzero(portnumstring, sizeof(portnumstring));
  sprintf(portnumstring,"%d",portnum);
  
  bzero(widthstring, sizeof(widthstring));
  sprintf(widthstring,"%d",width);

  bzero(heightstring, sizeof(heightstring));
  sprintf(heightstring,"%d",height);


  // Get the descriptive colors.
  for (ch = 0; ch < PLAYER_BLOBFINDER_MAX_CHANNELS; ch++)
  {
    color = cf->ReadTupleColor(section, "colors", ch, 0xFFFFFFFF);
    if (color == 0xFFFFFFFF)
      break;
    this->colors[ch] = color;
  }
}
    
// returns the enum representation of the given version string, or
// -1 on failure to match.
acts_version_t Acts::version_string_to_enum(char* versionstr)
{
  if(!strcmp(versionstr,ACTS_VERSION_1_0_STRING))
    return(ACTS_VERSION_1_0);
  else if(!strcmp(versionstr,ACTS_VERSION_1_2_STRING))
    return(ACTS_VERSION_1_2);
  else if(!strcmp(versionstr,ACTS_VERSION_2_0_STRING))
    return(ACTS_VERSION_2_0);
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
    case ACTS_VERSION_2_0:
      strncpy(versionstr, ACTS_VERSION_2_0_STRING, len);
      return(0);
    default:
      return(-1);
  }
}

int
Acts::Setup()
{
  int i;
  int j;

  char acts_bin_name[] = "acts";
  //char acts_configfile_flag[] = "-t";
  //char acts_port_flag[] = "-s";

  char* acts_args[32];

  static struct sockaddr_in server;
  char host[] = "localhost";
  struct hostent* entp;

  printf("ACTS vision server connection initializing...");
  fflush(stdout);

  player_blobfinder_data_t dummy;
  bzero(&dummy,sizeof(dummy));
  // zero the data buffer
  PutData((unsigned char*)&dummy,
          sizeof(dummy.width)+sizeof(dummy.height)+sizeof(dummy.header),0,0);

  i = 0;
  acts_args[i++] = acts_bin_name;
  // build the argument list, based on version
  switch(acts_version)
  {
    case ACTS_VERSION_1_0:
      acts_args[i++] = "-t";
      acts_args[i++] = configfilepath;
      if(strlen(portnumstring))
      {
        acts_args[i++] = "-s";
        acts_args[i++] = portnumstring;
      }
      if(strlen(devicepath))
      {
        acts_args[i++] = "-d";
        acts_args[i++] = devicepath;
      }
      break;
    case ACTS_VERSION_1_2:
      acts_args[i++] = "-t";
      acts_args[i++] = configfilepath;
      if(strlen(portnumstring))
      {
        acts_args[i++] = "-p";
        acts_args[i++] = portnumstring;
      }
      if(strlen(devicepath))
      {
        acts_args[i++] = "-d";
        acts_args[i++] = devicepath;
      }
      if(strlen(contrast))
      {
        acts_args[i++] = "-C";
        acts_args[i++] = contrast;
      }
      if(strlen(brightness))
      {
        acts_args[i++] = "-B";
        acts_args[i++] = brightness;
      }
      acts_args[i++] = "-W";
      acts_args[i++] = widthstring;
      acts_args[i++] = "-H";
      acts_args[i++] = heightstring;
      break;
    case ACTS_VERSION_2_0:
      acts_args[i++] = "-t";
      acts_args[i++] = configfilepath;
      if(strlen(minarea))
      {
        acts_args[i++] = "-w";
        acts_args[i++] = minarea;
      }
      if(strlen(portnumstring))
      {
        acts_args[i++] = "-p";
        acts_args[i++] = portnumstring;
      }
      if(strlen(fps))
      {
        acts_args[i++] = "-R";
        acts_args[i++] = fps;
      }
      if(strlen(drivertype))
      {
        acts_args[i++] = "-G";
        acts_args[i++] = drivertype;
      }
      if(invertp > 0)
        acts_args[i++] = "-i";
      if(strlen(devicepath))
      {
        acts_args[i++] = "-d";
        acts_args[i++] = devicepath;
      }
      if(strlen(channel))
      {
        acts_args[i++] = "-n";
        acts_args[i++] = channel;
      }
      if(strlen(norm))
      {
        acts_args[i++] = "-V";
        acts_args[i++] = norm;
      }
      if(pxc200p > 0)
        acts_args[i++] = "-x";
      if(strlen(brightness))
      {
        acts_args[i++] = "-B";
        acts_args[i++] = brightness;
      }
      if(strlen(contrast))
      {
        acts_args[i++] = "-C";
        acts_args[i++] = contrast;
      }
      acts_args[i++] = "-W";
      acts_args[i++] = widthstring;
      acts_args[i++] = "-H";
      acts_args[i++] = heightstring;
      break;
  }
  acts_args[i] = (char*)NULL;

  assert((unsigned int)i <= sizeof(acts_args) / sizeof(acts_args[0]));

  printf("\ninvoking ACTS with:\n\n    ");
  for(int j=0;acts_args[j];j++)
    printf("%s ", acts_args[j]);
  puts("\n");
  
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
  player_blobfinder_data_t local_data;

  // first, we'll read into these two temporary buffers
  uint8_t acts_hdr_buf[sizeof(local_data.header)];
  uint8_t acts_blob_buf[sizeof(local_data.blobs)];

  char acts_request_packet = ACTS_REQUEST_PACKET;

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
      for(i=0;i<PLAYER_BLOBFINDER_MAX_CHANNELS;i++)
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
      for(i=0;i<PLAYER_BLOBFINDER_MAX_CHANNELS;i++)
      {
        // convert 4-byte ACTS 1.2/2.0 encoded entries to byte-swapped integers
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
    for(i=0;i<PLAYER_BLOBFINDER_MAX_CHANNELS;i++)
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
      // convert 16-byte ACTS 1.2/2.0 blobs to new byte-swapped structured array
      for(i=0;i<num_blobs;i++)
      {
        int tmpptr = blob_size*i;

        // Figure out the blob channel number
        int ch = 0;
        for (int j = 0; j < PLAYER_BLOBFINDER_MAX_CHANNELS; j++)
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
                (PLAYER_BLOBFINDER_HEADER_SIZE + 
                 num_blobs*PLAYER_BLOBFINDER_BLOB_SIZE),0,0);
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
