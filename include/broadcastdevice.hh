///////////////////////////////////////////////////////////////////////////
//
// File: broadcastdevice.hh
// Author: Andrew Howard
// Date: 5 Deb 2000
// Desc: Device for inter-player communication using broadcast sockets.
//
// CVS info:
//  $Source$
//  $Author$
//  $Revision$
//
// Usage:
//  (empty)
//
// Theory of operation:
//  (empty)
//
// Known bugs:
//  (empty)
//
// Possible enhancements:
//  (empty)
//
///////////////////////////////////////////////////////////////////////////

#ifndef BROADCASTDEVICE_HH
#define BROADCASTDEVICE_HH

#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "lock.h"
#include "device.h"
#include "messages.h"

class CBroadcastDevice : public CDevice
{
    // Constructor
    public: CBroadcastDevice(int argc, char** argv);

    // Setup/shutdown routines
    public: virtual int Setup();
    public: virtual int Shutdown();
    public: virtual CLock* GetLock() {return &this->lock;};

    // Client interface
    public: virtual size_t GetData(unsigned char *, size_t maxsize);
    public: virtual void PutData(unsigned char *, size_t maxsize);
    public: virtual void GetCommand(unsigned char *, size_t maxsize);
    public: virtual void PutCommand(unsigned char *, size_t maxsize);
    public: virtual size_t GetConfig(unsigned char *, size_t maxsize);
    public: virtual void PutConfig(unsigned char *, size_t maxsize);
    
    // Send a packet
    public: void SendPacket(unsigned char *packet, size_t size);

    // Receive a packet
    public: size_t RecvPacket(unsigned char *packet, size_t size);

    // Lock object
    private: CLock lock;

    // Local copy of broadcast data
    private: player_broadcast_cmd_t cmd;
    private: player_broadcast_data_t data;

    // Write socket info
    private: int write_socket;
    private: sockaddr_in write_addr;

    // Read socket info
    private: int read_socket;
    private: sockaddr_in read_addr;
};

#endif

