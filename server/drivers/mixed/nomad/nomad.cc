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
 * Driver for the Nomadics Nomad 200 robot. Should be easily adapted for other Nomads.
 * Authors: Richard Vaughan (vaughan@sfu.ca), Pawel Zebrowski (pzebrows@sfu.ca)
 * Based on Brian Gerkey et al's P2OS driver.
 * 
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>  /* for abs() */
#include <netinet/in.h>
#include <termios.h>

#include <playertime.h>
extern PlayerTime* GlobalTime;

// so we can access the deviceTable and extract pointers to the sonar
// and position objects
#include <drivertable.h>
#include <devicetable.h>
extern CDeviceTable* deviceTable;
extern int global_playerport; // used to get at devices

#define NOMAD_DEFAULT_SERIAL_PORT "/dev/ttyS0"
#define NOMAD_SONAR_COUNT 16
#define NOMAD_RADIUS_MM 40 // TODO: measure the Nomad to get this exactly right
#define NOMAD_CONFIG_BUFFER_SIZE 256 // this should be bigger than the biggest config we could receive

class Nomad:public CDevice 
{
  private:
    static pthread_t thread;

    
    // device used to communicate with the robot
    static char serial_port[MAX_FILENAME_SIZE]; 

    // did we initialize the common data segments yet?
    static bool initdone;

  static int fd; // serial port fd

  protected:
  //static player_nomad_data_t* data;
  // static player_p2os_cmd_t* command;


    /* start a thread that will invoke Main() */
    virtual void StartThread();
    /* cancel (and wait for termination) of the thread */
    virtual void StopThread();

  public:

  Nomad(char* interface, ConfigFile* cf, int section);
  virtual ~Nomad();
  
  /* the main thread */
  virtual void Main();
  
  virtual int Setup();
  virtual int Shutdown();
  
  virtual void PutData(unsigned char *, size_t maxsize,
		       uint32_t timestamp_sec, uint32_t timestamp_usec);  
};

// declare the static members (this is an ugly requirement of C++ )

bool Nomad::initdone = FALSE;
int Nomad::fd = 0;

