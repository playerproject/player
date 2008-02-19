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
 Desc: Driver for the SICK S3000 laser
 Author: Toby Collett (based on lms200 by Andrew Howard)
 Date: 7 Nov 2000
 CVS: $Id$
*/

/** @ingroup drivers Drivers */
/** @{ */
/** @defgroup driver_sicknav200 sicknav200
 * @brief SICK NAV200 laser localisation unit

The sicknav200 driver interfaces to the NAV200 localiation unit and provides the current position
output of the device. 

By default, the driver will enter positioning mode and use the reflectors stored on the nav200.
To map the visible reflectors and store them on the nav200, set the mode property to mapping.
To copy the reflector positions from a vectormap onto the nav200, add a vectormap to the requires
list of the driver and set the mode property to fetch.
To get the reflector positions from the nav200 to display using the vectormap this driver
provides, set the mode to upload. Note: mapping and fetch also provide the reflectors positions
for display, as well as storing them on the nav200.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_position2d
- @ref interface_vectormap

@par Requires

- @ref opaque

@par Configuration requests

- PLAYER_POSITION2D_REQ_GET_GEOM
  
@par Configuration file options

- pose (length tuple)
  - Default: [0.0 0.0 0.0]
  - Pose (x,y,theta) of the laser, relative to its parent object (e.g.,
    the robot to which the laser is attached).

- size (length tuple)
  - Default: [0.15 0.15]
  - Footprint (x,y) of the laser.

- wheelBase (double)
  - Default: -1.0
  - Distance between front and rear wheels. Used to calculate velocities
    when vehicle movement data is provided.
      
@par Example 

@verbatim
driver
(
  name "sicknav200"
  provides ["position2d:0" "vectormap:0"]
  requires ["opaque:0"]
)
driver
(
	name "serialstream"
	provides ["opaque:0"]
	port "/dev/ttyS0"
)

@endverbatim

@author Kathy Fung, Toby Collett, David Olsen, inro technologies

*/
/** @} */
  

  
#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h> // for htons etc

#include <libplayercore/playercore.h>
// #include <replace/replace.h>
extern PlayerTime* GlobalTime;

#include "nav200.h"

#define DEFAULT_SICKNAV200_MODE "positioning"

// The laser device class.
class SickNAV200 : public Driver
{
  public:
    
    // Constructor
    SickNAV200(ConfigFile* cf, int section);
    ~SickNAV200();

    int Setup();
    int Shutdown();

    // MessageHandler
    int ProcessMessage(QueuePointer &resp_queue, 
		       player_msghdr * hdr, 
		       void * data);
  private:

    // Main function for device thread.
    virtual void Main();
    
    // Get device to map reflectors.
    void UpdateMap();
    // Get the reflector positions from the device.
    void GetReflectors();
    // Set the reflector positions.
    void SetReflectors(player_vectormap_layer_data_t* data);
    // Checks if the reflectors stored on the NAV200 are identical to those in the database.
    // If they are not, the reflectors in the database are copied to the NAV200.
    void FetchIfNeeded();
    // Fetch the reflector positions from a provided vectormap.
    player_vectormap_layer_data_t* FetchReflectors();
    // Extract reflector positions from vectormap layer. Returns number of reflectors.
    int InterpretLayerData(player_vectormap_layer_data_t* data, PositionXY* reflectors);
    // Build the well known binary view of the reflector positions.
    void BuildWKB();

  protected:

    // Laser pose in robot cs.
    double pose[3];
    double size[2];
    
    // TODO: Add reflector layer support.
    // Reflector positions.
    PositionXY reflectors[32];
    int numReflectors;
    uint8_t* wkbData;
    uint32_t wkbSize;
    double speed;
    double steeringAngle;
    
    DoubleProperty wheelBase;
    
    // If mode is set to mapping the reflector positions will be mapped,
    // and mode will be automatically set back to positioning.
    StringProperty mode;
    bool fetchOnStart; // If true, fetch reflectors whenever connecting.
    
    // Name of device used to communicate with the laser
    //char *device_name;
    
    // storage for outgoing data
    player_position2d_data_t data_packet;

    // nav200 parameters
    Nav200 Laser;
    int min_radius, max_radius;
    
    // Reflector Map Driver info
    // Provides reflector positions if not mapped by nav200
    Device *reflector_map;
    player_devaddr_t reflector_map_id;
    
