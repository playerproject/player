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

Currently the driver assumes the nav200 has been correctly initialised and loaded with the 
reflector layers.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_laser

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
      
@par Example 

@verbatim
driver
(
  name "sicknav200"
  provides ["position2d:0"]
  provides ["vectormap:0"]
  requires ["opaque:0"]
)
driver
(
	name "serialstream"
	provides ["opaque:0"]
	port "/dev/ttyS0"
)

@endverbatim

@author Kathy Fung, Toby Collett, inro technologies

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
    
    // Add new reflectors.
    void UpdateMap();
    // Get the reflector positions from the device.
    void GetReflectors();
    // Build the well known binary view of the reflector positions.
    void BuildWKB();

  protected:

    // Laser pose in robot cs.
    double pose[3];
    double size[2];
    
    // Reflector positions.
    PositionXY* reflectors;
    int numReflectors;
    uint8_t* wkbData;
    int wkbSize;
    
    // If mode is set to mapping the reflector positions will be mapped,
    // and mode will be automatically set back to positioning.
    StringProperty mode;
    
    // Name of device used to communicate with the laser
    //char *device_name;
    
    // storage for outgoing data
    player_position2d_data_t data_packet;

    // nav200 parameters
    Nav200 Laser;
    int min_radius, max_radius;
    
    // Opaque Driver info
    Device *opaque;
    player_devaddr_t opaque_id;
    
    // Position interface
    player_devaddr_t position_addr;
    // Vector map interface
    player_devaddr_t vectormap_addr;
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

  // Laser geometry.
  this->pose[0] = cf->ReadTupleLength(section, "pose", 0, 0.0);
  this->pose[1] = cf->ReadTupleLength(section, "pose", 1, 0.0);;
  this->pose[2] = cf->ReadTupleLength(section, "pose", 2, 0.0);;
  this->size[0] = 0.15;
  this->size[1] = 0.15;
  
  this->reflectors = NULL;
  this->numReflectors = 0;
  this->wkbData = NULL;
  this->wkbSize = 0;
  
  // Serial port - done in the opaque driver
  //this->device_name = strdup(cf->ReadString(section, "port", DEFAULT_PORT));

  // nav200 parameters, convert to cm
  this->min_radius = static_cast<int> (cf->ReadLength(section, "min_radius", 1) * 1000);
  this->max_radius = static_cast<int> (cf->ReadLength(section, "max_radius", 30) * 1000);
  this->RegisterProperty ("mode", &this->mode, cf, section);
  
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
  
  return;
}

SickNAV200::~SickNAV200()
{
  //free(device_name);
	delete [] reflectors;
	delete [] wkbData;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
int SickNAV200::Setup()
{
  PLAYER_MSG0(2, "NAV200 initialising");
  
  // Subscribe to the opaque device.
  if(Device::MatchDeviceAddress(this->opaque_id, this->device_addr))
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
  
  static char layerName[5] = "0"; // Dumb name.
  
	// Request for map info
	if (Message::MatchMessage(hdr,  PLAYER_MSGTYPE_REQ,
	                                PLAYER_VECTORMAP_REQ_GET_MAP_INFO,
	                                this->vectormap_addr))
	{
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
	  
	  static player_vectormap_layer_info_t layerInfo;
	  layerInfo.name = layerName;
	  layerInfo.name_count = strlen(layerInfo.name)+1;
	  layerInfo.extent = extent;
	  
	  player_vectormap_info_t response;
	  response.srid = 0;
	  response.layers_count = 1;
	  response.layers = &layerInfo;
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
	  static char featureName[6] = "point";
	  player_vectormap_feature_data feature;
	  memset(&feature,0,sizeof(feature));
	  feature.name = featureName;
	  feature.name_count = strlen(feature.name)+1;
	  feature.wkb = wkbData;
	  feature.wkb_count = wkbSize;
	  
	  player_vectormap_layer_data_t response;
	  response.name = layerName;
	  response.name_count = strlen(response.name)+1;
	  response.features_count = wkbSize > 0; // If we have no data, dont' publish a feature.
	  response.features = &feature;
	  
	  this->Publish(this->vectormap_addr,
	                resp_queue,
	                PLAYER_MSGTYPE_RESP_ACK,
	                PLAYER_VECTORMAP_REQ_GET_LAYER_DATA,
	                (void*)&response);
	
	  return(0);
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
	    		mode.SetValue("positioning"); // Automatically return to positioning.
	    	}
	    	else if (strncmp(mode, "positioning", 11) == 0)
	    	{
	    		// Default.
	    	}
	    	else
	    	{
	    		PLAYER_ERROR1("Unrecognised mode: %s", mode.GetValue());
	    	}
	    	
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
  
  // Download the reflector positions in case the vectormap is subscribed to.
  GetReflectors();
	
  LaserPos Reading;
  for(;;)
  {
    // test if we are supposed to cancel
    pthread_testcancel();
    
    // process any pending messages
    ProcessMessages();
    
    // get update and publish result
    if(Laser.GetPositionAuto(Reading))
    {
      data_packet.pos.px = static_cast<double> (Reading.pos.x)/1000;
      data_packet.pos.py = static_cast<double> (Reading.pos.y)/1000;
      double angle = M_PI + Reading.orientation/32768.0*M_PI;
      data_packet.pos.pa = atan2(sin(angle), cos(angle));
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
      usleep(100000);
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
	
	if (numReflectors < 0)
	{
		PLAYER_ERROR("Reflector mapping failed.\n");
		return;
	}

	delete [] reflectors; // Clear old reflector positions.
	reflectors = new PositionXY[numReflectors];
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
	
	delete [] reflectors; // Clear old reflector positions.
	reflectors = new PositionXY[32]; // Max 32 reflectors in a layer.
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

void SickNAV200::BuildWKB()
{
	// Encode reflector positions in well known binary format for publishing.
	delete [] wkbData; // Clear old data.
	wkbSize = 9 + 21 * numReflectors;
	wkbData = new uint8_t[wkbSize];
	wkbData[0] = 1; // Little endian (0 for big endian).
	*reinterpret_cast<uint32_t*>(wkbData + 1) = 4; // MultiPoint
	*reinterpret_cast<uint32_t*>(wkbData + 5) = numReflectors; // Num points.
	for (int i = 0; i < numReflectors; i++)
	{
		wkbData[9 + 21 * i] = 1; // Still little endian.
		uint32_t wkbType = 1; // Point
		*reinterpret_cast<uint32_t*>(wkbData + 9 + 21 * i + 1) = wkbType;
		double x = reflectors[i].x / 1000.0; // mm to metres.
		double y = reflectors[i].y / 1000.0;
		*reinterpret_cast<double*>(wkbData + 9 + 21 * i + 5) = x;
		*reinterpret_cast<double*>(wkbData + 9 + 21 * i + 13) = y;
	}
}
