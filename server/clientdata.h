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

#include <player.h>
#include "message.h"

// Forward declarations
class Driver;

// keep a linked list of these
class CDeviceSubscription
{
  public:
    player_device_id_t id;
    unsigned char access;
    Driver* driver;

    // record the last time we got fresh data
    uint32_t last_sec, last_usec; 

    CDeviceSubscription* next;

    CDeviceSubscription() 
    { 
      driver = NULL; 
      next = NULL; 
      access = PLAYER_ERROR_MODE; 
      last_sec = last_usec = 0;
    }
};

typedef enum
{
  PLAYER_AWAITING_FIRST_BYTE_STX,
  PLAYER_AWAITING_SECOND_BYTE_STX,
  PLAYER_AWAITING_REST_OF_HEADER,
  PLAYER_AWAITING_REST_OF_BODY,
  PLAYER_READ_ERROR
} player_read_state_t;

class ClientData 
{
  protected:
    char auth_key[PLAYER_KEYLEN];
    unsigned char *readbuffer;
    unsigned char *writebuffer;  // individual data messages are written here
    
    // added this so Player can manage multiple robots in Stage mode
    int port;

    // state machine for the read loop of this client
    player_read_state_t readstate;
    unsigned int readcnt;

    void PrintRequested(char*);

    bool CheckAuth(player_msghdr_t hdr, unsigned char* payload,
                   unsigned int payload_size);

    void RemoveBlanks();  
    void RemoveRequests();

    // Handle device list requests.
    void HandleListRequest(player_device_devlist_t *req,
                           player_device_devlist_t *rep);

    // Handle driver info requests.
    void HandleDriverInfoRequest(player_device_driverinfo_t *req,
                                 player_device_driverinfo_t *rep);

    // Handle nameservice requests.
    void HandleNameserviceRequest(player_device_nameservice_req_t *req,
                                  player_device_nameservice_req_t *rep);

    bool CheckWritePermissions(player_device_id_t id);
    bool CheckOpenPermissions(player_device_id_t id);
    unsigned char FindPermission(player_device_id_t id);
    void Unsubscribe(player_device_id_t id);
    int Subscribe(player_device_id_t id);

 public:
    unsigned char UpdateRequested(player_device_req_t req);

    size_t leftover_size; // bytes of totalwritebuffer that remain to be sent
    CDeviceSubscription* requested;
    int numsubs;
    MessageQueue OutQueue;
    unsigned char *replybuffer;
    unsigned char *totalwritebuffer; // data messages are then added here, for
                                     // one efficient write(2)
    size_t totalwritebuffersize; // size of currently allocated buffer
    player_msghdr_t hdrbuffer;
    bool auth_pending;
    unsigned char mode;
    unsigned short frequency;  // Hz
    bool datarequested;
    bool markedfordeletion;

    // used to address outgoing data in UDP mode.
    struct sockaddr_in clientaddr;
    int clientaddr_len;

    // used to identify the client in UDP mode.
    uint16_t client_id;

    // this is used decide when to write
    double last_write;

    int socket;

    ClientData(char* key, int port);
    virtual ~ClientData();

    // Loop through all devices currently open for this client, get data from
    // them, and assemble the results into totalwritebuffer.  Returns the
    // total size of the data that was written into that buffer.
    size_t BuildMsg(bool include_sync);

    // Copy len bytes of src to totalwritebuffer+offset.  Will realloc()
    // totalwritebuffer to make room, if necessary.
    void FillWriteBuffer(unsigned char* src, size_t offset, size_t len);

    int HandleRequests(player_msghdr_t hdr, unsigned char *payload,
                        size_t payload_size);

    virtual int Read() = 0;
    
    // Try to write() len bytes from totalwritebuffer, which should have been
    // filled with FillWriteBuffer().  If fewer than len bytes are written, 
    // the remaining bytes are moved up to the front of totalwritebuffer and 
    // their length is recorded in leftover_size.  Returns 0 on success (which
    // includes writing fewer than len bytes) and -1 on error (e.g., if the 
    // other end of the socket was closed).
    virtual int Write(size_t len) = 0;
};

class ClientDataTCP : public ClientData
{
  public:
    ClientDataTCP(char* key, int port) : ClientData(key, port) {};
    virtual int Read();
    virtual int Write(size_t len);
};

class ClientDataUDP : public ClientData
{
  public:
    ClientDataUDP(char* key, int port) : ClientData(key, port) {};
    virtual int Read();
    virtual int Write(size_t len);
};

#endif
