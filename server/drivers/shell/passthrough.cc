/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007 Alexis Maldonado Herrera
 *                     maldonad \/at/\ cs.tum.edu
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
 This is the new passthrough driver for Player 2.
 It does the following:
       - From the configuration file, reads requires ["SRC interface"], and provides ["DST interface"]
       - When a client connects to our DST interface, it subscribes to the SRC interface, and forwards all packets to the DST interface.
       - If the client sends a command to the DST, it gets forwarded to the SRC interfaces, and the ACK, NACK, or SYNCH answers get sent back to the DST interface
       - When the client disconnects from our DST interface, we unsubscribe from the SRC interface.
       - When forwarding packets, the header gets changed accordingly, specifically: devaddr (host, robot, interface, index)
*/

//Doxygen documentation taken partly from the old Player 1.x passthrough driver, and modified to fit the new one

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_passthrough passthrough
 * @brief General-purpose proxy driver

The @p passthrough driver relays packets between two player interfaces.
It connects as a @e client to an interface, and offers the same interface
to other clients. All communication packets are forwarded between the two.

This is specially useful to aggregate many player devices in a single Player
Server, and control them using only one connection. Probably the most
useful usage is to have a Player Server offering a stable set of interfaces,
that get forwarded to the appropriate Player Servers connected to the hardware.
If a device is moved around to a different computer, the clients don't have to
be reconfigured, since the change is done only in the server running the passthrough
driver.

@par Compile-time dependencies

- none

@par Provides

- The @p passthrough driver will support any of Player's interfaces,
  and can connect to any Player device.

@par Requires

- none

@par Configuration requests

- This driver will pass on any configuration requests.

@par Configuration file options

The @p passthrough driver can be used with local or remote interfaces using the
requires section in the configuration file.

For local interfaces, the format is:
@verbatim
driver
(
   name "passthrough"
   requires ["interface:index"]  // example: ["dio:0"]
   provides ["interface:anotherindex"] // example: ["dio:25"]
)
@endverbatim

To connect to an interface running on another server, the format is:
@verbatim
driver
(
   name "passthrough"
   requires [":hostname:port:interface:index"]   // example: [":someserver:6665:dio:0"]
   provides ["interface:someindex"] //example: [dio:0]
)
@endverbatim

Note that the in the case of connecting to remote interfaces, the provided interface can have
any index number. The driver changes the header accordingly.


@author Alexis Maldonado
*/
/** @} */




#include <unistd.h>
#include <string.h>

#include <libplayercore/playercore.h>

class PassThrough : public Driver {
public:

    PassThrough(ConfigFile* cf, int section);

    virtual int Setup();
    virtual int Shutdown();

    virtual int ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data);

private:

    virtual void Main();

    //Devices used to compare the header, and forward packets accordingly
    player_devaddr_t dstAddr;
    player_devaddr_t srcAddr;
    //the device that this server connects to to get data
    Device *srcDevice;

};

Driver*
PassThrough_Init(ConfigFile* cf, int section) {
    return((Driver*)(new PassThrough(cf, section)));
}


void PassThrough_Register(DriverTable* table) {
    table->AddDriver("passthrough", PassThrough_Init);
}


