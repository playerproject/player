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
 *   class for encapsulating all the data pertaining to a single client
 */

#ifndef _CLIENTDATA_H
#define _CLIENTDATA_H

#include <sys/types.h>  // for sendto(2)
#include <netinet/in.h>   /* for sockaddr_in type */
#include <sys/socket.h>  // for sendto(2)
#include <assert.h>

#include <libplayercore/playercore.h>

// Forward declarations
class Driver;

/// We build a linked list of these to keep track of subscribed devices.
class DeviceSubscription
{
  public:
    /// Id of device.
    player_device_id_t id;
    /// Our access to the device.
    unsigned char access;
    /// Pointer to the underlying driver.
    Driver* driver;

    /// Pointer to next element in list.
    DeviceSubscription* next;

    /// Constructor
    DeviceSubscription() 
    {
      driver = NULL; 
      next = NULL; 
      access = PLAYER_ERROR_MODE; 
    }
};

/// Possible states when reading a message from a client over TCP.
typedef enum
{
  PLAYER_AWAITING_FIRST_BYTE_STX,
  PLAYER_AWAITING_SECOND_BYTE_STX,
  PLAYER_AWAITING_REST_OF_HEADER,
  PLAYER_AWAITING_REST_OF_BODY
} player_read_state_t;

/**
 This class encapsulates the information regarding a single client.  Don't
 instantiate this class directly; instead select a transport-specific derived 
 class, such as ClientDataTCP or ClientDataUDP.
*/
class ClientData 
{
  protected:
    /// Authorization key that this client must supply.
    char auth_key[PLAYER_KEYLEN];
    /// Buffer into which we read messages.
    unsigned char *readbuffer;
    /// Unsubscribe from any devices to which this client is subscribed.
    void RemoveRequests();

    /// Handle an authorization request (i.e., check keys)
    bool CheckAuth(player_msghdr_t hdr, 
		   unsigned char* payload,
                   unsigned int payload_size);
    /// Handle device list requests.
    void HandleListRequest(player_device_devlist_t *req,
                           player_device_devlist_t *rep);
    /// Handle driver info requests.
    void HandleDriverInfoRequest(player_device_driverinfo_t *req,
                                 player_device_driverinfo_t *rep);
    /// Handle nameservice requests.
    void HandleNameserviceRequest(player_device_nameservice_req_t *req,
                                  player_device_nameservice_req_t *rep);
    /// Do we have write (or all) permission on this device?
    bool CheckWritePermissions(player_device_id_t id);
    /// Do we have any (read, write, or all) permission on this device?
    bool CheckOpenPermissions(player_device_id_t id);
    /// What is our current permission on this device?
    unsigned char FindPermission(player_device_id_t id);
    /// Unsubscribe from this device
    void Unsubscribe(player_device_id_t id);
    /// Subscribe to this device
    int Subscribe(player_device_id_t id);

 public:
    /// Port on which we're connected to the client.
    int port;
    /// The TCP/UDP socket itself, or -1 to indicate that this "client" is
    /// really internal to the server (e.g., the device is alwayson).
    int socket;
    /// List of devices to which we are subscribed.
    DeviceSubscription* requested;
    /// Length of requested.
    int numsubs;
    /// Queue for outgoing messages to the client.
    MessageQueue* OutQueue;
    /// Buffer in which replies are constructed.
    unsigned char *replybuffer;
    /// Buffer into which incoming message headers are read.
    player_msghdr_t hdrbuffer;
    /// Are we awaiting authorization from this client?
    bool auth_pending;
    /// The data delivery mode for this client.
    unsigned char mode;
    /// Delivery frequency, for use with PUSH data delivery modes (Hz).
    unsigned short frequency;
    /// Has this client requested data (for use only with PULL data
    /// delivery modes).
    bool datarequested;
    /// Does the client have a requst in its outgoing queue
    bool hasrequest;
    /// Has this clien been marked for deletion?  It gets set by various
    /// parts of ClientManager.  Should probably use an accessor function
    /// instead, and make this field private.
    bool markedfordeletion;
    /// Used to address outgoing data in UDP mode.
    struct sockaddr_in clientaddr;
    /// Used to address outgoing data in UDP mode.
    int clientaddr_len;
    /// used to identify the client in UDP mode.
    uint16_t client_id;
    /// Last time (in seconds since the epoch) that we wrote to this
    /// client.
    double last_write;

