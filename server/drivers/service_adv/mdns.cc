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
//       
///////////////////////////////////////////////////////////////////////////

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_service_adv_mdns service_adv_mdns

This driver can be used to publish a Player service using 
the proposed IETF standard for multicast DNS service discovery (MDNS-SD).
MDNS-SD is a part of the <a href="http://www.zeroconf.org">Zeroconf</a> protocols,
and has also been called "Rendezvous".

The MDNS-SD service type is "_player._tcp".    In addition to any 
"service_description" given in the configuration file (see below), any 
loaded device drivers will be represented in the service's TXT record.
Each device will be entered into the TXT record as: 
<code>device=</code><i>name</i><code>#</code><i>number</i><code>(</code><i>driver name</i><code>)</code>.

The <a href="http://www.porchdogsoft.com/products/howl/">Howl</a> library is 
used for MDNS-SD and must be available for this driver to be compiled into
the server.

This driver has no client proxy. When the driver is loaded and initialized,
the service is published, and Howl responds to queries in a background 
thread. Clients may use Howl or any other MDNS-SD implementation to 
query and find services.

@par Compile-time dependencies

- <a href="http://www.porchdogsoft.com/products/howl/">libhowl</a>

@par Provides

- none
  - Note, however, that you must specify a "provides" field in the config file, anyway.

@par Requires

- None

@par Configuration requests

- none

@par Configuration file options

- service_name (string)  
    - A short name for the service. If omitted, a default will be chosen based on the Player port number.
- service_description (string)
    - A longer "description" for the robot, which will be included in the service TXT record.
- service_tags (tuple string)
    - Tags to include in the service TXT record, in addition to device names.
 
@par Example 

@verbatim
driver
(
  name "service_adv_mdns"
  provides ["service_adv:0"]
  service_name "robot"
  service_description "This is a groovy robot which can be controlled with Player."
  service_tags [ "job=mapper" "operator=reed" "strength=12" "dexterity=18" "intelligence=4" "thac0=8" ]
)
@endverbatim

@par Authors

Reed Hedges

*/
/** @} */

#include <string>
#include <set>

#include <howl.h>

#include "playercommon.h"
#include "drivertable.h"
#include "deviceregistry.h"
#include "devicetable.h"
#include "player.h"


#define MDNS_SERVICE_TYPE "_player._tcp."

class SrvAdv_MDNS : public Driver {
  private:
    // MDNS objects
    sw_discovery howl_session;
    sw_discovery_oid id;
    std::set<std::string> extra_txt;
    std::string name, description;

  public:
    SrvAdv_MDNS( ConfigFile* cf, int section);
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



Driver* SrvAdv_MDNS_Init( ConfigFile* cf, int section) {
    return (Driver*)(new SrvAdv_MDNS( cf, section));
}

// a driver registration function
void ServiceAdvMDNS_Register(DriverTable* table)
{
  table->AddDriver("service_adv_mdns",  SrvAdv_MDNS_Init);
}


SrvAdv_MDNS::~SrvAdv_MDNS() {
    stop();
    sw_discovery_fina(howl_session);
}

// Constructor
SrvAdv_MDNS::SrvAdv_MDNS( ConfigFile* configFile, int configSection)
 : Driver(configFile, configSection, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_SERVICE_ADV_CODE, PLAYER_ALL_MODE)
{
    //alwayson = true;      // since there is no client interface
    // this breaks player so I commented it out

    // read name and description from config file. 
    assert(configFile);
    name = configFile->ReadString(configSection, "service_name", "");
    description = configFile->ReadString(configSection, "service_description", "");

    // read extra tags from player config file
    const char* x;
    for(int i = 0; (x = configFile->ReadTupleString(configSection, "service_tags", i, 0)); i++) {
        extra_txt.insert(std::string(x));
    }

}


// this C function is called when a query on our service is made.
static sw_result HOWL_API service_reply(
    sw_discovery discovery,
    sw_discovery_oid id,
    sw_discovery_publish_status status,
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

  printf("service_adv_mdns: prepare\n");
    sw_text_record txt;
    sw_result r;

    if(sw_discovery_init(&howl_session) != SW_OKAY) {
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
    for(Device* dev = deviceTable->GetFirstDevice(); dev != 0; dev = deviceTable->GetNextDevice(dev)) {
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
    r = sw_discovery_publish(
            howl_session,        // session
            0,                  // NIC index (0=all)
            name.c_str(),       // service name
            MDNS_SERVICE_TYPE,  // service type, defined above to _player._tcp
            NULL,               // service domain, NULL=defaut (.local)
            NULL,               // service hostname, NULL=default
            device_id.port,     // service port
            sw_text_record_bytes(txt),  // TXT record
            sw_text_record_len(txt),    // TXT record length
            &service_reply,     // callback function
            NULL,               // extra opaque data
            &id);               // id of this publication
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
    sw_discovery_run(howl_session); // (does not return)
}

// stop the service directory
void SrvAdv_MDNS::stop()
{
    sw_discovery_cancel(howl_session, id);
    sw_discovery_stop_run(howl_session);
    StopThread();
}


