/*
 * a utility to print out data from any of a number of interfaces
 */
#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
#include <string.h> /* for strcpy() */
#include <assert.h>
#include <assert.h>

//#include <sys/time.h>

#define USAGE \
  "USAGE: playerprint [-h <host>] [-p <port>] <device>\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -t : print the proxy's timestamp before the data\n" 

char host[256] = "localhost";
int port = PLAYER_PORTNUM;
int idx = 0;
char* dev = NULL;
bool print_timestamp = false;

/* easy little command line argument parser */
void
parse_args(int argc, char** argv)
{
  int i;
  char* colon;

  if(argc < 2)
  {
    puts(USAGE);
    exit(1);
  }

  i=1;
  while(i<argc-1)
  {
    if(!strcmp(argv[i],"-h"))
    {
      if(++i<argc-1)
        strcpy(host,argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc-1)
        port = atoi(argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-t"))
      {
	print_timestamp = true;
      }
    else
      {
	puts(USAGE);
	exit(1);
      }
    i++;
  }
  
  dev = argv[argc-1];
  if((colon = strchr(argv[argc-1],':')))
  {
    *colon = '\0';
    if(strlen(colon+1))
      idx = atoi(colon+1);
  }
}

int main(int argc, char **argv)
{
  //struct timeval last;

  parse_args(argc,argv);

  ClientProxy* cp;

  // connect to Player
  PlayerClient pclient(host,port);
  if(!strcmp(dev,PLAYER_POSITION_STRING))
    assert(cp = (ClientProxy*)new PositionProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_TRUTH_STRING))
    assert(cp = (ClientProxy*)new TruthProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_SONAR_STRING))
    assert(cp = (ClientProxy*)new SonarProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_LASER_STRING))
    assert(cp = (ClientProxy*)new LaserProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_LOCALIZE_STRING))
    assert(cp = (ClientProxy*)new LocalizeProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_FIDUCIAL_STRING))
    assert(cp = (ClientProxy*)new FiducialProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_GPS_STRING))
    assert(cp = (ClientProxy*)new GpsProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_PTZ_STRING))
    assert(cp = (ClientProxy*)new PtzProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_BLOBFINDER_STRING))
    assert(cp = (ClientProxy*)new BlobfinderProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_IR_STRING))
    assert(cp = (ClientProxy*)new IRProxy(&pclient,idx,'r'));
  else
  {
    printf("Unknown interface \"%s\"\n", dev);
    exit(1);
  }

  if(cp->GetAccess() != 'r')
  {
    puts("Couldn't get read access");
    exit(1);
  }

  //pclient.SetFrequency(1000);

  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(pclient.Read())
      exit(1);
    /*
    printf("%f\n", (pclient.timestamp.tv_sec + pclient.timestamp.tv_usec/1e6) - 
           (last.tv_sec + last.tv_usec/1e6));
    last=pclient.timestamp;
    */
    
    if( print_timestamp )
      {
	double timestamp = 
	  (double)cp->timestamp.tv_sec + ((double)cp->timestamp.tv_usec)/1e6;
	
	printf( "#timestamp: %.3f\n", timestamp );
      }
    
    cp->Print();
  }
}



