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
 *   The speech device.  This one interfaces to the Festival speech
 *   synthesis system (see http://www.cstr.ed.ac.uk/projects/festival/).
 *   It runs Festival in server mode and feeds it text strings to say.
 *
 *   Takes variable length commands which are just ASCII strings to say.
 *   Shouldn't return any data, but returns a single dummy byte right now
 *   Accepts no configuration (for now)
 */

#include <speechdevice.h>

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

#define FESTIVAL_SAY_STRING_PREFIX "(SayText \""
#define FESTIVAL_SAY_STRING_SUFFIX "\")\n"

#define FESTIVAL_QUIT_STRING "(quit)"

#define FESTIVAL_CODE_OK "LP\n"
#define FESTIVAL_CODE_ERR "ER"
#define FESTIVAL_RETURN_LEN 39

// don't change this unless you change the Festival init scripts as well
#define FESTIVAL_DEFAULT_PORTNUM 1314

// change this if Festival is installed somewhere else
#define FESTIVAL_LIBDIR_PATH "/usr/local/festival/lib"

/* time to let Festival to get going before trying to connect */
#define FESTIVAL_STARTUP_USEC 2500000

/* delay inside loop */
#define FESTIVAL_DELAY_USEC 100000


void* RunSpeechThread(void* speechdevice);
void QuitFestival(void* speechdevice);


CSpeechDevice::CSpeechDevice()
{
  sock = -1;
  data = new player_speech_data_t;
  command = new player_speech_cmd_t;
  command_size = 0;

  portnum = FESTIVAL_DEFAULT_PORTNUM;
}

CSpeechDevice::~CSpeechDevice()
{
  GetLock()->Shutdown(this);
  if(sock != -1)
    QuitFestival(this);
}

int
CSpeechDevice::Setup()
{
  char festival_bin_name[] = "festival";
  char festival_server_flag[] = "--server";
  char festival_libdir_flag[] = "--libdir";
  char festival_libdir_value[] = FESTIVAL_LIBDIR_PATH;

  char* festival_args[8];

  static struct sockaddr_in server;
  char host[] = "localhost";
  struct hostent* entp;

  pthread_attr_t attr;

  printf("Festival speech synthesis server connection initializing...");
  fflush(stdout);

  int i=0;
  festival_args[i++] = festival_bin_name;
  festival_args[i++] = festival_server_flag;
  festival_args[i++] = festival_libdir_flag;
  festival_args[i++] = festival_libdir_value;
  festival_args[i] = (char*)NULL;

  if(!(pid = fork()))
  {
    // make sure we don't get Festival output on console
    int dummy_fd = open("/dev/null",O_RDWR);
    dup2(dummy_fd,0);
    dup2(dummy_fd,1);
    dup2(dummy_fd,2);

    /* detach from controlling tty, so we don't get pesky SIGINTs and such */
    if(setpgrp() == -1)
    {
      perror("CSpeechDevice:Setup(): error while setpgrp()");
      exit(1);
    }

    if(execvp(festival_bin_name,festival_args) == -1)
    {
      /* 
       * some error.  print it here.  it will really be detected
       * later when the parent tries to connect(2) to it
       */
      perror("CSpeechDevice:Setup(): error while execlp()ing Festival");
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
      fprintf(stderr, "CSpeechDevice::Setup(): \"%s\" is unknown host; "
                      "can't connect to Festival\n", host);
      /* try to kill Festival just in case it's running */
      KillFestival();
      return(1);
    }

    memcpy(&server.sin_addr, entp->h_addr_list[0], entp->h_length);


    server.sin_port = htons(portnum);

    /* make our socket */
    if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror("CSpeechDevice::Setup(): socket(2) failed");
      KillFestival();
      return(1);
    }

    /* wait a bit, then connect to the server */
    usleep(FESTIVAL_STARTUP_USEC);

    /* 
     * hook it up
     */
    if(connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1)
    {
      perror("CSpeechDevice::Setup(): connect(2) failed");
      KillFestival();
      return(1);
    }

    puts("Done.");

    /* now spawn reading thread */
    if(pthread_attr_init(&attr) ||
       pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) ||
       pthread_create(&thread, &attr, &RunSpeechThread, this))
    {
      fputs("CSpeechDevice::Setup(): pthread creation messed up\n",stderr);
      KillFestival();
      return(1);
    }

    return(0);
  }

  // shut up compiler!
  return(0);
}

int
CSpeechDevice::Shutdown()
{
  /* if Setup() was never called, don't do anything */
  if(sock == -1)
    return(0);

  if(pthread_cancel(thread))
  {
    fputs("CSpeechDevice::Shutdown(): WARNING: pthread_cancel() on speech "
          "reading thread failed; killing Festival by hand\n",stderr);
    QuitFestival(this);
  }

  puts("Festival speech server has been shutdown");
  return(0);
}

void 
CSpeechDevice::KillFestival()
{
  if(kill(pid,SIGHUP) == -1)
    perror("CSpeechDevice::KillFestival(): some error while killing Festival");
  else
    fputs("CSpeechDevice::KillFestival(): killed Festival\n",stderr);
  sock = -1;
}

