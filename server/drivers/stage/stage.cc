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
 * Stage device that connects to a Stage server and interacts with it
 */

#define PLAYER_ENABLE_TRACE 1

#if  HAVE_CONFIG_H
#  include <config.h>
#endif

#define DEBUG
#undef VERBOSE

#include <assert.h>

#include <math.h>
#include <netinet/in.h> // for byte swapping macros
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h> // for rand()
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h> /* for exit(2) */

#include "device.h"
#include "entity.hh"
#include "playercommon.h" // For length-specific types, error macros, etc
#include "playerpacket.h"
#include "playertime.h"
#include "root.hh"
#include "stage.h"
#include "stagetime.h"
extern PlayerTime* GlobalTime;

#include "stg_puck.hh"
#include "stg_sonar.hh"
#include "stg_idar.hh"
#include "stg_position.hh"

#ifdef INCLUDE_RTK2
#include "rtkgui.hh"
// Basic GUI elements declared in rtkgui.cc
extern rtk_canvas_t *canvas; // the canvas
extern rtk_app_t *app; // the RTK/GTK application

#endif

/*
#include "stg_bumperdevice.hh"
#include "stg_broadcastdevice.hh"
#include "stg_gpsdevice.hh"
#include "stg_gripperdevice.hh"
#include "stg_idarturretdevice.hh"
#include "stg_fiducialfinderdevice.hh"
#include "stg_laserdevice.hh"
#include "stg_motedevice.hh"
#include "stg_powerdevice.hh"
#include "stg_ptzdevice.hh"
#include "stg_truthdevice.hh"
#include "stg_visiondevice.hh"
#include "stg_regularmcldevice.hh"
//#include "stg_bpsdevice.hh"
*/

// MODEL INSTALLATION --------------------------------------------
//
// this array defines the models that are available to Stage. New
// devices must be added here.

stage_libitem_t library_items[] = { 
  { "box", "black", (CFP)CEntity::Creator},
  { "puck", "green", (CFP)CPuck::Creator},
  { "sonar", "red", (CFP)CSonarModel::Creator},
  { "idar", "blue", (CFP)CIdarModel::Creator },
  { "position", "purple", (CFP)CPositionModel::Creator },
  /*
    { "bitmap", "blue", (CFP)CBitmap::Creator},
  { "laser", "blue", (CFP)CLaserDevice::Creator},
  { "position", "red", (CFP)CPositionDevice::Creator},
  { "box", "yellow", (CFP)CBox::Creator},
  { "gps", "gray", (CFP)CGpsDevice::Creator},
  { "gripper", "blue", (CFP)CGripperDevice::Creator},
  { "idar", "DarkRed", (CFP)CIdarDevice::Creator},
  { "idarturret", "DarkRed", (CFP)CIdarTurretDevice::Creator},
  { "lbd", "gray", (CFP)CFiducialFinder::Creator},
  { "fiducialfinder", "gray", (CFP)CFiducialFinder::Creator},
  { "mote", "orange", (CFP)CMoteDevice::Creator},
  { "power", "wheat", (CFP)CPowerDevice::Creator},
  { "ptz", "magenta", (CFP)CPtzDevice::Creator},
  { "truth", "purple", (CFP)CTruthDevice::Creator},
  { "vision", "gray", (CFP)CVisionDevice::Creator},
  { "blobfinder", "gray", (CFP)CVisionDevice::Creator},
  { "broadcast", "brown", (CFP)CBroadcastDevice::Creator},
  { "bumper", "LightBlue", (CFP)CBumperDevice::Creator},
  { "regularmcl", "blue", (CFP)CRegularMCLDevice::Creator},
  // { "bps", BpsType, (CFP)CBpsDevice::Creator},
  */

  {NULL, NULL, NULL } // marks the end of the array
};  


///////////////////////////////////////////////////////////////////////////
// Global vars

double update_interval = 0.01; // seconds

// Quit signal
int quit = 0;
int paused = 0;

class StageDevice : public CDevice
{
public: 
  // the usual device interface methods
  
  // constructor
  StageDevice();
  
  // destructor  // Main function for device thread
  virtual void Main();
  
  ~StageDevice();
  
