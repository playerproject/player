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
#include <errno.h>
#include "broadcastdevice.hh"

#define PLAYER_BROADCAST_IP "10.255.255.255"
#define PLAYER_BROADCAST_PORT 6013


///////////////////////////////////////////////////////////////////////////
// Constructor
//
CBroadcastDevice::CBroadcastDevice(int argc, char** argv)
{
    this->read_socket = 0;
    this->write_socket = 0;
}


///////////////////////////////////////////////////////////////////////////
// Start device
//
int CBroadcastDevice::Setup()
{
    PLAYER_TRACE0("Broadcast device initialising...");
    
    // Set up the write socket
    this->write_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->write_socket == -1)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }
    memset(&this->write_addr, 0, sizeof(this->write_addr));
    this->write_addr.sin_family = AF_INET;
    this->write_addr.sin_addr.s_addr = inet_addr(PLAYER_BROADCAST_IP);
    this->write_addr.sin_port = htons(PLAYER_BROADCAST_PORT);

    // Set write socket options to allow broadcasting
    u_int broadcast = 1;
    if (setsockopt(this->write_socket, SOL_SOCKET, SO_BROADCAST,
                   (const char*)&broadcast, sizeof(broadcast)) < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }
    
    // Set up the read socket
    this->read_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (this->read_socket == -1)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }

    // Set socket options to allow sharing of port
    u_int share = 1;
    if (setsockopt(this->read_socket, SOL_SOCKET, SO_REUSEADDR,
                   (const char*)&share, sizeof(share)) < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }
    
    // Bind socket to port
    memset(&this->read_addr, 0, sizeof(this->read_addr));
    this->read_addr.sin_family = AF_INET;
    this->read_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this->read_addr.sin_port = htons(PLAYER_BROADCAST_PORT);
    if (bind(this->read_socket, (sockaddr*) &this->read_addr, sizeof(this->read_addr)) < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }

    // Set socket to non-blocking
    if (fcntl(this->read_socket, F_SETFL, O_NONBLOCK) < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return 1;
    }

    // Dummy call to get around #(*%&@ mutex
    GetLock()->PutData(this, NULL, 0);

    PLAYER_TRACE0("done\n");
    
    return 0;
}


///////////////////////////////////////////////////////////////////////////
// Shutdown device
//
int CBroadcastDevice::Shutdown()
{
    PLAYER_TRACE0("Broadcast device shuting down...");
    
    // Close sockets
    //
    close(this->write_socket);
    close(this->read_socket);

    PLAYER_TRACE0("done\n");
    
    return 0;
}


///////////////////////////////////////////////////////////////////////////
// Get incoming data
size_t CBroadcastDevice::GetData(unsigned char *data, size_t maxsize)
{    
    this->data.len = 0;

    // Read all the currently queued packets
    // and concatenate them into a broadcast data packet.
    while (true)
    {
        int max_bytes = sizeof(this->data.buffer) - this->data.len;
        int bytes = RecvPacket(this->data.buffer + this->data.len, max_bytes);
        if (bytes == 0)
        {
            PLAYER_TRACE0("read no bytes");
            break;
        }
        if (bytes >= max_bytes)
        {
            PLAYER_TRACE0("broadcast packet overrun; packets have been discarded\n");
            break;
        }
        PLAYER_TRACE1("read msg len = %d", bytes);
        this->data.len += bytes;
    }
    
    // Do some byte swapping
    this->data.len = htons(this->data.len);

    PLAYER_TRACE1("data.buffer [%s]", this->data.buffer);
    
    // Copy data
    memcpy(data, &this->data, maxsize); 

    // Return actual length of data
    return this->data.len + sizeof(this->data.len);
}


///////////////////////////////////////////////////////////////////////////
// Not used
void CBroadcastDevice::PutData(unsigned char *data, size_t maxsize)
{
}


///////////////////////////////////////////////////////////////////////////
// Not used
void CBroadcastDevice::GetCommand(unsigned char *, size_t maxsize)
{
}


///////////////////////////////////////////////////////////////////////////
// Send data
void CBroadcastDevice::PutCommand(unsigned char *cmd, size_t maxsize)
{
    assert(maxsize < sizeof(this->cmd));
    memcpy(&this->cmd, cmd, maxsize);
    
    // Do some bute swapping
    this->cmd.len = ntohs(((player_broadcast_cmd_t*) cmd)->len);

    // Send all the messages in the command at once
    SendPacket(this->cmd.buffer, this->cmd.len);

    PLAYER_TRACE2("cmd.buffer [%s] %d bytes", this->cmd.buffer, this->cmd.len);
}


///////////////////////////////////////////////////////////////////////////
// Not used
size_t CBroadcastDevice::GetConfig(unsigned char *, size_t maxsize)
{
    return 0;
}


///////////////////////////////////////////////////////////////////////////
// Not used
void CBroadcastDevice::PutConfig(unsigned char *, size_t maxsize)
{
}


///////////////////////////////////////////////////////////////////////////
// Send a packet
void CBroadcastDevice::SendPacket(unsigned char *packet, size_t size)
{    
    if (sendto(this->write_socket, (const char*)packet, size,
                 0, (sockaddr*) &this->write_addr, sizeof(this->write_addr)) < 0)
    {
        perror(__PRETTY_FUNCTION__);
        return;
    }

    PLAYER_TRACE1("sent msg len = %d", (int) size);
}


///////////////////////////////////////////////////////////////////////////
// Receive a packet
size_t CBroadcastDevice::RecvPacket(unsigned char *packet, size_t size)
{
#ifdef PLAYER_LINUX
    size_t addr_len = sizeof(this->read_addr);    
#else
    int addr_len = (int)sizeof(this->read_addr);    
#endif
    size_t packet_len = recvfrom(this->read_socket, (char*)packet, size,
                                 0, (sockaddr*) &this->read_addr, &addr_len);
    if ((int) packet_len < 0)
    {
        if (errno == EAGAIN)
            return 0;
        else
        {
            perror(__PRETTY_FUNCTION__);
            return 0;
        }
    }

    PLAYER_TRACE1("read packet len = %d", (int) packet_len);
    
    return packet_len;
}







