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

///////////////////////////////////////////////////////////////////////////
//
// File: lsd.cc
// Author: Reed Hedges, LPR, Dept. of Computer Science, UMass, Amherst
// Date: 23 June 2003
///////////////////////////////////////////////////////////////////////////

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_service_adv_lsd service_adv_lsd

@deprecated 

Driver for libservicediscovery.   This driver is deprecated, since 
libservicediscovery is no longer maintained, and was never very good
to begin with. Try the MDNS service discovery driver instead (player_driver_service_adv_mdns).

This driver has no client proxy. The driver responds to queries in a background
thread when loaded and initialized.

The service is advertised using a URL in this format: player://<hostname>:<port>.
In addition to any service_tags given in the configuration file, a tag is added
for each device currently loaded, in the format: <device name>#<index>(<driver name).

@par Compile-time dependencies

- <a href=http://interreality.org/software/servicediscovery>libservicediscovery</a>

@par Provides

- none

@par Requires

- None

@par Configuration requests

- none

@par Configuration file options

- none
 
@par Example 

@verbatim
driver
(
  name "service_adv_lsd"
  service_name "MyRobot"
  service_description "This is my groovy robot."
  service_tags [ "strength=12" "intelligence=5" "dexterity=9" ]
)
@endverbatim

@par Authors

Reed Hedges

*/
/** @} */

#if HAVE_CONFIG_H
# include <config.h>
#endif

//#ifdef HAVE_UNISTD_H
# include <unistd.h>
//#endif

#include <string>
#include <iostream>
using namespace std;

#include <servicediscovery/servicedirectory.hh>
using namespace LSD;

#include "playercommon.h"
#include "drivertable.h"
#include "deviceregistry.h"
#include "devicetable.h"
#include "player.h"


class SrvAdv_LSD : public Driver {
  private:
    // LSD objects
    ServiceDirectory* serviceDir;
    Service service;

    // Main function for device thread.
    virtual void Main();


  public:
    
    // Constructor
    SrvAdv_LSD( ConfigFile* cf, int section);

    // Destructor
    virtual ~SrvAdv_LSD();

    // Create service directory, find values, and add this service to it.
    virtual void Prepare();
   
    
    // called when a client (the first client?) connects
    virtual int Setup() {
        return 0;
    }

    // called when a client (the last client?) disconnects
    virtual int Shutdown() {
        return 0;
    }

    // you could stop the device here if you want but only the destructor
    // currently calls this
    void stop();
};



Driver* SrvAdv_LSD_Init( ConfigFile* cf, int section) {
    SrvAdv_LSD* dev = new SrvAdv_LSD( cf, section);
    return dev;
}

// a driver registration function
void ServiceAdvLSD_Register(DriverTable* table)
{
  //printf("player: service_adv_lsd register function called.\n");
  table->AddDriver("service_adv_lsd",  SrvAdv_LSD_Init);
}


SrvAdv_LSD::~SrvAdv_LSD() {
    stop();
}

// Constructor
SrvAdv_LSD::SrvAdv_LSD( ConfigFile* configFile, int configSection)
    : Driver(configFile, configSection, 
             PLAYER_SERVICE_ADV_CODE, PLAYER_READ_MODE, 0,0,0,0)
{
    alwayson = true;      // since there is no client interface

    service.url = configFile->ReadString(configSection, "url", ""); // will be detected in Prepare() if ""
    service.title = configFile->ReadString(configSection, "service_name", "robot");
    service.description = configFile->ReadString(configSection, "service_description", "Player Robot Server");   

    // read types from player config file
    service.types.insert("player");
    const char* x;
    for(int i = 0; (x = configFile->ReadTupleString(configSection, "service_tags", i, 0)); i++) {
        service.types.insert(string(x));
    }

}

void SrvAdv_LSD::Prepare() {

    // add a tag for each device in the device table
    for(Device* dev = deviceTable->GetFirstDevice(); dev != 0; dev = deviceTable->GetNextDevice(dev)) {
        char* devname = lookup_interface_name(0, dev->id.code);
        if(devname) {
            char deviceTag[512];
            snprintf(deviceTag, sizeof(deviceTag), "device:%s#%d(%s)", devname, dev->id.index, dev->drivername);
            service.types.insert(deviceTag);
        }
    }

    // if url wasn't set in the config file, set it here
    if(service.url == "") {
        char host[128];
        gethostname(host, sizeof(host));
        char url[256];
        snprintf(url, sizeof(url), "player://%s:%d", host, device_id.port);
        service.url = url;
    }

    /*
    cerr << "player: service_adv_lsd: url is: " << service.url << endl;
    cerr << "player: service_adv_lsd: title (name) is: " << service.title << endl;
    cerr << "player: service_adv_lsd: description is: " << service.description << endl;
    cerr << "player: service_adv_lsd: there are " << service.types.size() << " type tags.\n";
    */

    // create service directory and advertise service
    serviceDir = new ServiceDirectory();
    serviceDir->addService(&service);

    // start thread
    StartThread();
}


// stop the service directory
void SrvAdv_LSD::stop()
{
    StopThread();
    serviceDir->removeService(&service);
    delete serviceDir;
}


// Main function for device thread
void SrvAdv_LSD::Main() 
{
    while(true) {
        serviceDir->handleIncoming();
        cerr << "service_adv_lsd: pong\n";
    }
}