  // Initialise the device
  virtual int Setup();
  
  // Terminate the device
  virtual int Shutdown();

  int HandleConfigRequests();
  
  int WaitForWallClock();
  void PrintVersion( void );
  void PrintUsage( void );
  double GetRealTime();
  
  int GuiInit( int argc, char** argv );
  int GuiEntityShutdown( CEntity* ent );
  int GuiEntityStartup( CEntity* ent );
  int GuiEntityPropertyChange( CEntity* ent, stage_prop_id_t prop );
  int GuiUpdate( void );
  
  // mutex to manage access to model data
  pthread_mutex_t mutex; 
  // manipulate the mutex
  int ModelLock();
  int ModelUnlock();

  StageTime stagetime;
};


// catch SIGUSR1 to toggle pause
void CatchSigUsr1( int signo )
{
  paused = !paused; 
  paused ? PRINT_MSG("CLOCK STARTED" ) : PRINT_MSG( "CLOCK STOPPED" );
}


StageDevice::StageDevice()
	: CDevice(sizeof(player_stage_data_t),
		  0, // no commands 
		  50, 50) // plenty of room on the queues
{
  // hello world
  printf("\n** Stage v%s ** ", VERSION );
  fflush( stdout );
  puts( "" ); // end the startup output line
  
  // catch clock start/stop commands
  signal(SIGUSR1, CatchSigUsr1 );
 
  
  // parse arguments
  for( int a=1; a<argc; a++ )
    {
      // a version request prints a version string and quits
      if( 
	 (strcmp( argv[a], "--version" ) == 0 ) ||
	 (strcmp( argv[a], "-v" ) == 0) 
	 )
	{
	  PrintVersion();
	  exit( 0 );
	} 
    }

  // create the root object
  CEntity::root = new CRootEntity( library_items );
  
#ifdef INCLUDE_RTK2
  // bring up the GUI
  RtkGuiInit( argc, argv );
  RtkGuiLoad();
  CEntity::root->RtkStartup(canvas );
#endif
  

  // replace Player's wall clock with our clock
  if( GlobalTime ) delete GlobalTime;
  assert(GlobalTime = (PlayerTime*)&stagetime );
  
  // this runs our Main() method in it's own thread
  StartThread();

  printf( "STAGEDEVICE: stage model created" );
}


// destructor
StageDevice::~StageDevice( void )
{
 // Stop the device thread
  StopThread();
}


// initialization function
CDevice* 
StageDeviceInit()
{
  return((CDevice*)(new StageDevice()));
}

// our thread method    
void StageDevice::Main( void )
{
 PRINT_WARN( "main loop" );

  for(;;) 
  {
    // test if we are supposed to cancel
    pthread_testcancel();
    
    // Update the configuration.
    if( HandleConfigRequests() == -1 )
      {
	PLAYER_WARN( "Stage failed to handle a config request" );
      }
    
    // grab a lock on the stage fd
    pthread_mutex_lock(&mutex);
    
    // update the simulation model
    if( CEntity::root && !paused  ) 
      {
	printf( "." ); fflush( stdout );
	//CEntity::root->Update();
	CEntity::simtime += update_interval; 
	
	struct timeval tv;
	tv.tv_sec = (long int)floor(CEntity::simtime);
	tv.tv_usec = (long int)((CEntity::simtime - (double)tv.tv_sec) * MILLION);
	stagetime.SetTime( &tv );
      }
    
    // Make data available
    player_stage_data_t data;
    data.interval_ms = htonl((int)(update_interval * 1000.0));
    data.model_count = htonl(CEntity::root->NumModels()-1);
    //printf( "putting data" );
    PutData((uint8_t*) &data, sizeof(data), 0,0 );

    // process GUI events
    RtkGuiUpdate();
    
    // release the stage fd
    pthread_mutex_unlock(&mutex);
  
    // if( real_time_mode )
    WaitForWallClock();
    
  }
}

//////////////////////////////////////////////////////////////xx/////////////
// Initialise the device
//
int StageDevice::Setup()
{
  PRINT_WARN( "setup" );


  return(0);
}