    // Velocity Driver info
    Device *velocity;
    player_devaddr_t velocity_id;
    
    // Opaque Driver info
    Device *opaque;
    player_devaddr_t opaque_id;
    
    // Position interface
    player_devaddr_t position_addr;
    // Vector map interface
    player_devaddr_t vectormap_addr;
    // Position interface
    player_devaddr_t debug_addr;
    player_position2d_data_t debug_packet;
    struct timeval previous;
};

// a factory creation function
Driver* SickNAV200_Init(ConfigFile* cf, int section)
{
  return((Driver*)(new SickNAV200(cf, section)));
}

// a driver registration function
void SickNAV200_Register(DriverTable* table)
{
	puts("Registering driver");
  table->AddDriver("sicknav200", SickNAV200_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Error macros
#define RETURN_ERROR(erc, m) {PLAYER_ERROR(m); return erc;}
 
////////////////////////////////////////////////////////////////////////////////
// Constructor
SickNAV200::SickNAV200(ConfigFile* cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN),
	  wheelBase ("wheelbase", -1.0, 0),
      mode ("mode", DEFAULT_SICKNAV200_MODE, 0)
{
  // Create position interface
  if (cf->ReadDeviceAddr(&(this->position_addr), section, 
                       "provides", PLAYER_POSITION2D_CODE, -1, NULL) != 0)
  {
	PLAYER_ERROR("Could not read position interface device address.");
	this->SetError(-1);
	return;
  }  
  if (this->AddInterface(this->position_addr))
  {
	PLAYER_ERROR("Could not add position interface.");
	this->SetError(-1);
	return;
  }
  
  // Create vectormap interface
  if (cf->ReadDeviceAddr(&(this->vectormap_addr), section, 
	                     "provides", PLAYER_VECTORMAP_CODE, -1, NULL) != 0)
  {
	PLAYER_ERROR("Could not read vectormap interface device address.");
	this->SetError(-1);
	return;
  }
  if (this->AddInterface(this->vectormap_addr))
  {
	PLAYER_ERROR("Could not add vectormap interface.");
	this->SetError(-1);
	return;
  }
  
  // Create debug interface
  if (cf->ReadDeviceAddr(&(this->debug_addr), section, 
                       "provides", PLAYER_POSITION2D_CODE, 2, NULL) != 0)
  {
	PLAYER_ERROR("Could not read debug interface device address.");
	this->SetError(-1);
	return;
  }  
  if (this->AddInterface(this->debug_addr))
  {
	PLAYER_ERROR("Could not add debug interface.");
	this->SetError(-1);
	return;
  }

  // Laser geometry.
  this->pose[0] = cf->ReadTupleLength(section, "pose", 0, 0.0);
  this->pose[1] = cf->ReadTupleLength(section, "pose", 1, 0.0);;
  this->pose[2] = cf->ReadTupleLength(section, "pose", 2, 0.0);;
  this->size[0] = 0.15;
  this->size[1] = 0.15;
  
  this->numReflectors = 0;
  this->wkbData = NULL;
  this->wkbSize = 0;
  
  this->speed = 0;
  this->steeringAngle = 0;
  this->fetchOnStart = false;
  
  // Serial port - done in the opaque driver
  //this->device_name = strdup(cf->ReadString(section, "port", DEFAULT_PORT));

  // nav200 parameters, convert to mm
  this->min_radius = static_cast<int> (cf->ReadLength(section, "min_radius", 1) * 1000);
  this->max_radius = static_cast<int> (cf->ReadLength(section, "max_radius", 30) * 1000);
  this->RegisterProperty ("mode", &this->mode, cf, section);
  this->RegisterProperty ("wheelbase", &this->wheelBase, cf, section);
  
  this->opaque = NULL;
  // Must have an opaque device
  PLAYER_MSG0(2, "reading opaque id now");
  if (cf->ReadDeviceAddr(&this->opaque_id, section, "requires",
                       PLAYER_OPAQUE_CODE, -1, NULL) != 0)
  {
	PLAYER_MSG0(2, "No opaque driver specified");
    this->SetError(-1);    
    return;
  }
  PLAYER_MSG0(2, "reading opaque id now");
  
  PLAYER_MSG0(2, "reading velocity id now");
  this->velocity = NULL;
  memset(&this->velocity_id, 0, sizeof(this->velocity_id));
  cf->ReadDeviceAddr(&this->velocity_id, section, "requires",
		  PLAYER_POSITION2D_CODE, -1, NULL);
  
  PLAYER_MSG0(2, "reading reflector map id now");
  this->reflector_map = NULL;
  memset(&this->reflector_map_id, 0, sizeof(this->reflector_map_id));
  cf->ReadDeviceAddr(&this->reflector_map_id, section, "requires",
		  PLAYER_VECTORMAP_CODE, -1, NULL);
  
  gettimeofday(&previous, NULL);
  
  return;
}

SickNAV200::~SickNAV200()
{
  //free(device_name);
	delete [] wkbData;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
int SickNAV200::Setup()
{
  PLAYER_MSG0(2, "NAV200 initialising");
  
  // Subscribe to the opaque device.
  if(Device::MatchDeviceAddress(this->opaque_id, this->position_addr) || Device::MatchDeviceAddress(this->opaque_id, this->vectormap_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return(-1);
  }
  
  if(!(this->opaque = deviceTable->GetDevice(this->opaque_id)))
  {
    PLAYER_ERROR("unable to locate suitable opaque device");
    return(-1);
  }
   
  if(this->opaque->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to opaque device");
    return(-1);
  }
  
  if (this->velocity_id.interf == PLAYER_POSITION2D_CODE) // Velocity is provided.
  {
	if(!(this->velocity = deviceTable->GetDevice(this->velocity_id)))
    {
      PLAYER_ERROR("unable to locate suitable position2d device");
      return(-1);
    }
     
    if(this->velocity->Subscribe(this->InQueue) != 0)
    {
      PLAYER_ERROR("unable to subscribe to position2d device");
      return(-1);
    }
  }
  
  if (this->reflector_map_id.interf == PLAYER_VECTORMAP_CODE) // Reflector positions are provided.
  {
	if(!(this->reflector_map = deviceTable->GetDevice(this->reflector_map_id)))
	{
	  PLAYER_ERROR("unable to locate suitable vectormap device");
	  return(-1);
	}
	   
	if(this->reflector_map->Subscribe(this->InQueue) != 0)
	{
	  PLAYER_ERROR("unable to subscribe to vectormap device");
	  return(-1);
	}
  }
  
  // Open the terminal
  Laser.Initialise(this, opaque, opaque_id);
  PLAYER_MSG0(2, "Laser initialised");

  PLAYER_MSG0(2, "NAV200 ready");

  // Start the device thread
  StartThread();

  return 0;
}



////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int SickNAV200::Shutdown()
{
  // shutdown laser device
  StopThread();

  opaque->Unsubscribe(InQueue);
  
  PLAYER_MSG0(2, "laser shutdown");
  
  return(0);
}


int 
SickNAV200::ProcessMessage(QueuePointer &resp_queue, 
                           player_msghdr * hdr,
                           void * data)
{
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_OPAQUE_DATA_STATE, opaque_id))
  {
    player_opaque_data_t * recv = reinterpret_cast<player_opaque_data_t * > (data);
    memmove(&Laser.receivedBuffer[Laser.bytesReceived], recv->data, recv->data_count);
    Laser.bytesReceived += recv->data_count;
    return 0;
  }

  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, velocity_id))
  {
    player_position2d_data_t * recv = reinterpret_cast<player_position2d_data_t * > (data);
    speed = recv->vel.px;
    steeringAngle = recv->pos.pa;
    return 0;
  }

	
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                 PLAYER_POSITION2D_REQ_GET_GEOM,
                                 this->position_addr))
  {
    player_position2d_geom_t geom={{0}};
    geom.pose.px = this->pose[0];
    geom.pose.py = this->pose[1];
    geom.pose.pyaw = this->pose[2];
    geom.size.sl = this->size[0];
    geom.size.sw = this->size[1];

    this->Publish(this->position_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_POSITION2D_REQ_GET_GEOM,
                  (void*)&geom, sizeof(geom), NULL);
    return(0);
  }
  
  char* layerName = "0"; // Dumb name.
  
	// Request for map info
	if (Message::MatchMessage(hdr,  PLAYER_MSGTYPE_REQ,
	                                PLAYER_VECTORMAP_REQ_GET_MAP_INFO,
	                                this->vectormap_addr))
	{
	  // I am not sure if this and the below block of code leak memory. Based on postgis.cc
	  player_extent2d_t extent;
	  if (numReflectors > 0)
	  {
		  extent.x0 = extent.x1 = reflectors[0].x;
		  extent.y0 = extent.y1 = reflectors[0].y;
		  for (int i = 1; i < numReflectors; i++)
		  {
			  if (reflectors[i].x < extent.x0)
				  extent.x0 = reflectors[i].x;
			  if (reflectors[i].x > extent.x1)
				  extent.x1 = reflectors[i].x;
			  if (reflectors[i].y < extent.y0)
  				  extent.y0 = reflectors[i].y;
  			  if (reflectors[i].y > extent.y1)
  				  extent.y1 = reflectors[i].y;
		  }
		  extent.x0 = extent.x0 / 1000.0 - 1; // Convert to metres, 1 metre as a margin.
		  extent.x1 = extent.x1 / 1000.0 + 1;
		  extent.y0 = extent.y0 / 1000.0 - 1;
		  extent.y1 = extent.y1 / 1000.0 + 1;
	  }
	  else
		  extent.x0 = extent.y0 = extent.x1 = extent.y1 = 0;
	  
	  player_vectormap_layer_info_t* layerInfo = new player_vectormap_layer_info_t;
	  layerInfo->name = strdup(layerName);
	  layerInfo->name_count = strlen(layerInfo->name)+1;
	  layerInfo->extent = extent;
	  
	  static player_vectormap_info_t response;
	  response.srid = 0;
	  response.layers_count = 1;
	  response.layers = layerInfo;
	  response.extent = extent;
	
	  this->Publish(this->vectormap_addr,
	                resp_queue,
	                PLAYER_MSGTYPE_RESP_ACK,
	                PLAYER_VECTORMAP_REQ_GET_MAP_INFO,
	                (void*)&response);
	  return(0);
	}
	// Request for layer data
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
	                           PLAYER_VECTORMAP_REQ_GET_LAYER_DATA,
	                           this->vectormap_addr))
	{
	  char* featureName = "point";
	  player_vectormap_feature_data* feature = new player_vectormap_feature_data;
	  memset(feature,0,sizeof(feature));
	  feature->name = strdup(featureName);
	  feature->name_count = strlen(feature->name)+1;
	  feature->wkb = new uint8_t[wkbSize];
	  feature->wkb_count = wkbSize;
	  for (uint32_t i=0; i < wkbSize; ++i)
		  feature->wkb[i] = wkbData[i];
	  
	  static player_vectormap_layer_data_t response;
	  response.name = strdup(layerName);
	  response.name_count = strlen(response.name)+1;
	  response.features_count = numReflectors > 0; // If we have no data, don't publish a feature.
	  response.features = numReflectors > 0 ? feature : NULL;
	  
	  this->Publish(this->vectormap_addr,
	                resp_queue,
	                PLAYER_MSGTYPE_RESP_ACK,
	                PLAYER_VECTORMAP_REQ_GET_LAYER_DATA,
	                (void*)&response);
	
	  return(0);
	}
	// Write layer data
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
	                               PLAYER_VECTORMAP_REQ_WRITE_LAYER,
		                           this->vectormap_addr))
	{
		player_vectormap_layer_data_t* layerData = reinterpret_cast<player_vectormap_layer_data_t*>(data);
		
		SetReflectors(layerData);
		
		// Does the ack need the layer data?
		this->Publish(this->vectormap_addr,
	                  resp_queue,
	                  PLAYER_MSGTYPE_RESP_ACK,
	                  PLAYER_VECTORMAP_REQ_WRITE_LAYER,
	                  (void*)layerData,
	                  sizeof(player_vectormap_layer_data_t),
	                  NULL);
		
		return 0;
	}
	

	if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_STRPROP_REQ, this->vectormap_addr) ||
			Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_STRPROP_REQ, this->position_addr))
	{
	    player_strprop_req_t req = *reinterpret_cast<player_strprop_req_t*> (data);
	    PLAYER_MSG1(2, "%s", req.key);
		if (strcmp("mode", req.key) == 0)
	    {
	    	mode.SetValueFromMessage(reinterpret_cast<void*> (&req));
	    	if (strncmp(mode, "mapping", 7) == 0)
	    	{
	    		UpdateMap();
	    		mode.SetValue("positioning"); // Automatically return to positioning mode.
	    	}
	    	else if (strncmp(mode, "fetch", 5) == 0)
	    	{
	    		SetReflectors(FetchReflectors()); // Fetch reflectors from database
	    		mode.SetValue("positioning");
	    	}
	    	else if (strncmp(mode, "upload", 5) == 0)
	    	{
	    		GetReflectors(); // Get reflectors from NAV200
	    		mode.SetValue("positioning");
	    	}
	    	else if (strncmp(mode, "positioning", 11) == 0)
	    	{
	    		// Default.
	    	}
	    	else
	    	{
	    		PLAYER_ERROR1("Unrecognised mode: %s", mode.GetValue());
		  	    this->Publish(hdr->addr,
		  	                resp_queue,
		  	                PLAYER_MSGTYPE_RESP_NACK,
		  	                PLAYER_SET_STRPROP_REQ);
	    	}
	  	    this->Publish(hdr->addr,
	  	                resp_queue,
	  	                PLAYER_MSGTYPE_RESP_ACK,
	  	                PLAYER_SET_STRPROP_REQ);
	    	return 0;
	    }
	}

  // Don't know how to handle this message.
  return(-1);
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void SickNAV200::Main() 
{
  if (!Laser.EnterStandby())
  {
      PLAYER_ERROR("unable to enter standby mode\n");
  }
  if (!Laser.SetReflectorRadius(0, 45))
  {
      PLAYER_ERROR("unable to set reflector radius\n");
      return ;
  }
  
  if (!Laser.EnterPositioning())
  {
      PLAYER_ERROR("unable to enter position mode\n");
      return ;
  }
  if (!Laser.SetActionRadii(min_radius, max_radius))
  {
      PLAYER_ERROR("failed to set action radii\n");
      return ;
  }
  
  BuildWKB(); // Build an empty WKB.
  
  PLAYER_MSG1(2, "Wheel base: %lf", wheelBase.GetValue());
  
  if (strncmp(mode, "fetch", 5) == 0)
  {
	fetchOnStart = true;
    FetchIfNeeded();
    mode.SetValue("positioning");
  }
  
  LaserPos Reading;
  for(;;)
  {
    // test if we are supposed to cancel
    pthread_testcancel();
    
    // process any pending messages
    ProcessMessages();
    
    bool gotReading;
    if (velocity)
    {
    	if (wheelBase.GetValue() <= 0)
    	{
    		PLAYER_WARN("vehicle movement data provided but wheelbase not set. Add wheelbase to the NAV config file");
    		gotReading = Laser.GetPositionAuto(Reading);
    	}
    	else
    	{
	    	// Convert vehicle movement data into NAV200 format and coordinates.
	    	//double arcRadius = wheelBase / tan(steeringAngle);
	    	//double angularVelocity = speed / arcRadius;
	    	double angularVelocity = speed * tan(steeringAngle) / wheelBase.GetValue();
	    	double velX = speed - pose[1] * angularVelocity;
	    	double velY = pose[0] * angularVelocity;
	    	double navVelX = velX * cos(pose[2]) + velY * sin(pose[2]);
	    	double navVelY = velY * cos(pose[2]) - velX * sin(pose[2]);
	    	debug_packet.vel.pa = angularVelocity;
	    	debug_packet.vel.px = navVelX;
	    	debug_packet.vel.py = navVelY;
	    	debug_packet.stall = 0;
	    	this->Publish(this->debug_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, (void*)&debug_packet, sizeof(debug_packet), NULL);
	    	gotReading = Laser.GetPositionSpeedVelocity(short(navVelX * 1000), short(navVelY * 1000), short(angularVelocity * 32768.0 / M_PI), Reading);
    	}
    }
    else
    	gotReading = Laser.GetPositionAuto(Reading);
    
    // get update and publish result
    if(gotReading)
    {
      // Use NAV200 position and orientation data to determine vehicle position and orientation.
      double angle = Reading.orientation/32768.0*M_PI - pose[2];
      double forwardx = cos(angle);
      double forwardy = sin(angle);
      double leftx = -sin(angle);
      double lefty = cos(angle);
      double newAngle = atan2(forwardy, forwardx);
      double newX = static_cast<double> (Reading.pos.x)/1000 - forwardx * pose[0] - leftx * pose[1];
      double newY = static_cast<double> (Reading.pos.y)/1000 - forwardy * pose[0] - lefty * pose[1];
      
      // Begin debug stuff
      struct timeval current;
      gettimeofday(&current, NULL);
      double dt = static_cast<double> (current.tv_sec - previous.tv_sec) + static_cast<double>(current.tv_usec - previous.tv_usec) / 1000000.0;
      
      double angVel = (newAngle - data_packet.pos.pa) / dt;
      double velX = (newX - data_packet.pos.px) / dt;
      double velY = (newY - data_packet.pos.py) / dt;
      PLAYER_MSG3(2, "Vx: %lf\tVy: %lf\tVa: %lf", velX, velY, angVel);
      data_packet.vel.pa = angVel;
      data_packet.vel.px = velX;
      data_packet.vel.py = velY;
      // End debug stuff
      
      data_packet.pos.pa = newAngle;
      data_packet.pos.px = newX;
      data_packet.pos.py = newY;
      if(Reading.quality==0xFF || Reading.quality==0xFE || Reading.quality==0x00)
      {
	data_packet.stall = 1;
      }
      else
      {
	data_packet.stall = 0;
      }

      this->Publish(this->position_addr,
                   PLAYER_MSGTYPE_DATA,
                   PLAYER_POSITION2D_DATA_STATE,
                   (void*)&data_packet, sizeof(data_packet), NULL);
    }
    else
    {
      PLAYER_WARN("Failed to get reading from laser scanner\n");
      sleep(1);
      // May have been disconnected. Attempt to return to positioning mode.
	  if (fetchOnStart)
		FetchIfNeeded();
    }
  }
}

