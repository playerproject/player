#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <signal.h>
#include <playerclient.h>  // for player client stuff
#include <string.h> /* for strcpy() */

#include "worldfile.hh"

#define USAGE \
  "USAGE: stage [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" 

char host[256] = "localhost";
int port = PLAYER_PORTNUM;

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
    else
    {
      puts(USAGE);
      exit(1);
    }
    i++;
  }
}

int quit = 0;

void Interrupt( int signum )
{
  // setting this will exit the main loop next time round
  quit = 1;
  printf( "Stage shutting down.\n" );
}

int main(int argc, char **argv)
{
  int create = 0;

  //parse_args(argc,argv);

  CWorldFile worldfile;

  worldfile.Load( argv[argc-1] );

  // connect to Player
  PlayerClient pclient(host,port);

  // access the Truth device
  StageProxy sp( &pclient, 0, 'a' );

  // setup signal responses
  if(signal(SIGINT, Interrupt ) == SIG_ERR)
    {
      perror("signal(2) failed while setting up for SIGINT");
      exit(1);
    }
  
  /*  if(signal(SIGHUP, Interrupt ) == SIG_ERR)
    {
      perror("signal(2) failed while setting up for SIGHUP");
      exit(1);
    }
  
  if(signal(SIGTERM, Interrupt) == SIG_ERR)
    {
      perror("signal(2) failed while setting up for SIGTERM");
      exit(1);
    }
  */

  // LOAD THE FILE
  // set the top-level matrix resolution
  //ppm = 1.0 / worldfile.ReadLength( 0, "resolution", 1.0 / ppm );
  
  // Get the authorization key to pass to player
  //const char *authkey = this->worldfile->ReadString(0, "auth_key", "");
  //strncpy(m_auth_key, authkey, sizeof(m_auth_key));
  //m_auth_key[sizeof(m_auth_key)-1] = '\0';
  
  // Get the real update interval
  //m_real_timestep = this->worldfile->ReadFloat(0, "real_timestep", 
  //				      m_real_timestep);

  // Get the simulated update interval
  //m_sim_timestep = this->worldfile->ReadFloat(0, "sim_timestep", m_sim_timestep);
  
  // Iterate through sections and create entities as needs be
  for (int model = 1; model < worldfile.GetEntityCount(); model++)
  {
    // Find out what type of entity this is,
    // and what line it came from.
    //char *type = worldfile.GetEntityType(model);
    
    int line = worldfile.ReadInt(model, "line", -1);
    const char* type =  worldfile.GetEntityType(model);
    const char* name =  worldfile.ReadString(model, "name", "unknown" );
    int parent = worldfile.GetEntityParent(model);
    double px = worldfile.ReadTupleLength( model, "pose", 0, 0.0 );
    double py = worldfile.ReadTupleLength( model, "pose", 1, 0.0 );
    double pa = worldfile.ReadTupleAngle( model, "pose", 2, 0.0 );
    
    int stage_id = sp.CreateModel( type, name, parent, px, py, pa );

    if( stage_id < 1 )
      printf( "Line %d. Error creating model name \"%s\" type \"%s\" \n",
	       line, name, type );
    else
      printf( "Created model name \"%s\" type \"%s\" received id %d\n",
	      name, type, stage_id );
    
  }
  /* go into read-think-act loop */
  while( quit == 0 )
  {
    puts( "reading..." );
    /* this blocks until new data comes; 10Hz by default */
    if( pclient.Read())
      exit(1);

    puts( "printing" );
    /* print data to console */
    sp.Print();

  }

  // destroy everything and exit
  printf( "killing all my models\n" );

  //sp.DestroyModel( "sonar" );
  sp.DestroyAllModels();

  return 0;
}