///////////////////////////////////////////////////////////////////////////
// Terminate the device
//
int StageDevice::Shutdown()
{
  PRINT_WARN( "SHUTDOWN stage" );

  return 0;
};


////////////////////////////////////////////////////////////////////////////////
// Process configuration requests.  Returns 1 if the configuration has changed.
int StageDevice::HandleConfigRequests()
{
  int len;
  void *client;
  char buffer[PLAYER_MAX_REQREP_SIZE];
  bool ok = true;
  
  while ((len = GetConfig(&client, &buffer, sizeof(buffer))) > 0)
    {
      switch (buffer[0])
	{
	case PLAYER_STAGE_CREATE_MODEL:
	  if (len != sizeof(player_stage_model_t))
	    {
	      PLAYER_ERROR2("config request len is invalid (%d != %d)", 
			    len,  sizeof(player_stage_model_t));
	      
	      if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
		PLAYER_ERROR("PutReply() failed");
	    }
	  else
	    {
	      player_stage_model_t* model = (player_stage_model_t*)buffer;
	      
	      printf( "received create model request for:\n"
		      "type %s name %s  parent %d at (%.2f %.2f %.2f)\n",
		      model->type, model->name, model->parent, 
		      model->px, model->py, model->pa );
	      
	      this->ModelLock();
	      assert( CEntity::root->CreateModel( model ) == 0 );
	      this->ModelUnlock();

	      if( PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
			   model, sizeof(player_stage_model_t)) != 0 )
		PLAYER_ERROR("PutReply() failed responding to "
			     "PLAYER_STAGE_CREATE_MODEL");
	    }
	  break;
	  
	case PLAYER_STAGE_DESTROY_MODEL:
	  {
	    PLAYER_TRACE0( "received config PLAYER_STAGE_DESTROY_MODEL" );
	    
	    // cast the data to the correct type for this IOCTL
	    player_stage_model_t *model =  
	      (player_stage_model_t*)buffer;
	    
	    this->ModelLock();
	    CEntity::root->DestroyModel( model->id );
	    this->ModelUnlock();
	    
	    if( PutReply(client, PLAYER_MSGTYPE_RESP_ACK ) != 0 )
	      PLAYER_ERROR("PutReply() failed responding to "
			   "PLAYER_STAGE_DESTROY_ALL");
	  }
	  break;
	  
	case PLAYER_STAGE_DESTROY_ALL:
	  PLAYER_TRACE0( "received config PLAYER_STAGE_DESTROY_ALL" );

	  this->ModelLock();
	  CEntity::root->DestroyAll();
	  this->ModelUnlock();

	  if( PutReply(client, PLAYER_MSGTYPE_RESP_ACK ) != 0 )
	    PLAYER_ERROR("PutReply() failed responding to "
			 "PLAYER_STAGE_DESTROY_ALL");
	  break;
	  
	default:
	  PLAYER_WARN1( "received unknown config (%d)", buffer[0] );
	}
    }  
  return 0; // ok.
}


///////////////////////////////////////////////////////////////////////////
// Print the version and credits
void StageDevice::PrintVersion( void )
{
  printf("\nstage %s.\n\n"
	 "Copyright (C) 1999-2003 \n"
	 "  Richard Vaughan <vaughan@hrl.com>\n"
	 "  Andrew Howard <ahoward@usc.edu>\n"
	 "  Brian Gerkey <gerkey@usc.edu>\n"
	 "  and contributors.\n\n"

	 "Part of the Player/Stage Project "
	 "[http://playerstage.sourceforge.net]\n"

	 // GNU disclaimer
	 "This is free software; see the source for copying conditions.  "
	 "There is NO\nwarranty; not even for MERCHANTABILITY or "
	 "FITNESS FOR A PARTICULAR PURPOSE.\n\n"
	 , VERSION );
}


