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
#include <unistd.h> /* close(2),fcntl(2),getpid(2),usleep(3),execlp(3),fork(2)*/
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

#define OLD_ACTS_PORT_NUM 5441
#define OUR_OLD_ACTS_PORT_NUM 5440
#define OLD_ACTS_CONNECT_STRING "T bfoo pbar n0 F bACTS pBlobInfo s N r0"

/* time to let ACTS get going before trying to connect */
#define ACTS_STARTUP_USEC 6000000


void* RunVisionThread(void* visiondevice);
void QuitACTS(void* visiondevice);


CVisionDevice::CVisionDevice(int num, char* configfile, bool oldacts)
{
  sock = -1;
  //data = new unsigned char[sizeof(unsigned short)+ACTS_TOTAL_MAX_SIZE];
  data = new player_internal_vision_data_t;

  // save that number.  however, command-line flag to ACTS doesn't seem
  // to work right now...
  portnum = num;
  useoldacts = oldacts;
  if(configfile)
    strncpy(configfilepath,configfile,sizeof(configfilepath));
  else
    configfilepath[0] = '\0';
}

CVisionDevice::~CVisionDevice()
{
  GetLock()->Shutdown(this);
  if(sock != -1)
    QuitACTS(this);
}

int
CVisionDevice::Setup()
{
  int i = 0;

  char acts_bin_name[] = "acts";
  char acts_configfile_flag[] = "-t";
  char acts_port_flag[] = "-s";

  char msg[] = OLD_ACTS_CONNECT_STRING;

  char old_acts_bin_name[] = "ACTSServer";
  char old_acts_configfile_flag[] = "-s";

  char acts_port_num[MAX_FILENAME_SIZE];
  char* acts_args[8];

  static struct sockaddr_in server;
  struct sockaddr_in dummy;
  char host[] = "localhost";
  struct hostent* entp;

  int num_read;
  char buffer[64];

  pthread_attr_t attr;

  printf("ACTS vision server connection initializing...");
  fflush(stdout);

  if(useoldacts)
  {
    acts_args[i++] = old_acts_bin_name;
    if(strlen(configfilepath))
    {
      acts_args[i++] = old_acts_configfile_flag;
      acts_args[i++] = configfilepath;
    }
    acts_args[i] = (char*)NULL;
  }
  else
  {
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
  }

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

    if(execvp((useoldacts) ? old_acts_bin_name : acts_bin_name,acts_args) == -1)
    {
      /* 
       * some error.  print it here.  it will really be detected
       * later when the parent tries to connect(2) to it
       */
      perror("CVisionDevice:Setup(): error while execlp()ing ACTS");
      exit(1);
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


    if(useoldacts)
    {
      /* make our socket */
      if((sock = create_and_bind_socket(&dummy, 1, OUR_OLD_ACTS_PORT_NUM,
                                      SOCK_DGRAM, 0)) < 0)
      {
        perror("CVisionDevice::Setup(): create_and_bind_socket() failed");
        KillACTS();
        return(1);
      }

      /* wait a bit, then connect to the server */
      usleep(ACTS_STARTUP_USEC);

      server.sin_port = htons(OLD_ACTS_PORT_NUM);

      puts("sending conn string");
      /* send the ayllu string to get the server spitting out data */
      if(sendto(sock, (const char*)msg, strlen(msg), 0, 
            (const struct sockaddr*)&server, sizeof(server)) == -1)
      {
        perror("CVisionDevice::Setup():sendto() failed");
        sock = -1;
        KillACTS();
        return(1);
      }

      puts("getting ACK");
      /* get the ACK */
      if((num_read = recvfrom(sock, (char*)buffer, sizeof(buffer), 0, NULL, 
                                      0)) == -1)
      {
        perror("CVisionDevice::Setup():recvfrom() failed");
        sock = -1;
        KillACTS();
        return(1);
      }
      puts("got ACK");
    }
    else
    {
      /* should be htons(5001), but ACTS got it wrong, and it's actually
       * listening on unswapped 5001, which is 35091...*/
      //server.sin_port = htons(portnum);
      server.sin_port = portnum;

      /* make our socket */
      if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
      {
        perror("CVisionDevice::Setup(): socket(2) failed");
        KillACTS();
        return(1);
      }

      /* wait a bit, then connect to the server */
      usleep(ACTS_STARTUP_USEC);

      /* 
       * hook it up
       */
      if(connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1)
      {
        perror("CVisionDevice::Setup(): connect(2) failed");
        KillACTS();
        return(1);
      }
    }

    puts("Done.");

    /* now spawn reading thread */
    if(pthread_attr_init(&attr) ||
       pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) ||
       pthread_create(&thread, &attr, &RunVisionThread, this))
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
  /* if Setup() was never called, don't do anything */
  if(sock == -1)
    return(0);

  if(pthread_cancel(thread))
  {
    fputs("CVisionDevice::Shutdown(): WARNING: pthread_cancel() on vision "
          "reading thread failed; killing ACTS by hand\n",stderr);
    QuitACTS(this);
  }

  puts("ACTS vision server has been shutdown");
  return(0);
}

