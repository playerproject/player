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

class CBroadcastDevice /* : public CDevice */
{
    // Constructor
    //
    public: CBroadcastDevice();

    // Setup/shutdown routines
    //
    public: virtual int Setup();
    public: virtual int Shutdown();
    //public: virtual CLock* GetLock() {return &lock;};

    // Client interface
    //
    public: virtual size_t GetData(unsigned char *, size_t maxsize);
    public: virtual void PutData(unsigned char *, size_t maxsize);
    public: virtual size_t GetCommand(unsigned char *, size_t maxsize);
    public: virtual void PutCommand(unsigned char *, size_t maxsize);
    public: virtual size_t GetConfig(unsigned char *, size_t maxsize);
    public: virtual void PutConfig(unsigned char *, size_t maxsize);
    
    // Send a packet
    //
    public: void SendPacket(unsigned char *packet, size_t size);

    // Receive a packet
    //
    public: size_t RecvPacket(unsigned char *packet, size_t size);

    // Write socket info
    //
    private: int m_write_socket;
    private: sockaddr_in m_write_addr;

    // Read socket info
    //
    private: int m_read_socket;
    private: sockaddr_in m_read_addr;
};

#endif