void SickNAV200::UpdateMap()
{
	if (!Laser.EnterStandby())
	{
		PLAYER_ERROR("Unable to enter standby mode.\n");
	}
	
	PLAYER_MSG0(2, "Deleting old reflectors.");
	PositionXY pos; // Just somewhere for delete to output the position to.
	while(Laser.DeleteReflectorPosition(0,0,pos))
	{
		PLAYER_MSG0(4, "Deleted a reflector.");
	}
	
	if (!Laser.EnterMapping())
	{
		PLAYER_ERROR("Unable to enter mapping mode.\n");
		return;
	}
	// Map the reflectors.
	PLAYER_MSG0(2, "Started mapping.");
	numReflectors = Laser.StartMapping(0, 0, 0, 0, 45); // Radius may not be needed.
	PLAYER_MSG1(2, "Mapped %d reflectors.", numReflectors);
	if (numReflectors > 32)
		PLAYER_ERROR("More reflectors mapped than NAV200 supports.\n");
	
	if (numReflectors < 0)
	{
		PLAYER_ERROR("Reflector mapping failed.\n");
		return;
	}

	for (uint8_t i = 0; i < numReflectors; i++)
	{
		if (!Laser.MappingPosition(0, i, reflectors[i]))
			PLAYER_ERROR1("Failed to get reflector %d\n", i);
		else
			PLAYER_MSG2(4, "Got reflector. X = %d, Y = %d", reflectors[i].x, reflectors[i].y);
	}
	
	if (!Laser.EnterStandby())
	{
		PLAYER_ERROR("Unable to return to standby mode after mapping.\n");
		return;
	}
	
	PLAYER_MSG0(2, "Inserting reflectors.");
	for (int i = 0; i < numReflectors; i++)
	{
		if (!Laser.InsertReflectorPosition(0,i,reflectors[i].x,reflectors[i].y))
		{
			PLAYER_ERROR1("Unable to insert reflector %d.\n", i);
			return;
		}
	}
	
	BuildWKB(); // Update wkb to show new reflectors.
	
	if (!Laser.EnterPositioning())
	{
		PLAYER_ERROR("Unable to return to positioning mode after mapping.\n");
		return;
	}
	
	PLAYER_MSG0(2, "Mapping complete.");
}

