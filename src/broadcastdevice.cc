/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
// File: broadcastdevice.cc
// Author: Andrew Howard
// Date: 5 Feb 2000
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
//  This device actually uses IPv4 broadcasting (not multicasting).
//  Be careful not to run this on the university nets: you will get
//  disconnected and spanked!
//
// Known bugs:
//  (empty)
//
// Possible enhancements:
//  (empty)
//
///////////////////////////////////////////////////////////////////////////

#define PLAYER_ENABLE_TRACE 1

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include "broadcastdevice.hh"

#define PLAYER_BROADCAST_IP "10.255.255.255"
#define PLAYER_BROADCAST_PORT 6013


///////////////////////////////////////////////////////////////////////////
// Constructor
//
CBroadcastDevice::CBroadcastDevice()
{
    m_read_socket = 0;
    m_write_socket = 0;
}


///////////////////////////////////////////////////////////////////////////
// Start device
//
int CBroadcastDevice::Setup()
{
    printf("Broadcast device initialising...");
    
    // Set up the write socket
    //
    m_write_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_write_socket == -1)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }
    memset(&m_write_addr, 0, sizeof(m_write_addr));
    m_write_addr.sin_family = AF_INET;
    m_write_addr.sin_addr.s_addr = inet_addr(PLAYER_BROADCAST_IP);
    m_write_addr.sin_port = htons(PLAYER_BROADCAST_PORT);

    // Set write socket options to allow broadcasting
    //
    u_int broadcast = 1;
    if (setsockopt(m_write_socket, SOL_SOCKET, SO_BROADCAST,
                   &broadcast, sizeof(broadcast)) < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }
    
    // Set up the read socket
    //
    m_read_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (m_read_socket == -1)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }

    // Set socket options to allow sharing of port
    //
    u_int share = 1;
    if (setsockopt(m_read_socket, SOL_SOCKET, SO_REUSEADDR,
                   &share, sizeof(share)) < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }
    
    // Bind socket to port
    //
    memset(&m_read_addr, 0, sizeof(m_read_addr));
    m_read_addr.sin_family = AF_INET;
    m_read_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_read_addr.sin_port = htons(PLAYER_BROADCAST_PORT);
    if (bind(m_read_socket, (sockaddr*) &m_read_addr, sizeof(m_read_addr)) < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }

    // Set socket to non-blocking
    //
    if (fcntl(m_read_socket, F_SETFL, O_NONBLOCK) < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }

    // Dummy call to get around #(*%&@ mutex
    //
    GetLock()->PutData(this, NULL, 0);

    printf("done\n");
    
    return 0;
}


///////////////////////////////////////////////////////////////////////////
// Shutdown device
//
int CBroadcastDevice::Shutdown()
{
    printf("Broadcast device shuting down...");
    
    // Close sockets
    //
    close(m_write_socket);
    close(m_read_socket);

    printf("done\n");
    
    return 0;
}

///////////////////////////////////////////////////////////////////////////
// Get incoming data
//
size_t CBroadcastDevice::GetData(unsigned char *data, size_t maxsize)
{
    return RecvPacket(data, maxsize);
}


///////////////////////////////////////////////////////////////////////////
// Not used
//
void CBroadcastDevice::PutData(unsigned char *data, size_t maxsize)
{
}


///////////////////////////////////////////////////////////////////////////
// Not used
//
void CBroadcastDevice::GetCommand(unsigned char *, size_t maxsize)
{
}


///////////////////////////////////////////////////////////////////////////
// Send data
//
void CBroadcastDevice::PutCommand(unsigned char *data, size_t maxsize)
{
    //SendPacket(data, maxsize);
}


///////////////////////////////////////////////////////////////////////////
// Not used
//
size_t CBroadcastDevice::GetConfig(unsigned char *, size_t maxsize)
{
    return 0;
}


///////////////////////////////////////////////////////////////////////////
// Not used
//
void CBroadcastDevice::PutConfig(unsigned char *, size_t maxsize)
{
}


///////////////////////////////////////////////////////////////////////////
// Send a packet
//
void CBroadcastDevice::SendPacket(unsigned char *packet, size_t size)
{
    if (sendto(m_write_socket, packet, size,
                 0, (sockaddr*) &m_write_addr, sizeof(m_write_addr)) < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return;
    }
}


///////////////////////////////////////////////////////////////////////////
// Receive a packet
//
size_t CBroadcastDevice::RecvPacket(unsigned char *packet, size_t size)
{
    PLAYER_TRACE0("reading packet");
    
    size_t addr_len = sizeof(m_read_addr);    
    size_t packet_len = recvfrom(m_read_socket, packet, size,
                                 0, (sockaddr*) &m_read_addr, &addr_len);
    if (packet_len < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return 0;
    }

    PLAYER_TRACE0("reading packet -- done");
    
    return packet_len;
}







