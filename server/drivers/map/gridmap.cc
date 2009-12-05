/*  gridmap
 *  Player PLUGIN by Marco Paladini
 *  paladini.marco(at)gmail.com
 *
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  
 *     Brian Gerkey, Andrew Howard
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
 * Desc: A Mapping driver which maps from sonar data
 * from the multidriver example by Andrew Howard
 * Author: Marco Paladini, (mail: breakthru@inwind.it)
 * Date: 29 April 2006
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_mapping gridmap
  * @brief Provides a map using sonars
Mapping Driver plugin

This driver is a very simple start to
have the robot mapping its own environment.
It works with an occupancy grid map, using
the @ref player_map_data_t data type.
this driver subscribes to odometry position2d and sonar device, every 100milliseconds puts an occupied line on the map, based on the endpoint of the sonar ray.

this plugin driver uses
- "odometry" @ref interface_position2d : source of odometry information
- @ref interface_sonar : source of sonar data

@par Configuration requests
provides ["map:0"]
requires ["position2d:0" "sonar:0"]

@par Configuration file options
  - width (integer)
     map width in pixels, defaults to 0 if not specified.
  - height (integer)
     map height in pixels, defaults to 0 if not specified.
  - startx (integer)
     initial global robot x position in pixels
  - starty (integer)
     initial global robot y position in pixels
  - scale (float)
     pixels/meter, the stage simple.world
     has a 16 meter map with a 0.028 scale, so 1 meter is (int) 1/0.028 = 36 pixels
  - sonartreshold (float)
     sonar data above the treshold is ignored.
     this is the most simple way to account for maximum range readings due to reflections.
     for a stage simulated pioneer robot the sonar range is 5 meters, so treshold should be less than 5. defaults to 1 if not specified.

The following configuration file illustrates the use of the driver in a stage simulated world (here map width height and resolution were taken from the world file for an easy comparison of the stage world with the map):

@verbatim
driver
(		
  name "stage"
  provides ["simulation:0" ]
  plugin "libstageplugin"

  # load the named file into the simulator
  worldfile "simple.world"	
)

# Create a Stage driver and attach position2d and laser interfaces 
# to the model "robot1"
driver
( 
  name "stage"
  provides ["position2d:0" "sonar:0" "laser:0"]
  model "robot1" 
)

driver
( 
  name "gridmap"
  provides ["map:0"]
  requires ["position2d:0" "sonar:0"]
  plugin "mapping.so"
  width 809
  height 689
  startx 0
  starty 0
  scale 0.028
  treshold 4.5
)
@endverbatim
*/

// ONLY if you need something that was #define'd as a result of configure 
// (e.g., HAVE_CFMAKERAW), then #include <config.h>, like so:
/*
#if HAVE_CONFIG_H
  #include <config.h>
#endif
*/


#include "gridmap.h"
#include <libplayercore/playercore.h>

//#include <gtk/gtk.h>
//#define USEGTK 0
#define DEBUG 0

using namespace std;

////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class gridmap : public ThreadedDriver
{
  public:
      // function to decide empty and occupied cells
    int mapTreshold();
    Map map_data;
    // Constructor; need that
    gridmap(ConfigFile* cf, int section);

    // Must implement the following methods.
    virtual int Setup();
    virtual int Shutdown();
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr, 
                               void * data);

  private:
    // Main function for device thread.
    virtual void Main();

    // Method for Mapping
    virtual int UpdateMap(player_position2d_data_t* odom, player_sonar_data_t* sonar);
    //my map interface
    player_devaddr_t map_addr;
    player_map_data_t map,published_map;
    player_map_info_t map_info;
  
    // odometry position interface
    player_devaddr_t odom_addr;
    Device* odom_dev;
    player_position2d_data_t last_odom_data;

    // Address of and pointer to the sonar device to which I'll subscribe
    player_devaddr_t sonar_addr;
    Device* sonar_dev;
    player_sonar_data_t last_sonar_data;
    player_sonar_geom_t sonar_geom;
    // distance from sonar to the center of the robot. in pixels
    int *sonar_dist;

    // What distance from the robot should be accounted as "obstacle"?
    float sonar_treshold;
    // how many times do i have to sense an obstacle to say it's there?
    int map_treshold;
    // pixel offset from position2d (0,0,0)
    int startx,starty;

#ifdef USEGTK
  GtkWidget *mainwin;