void SickNAV200::GetReflectors()
{
	PLAYER_MSG0(2, "Uploading reflectors.");
	
	if (!Laser.EnterStandby())
	{
		PLAYER_ERROR("Unable to enter standby mode.\n");
	}
	
	if (!Laser.EnterUpload())
	{
		PLAYER_ERROR("Unable to enter upload mode.\n");
		return;
	}
	
	numReflectors = 0;
	ReflectorData reflector;
	while (true)
	{
		if (!Laser.GetUploadTrans(0, reflector))
		{
			PLAYER_ERROR("Error getting reflector.\n");
			return;
		}
		if (reflector.number <= 32) // If it's a valid reflector.
		{
			PLAYER_MSG1(4, "Reflector %d", reflector.number);
			reflectors[reflector.number] = reflector.pos;
			if (reflector.number >= numReflectors)
				numReflectors = reflector.number + 1;
		}
		else
			break;
	}
	
	BuildWKB();
	
	if (!Laser.EnterStandby())
	{
		PLAYER_ERROR("Unable to return to standby mode after getting reflectors.\n");
	}
	
	if (!Laser.EnterPositioning())
	{
		PLAYER_ERROR("Unable to return to positioning mode after getting reflectors.\n");
		return;
	}
}

const unsigned wkbHeaderSize = 9;
const unsigned wkbPointSize = 21;