// a factory creation function
CDevice* NomadInit(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
    {
      PLAYER_ERROR1("driver \"nomad\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else
    return((CDevice*)(new Nomad(interface, cf, section)));
}

// a driver registration function
void NomadRegister(DriverTable* table)
{
  table->AddDriver("nomad", PLAYER_READ_MODE, NomadInit);
}




Nomad::Nomad(char* interface, ConfigFile* cf, int section)
  : CDevice( sizeof(player_position_data_t), 
	     sizeof(player_position_cmd_t), 1, 1 )
{
  if(!initdone)
    {
      strncpy( serial_port,
	       cf->ReadString(section, "port", serial_port),
	       sizeof(serial_port));
      
    initdone = TRUE;
    }
  else
    {
      
      
    }
  
}

Nomad::~Nomad()
{
  puts( "Destroying Nomad driver" );
  // stop the robot, leave it sane.
  // shut down the serial port
}

int Nomad::Setup()
{
  printf("Nomad connection initializing (%s)...", serial_port);
  fflush(stdout);
  
  if((fd = open(serial_port, 
                     O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR )) < 0 )
    {
      perror("Nomad::Setup():open():");
      return(1);
    }  
 
  struct termios term;
  if( tcgetattr( fd, &term ) < 0 )
    {
      perror("Nomad::Setup():tcgetattr():");
      close(fd);
      fd = -1;
      return(1);
  }

#if HAVE_CFMAKERAW
  cfmakeraw( &term );
#endif
  
  cfsetispeed( &term, B9600 );
  
  if( tcsetattr( fd, TCSAFLUSH, &term ) < 0 )
    {
      perror("Nomad::Setup():tcsetattr():");
      close(fd);
      fd = -1;
      return(1);
    }
  
  if( tcflush( fd, TCIOFLUSH ) < 0 )
    {
      perror("Nomad::Setup():tcflush():");
      close(fd);
      fd = -1;
      return(1);
    }
  
  int flags=0;
  if((flags = fcntl(fd, F_GETFL)) < 0)
    {
      perror("Nomad::Setup():fcntl()");
      close(fd);
      fd = -1;
      return(1);
    }

  
  /* now spawn reading thread */
  StartThread();
  return(0);
}

int Nomad::Shutdown()
{
  if(fd == 0 )
    {
      puts("Nomad is already shut down");
      return(0);
    }
  
  StopThread();
  
  close( fd );
  fd = 0;
  
  puts("Nomad has been shutdown");
  return(0);
}

void Nomad::PutData( unsigned char* src, size_t maxsize,
                         uint32_t timestamp_sec, uint32_t timestamp_usec)
{
  Lock();

  *((player_position_data_t*)device_data) = *((player_position_data_t*)src);

  if(timestamp_sec == 0)
  {
    struct timeval curr;
    GlobalTime->GetTime(&curr);
    timestamp_sec = curr.tv_sec;
    timestamp_usec = curr.tv_usec;
  }

  data_timestamp_sec = timestamp_sec;
  data_timestamp_usec = timestamp_usec;
  
  // todo - copy our data into device_data
  

  Unlock();
}

void 
Nomad::Main()
{  
  unsigned char config[NOMAD_CONFIG_BUFFER_SIZE];
  
  for(;;)
    {
      void* client;
      player_device_id_t id;
      size_t config_size = 0;
      //player_position_data_t

      // first, check if there is a new config command
      if((config_size = GetConfig(&id, &client, (void*)config, sizeof(config))))
	{
	  switch(id.code)
	    {
	    case PLAYER_SONAR_CODE:
	      switch(config[0])
		{
		case PLAYER_SONAR_GET_GEOM_REQ:
		  /* Return the sonar geometry. */
		  if(config_size != 1)
		    {
		      puts("Arg get sonar geom is wrong size; ignoring");
		      if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, 
				  NULL, NULL, 0))
			PLAYER_ERROR("failed to PutReply");
		      break;
		    }
		  
		  player_sonar_geom_t geom;
		  geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;
		  geom.pose_count = NOMAD_SONAR_COUNT;
		  for (int i = 0; i < PLAYER_SONAR_MAX_SAMPLES; i++)
		    {
		      // todo - get the geometry properly
		      geom.poses[i][0] = htons((short) 0);
		      geom.poses[i][1] = htons((short) 0);
		      geom.poses[i][2] = htons((short) 360/NOMAD_SONAR_COUNT * i);
		    }
		  
		  if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, 
			      sizeof(geom)))
		    PLAYER_ERROR("failed to PutReply");
		  break;
		default:
		  puts("Sonar got unknown config request");
		  if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK,
			      NULL, NULL, 0))
		    PLAYER_ERROR("failed to PutReply");
		  break;
		}
	      break;
	      
	    case PLAYER_POSITION_CODE:
          switch(config[0])
	    {
            case PLAYER_POSITION_GET_GEOM_REQ:
	      {
		/* Return the robot geometry. */
		if(config_size != 1)
		  {
		    puts("Arg get robot geom is wrong size; ignoring");
		    if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, 
				NULL, NULL, 0))
		      PLAYER_ERROR("failed to PutReply");
		    break;
		  }
		
		// TODO : get values from somewhere.
		player_position_geom_t geom;
		geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
		geom.pose[0] = htons((short) (0)); // x offset
		geom.pose[1] = htons((short) (0)); // y offset
		geom.pose[2] = htons((short) (0)); // a offset
		geom.size[0] = htons((short) (2 * NOMAD_RADIUS_MM )); // x size
		geom.size[1] = htons((short) (2 * NOMAD_RADIUS_MM )); // y size
		
		if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, 
			    sizeof(geom)))
		  PLAYER_ERROR("failed to PutReply");
		break;
	      }
            default:
              puts("Position got unknown config request");
              if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK,
                          NULL, NULL, 0))
                PLAYER_ERROR("failed to PutReply");
              break;
	    }
          break;
	  
	    default:
	      printf("Nomad: got unknown config request \"%c\"\n",
		     config[0]);
	      
	      if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
		PLAYER_ERROR("failed to PutReply");
	      break;
	    }
	}
      
      /* read the latest data from the robot */
      printf( "read data from robot" );
      
      // TODO

      /* read the latest Player client commands */
      player_position_cmd_t command;
      GetCommand((unsigned char*)&command, sizeof(command));
      
      /* write the command to the robot */
      printf( "command: x:%d y:%d a:%d\n", 
	      ntohl(command.xspeed),
	      ntohl(command.yspeed),
	      ntohl(command.yawspeed) );
      
    }
  pthread_exit(NULL);
}


/* start a thread that will invoke Main() */
void 
Nomad::StartThread()
{
  pthread_create(&thread, NULL, &DummyMain, this);
}

/* cancel (and wait for termination) of the thread */
void 
Nomad::StopThread()
{
  void* dummy;
  pthread_cancel(thread);
  if(pthread_join(thread,&dummy))
    perror("Nomad::StopThread:pthread_join()");
}
