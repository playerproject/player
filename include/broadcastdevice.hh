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

#include "device.h"
#include "messages.h"

class CBroadcastDevice : public CDevice
{
    // Constructor
    public: CBroadcastDevice(int argc, char** argv);

    // Setup/shutdown routines
    public: virtual int Setup();
    public: virtual int Shutdown();

    // Client interface
    public: virtual size_t GetData(unsigned char *, size_t maxsize,
                                   uint32_t* timestamp_sec, 
                                   uint32_t* timestamp_usec);
    public: virtual void PutCommand(unsigned char *, size_t maxsize);
    
    // Send a packet
    public: void SendPacket(unsigned char *packet, size_t size);

    // Receive a packet
    public: size_t RecvPacket(unsigned char *packet, size_t size);

    // Local copy of broadcast data
    private: player_broadcast_cmd_t cmd;
    private: player_broadcast_data_t data;

    // Address and port to broadcast on
    private: char *addr;
    private: int port;

    // Write socket info
    private: int write_socket;
    private: sockaddr_in write_addr;

    // Read socket info
    private: int read_socket;
    private: sockaddr_in read_addr;
};

#endif