int SickNAV200::InterpretLayerData(player_vectormap_layer_data_t* data, PositionXY* reflectors)
{
	int numReflectors = 0;
	for (unsigned f = 0; f < data->features_count; f++)
	{
		player_vectormap_feature_data_t feature = data->features[f];
		uint8_t* wkb = feature.wkb;
		if (feature.wkb_count < wkbHeaderSize)
		{
			PLAYER_WARN("WKB too small in InterpretLayerData\n");
			continue;
		}
		if (wkb[0] == 0)
		{
			PLAYER_WARN("InterpretLayerData does not support big endian wkb data\n");
			continue;
		}
		uint32_t type = *reinterpret_cast<uint32_t*>(wkb + 1);
		bool extendedWKB = type >> 24 == 32; // Extended WKBs seem to store a flag in the type variable.
		int headerSize = extendedWKB ? wkbHeaderSize + 4 : wkbHeaderSize;
		if (type & 0xffffff != 4) // Ignore the most significant byte, it might have a flag in.
		{
			PLAYER_WARN1("InterpretLayerData only supports MultiPoint data %d\n", *reinterpret_cast<uint32_t*>(wkb + 1));
			continue;
		}
		unsigned reflectorsInFeature = 0;
		if (extendedWKB)
		{
			unsigned spacialReferenceID = *reinterpret_cast<uint32_t*>(wkb + 5);
			reflectorsInFeature = *reinterpret_cast<uint32_t*>(wkb + 9);
		}
		else
			reflectorsInFeature = *reinterpret_cast<uint32_t*>(wkb + 5);
		if (!reflectorsInFeature)
			continue;
		if (feature.wkb_count != headerSize + wkbPointSize * reflectorsInFeature)
		{
			PLAYER_WARN("Unexpected WKB size in InterpretLayerData\n");
			continue;
		}
		
		// Expand the reflectors array
		int startIndex = numReflectors;
		numReflectors += reflectorsInFeature;
		if (numReflectors > 32)
			PLAYER_ERROR("More reflectors passed than NAV200 supports\n");
		
		// Copy in new reflectors
		for (unsigned r = 0; r < reflectorsInFeature; r++)
		{
			uint8_t* pointData = wkb + headerSize + wkbPointSize * r;
			if (pointData[0] == 0)
				PLAYER_ERROR("InterpretLayerData does not support big endian wkb data, let alone inconsistently\n");
			if (*reinterpret_cast<uint32_t*>(pointData + 1) != 1)
				PLAYER_ERROR("Malformed wkb data, expected point\n");
			reflectors[startIndex + r].x = int(*reinterpret_cast<double*>(pointData + 5) * 1000.0);
			reflectors[startIndex + r].y = int(*reinterpret_cast<double*>(pointData + 13) * 1000.0);
		}
	}
	return numReflectors;
}