void 
CSpeechDevice::PutData(unsigned char* src, size_t size)
{
}

// just give a dummy byte
size_t
CSpeechDevice::GetData(unsigned char* dest, size_t maxsize)
{
  *((player_speech_data_t*)dest) = *data;
  return(sizeof(player_speech_data_t));
}

void 
CSpeechDevice::GetCommand(unsigned char* dest, size_t maxsize)
{
  *((player_speech_cmd_t*)dest) = *command;
  command_size = 0;
}
void 
CSpeechDevice::PutCommand(unsigned char* src, size_t maxsize)
{
  *command = *((player_speech_cmd_t*)src);
  // NULLify extra bytes at end
  if(maxsize > sizeof(player_speech_cmd_t))
  {
    fputs("RunSpeechThread: got command too large; ignoring extra bytes\n",
          stderr);
  }
  else
    bzero((command->string)+maxsize,sizeof(player_speech_cmd_t)-maxsize);
  
  // make ABSOLUTELY sure we've got one NULL
  (command->string)[sizeof(player_speech_cmd_t)-1] = '\0';
  
  // now strlen() should return the right length
  command_size = strlen((const char*)(command->string));
}
size_t
CSpeechDevice::GetConfig(unsigned char* dest, size_t maxsize)
{
  return(0);
}
void 
CSpeechDevice::PutConfig(unsigned char* src, size_t maxsize)
{
}

void* 
RunSpeechThread(void* speechdevice)
{
  player_speech_cmd_t cmd;
  char prefix[] = FESTIVAL_SAY_STRING_PREFIX;
  char suffix[] = FESTIVAL_SAY_STRING_SUFFIX;
  // use this to hold temp data
  char buf[256];
  int numread;

  CSpeechDevice* sd = (CSpeechDevice*)speechdevice;

  /* make sure we aren't canceled at a bad time */
  if(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL))
  {
    fputs("RunSpeechThread: pthread_setcanceltype() failed. exiting\n",
                    stderr);
    pthread_exit(NULL);
  }

  sigblock(SIGINT);
  sigblock(SIGALRM);

  /* make sure we kill Festival on exiting */
  pthread_cleanup_push(QuitFestival,sd);

  /* loop and read */
  for(;;)
  {
    /* test if we are supposed to cancel */
    pthread_testcancel();

    /* did we get a new command? */
    if(sd->command_size)
    {
      /* get the string */
      sd->GetLock()->GetCommand(sd,(unsigned char*)&cmd,sizeof(cmd));

      /* send prefix to Festival */
      if(write(sd->sock,(const void*)prefix,strlen(prefix)) == -1)
      {
        perror("RunSpeechThread: write() failed sending prefix; exiting.");
        break;
      }

      /* send string to Festival */
      if(write(sd->sock,(const void*)(cmd.string),
               strlen((const char*)cmd.string)) == -1)
      {
        perror("RunSpeechThread: write() failed sending string; exiting.");
        break;
      }

      /* send suffix to Festival */
      if(write(sd->sock,(const void*)suffix,strlen(suffix)) == -1)
      {
        perror("RunSpeechThread: write() failed sending suffix; exiting.");
        break;
      }

      /* read the resultant string back */
      /* get the code first */
      if((numread = read(sd->sock,buf,strlen(FESTIVAL_CODE_OK))) == -1)
      {
        perror("RunSpeechThread: read() failed for code: exiting");
        break;
      }
      else if((size_t)numread != strlen(FESTIVAL_CODE_OK))
      {
        fprintf(stderr,"RunSpeechThread: something went wrong\n"
                "              expected %d bytes of code, but only got %d\n", 
                strlen(FESTIVAL_CODE_OK),numread);
        break;
      }
      buf[numread]='\0';

      // NULLify the end
      buf[numread]='\0';

      if(!strcmp(buf,FESTIVAL_CODE_OK))
      {
        /* get the other stuff that comes back */
        numread = 0;
        while(numread < FESTIVAL_RETURN_LEN)
        {
          if((numread += read(sd->sock,buf+numread,
                              FESTIVAL_RETURN_LEN-numread)) == -1)
          {
            perror("RunSpeechThread: read() failed for code: exiting");
            break;
          }
        }
        if(numread != FESTIVAL_RETURN_LEN)
        {
          fputs("RunSpeechThread: something went wrong while reading\n",stderr);
          break;
        }
      }
      else
      {
        /* got wrong code back */
        fprintf(stderr, "RunSpeechThread: got strange code back: %s\n", buf);
      }
    }
    
    // so we don't run too fast
    usleep(FESTIVAL_DELAY_USEC);
  }

  pthread_cleanup_pop(1);
  pthread_exit(NULL);
}

void 
QuitFestival(void* speechdevice)
{
  char quit[] = FESTIVAL_QUIT_STRING;

  CSpeechDevice* sd = (CSpeechDevice*)speechdevice;
  /* send quit cmd to Festival */
  if(write(sd->sock,(const void*)quit,strlen(quit)) == -1)
  {
    perror("RunSpeechThread: write() failed sending quit.");
  }
  /* don't know how to tell the Festival server to exit yet, so Kill */
  sd->KillFestival();
}
