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
 *   class for encapsulating all the data pertaining to a single
 *   client
 */

#ifndef _CLIENTDATA_H
#define _CLIENTDATA_H

#include <pthread.h>
#include <messages.h>

typedef enum
{
  CONTINUOUS,
  REQUESTREPLY
} server_mode_t;

// keep a linked list of these
class CDeviceSubscription
{
  public:
    unsigned short code;
    unsigned short index;
    unsigned char access;
    CDeviceSubscription* next;

    CDeviceSubscription() { next = NULL; access = 'e'; }
};
    

class CClientData 
{
  CDeviceSubscription* requested;
  int numsubs;
  pthread_mutex_t access, requesthandling;

  void MotorStop();
  void PrintRequested(char*);

 public:
  int client_index;
  int writeThreadId, readThreadId;
  pthread_t writeThread, readThread;

  server_mode_t mode;
  unsigned short frequency;  // Hz
  pthread_mutex_t datarequested, socketwrite;

  int socket;

  CClientData();
  ~CClientData();

  void HandleRequests( unsigned char *buffer, int readcnt );
  void RemoveBlanks();  
  void RemoveRequests();
  void UpdateRequested(player_device_req_t req);
  bool CheckPermissions(unsigned short code, unsigned short index);
  unsigned char FindPermission( unsigned short code, unsigned short index);
  int BuildMsg( unsigned char *data, size_t maxsize );
  void Unsubscribe( unsigned short code, unsigned short index );
  int Subscribe( unsigned short code, unsigned short index );
};


#endif