    /// Constructor: authorization key (NULL for no key) and port we're
    /// connected on.
    ClientData(char* key, int port);
    /// Destructor.
    virtual ~ClientData();
    /// Handle a subscription request.  It's public so that it can be
    /// called in main.cc for alwayson devices.
    unsigned char UpdateRequested(player_device_req_t req);
    /// Handle a message (e.g., pass it off to the appropriate device for
    /// processing).
    int HandleRequests(player_msghdr_t hdr, 
                       unsigned char *payload,
                       size_t payload_size);
    /// Put message into clients outgoing queue
    virtual void PutMsg(uint8_t type, 
			uint8_t subtype, 
			uint16_t device, 
			uint16_t device_index, 
			struct timeval* timestamp,
			uint32_t size, 
			unsigned char * data);
    /// Read from the client.  Must be implemented by the derived class, in
    /// a transport-specific way.
    virtual int Read() = 0;
    /// Write any pending data for the client.  Generally this involves 
    /// iterating over the outoging message queue. If there is any data still 
    /// to be sent return a positive value, 0 for success and < 0 for
    /// error.
    virtual int Write(bool requestonly = false) = 0;
};

/**
 ClientData for use with TCP.
 */
class ClientDataTCP : public ClientData
{
  public:
    /// Make a new ClientDataTCP object, with the given authorization key
    /// and port.
    ClientDataTCP(char* key, int port);
    ~ClientDataTCP();

    /// Read incoming messages.
    int Read();
    /// Write outgoing messages.
    int Write(bool requestonly = false);

    /// Data messages are built up here, for one efficient write(2)
    unsigned char *totalwritebuffer; 
    /// Current length of totalwritebuffer.
    size_t totalwritebuffersize; 
    /// Number of bytes currently used within totalwritebuffer.
    size_t usedwritebuffersize; 

    /// Bytes of totalwritebuffer that remain to be sent
    size_t leftover_size;

  private:
    /// Have we already complained on the console about buffer overflow on
    /// this client?
    bool warned;
    /// State machine for the read loop of this client
    player_read_state_t readstate;
    /// State machine for the read loop of this client
    unsigned int readcnt;
};

/**
 ClientData for use with UDP.
 */
class ClientDataUDP : public ClientData
{
  public:
    ClientDataUDP(char* key, int port) : ClientData(key, port) {};
    int Read();
    int Write(bool requestonly = false);
};

/**
 ClientData used for drivers that are a client of other drivers within 
 the server.
*/
class ClientDataInternal : public ClientData
{
  public:
    ClientDataInternal(Driver * _driver, char * key="", int port=6665);
    ~ClientDataInternal();
    int Read();
    int Write(bool requestonly = false);

    /// Send a message to a subscribed device.
    int SendMsg(player_device_id_t id,
		uint8_t type, 
		uint8_t subtype,
		uint8_t* src, 
		size_t len = 0,
		struct timeval* timestamp = NULL);

    /// Override this since we dont want to do the host to net transform.
    void PutMsg(uint8_t type, 
		uint8_t subtype, 
		uint16_t device, 
		uint16_t device_index, 
		struct timeval* timestamp,
		uint32_t size, 
		unsigned char * data);

    int Subscribe(player_device_id_t device, char access='a');
    int Unsubscribe(player_device_id_t device);
    int SetDataMode(uint8_t datamode);
  protected:
    MessageQueue* InQueue;
    Driver * driver;
};

#endif