void 
CVisionDevice::KillACTS()
{
  if(kill(pid,SIGINT) == -1)
    perror("CVisionDevice::KillACTS(): some error while killing ACTS");
}

/* could use this later to send the QUIT packet to ACTS...*/

void 
CVisionDevice::PutData(unsigned char* src, size_t size)
{
  *data = *((player_internal_vision_data_t*)src);
}

size_t
CVisionDevice::GetData(unsigned char* dest, size_t maxsize)
{
  /* size is stored at first two bytes */
  *((player_vision_data_t*)dest) = data->data;
  return(data->size);
}

void 
CVisionDevice::GetCommand(unsigned char* dest, size_t maxsize)
{
}
void 
CVisionDevice::PutCommand(unsigned char* src, size_t maxsize)
{
}
size_t
CVisionDevice::GetConfig(unsigned char* dest, size_t maxsize)
{
  return(0);
}
void 
CVisionDevice::PutConfig(unsigned char* src, size_t maxsize)
{
}

void* 
RunVisionThread(void* visiondevice)
{
  int numread;
  int num_blobs;
  int i;

  CVisionDevice* vd = (CVisionDevice*)visiondevice;

  player_internal_vision_data_t local_data;

  char acts_request_packet = ACTS_REQUEST_PACKET;

  bzero(&local_data,sizeof(local_data));

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

    //if(vd->useoldacts)
    //{
      ///* get the packet */
      //if((num_read = recvfrom(vd->sock, (void*)local_data, sizeof(local_data), 
                                      //0, NULL, 0)) == -1)
      //{
        //perror("RunVisionThread:recvfrom() failed");
        //break;
      //}
    //}
    //else
    //{
    /* request a packet from ACTS */
    if(write(vd->sock,&acts_request_packet,sizeof(acts_request_packet)) == -1)
    {
      perror("RunVisionThread: write() failed sending ACTS_REQUEST_PACKET;"
                      "exiting.");
      break;
    }

    /* get the header first */
    if((numread = read(vd->sock,local_data.data.header,ACTS_HEADER_SIZE)) == -1)
    {
      perror("RunVisionThread: read() failed for header: exiting");
      break;
    }
    else if(numread != ACTS_HEADER_SIZE)
    {
      fprintf(stderr,"RunVisionThread: something went wrong\n"
             "              expected %d bytes of header, but only got %d\n", 
             ACTS_HEADER_SIZE,numread);
      break;
    }

    /* sum up the data we expect */
    for(num_blobs=0,i=1;i<ACTS_HEADER_SIZE;i+=2)
    {
      num_blobs += local_data.data.header[i]-1;
    }


    if((numread = read(vd->sock,local_data.data.blobs,
                                    num_blobs*ACTS_BLOB_SIZE)) == -1)
    {
      perror("RunVisionThread: read() failed on blob data; exiting.");
      break;
    }
    else if(numread != num_blobs*ACTS_BLOB_SIZE)
    {
      fprintf(stderr,"RunVisionThread: something went wrong\n"
             "              expected %d bytes of blob data, but only got %d\n", 
             num_blobs*ACTS_BLOB_SIZE,numread);
      break;
    }
    
    /* byte-swap everything */
    /* no, don't.  since the vision data is a sequence of
     * individual bytes, there's nothing to be swapped */

    /* store total size */
    local_data.size = (uint16_t)(ACTS_HEADER_SIZE + num_blobs*ACTS_BLOB_SIZE);

    /* test if we are supposed to cancel */
    pthread_testcancel();

    /* got the data. now fill it in */
    vd->GetLock()->PutData(vd, (unsigned char*)&local_data, sizeof(local_data));
    //}
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
