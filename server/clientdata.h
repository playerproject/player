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

#include <player.h>

// get around circular inclusion;
class CDevice;

// keep a linked list of these
class CDeviceSubscription
{
  public:
    player_device_id_t id;
    unsigned char access;
    CDevice* devicep;

    // record the last time we got fresh data
    uint32_t last_sec, last_usec; 

    CDeviceSubscription* next;

    CDeviceSubscription() 
    { 
      devicep = NULL; 
      next = NULL; 
      access = PLAYER_ERROR_MODE; 
    }
};

typedef enum
{
  PLAYER_AWAITING_FIRST_BYTE_STX,
  PLAYER_AWAITING_SECOND_BYTE_STX,
  PLAYER_AWAITING_REST_OF_HEADER,
  PLAYER_AWAITING_REST_OF_BODY,
  PLAYER_READ_ERROR,
} player_read_state_t;


class CClientData 
{
  private:
    char auth_key[PLAYER_KEYLEN];
    unsigned char *readbuffer;
    unsigned char *writebuffer;  // individual data messages are written here
    unsigned char *totalwritebuffer; // data messages are then added here, for
                                     // one efficient write(2)
    size_t totalwritebuffersize; // size of currently allocated buffer
    player_msghdr_t hdrbuffer;
    
    // added this so Player can manage multiple robots in Stage mode
    int port;

    // state machine for the read loop of this client
    player_read_state_t readstate;
    unsigned int readcnt;

    void MotorStop();
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

    void UpdateRequested(player_device_req_t req);
    bool CheckWritePermissions(player_device_id_t id);
    bool CheckOpenPermissions(player_device_id_t id);
    unsigned char FindPermission(player_device_id_t id);
    void Unsubscribe(player_device_id_t id);
    int Subscribe(player_device_id_t id);

 public:
    size_t leftover_size; // bytes of totalwritebuffer that remain to be sent
    CDeviceSubscription* requested;
    int numsubs;
    unsigned char *replybuffer;
    bool auth_pending;
    unsigned char mode;
    unsigned short frequency;  // Hz
    bool datarequested;
    bool markedfordeletion;

    // this is used decide when to write
    double last_write;

    int socket;

    CClientData(char* key, int port);
    ~CClientData();

    size_t BuildMsg();
    void FillWriteBuffer(unsigned char* src, size_t offset, size_t len);
    int HandleRequests(player_msghdr_t hdr, unsigned char *payload,
                        size_t payload_size);

    int Read();
    int Write(size_t len);
    int WriteIdentString();
};


#endif
