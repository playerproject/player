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

#include <stdio.h>
#include <errno.h>
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
#include <socket_util.h>

#include <device.h>
#include <messages.h>

class Festival:public CDevice 
{
  private:
    int pid;      // Festival's pid so we can kill it later (if necessary)

    int portnum;  // port number where Festival will run (default 1314)
    char festival_libdir_value[MAX_FILENAME_SIZE]; // the libdir

    /* a queue to hold incoming speech strings */
    PlayerQueue* queue;

    bool read_pending;


  public:
    int sock;               // socket to Festival
    void KillFestival();
    
    // constructor 
    Festival(int argc, char** argv);

    ~Festival();
    virtual void Main();

    int Setup();
    int Shutdown();

    size_t GetCommand(unsigned char *, size_t maxsize);
    void PutCommand(unsigned char *, size_t maxsize);
};


// a factory creation function
CDevice* Festival_Init(int argc, char** argv)
{
  return((CDevice*)(new Festival(argc,argv)));
}

#define FESTIVAL_SAY_STRING_PREFIX "(SayText \""
#define FESTIVAL_SAY_STRING_SUFFIX "\")\n"

#define FESTIVAL_QUIT_STRING "(quit)"

#define FESTIVAL_CODE_OK "LP\n"
#define FESTIVAL_CODE_ERR "ER\n"
#define FESTIVAL_RETURN_LEN 39


/* the following setting mean that we first try to connect after 1 seconds,
 * then try every 100ms for 6 more seconds before giving up */
#define FESTIVAL_STARTUP_USEC 1000000 /* wait before first connection attempt */
#define FESTIVAL_STARTUP_INTERVAL_USEC 100000 /* wait between connection attempts */
#define FESTIVAL_STARTUP_CONN_LIMIT 60 /* number of attempts to make */

/* delay inside loop */
#define FESTIVAL_DELAY_USEC 20000

void QuitFestival(void* speechdevice);

Festival::Festival(int argc, char** argv) :
  CDevice(0,sizeof(player_speech_cmd_t),0,0)
{
  int queuelen = SPEECH_MAX_QUEUE_LEN;
  sock = -1;
  read_pending = false;


  portnum = DEFAULT_FESTIVAL_PORTNUM;
  strncpy(festival_libdir_value,DEFAULT_FESTIVAL_LIBDIR,
          sizeof(festival_libdir_value));
  for(int i=0;i<argc;i++)
  {
    if(!strcmp(argv[i],"port"))
    {
      if(++i<argc)
        portnum = atoi(argv[i]);
      else
        fprintf(stderr, "Festival: missing port; using default: %d\n",
                portnum);
    }
    else if(!strcmp(argv[i],"libdir"))
    {
      if(++i<argc)
      {
        strncpy(festival_libdir_value,argv[i],sizeof(festival_libdir_value));
        festival_libdir_value[sizeof(festival_libdir_value)-1] = '\0';
      }
      else
        fprintf(stderr, "Festival: missing configfile; "
                "using default: \"%s\"\n", festival_libdir_value);
    }
    else if(!strcmp(argv[i],"queuelen"))
    {
      if(++i<argc)
        queuelen = atoi(argv[i]);
      else
        fprintf(stderr, "Festival: missing queuelen; "
                "using default: %d\n", queuelen);
    }
    else
      fprintf(stderr, "Festival: ignoring unknown parameter \"%s\"\n",
              argv[i]);
  }

  queue = new PlayerQueue(queuelen);
  assert(queue);
}

Festival::~Festival()
{
  Shutdown();
  if(sock != -1)
    QuitFestival(this);
}

