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
// Desc: Driver for libservicediscovery.
//       Based in part on Andrew Howard's SICK laser driver.
//       Requires libservicediscovery, available at:
//       http://interreality.org/software/servicediscovery
//
//       Note that the service_adv device has no client proxy, and 
//       clients cannot "subscribe" and read from it: the Init function
//       just starts the man thread and it runs "in the background" forever.
//
///////////////////////////////////////////////////////////////////////////

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
    if(strcmp(interface, PLAYER_SERVICE_ADV_STRING)) {
        PLAYER_ERROR1("driver \"service_adv_lsd\" does not support interface \"%s\"\n", interface);
        return(0);
    }
    SrvAdv_LSD* dev = new SrvAdv_LSD(interface, cf, section);
    return dev;
}

// a driver registration function
void ServiceAdvLSD_Register(DriverTable* table)
{
  printf("player: service_adv_lsd register function called.\n");
  table->AddDriver("service_adv_lsd", PLAYER_ALL_MODE, SrvAdv_LSD_Init);
}


SrvAdv_LSD::~SrvAdv_LSD() {
    stop();
}

// Constructor
SrvAdv_LSD::SrvAdv_LSD( ConfigFile* configFile, int configSection)
    : 
    Driver(cf, section, 0,0,0,0)
{
    alwayson = true;      // since there is no client interface

    service.url = configFile->ReadString(configSection, "url", ""); // will be detected in Prepare() if ""
    service.title = configFile->ReadString(configSection, "name", "robot");
    service.description = configFile->ReadString(configSection, "description", "Player Robot Server");   

    // read types from player config file
    service.types.insert("player");
    const char* x;
    for(int i = 0; (x = configFile->ReadTupleString(configSection, "tags", i, 0)); i++) {
        service.types.insert(string(x));
    }

}

void SrvAdv_LSD::Prepare() {

    // add a tag for each device in the device table
    for(DriverEntry* dev = deviceTable->GetFirstEntry(); dev != 0; dev = deviceTable->GetNextEntry(dev)) {
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


