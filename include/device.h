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
 *  the virtual class from which all device classes inherit.  this
 *  defines the interface that all devices must implement.
 */

#ifndef _DEVICE_H
#define _DEVICE_H

#include <pthread.h>
#include <lock.h>

#include <stddef.h> /* for size_t */
#include <playercommon.h>
#include <playerqueue.h>

extern bool debug;
extern bool experimental;

// getting around circular inclusion
class CLock;

class CDevice 
{
  public:
    // buffers for data and command
    static unsigned char* device_data;
    static unsigned char* device_command;

    static size_t device_datasize;
    static size_t device_commandsize;

    // queues for incoming requests and outgoing replies
    static PlayerQueue* device_reqqueue;
    static PlayerQueue* device_repqueue;

    virtual ~CDevice() {};

    CDevice() {/*puts("Warning: called default CDevice() constructor");*/};

    // this is the main constructor, used by most non-Stage devices.
    // storage will be allocated by this constructor
    CDevice(size_t datasize, size_t commandsize, 
            int reqqueuelen, int repqueuelen);
    
    // this is the other constructor, used mostly by Stage devices.
    // storage must be pre-allocated
    CDevice(unsigned char* data, size_t datasize, 
            unsigned char* command, size_t commandsize, 
            unsigned char* reqqueue, int reqqueuelen, 
            unsigned char* repqueue, int repqueuelen);

    virtual int Setup() = 0;
    virtual int Shutdown() = 0;
    virtual size_t GetData( unsigned char *, size_t );
    virtual void PutData(unsigned char *, size_t );
    // TODO: swap in size_t return version
    //virtual size_t GetCommand( unsigned char *, size_t );
    virtual void GetCommand( unsigned char *, size_t );
    virtual void PutCommand( unsigned char * , size_t );
    
    virtual size_t GetConfig( unsigned char *, size_t);
    virtual void PutConfig( unsigned char * , size_t);

    virtual size_t GetReply( unsigned char *, size_t);
    virtual void PutReply(unsigned char * , size_t);

    virtual CLock* GetLock( void ) = 0;

    virtual size_t ConsumeData( unsigned char * data, size_t sz ) 
    {
      puts( "PLAYER WARNING: ConsumeData() not implemented"
            " for this device. Calling GetData() instead" );
      return GetData( data, sz );
    }

    // to record the time at which the device gathered the data
    uint32_t data_timestamp_sec;
    uint32_t data_timestamp_usec;

    // quick hack to get around the fact that P2OS devices share a 
    // subscription counter, which precludes us from determining, for
    // example, how many clients are connected to the Sonar device,
    // as opposed to the Position device.  solution: keep another
    // per-device subscription counter here.
    int subscrcount;
};

#endif
