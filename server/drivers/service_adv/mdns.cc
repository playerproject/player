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
// File: mdns.cc
// Author: Reed Hedges, LPR, Dept. of Computer Science, UMass, Amherst
// Date: 23 June 2003
// Desc: Service advertisement driver for howl MDNS (Zeroconf aka "Rendezvous").
//       Requires libhowl, available at http://www.porchdogsoft.com
//
//       Note that the service_adv device has no client proxy, and 
//       clients cannot "subscribe" and read from it: the Init function
//       just starts the Howl threads and they run "in the background" forever.
//
///////////////////////////////////////////////////////////////////////////

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <string>
#include <set>

#include <howl.h>

#include "playercommon.h"
#include "drivertable.h"
#include "deviceregistry.h"
#include "devicetable.h"
#include "player.h"


#define MDNS_SERVICE_TYPE "_player._tcp."

class SrvAdv_MDNS : public CDevice {
  private:
    // MDNS objects
    sw_discovery howl_client;
    sw_discovery_publish_id id;
    std::set<std::string> extra_txt;
    std::string name, description;

  public:
    SrvAdv_MDNS(char* interface, ConfigFile* cf, int section);
    virtual ~SrvAdv_MDNS();

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

    // Main function for device thread
    virtual void Main();
};



CDevice* SrvAdv_MDNS_Init(char* interface, ConfigFile* cf, int section) {
    if(strcmp(interface, PLAYER_SERVICE_ADV_STRING)) {
        PLAYER_ERROR1("driver \"service_adv_mdns\" does not support interface \"%s\"\n", interface);
        return(0);
    }
    return new SrvAdv_MDNS(interface, cf, section);
}

// a driver registration function
void ServiceAdvMDNS_Register(DriverTable* table)
{
  table->AddDriver("service_adv_mdns", PLAYER_ALL_MODE, SrvAdv_MDNS_Init);
}


SrvAdv_MDNS::~SrvAdv_MDNS() {
    stop();
    sw_discovery_fina(howl_client);
}

// Constructor
SrvAdv_MDNS::SrvAdv_MDNS(char* interface, ConfigFile* configFile, int configSection)
    : 
    CDevice(0,0,0,0)
{
    //alwayson = true;      // since there is no client interface
    // this breaks player so I commented it out

    // read name and description from config file. 
    assert(configFile);
    assert(interface);
    name = configFile->ReadString(configSection, "name", "");
    description = configFile->ReadString(configSection, "description", "");

    // read extra tags from player config file
    const char* x;
    for(int i = 0; (x = configFile->ReadTupleString(configSection, "tags", i, 0)); i++) {
        extra_txt.insert(std::string(x));
    }

}


// this C function is called when a query on our service is made.
static sw_result HOWL_API service_reply(
    sw_discovery_publish_handler handler,
    sw_discovery discovery,
    sw_discovery_publish_status status,
    sw_discovery_publish_id id,
    sw_opaque extra_data)
{
    printf("service_adv_mdns: ");
    switch(status) {
        case 0:
            printf("Started.\n");
            return SW_OKAY;
        case 1:
            printf("Stopped.\n");
            return SW_OKAY;
        case 2:
            printf("Name Collision!\n");
            return SW_OKAY;
        case 3:
            printf("Invalid!\n");
            return SW_OKAY;
        default:
            printf("(unknown result!)\n");
            return SW_OKAY;
    }
    return SW_OKAY;
}

void SrvAdv_MDNS::Prepare() {

    sw_text_record txt;
    sw_result r;

    if(sw_discovery_init(&howl_client) != SW_OKAY) {
        fprintf(stderr, "service_adv_mdns: Error: Howl initialization failed. (Is mdnsresponder running?)\n");
        return;
    }
    if(sw_text_record_init(&txt) != SW_OKAY) {
        fprintf(stderr, "service_adv_mdns: Error: sw_text_record_init failed! (Memory error?)\n");
        return;
    }

    // determine a suitible default name if it was unset in the config file:
    if(name == "") {
        char portstr[12];
        snprintf(portstr, 12, "%d", (device_id.port + 1) - PLAYER_PORTNUM);
        name = std::string("robot") + portstr;
    }

    // add a description to the TXT record if given in the config file
    if(description != "") {
        description = "description="+description; 
        if(sw_text_record_add_string(txt, description.c_str()) != SW_OKAY)
            fprintf(stderr, "service_adv_mdns: Error: could not add description tag \"%s\" to text record.\n", description.c_str());
    }
        

    // add a tag to the TXT record for each device in the device table
    for(CDeviceEntry* dev = deviceTable->GetFirstEntry(); dev != 0; dev = deviceTable->GetNextEntry(dev)) {
        char* devname = lookup_interface_name(0, dev->id.code);
        if(devname) {
            char deviceTag[512];
            snprintf(deviceTag, sizeof(deviceTag), "device=%s#%d(%s)", devname, dev->id.index, dev->drivername);
            if(sw_text_record_add_string(txt, deviceTag) != SW_OKAY)
                fprintf(stderr, "service_adv_mdns: Error: could not add device tag \"%s\" to text record.\n", deviceTag);
        }
    }

    for(std::set<std::string>::const_iterator i = extra_txt.begin();
                i != extra_txt.end(); i++) 
    {
        if(sw_text_record_add_string(txt, (*i).c_str()) != SW_OKAY)
            fprintf(stderr, "service_adv_mdns: Error: could not add device tag \"%s\" to text record.\n", (*i).c_str());
    }

    printf("service_adv_mdns: Publishing service with MDNS type \"_player._tcp\", port %d, and name \"%s\".\n", device_id.port, name.c_str());
    r = sw_discovery_publish(howl_client, name.c_str(), MDNS_SERVICE_TYPE, NULL, 
            NULL, device_id.port, sw_text_record_bytes(txt),
            sw_text_record_len(txt), NULL, service_reply, NULL, &id);
    if(r != SW_OKAY) {
        fprintf(stderr, "service_adv_mdns: Error: Service publishing failed!  (%ld)\n", r);
        sw_text_record_fina(txt);
        return;
    }
        

    sw_text_record_fina(txt);
    StartThread();

}

void SrvAdv_MDNS::Main() {
    printf("service_adv_mdns: running howl...\n");
    sw_discovery_run(howl_client); // (does not return)
}

// stop the service directory
void SrvAdv_MDNS::stop()
{
    sw_discovery_stop_publish(howl_client, id);
    sw_discovery_stop_run(howl_client);
    StopThread();
}