int
Festival::Setup()
{
  char festival_bin_name[] = "festival";
  char festival_server_flag[] = "--server";
  char festival_libdir_flag[] = "--libdir";
  //char festival_libdir_value[] = DEFAULT_FESTIVAL_LIBDIR;

  int j;

  char* festival_args[8];

  static struct sockaddr_in server;
  char host[] = "localhost";
  struct hostent* entp;

  // start out with a clean slate
  PutCommand((unsigned char*)"",0);
  queue->Flush();
  read_pending = false;

  printf("Festival speech synthesis server connection initializing (%s,%d)...",
         festival_libdir_value,portnum);
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
      perror("Festival:Setup(): error while setpgrp()");
      exit(1);
    }

    if(execvp(festival_bin_name,festival_args) == -1)
    {
      /* 
       * some error.  print it here.  it will really be detected
       * later when the parent tries to connect(2) to it
       */
      perror("Festival:Setup(): error while execlp()ing Festival");
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
      fprintf(stderr, "Festival::Setup(): \"%s\" is unknown host; "
                      "can't connect to Festival\n", host);
      /* try to kill Festival just in case it's running */
      KillFestival();
      return(1);
    }

    memcpy(&server.sin_addr, entp->h_addr_list[0], entp->h_length);


    server.sin_port = htons(portnum);



    /* ok, we'll make this a bit smarter.  first, we wait a baseline amount
     * of time, then try to connect periodically for some predefined number
     * of times
     */
    usleep(FESTIVAL_STARTUP_USEC);

    for(j = 0;j<FESTIVAL_STARTUP_CONN_LIMIT;j++)
    {
      // make a new socket, because connect() screws with the old one somehow
      if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
      {
        perror("Festival::Setup(): socket(2) failed");
        KillFestival();
        return(1);
      }

      /* 
      * hook it up
       */
      if(connect(sock, (struct sockaddr*)&server, sizeof(server)) == 0)
        break;
      usleep(FESTIVAL_STARTUP_INTERVAL_USEC);
    }
    if(j == FESTIVAL_STARTUP_CONN_LIMIT)
    {
      perror("Festival::Setup(): connect(2) failed");
      KillFestival();
      return(1);
    }
    puts("Done.");

    /* make it nonblocking */
    if(fcntl(sock,F_SETFL,O_NONBLOCK) < 0)
    {
      perror("Festival::Setup(): fcntl(2) failed");
      KillFestival();
      return(1);
    }

    /* now spawn reading thread */
    StartThread();

    return(0);
  }

  // shut up compiler!
  return(0);
}

int
Festival::Shutdown()
{
  /* if Setup() was never called, don't do anything */
  if(sock == -1)
    return(0);

  StopThread();

  sock = -1;
  puts("Festival speech server has been shutdown");
  return(0);
}

void 
Festival::KillFestival()
{
  if(kill(pid,SIGHUP) == -1)
    perror("Festival::KillFestival(): some error while killing Festival");
  sock = -1;
}

size_t 
Festival::GetCommand(unsigned char* dest, size_t maxsize)
{
  int retval = device_used_commandsize;

  if(device_used_commandsize)
  {
    //*((player_speech_cmd_t*)dest) = *((player_speech_cmd_t*)device_command);
    memcpy(dest,device_command,device_used_commandsize);
    device_used_commandsize = 0;
  }
  return(retval);
}
void 
Festival::PutCommand(unsigned char* src, size_t maxsize)
{
  Lock();

  *((player_speech_cmd_t*)device_command) = *((player_speech_cmd_t*)src);
  // NULLify extra bytes at end
  if(maxsize > sizeof(player_speech_cmd_t))
  {
    fputs("RunSpeechThread: got command too large; ignoring extra bytes\n",
          stderr);
  }
  else
    bzero((((player_speech_cmd_t*)device_command)->string)+maxsize,
          sizeof(player_speech_cmd_t)-maxsize);
  
  // make ABSOLUTELY sure we've got one NULL
  (((player_speech_cmd_t*)device_command)->string)[sizeof(player_speech_cmd_t)-1] = '\0';
  
  // now strlen() should return the right length
  device_used_commandsize = 
          strlen((const char*)(((player_speech_cmd_t*)device_command)->string));

  Unlock();
}