void SickNAV200::SetReflectors(player_vectormap_layer_data_t* data)
{
	PLAYER_MSG0(2, "Downloading reflectors.");
	
	numReflectors = InterpretLayerData(data, reflectors);
	
	BuildWKB(); // Might be something odd about the passed wkb, so build it the usual way.
	
	// Download the reflectors to the device.
	
	if (!Laser.EnterStandby())
		PLAYER_ERROR("Unable to enter standby mode.\n");
	
	if (!Laser.EnterDownload())
	{
		PLAYER_ERROR("Unable to enter download mode.\n");
		return;
	}
	
	for (int r = 0; r < numReflectors; r++)
		Laser.DownloadReflector(0, r, reflectors[r].x, reflectors[r].y);
	Laser.DownloadReflector(0, -1, 0, 0); // Let the NAV know that's all of them.
	
	if (!Laser.EnterStandby())
		PLAYER_ERROR("Unable to return to standby mode after getting reflectors.\n");
	
	if (!Laser.EnterPositioning())
		PLAYER_ERROR("Unable to return to positioning mode after getting reflectors.\n");
}

void SickNAV200::FetchIfNeeded()
{
	// Check if reflectors are correct.
	player_vectormap_layer_data_t* dbData = FetchReflectors();
	PositionXY dbReflectors[32];
	int numDBReflectors = InterpretLayerData(dbData, dbReflectors);
	GetReflectors();
	// Determine if db and nav reflectors are identical.
	bool same = numDBReflectors == numReflectors;
	for (int i = 0; i < numReflectors && same; i++)
		same = reflectors[i].x == dbReflectors[i].x && reflectors[i].y == dbReflectors[i].y;
	if (!same) // If the reflectors are different.
	{
		PLAYER_MSG0(2, "Updating reflectors.");
		SetReflectors(dbData); // Update the nav200 ones.
	}
	else
		PLAYER_MSG0(2, "No reflector update needed.");
}