#endif

};


// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver* gridmap_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return ((Driver*) (new gridmap(cf, section)));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void gridmap_Register(DriverTable* table)
{
  table->AddDriver("gridmap", gridmap_Init);
}




////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
gridmap::gridmap(ConfigFile* cf, int section)
    : ThreadedDriver(cf, section)
{

  // Create my map interface
  if (cf->ReadDeviceAddr(&(this->map_addr), section, 
                         "provides",PLAYER_MAP_CODE , 0, NULL) != 0)
  {
   this->SetError(-1);
   return;
  }

  if (this->AddInterface(this->map_addr))
    {
      PLAYER_ERROR("Can't add map interface");
      this->SetError(-1);    
      return;
    }

  // let the config file tell us the initial pixel offset
  this->startx = cf->ReadInt(section, "startx", 0);
  this->starty = cf->ReadInt(section, "starty", 0);
  // fill config values in the map
  this->map.width = cf->ReadInt(section, "width", 0);
  this->map.height = cf->ReadInt(section, "height", 0);
  this->map.col = 0;
  this->map.row = 0;
  this->map.data_count = map.width*map.height;
  this->published_map.data = (int8_t *)malloc(this->map.data_count*sizeof(int8_t));
  // fill the map_info data
  this->map_info.scale = cf->ReadFloat(section, "scale", 1.0);
  // What distance from the robot should be accounted as "obstacle"?
  this->sonar_treshold = cf->ReadFloat(section, "sonartreshold", 1.0);
  this->map_treshold = cf->ReadInt(section, "maptreshold", 3);
  this->map_info.width = this->map.width;
  this->map_info.height = this->map.height;
  this->map_info.origin.px = this->map.col;
  this->map_info.origin.py = this->map.row;
  this->map_info.origin.pa = 0;
  // fill the map data with free space
  // reasons for this are some good obstacle avoidance always there
  // can't bear to say all the world is unknown
  for(unsigned int i=0;i<this->map.width*this->map.height;i++)
     this->published_map.data[i]=-1;

  // Open the position interface
  if (cf->ReadDeviceAddr(&(this->odom_addr), section, 
                         "requires", PLAYER_POSITION2D_CODE, 0, NULL) != 0)
  {
    PLAYER_ERROR("Can't open position2d interface");
    this->SetError(-1);
    return;
  }

  // Find out which sonar I'll subscribe to
  this->sonar_dev = NULL;
  memset(&this->sonar_addr,0,sizeof(player_devaddr_t));
  if(cf->ReadDeviceAddr(&this->sonar_addr, section, "requires",
                     PLAYER_SONAR_CODE, -1, NULL) != 0 )
  {
    PLAYER_ERROR("Can't find sonar");
    this->SetError(-1);
    return;
  }

printf("creating a %dx%d pixels map, sonar treshold at %f\n",this->map.width,this->map.height,this->sonar_treshold);
/// creating the Map object
this->map_data.width=this->map.width;
this->map_data.height=this->map.height;
#ifdef USEGTK

  /* Initialize the widget set */
  gtk_init(NULL,NULL);

  /* Create the main window */
  this->mainwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  /* Set up our GUI elements */


  /* Show the application window */
  gtk_widget_show_all (mainwin);

  /* Enter the main event loop, and wait for user interaction */
  gtk_main ();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int gridmap::Setup()
{   
  puts("Map driver initialising");

  // Subscribe to the sonar device
  if(!(this->sonar_dev = deviceTable->GetDevice(this->sonar_addr)))
  {
    PLAYER_ERROR("unable to locate suitable sonar device");
    return(-1);
  }
  if(this->sonar_dev->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to sonar device");
    return(-1);
  }
  // request sonar geometry
  Message* msg;
  // Get the sonar poses
  if(!(msg = this->sonar_dev->Request(this->InQueue,
                                  PLAYER_MSGTYPE_REQ,
                                  PLAYER_SONAR_REQ_GET_GEOM,
                                  NULL, 0, NULL,false)))
  {
    PLAYER_ERROR("failed to get sonar geometry");
    return(-1);
  }
  else
  {
    // write the geometry inside the driver
    this->sonar_geom = *(player_sonar_geom_t*) msg->GetPayload();
    // calculate the distance from the sonar to the center of the robot (in pixels).
    sonar_dist=(int *)malloc(sonar_geom.poses_count*sizeof(int));
    for(unsigned int s=0;s<this->sonar_geom.poses_count;s++)
    this->sonar_dist[s]=(int)(sqrt(pow(this->sonar_geom.poses[s].px,2) +
				   pow(this->sonar_geom.poses[s].py,2)) /
				   this->map_info.scale);
  }

  // Subscribe to the odometry device
  if(!(this->odom_dev = deviceTable->GetDevice(this->odom_addr)))
  {
    PLAYER_ERROR("unable to locate suitable odometry device");
    return(-1);
  }
  if(this->odom_dev->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to odometry device");
    return(-1);
  }

  // Here you do whatever else is necessary to setup the device, like open and
  // configure a serial port.
    
  puts("Mapping driver ready");

  // Start the device thread; spawns a new thread and executes
  // gridmap::Main(), which contains the main loop for the driver.
  this->StartThread();

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int gridmap::Shutdown()
{
  puts("Shutting example driver down");

  // Stop and join the driver thread
  //  this->StopThread();

  // Unsubscribe from the laser
  this->sonar_dev->Unsubscribe(this->InQueue);

  // Here you would shut the device down by, for example, closing a
  // serial port.

  puts("Mapping driver has been shutdown");

  return(0);
}

// decide which cells are occupied and which cells are free
int gridmap::mapTreshold()
{

    // fill config values in the map
  published_map.width = this->map.width;
  published_map.height = this->map.height;
  published_map.col = 0;
  published_map.row = 0;
  published_map.data_count = this->map.data_count;
  for(unsigned int i=0;i<this->map.width;i++) {
      for (unsigned int j=0;j<this->map.height;j++){
	MAP_POINT x(i,j);
	if(this->map_data.find(x) != this->map_data.end()){
	if (map_data[x].P > this->map_treshold) published_map.data[i+this->map.width*j]=1;
	else  published_map.data[i+this->map.width*j]=0;
	}
      }
    }


#ifdef USEGTK

  gtk_main_quit ();

#endif

  return(1);
}

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                                                                          //
//            UpdateMap function, the real work of this driver              //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

int gridmap::UpdateMap(player_position2d_data_t* odom, player_sonar_data_t* sonar)
{
  /// TODO: choose a update policy
  /// don't overwrite the map if the robot stands still
  if (odom->vel.px == 0 && odom->vel.py == 0 && odom->vel.pa == 0) return(0);

  unsigned int s,x,y;
  int px,py,r,o;
  float th_sonar;
  // compute r * tan(alfa/2), where alfa is the sonar angular aperture
  // and r is the sonar distance
  // using pi/8 for the angular sonar aperture
  for (s=0;s< sonar->ranges_count;s++)
  {
  Sonar sonar_model;
  if(DEBUG) printf("sonar %d, %f\n",s,sonar->ranges[s]);
  r =(int) (sonar->ranges[s]  / this->map_info.scale);
  // if above treshold ignore this sonar reading
  if(sonar->ranges[s] <= 0.001 || sonar->ranges[s] > this->sonar_treshold) continue;
  th_sonar = this->sonar_geom.poses[s].pyaw + odom->pos.pa;
  // using a line 10 times smaller 'cause real sonar wave was too big
  o = (int) (r*tanf(M_PI/16) / 10);
  // px and py are the global coordinates for the starting point of the sonar ray
  // map width/2 and height/2 are added 'cause stage puts 0,0 in the center of the
  // window, maps have 0,0 on the bottom left corner
  px=(int) (odom->pos.px/this->map_info.scale +startx + this->map_info.width/2);
  py=(int) (odom->pos.py/this->map_info.scale +starty + this->map_info.height/2);
//  i=r+this->sonar_dist[s]; // along the sonar ray
  //for(float th=th_sonar-sonar_aperture;th<th_sonar+sonar_aperture;th+=0.01)
  //  {
    float th=th_sonar;
    // for every degree of the arc
    x = (int) LOCAL2GLOBAL_X(r,0,px,py,th);
    y = (int) LOCAL2GLOBAL_Y(r,0,px,py,th);
	//point[0]=x; point[1]=y;
	//int *point,pose;
	MAP_POINT point(x,y);

/*	pose[0]=(double)px;
	pose[1]=(double)py;
	pose[2]=(double)odom->pos.pa;
	pose[3]=sonar_model.sensor_model(x,y,r);*/
	double prob; // probability of point beinng occupied
	if(this->map_data.find(point) != this->map_data.end())
	  { prob=this->map_data[point].P;	} 
	else	{ prob=0.5; }

	// this point can be added on the map		
	MAP_POSE f((double)px,(double)py,(double)odom->pos.pa,
		   prob * // a simple multiplication
		   sonar_model.sensor_model(x,y,r));
	this->map_data[point] = f;
	// put a lousy player map in addition to updating the probability
	//if (MAP_VALID(this->map,x,y)) this->map.data[MAP_INDEX(this->map,x,y)]=1;
	

  }
  return(1);

}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void gridmap::Main() 
{
  // The main loop; interact with the device here
  for(;;)
  {
    // test if we are supposed to cancel
    pthread_testcancel();

    // Process incoming messages.  Calls ProcessMessage() on each pending
    // message.
    this->ProcessMessages();

    // Do work here.  
    this->UpdateMap(&this->last_odom_data,&this->last_sonar_data);
    // Send out new messages with Driver::Publish()
    //

    // Sleep (or you might, for example, block on a read() instead)
    usleep(100000);
  }
  return;
}


int gridmap::ProcessMessage(QueuePointer & resp_queue, 
                                player_msghdr * hdr, 
                                void * data)
{ //puts("gridmap processing messages..");
  // Handle new data from the sonar
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_SONAR_DATA_RANGES, 
                           this->sonar_addr))
  {
    // Put the sonar data in the last_sonar_data variable
    this->last_sonar_data = * (player_sonar_data_t *)data;
    /*  uncomment this to see sonar data printed on the console.
        should set a debug level for this
    puts("Sonar Data Received.");  
    for(int i=0;i<sonar_data->ranges_count;i++)
      {
       printf(" %d:%f ",i,sonar_data->ranges[i]);
      }
    */
    return(0);
  }

  // Handle new data from the position2d
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                              PLAYER_POSITION2D_DATA_STATE, this->odom_addr))
  {
    // Put the odometry data in the last_odom_data variable
    this->last_odom_data = * (player_position2d_data_t *)data;
    return(0);
  }


  // Is it a request for the map info?
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, 
			    PLAYER_MAP_REQ_GET_INFO, 
			    this->map_addr) )
  {
    this->Publish(this->map_addr, 
                 resp_queue, 
                 PLAYER_MSGTYPE_RESP_ACK, 
                 PLAYER_MAP_DATA_INFO,
                 &this->map_info, 
                 sizeof(player_map_info_t),
                 NULL);
    return(0);
  }

  // Is it a request for the map data?
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, 
			    PLAYER_MAP_REQ_GET_DATA, 
			    this->map_addr) )
  {
    this->mapTreshold();
    player_map_data_t* mapreq = (player_map_data_t*)data;

    // Can't declare a map tile on the stack (it's too big)
    /*
    size_t mapsize = (sizeof(player_map_data_t) - PLAYER_MAP_MAX_TILE_SIZE +
                      (mapreq->width * mapreq->height));
                      */
    size_t mapsize = sizeof(player_map_data_t);
    player_map_data_t* mapresp = (player_map_data_t*)calloc(1,mapsize);
    assert(mapresp);

    int i, j;
    int oi, oj, si, sj;

    // Construct reply
    oi = mapresp->col = mapreq->col;
    oj = mapresp->row = mapreq->row;
    si = mapresp->width = mapreq->width;
    sj = mapresp->height = mapreq->height;
    mapresp->data_count = mapresp->width * mapresp->height;
    mapresp->data = new int8_t [mapresp->data_count];
    // Grab the pixels from the map
    for(j = 0; j < sj; j++)
    {
      for(i = 0; i < si; i++)
      {
        if(MAP_VALID(this->map, i + oi, j + oj))
          mapresp->data[i + j * si] = this->published_map.data[MAP_INDEX(this->map, i+oi, j+oj)];
        else
        {
          PLAYER_WARN2("requested cell (%d,%d) is offmap", i+oi, j+oj);
          mapresp->data[i + j * si] = 0;
        }
      }
    }

    this->Publish(this->device_addr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_MAP_REQ_GET_DATA,
                  (void*)mapresp);
    delete [] mapresp->data;
    free(mapresp);
    puts("Map data sent!");
    return(0);
  }

  // Tell the caller that you don't know how to handle this message
  return(-1);
}

