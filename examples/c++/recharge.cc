/*
 * recharge.cc: a demo of the energy proxy. A robot wanders using the
 * laser until it gets low on energy, when it approaches the charging
 * station marked with a fiducial value of 99. When the robot is
 * recharged, it turns around and wanders off again. Works with
 * Stage's example energy.cfg and energy.worldx
 *
 * Authors: Richard Vaughan 
 * Created: 27 Feb 2005
 * History: adapted from laserobstacleavoid.cc by Brian Gerkey et al.
 * CVS: $Id$
 */

#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
#include <limits.h>  // for INT_MAX
#include <string.h> /* for strcmp() */
#include <unistd.h> /* for usleep() */

#define USAGE \
  "USAGE: recharge [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n"

#ifndef MIN
  #define MIN(a,b) ((a < b) ? (a) : (b))
#endif
#ifndef MAX
  #define MAX(a,b) ((a > b) ? (a) : (b))
#endif

char host[256] = "localhost";
int port = PLAYER_PORTNUM;
int device_index = 0; // use this to access the nth indexed position and laser devices

#define CHARGER_FIDUCIAL_ID 99
#define FEEDING 0
#define SEEKING 1
#define WANDERING 2
#define TURNING  3
#define RECHARGE_THRESHOLD 0.7

/* easy little command line argument parser */
void
parse_args(int argc, char** argv)
{
  int i;

  i=1;
  while(i<argc)
  {
    if(!strcmp(argv[i],"-h"))
    {
      if(++i<argc)
        strcpy(host,argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc)
        port = atoi(argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-i"))
    {
      if(++i<argc)
        device_index = atoi(argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else
    {
      puts(USAGE);
      exit(1);
    }
    i++;
  }
}

int main(int argc, char **argv)
{
  double minR, minL;

  parse_args(argc,argv);

  PlayerClient robot(host,port);
  PositionProxy pp(&robot,device_index,'a');
  LaserProxy lp(&robot,device_index,'r');
  EnergyProxy ep(&robot,device_index,'r');
  FiducialProxy fp(&robot,device_index,'r');

  /* this blocks until new data comes; 10Hz by default */
  for( int r=0; r<10; r++ )
    if(robot.Read())
      exit(1);
  
  double initial_joules = ep.joules;

  printf("%s\n",robot.conn.banner);

  if(lp.access != 'r')
    {
      puts( "can't read from laser" );
      exit(-1);
    }  

  int mode = WANDERING;
  double goal = 0.0;
 
  double newspeed, newturnrate;

  /* go into read-think-act loop */
  for(;;)
    {
      /* this blocks until new data comes; 10Hz by default */
      if(robot.Read())
	exit(1);
      
      /* print current sensor data to console */
      //lp.Print();
      //pp.Print();
      //ep.Print();
      
      switch( mode )
	{
	case FEEDING:
	  puts( "Feeding" );
	  pp.SetSpeed( 0, 0 ); // stop
	  
	  if( ep.charging )
	    {
	      // if we're fully charged again
	      if( ep.joules >= initial_joules )
		{
		  mode = TURNING;	      
		  goal = pp.theta - M_PI;
		}
	    }
	  else
	    mode = SEEKING;
	  
	  break;
	  
	case TURNING:
	  {
	    puts( "Turning" );
	    double error = NORMALIZE(goal - pp.theta);
	    
	    printf( "error: %.2f\n", error );

	    if( fabs(error) < 0.1 )
	      mode = WANDERING;
	    else
		pp.SetSpeed( 0, 0.5 );
	  }
	  break;
	  
	case SEEKING:
	  {
	    puts( "Seeking" );
	    
	    if( ep.charging )
	      mode = FEEDING;
	    else
	      {
		double error = 0.0;
		
		bool found = false;
		// look for a charger
		for( int f=0; f<fp.count; f++ )
		  if( fp.beacons[f].id == CHARGER_FIDUCIAL_ID )
		    {
		      printf( "i see a charger at %.2f,%.2f : heading %.2f\n",
			      fp.beacons[f].pose[0],
			      fp.beacons[f].pose[1],
			      atan2( fp.beacons[f].pose[1], fp.beacons[f].pose[0] ));
		      
		      error = atan2( fp.beacons[f].pose[1], fp.beacons[f].pose[0] );
		      found = true;
		      break;
		    }
		
		if( found )
		  {
		    // head for the charger		
		    double speed = 0.3;
		    if( fabs(error) > 0.2 )
		      speed = 0;
		    
		    pp.SetSpeed( speed, error/2.0 );
		  }
		else
		  mode = WANDERING;
	      }
	  }
	  break;
	  
	case WANDERING:
	  {
	    puts( "Wandering" );
	    
	    /*
	     * laser obstacle avoid
	     */
	    
	    minL=1e9;
	    minR=1e9;
	    for (int j=0; j<lp.scan_count/2; j++) {
	      if (minR>lp[j])
		minR=lp[j];
	    }
	    for (int j=lp.scan_count/2; j<lp.scan_count; j++) {
	      if (minL>lp[j])
		minL=lp[j];
	    }
	    //printf("minR:%.3f\tminL:%.3f\n", minR,minL);
	    double l=(1e5*minR)/500-100;
	    double r=(1e5*minL)/500-100;
	    if (l>100) 
	      l=100; 
	    if (r>100) 
	      r=100;
	    
	    newspeed = (r+l)/1e3;
	    
	    newturnrate = (r-l);
	    newturnrate = MIN(newturnrate,40);
	    newturnrate = MAX(newturnrate,-40);
	    newturnrate = DTOR(newturnrate);
	    
	    pp.SetSpeed( newspeed, newturnrate );
	    
	    // next time around, seek if we're hungry
	    if( ep.joules < initial_joules * RECHARGE_THRESHOLD )

	      mode = SEEKING;
	  }	    
	  break;
	  
	default:
	  printf( "Warning: unhandled controller state %d\n",
		  mode );
	  break;
	      
	}
    }
}