player_vectormap_layer_data_t* SickNAV200::FetchReflectors()
{
	player_vectormap_layer_data_t* layerData = NULL;
	PLAYER_MSG0(2, "Fetching reflectors from vectormap");
	if (reflector_map)
	{
		bool gotReflectors = false;
		
		Message* mapInfoMessage = reflector_map->Request(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_VECTORMAP_REQ_GET_MAP_INFO, NULL, 0, NULL);
		if (mapInfoMessage->GetHeader()->type == PLAYER_MSGTYPE_RESP_ACK && mapInfoMessage->GetHeader()->subtype == PLAYER_VECTORMAP_REQ_GET_MAP_INFO)
		{
			player_vectormap_info_t* mapInfo = reinterpret_cast<player_vectormap_info_t*>(mapInfoMessage->GetPayload());
			
			if (mapInfo->layers_count == 1)
			{
				player_vectormap_layer_info_t layer = mapInfo->layers[0];
				player_vectormap_layer_data_t request;
				memset(&request, 0, sizeof(request));
				request.name = layer.name;
				request.name_count = layer.name_count;
			  
				Message* response = reflector_map->Request(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_VECTORMAP_REQ_GET_LAYER_DATA, (void*)&request, 0, NULL);
				if (response->GetHeader()->type == PLAYER_MSGTYPE_RESP_ACK && response->GetHeader()->subtype == PLAYER_VECTORMAP_REQ_GET_LAYER_DATA)
				{
					layerData = reinterpret_cast<player_vectormap_layer_data_t*>(response->GetPayload());
					gotReflectors = true;
				}
			}
		}
		
		if (!gotReflectors)
			PLAYER_WARN("failed to get reflectors from vectormap\n");
	}
	else
		PLAYER_WARN("no vectormap provided to fetch reflectors from\n");
	return layerData;
}

void SickNAV200::BuildWKB()
{
	// Encode reflector positions in well known binary format for publishing.
	delete [] wkbData; // Clear old data.
	wkbSize = wkbHeaderSize + wkbPointSize * numReflectors;
	wkbData = new uint8_t[wkbSize];
	wkbData[0] = 1; // Little endian (0 for big endian).
	*reinterpret_cast<uint32_t*>(wkbData + 1) = 4; // MultiPoint
	*reinterpret_cast<uint32_t*>(wkbData + 5) = numReflectors; // Num points.
	for (int i = 0; i < numReflectors; i++)
	{
		uint8_t* pointData = wkbData + wkbHeaderSize + wkbPointSize * i;
		pointData[0] = 1; // Still little endian.
		uint32_t wkbType = 1; // Point
		*reinterpret_cast<uint32_t*>(pointData + 1) = wkbType;
		double x = reflectors[i].x / 1000.0; // mm to metres.
		double y = reflectors[i].y / 1000.0;
		*reinterpret_cast<double*>(pointData + 5) = x;
		*reinterpret_cast<double*>(pointData + 13) = y;
	}
}