PassThrough::PassThrough(ConfigFile* cf, int section)
        : Driver(cf, section) {
    memset(&srcAddr, 0, sizeof(player_devaddr_t));
    memset(&dstAddr, 0, sizeof(player_devaddr_t));

    //let's create our DST interface
    if (cf->ReadDeviceAddr(&dstAddr, section, "provides",
                           -1, 0, NULL) == 0) {
        if (AddInterface(dstAddr) != 0) {
            SetError(-1);
            return;
        }
    } else {
        PLAYER_ERROR("PassThrough: Missing 'provides' section, aborting.");
        SetError(-1);
        return;
    }

    //Find our SRC inteface
    if (cf->ReadDeviceAddr(&srcAddr, section, "requires", -1, 0, NULL) != 0) {
        PLAYER_ERROR("PassThrough: Missing 'requires' section, aborting.");
        this->SetError(-1);
        return;
    }


    return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int PassThrough::Setup() {
    PLAYER_MSG0(1,"PassThrough driver initialising");

    PLAYER_MSG0(1,"PassThrough driver ready");


    srcDevice=deviceTable->GetDevice(srcAddr);

    if (!srcDevice) {
        PLAYER_ERROR3("Could not locate device [%d:%s:%d] for forwarding",
                      srcDevice->addr.robot,::lookup_interface_name(0, srcDevice->addr.interf),
                      srcDevice->addr.index);
        return -1;
    }
    if (srcDevice->Subscribe(this->InQueue) != 0) {
        PLAYER_ERROR("unable to subscribe to device");
        return -1;
    }


    StartThread();

    return(0);
}


int PassThrough::Shutdown() {
    PLAYER_MSG0(1,"Shutting PassThrough driver down");


    StopThread();

    //Our clients disconnected, so let's disconnect from our SRC interface
    srcDevice->Unsubscribe(this->InQueue);

    PLAYER_MSG0(1,"PassThrough driver has been shutdown");

    return(0);
}

int PassThrough::ProcessMessage(QueuePointer & resp_queue,
                                player_msghdr * hdr,
                                void * data) {


    bool inspected(false);

    PLAYER_MSG0(9,"PassThrough::ProcessMessage: Received a packet!");

    if (Device::MatchDeviceAddress(hdr->addr,srcAddr) && 
        ((hdr->type == PLAYER_MSGTYPE_DATA) || 
         (hdr->type == PLAYER_MSGTYPE_RESP_ACK) || 
         (hdr->type == PLAYER_MSGTYPE_SYNCH) || 
         (hdr->type == PLAYER_MSGTYPE_RESP_NACK)))  
    {
        PLAYER_MSG7(8,"PassThrough: Forwarding SRC->DST Interface code=%d  %d:%d:%d -> %d:%d:%d",hdr->addr.interf, hdr->addr.host,hdr->addr.robot, hdr->addr.index, dstAddr.host, dstAddr.robot, dstAddr.index);

        hdr->addr=dstAddr; //will send to my clients, making it seem like it comes from my DST interface

        this->Publish (hdr,data);  //publish to all clients that subscribed to my DST interface. *data remains unchanged
        inspected=true;
    }

    if (Device::MatchDeviceAddress(hdr->addr,dstAddr) && 
        (hdr->type == PLAYER_MSGTYPE_CMD))
    {
        PLAYER_MSG7(8,"PassThrough: Forwarding DST->SRC Interface code=%d  %d:%d:%d -> %d:%d:%d",hdr->addr.interf, hdr->addr.host,hdr->addr.robot, hdr->addr.index, srcAddr.host, srcAddr.robot, srcAddr.index);


        hdr->addr=srcAddr;  //send to the device to which I subscribed, making it seem like it comes from my original interface

        srcDevice->PutMsg(this->InQueue,hdr,data);  //putMsg is the correct way to talk to this subscribed device, any answer comes to the queue of this driver
        inspected=true;
    }
    else if (Device::MatchDeviceAddress(hdr->addr,dstAddr) && 
             (hdr->type == PLAYER_MSGTYPE_REQ))
    {
      // If it's a request, do it in-place and await the reply.

      hdr->addr=srcAddr;  //send to the device to which I subscribed, making it seem like it comes from my original interface

      Message* msg;
      if(!(msg = srcDevice->Request(this->InQueue,
                                    hdr->type,
                                    hdr->subtype,
                                    data, 0,
                                    NULL, true)))
      {
        PLAYER_WARN("failed to forward request");
        // Returning -1 here causes a NACK to be sent
        return(-1);
      }

      // Got the response, so adjust the address and forward it
      player_msghdr_t newhdr = *(msg->GetHeader());
      newhdr.addr = dstAddr;
      this->Publish(resp_queue, 
                    &newhdr,
                    msg->GetPayload());
      delete msg;
      inspected=true;
    }


    //Check if a packet went thorugh, without being forwarded
    if (!inspected) {
        static bool reported(false);
        if (!reported) {
            PLAYER_WARN("PassThrough: A packet did not get forwarded.\nThis warning is only shown once.\nRun player -d 2 or -d 3 for more information.");
            reported=true;
        }
        PLAYER_MSG0(2,"PassThrough: Something did not get forwarded. Check the header:");
        PLAYER_MSG4(2,"host: %d  robot: %d  interf: %d  index: %d",hdr->addr.host,hdr->addr.robot,hdr->addr.interf,hdr->addr.index);
        PLAYER_MSG2(2,"type: %d  subtype: %d",hdr->type,hdr->subtype);
    }


    return(0);
}



void PassThrough::Main() {
    //The forwarding is done in the ProcessMessage method. Called once per each message by ProcessMessages()
    while (true) {
        pthread_testcancel();

        ProcessMessages();

        usleep(100);
    }
}

