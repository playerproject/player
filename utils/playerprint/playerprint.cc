/*
 * a utility to print out data from any of a number of interfaces
 */

/** @addtogroup utils Utilities */
/** @{ */
/** @defgroup player_util_playerprint playerprint

@par Synopsis

playerprint prints out sensor data to the console.  It is useful mainly
for verifying that a device is working during the setup or debugging
process.  If you want to visualize data, try @ref player_util_playerv.
If you want to log data to a file, try the @ref player_driver_writelog
driver.

@par Usage

playerprint is installed alongside player in $prefix/bin, so if player is
in your PATH, then playerprint should also be.  Command-line usage is:
@verbatim
$ playerprint [-t] [-u <rate>] [-h <host>] [-p <port>] <device>
@endverbatim
Where the options are:
- -h &lt;host&gt;: connect to Player on this host (default: localhost)
- -p &lt;port&gt;: connect to Player on this TCP port (default: 6665)
- -t : print the timestamp before the data (default: don't print timestamps)
- -u &lt;rate&gt;: request data update at &lt;rate&gt; in Hz (default: 10Hz)

For example:
<pre>
  $ playerprint -p 7000 laser
</pre>

@par Features

playerprint can print out data for the following kinds of devices:
- @ref player_interface_position
- @ref player_interface_position3d
- @ref player_interface_truth
- @ref player_interface_sonar
- @ref player_interface_laser
- @ref player_interface_localize
- @ref player_interface_fiducial
- @ref player_interface_gps
- @ref player_interface_ptz
- @ref player_interface_blobfinder
- @ref player_interface_ir
- @ref player_interface_energy
- @ref player_interface_bumper
- @ref player_interface_wifi

*/

/** @} */

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
  "       -t : print the proxy's timestamp before the data\n" \
  "       -u <rate>: request data update at <rate> in Hz\n"

char host[256] = "localhost";
int port = PLAYER_PORTNUM;
int idx = 0;
char* dev = NULL;
bool print_timestamp = false;
int data_rate = 10;

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
    else if(!strcmp(argv[i],"-u"))
    {
      if(++i<argc-1)
        data_rate = atoi(argv[i]);
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
  parse_args(argc,argv);

  ClientProxy* cp;

  // connect to Player
  PlayerClient pclient(host,port);
  assert(!pclient.SetFrequency(data_rate));
  if(!strcmp(dev,PLAYER_POSITION_STRING))
    assert(cp = (ClientProxy*)new PositionProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_POSITION3D_STRING))
    assert(cp = (ClientProxy*)new Position3DProxy(&pclient,idx,'r'));
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
  else if(!strcmp(dev,PLAYER_ENERGY_STRING))
    assert(cp = (ClientProxy*)new EnergyProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_BUMPER_STRING))
    assert(cp = (ClientProxy*)new BumperProxy(&pclient,idx,'r'));
  else if(!strcmp(dev,PLAYER_WIFI_STRING))
    assert(cp = (ClientProxy*)new WiFiProxy(&pclient,idx,'r'));
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
  printf("Entering Main Read Loop\n");
  double last = -1.0;
  for(;;)
  {
  	printf("Executing read\n");
    /* this blocks until new data comes; 10Hz by default */
    if(pclient.Read())
      exit(1);
    
    if(pclient.fresh)
    {
      cp->Print();
      pclient.fresh = false;
      
      if( print_timestamp )
      {
        double timestamp = 
                (double)cp->timestamp.tv_sec + ((double)cp->timestamp.tv_usec)/1e6;

        printf( "#timestamp: %.6f\n", timestamp );
        if(last >= 0.0)
          printf("#diff: %.6f\n", timestamp - last);
        last = timestamp;
      }
    }
    else
    	printf("Not fresh data\n");
  }
}