///////////////////////////////////////////////////////////////////////////
// Print the usage string
void StageDevice::PrintUsage( void )
{
  printf("\nUsage: stage [options] <worldfile>\n"
	 "Options: <argument> [default]\n"
	 " -h\t\tPrint this message\n" 
	 " -g\t\tDo not start the X11 GUI\n"
	 " -n \t\tDo not start Player\n"
	 " -o\t\tEnable console status output\n"
	 " -v <float>\tSet the simulated time increment per cycle [0.1sec].\n"
	 " -u <float>\tSet the desired real time per cycle [0.1 sec].\n"
	 " -f \t\tRun as fast as possible; don't try to match real time\n"
	 " -s\t\tStart stage with the clock stopped (send SIGUSR1 to toggle clock)\n"
	 "\nSwitches for experimental/undocumented features:\n"
	 " -p <portnum>\tSet the server port [6601]\n"
	 " -c <hostname>\tRun as a client to a Stage server on hostname\n"
	 " -cl\t\tRun as a client to a Stage server on localhost\n"
	 " -l <filename>\tLog some timing and throughput statistics into <filename>.<incremental suffix>\n"
	 "\nCommand-line options override any configuration file equivalents.\n"
	 "See the Stage manual for details.\n"
	 "\nPart of the Player/Stage Project [http://playerstage.sourceforge.net].\n"
	 "Copyright 2000-2003 Richard Vaughan, Andrew Howard, Brian Gerkey and contributors\n"
	 "Released under the GNU General Public License"
	 " [http://www.gnu.org/copyleft/gpl.html].\n"
	 "\n"
	 );
}

///////////////////////////////////////////////////////////////////////////
// Handle quit signals
void sig_quit(int signum)
{
  PRINT_DEBUG1( "SIGNAL %d", signum );
  //quit = 1;
  //StageQuit();
  exit( 0 );
}

double TimevalSeconds( struct timeval *tv )
{
  return( (double)(tv->tv_sec) + (double)(tv->tv_usec) / (double)MILLION );
}

///////////////////////////////////////////////////////////////////////////
// Get the real time
// Returns time in sec since the first call of this function
double StageDevice::GetRealTime()
{
  static double start_time;
  static bool init = true;
  struct timeval tv;
 
  if( init )
    {
      gettimeofday( &tv, NULL );
      start_time = TimevalSeconds( &tv );
      init = false;
      PRINT_DEBUG1( "start time %.4f", start_time );
    }
  
  gettimeofday( &tv, NULL );
  return( TimevalSeconds( &tv ) - start_time );
}

double TimespecSeconds( struct timespec *ts )
{
  return( (double)(ts->tv_sec) + (double)(ts->tv_nsec) / (double)BILLION);
}


void PackTimespec( struct timespec *ts, double seconds )
{
  ts->tv_sec = (time_t)seconds;
  ts->tv_nsec = (long)(fmod(seconds, 1.0 ) * BILLION );
}

int StageDevice::ModelLock()
{
  // acquire the lock on the socket
  return pthread_mutex_lock(&this->mutex);
}

int StageDevice::ModelUnlock()
{
  // release the lock on the socket
  return pthread_mutex_unlock(&this->mutex);
}

int  StageDevice::WaitForWallClock()
{
  static double min_sleep_time = 0.0;
  static double avg_interval = 0.01;

  struct timespec ts;
  
  static double last_time = 0.0;
  
  double timenow = GetRealTime();
  double interval = timenow - last_time;
  
  last_time += interval;
  
  avg_interval = 0.9 * avg_interval + 0.1 * interval;
  
  double freq = 1.0 / avg_interval;
  
  double outputtime =  GetRealTime();
  //PRINT_DEBUG4( "time %.4f freq %.2f interval %.4f avg %.4f", 
  //	outputtime, freq, interval, avg_interval );
  //printf( "time %.4f freq %.2f interval %.4f avg %.4f\n", 
  //	outputtime, freq, interval, avg_interval );

  //PRINT_MSG2( "time %.4f freq %.2fHz", timenow, freq );
  
  // if we have spare time, go to sleep
  double spare_time = update_interval - avg_interval;
  
  if( spare_time > min_sleep_time )
    {
      //printf( "sleeping for %.6f seconds\n", spare_time );
      ts.tv_sec = (time_t)spare_time;
      ts.tv_nsec = (long)(fmod( spare_time, 1.0 ) * BILLION );
      
      nanosleep( &ts, NULL ); // just stop the powerbook getting hot :)
    }
  //else
  //printf( "not sleeping\n" );
  
  return 0; // success
}