void
Festival::Main()
{
  player_speech_cmd_t cmd;
  char prefix[] = FESTIVAL_SAY_STRING_PREFIX;
  char suffix[] = FESTIVAL_SAY_STRING_SUFFIX;
  // use this to hold temp data
  char buf[256];
  int numread;
  int numthisread;

  /* make sure we aren't canceled at a bad time */
  if(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL))
  {
    fputs("RunSpeechThread: pthread_setcanceltype() failed. exiting\n",
                    stderr);
    pthread_exit(NULL);
  }

  /* make sure we kill Festival on exiting */
  pthread_cleanup_push(QuitFestival,this);

  /* loop and read */
  for(;;)
  {
    /* test if we are supposed to cancel */
    pthread_testcancel();

    /* did we get a new command? */
    if(GetCommand((unsigned char*)&cmd,sizeof(cmd)))
    {
      /* if there's space, put it in the queue */
      if(queue->Push((unsigned char*)&cmd,sizeof(cmd)) < 0)
        fprintf(stderr, "Festival: not enough room in queue; discarding "
                "string:\n   \"%s\"\n", (const char*)(cmd.string));
    }

    /* do we have a string to send and is there not one pending? */
    if(!(queue->Empty()) && !(read_pending))
    {
      /* send prefix to Festival */
      if(write(sock,(const void*)prefix,strlen(prefix)) == -1)
      {
        perror("RunSpeechThread: write() failed sending prefix; exiting.");
        break;
      }

      unsigned char tmpstr[sizeof(player_speech_cmd_t)];
      int tmpstrlen;
      tmpstrlen = queue->Pop(tmpstr,sizeof(tmpstr));
      /* send the first string from the queue to Festival */
      if(write(sock,tmpstr,tmpstrlen) == -1)
      {
        perror("RunSpeechThread: write() failed sending string; exiting.");
        break;
      }

      /* send suffix to Festival */
      if(write(sock,(const void*)suffix,strlen(suffix)) == -1)
      {
        perror("RunSpeechThread: write() failed sending suffix; exiting.");
        break;
      }
      read_pending = true;
    }

    /* do we have a read pending? */
    if(read_pending)
    {
      /* read the resultant string back */
      /* try to get one byte first */
      if((numread = read(sock,buf,1)) == -1)
      {
        /* was there no data? */
        if(errno == EAGAIN)
          continue;
        else
        {
          perror("RunSpeechThread: read() failed for code: exiting");
          break;
        }
      }

      /* now get the rest of the code */
      while((size_t)numread < strlen(FESTIVAL_CODE_OK))
      {
        /* i should really try to intrepret this some day... */
        if((numthisread = 
            read(sock,buf+numread,strlen(FESTIVAL_CODE_OK)-numread)) == -1)
        {
          /* was there no data? */
          if(errno == EAGAIN)
            continue;
          else 
          {
            perror("RunSpeechThread: read() failed for code: exiting");
            break;
          }
        }
        numread += numthisread;
      }
      if((size_t)numread != strlen(FESTIVAL_CODE_OK))
      {
        fprintf(stderr,"RunSpeechThread: something went wrong\n"
                "              expected %d bytes of code, but got %d\n", 
                strlen(FESTIVAL_CODE_OK),numread);
        break;
      }
      // NULLify the end
      buf[numread]='\0';


      if(!strcmp(buf,FESTIVAL_CODE_OK))
      {
        /* get the other stuff that comes back */
        numread = 0;
        while(numread < FESTIVAL_RETURN_LEN)
        {
          if((numthisread = read(sock,buf+numread,
                              FESTIVAL_RETURN_LEN-numread)) == -1)
          {
            if(errno == EAGAIN)
              continue;
            else
            {
              perror("RunSpeechThread: read() failed for code: exiting");
              break;
            }
          }
          numread += numthisread;
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

      read_pending = false;
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

  Festival* sd = (Festival*)speechdevice;
  /* send quit cmd to Festival */
  if(write(sd->sock,(const void*)quit,strlen(quit)) == -1)
  {
    perror("RunSpeechThread: write() failed sending quit.");
  }
  /* don't know how to tell the Festival server to exit yet, so Kill */
  sd->KillFestival();
}
