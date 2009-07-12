/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) <insert dates here>
 *     <insert author's name(s) here>
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
/********************************************************************
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ********************************************************************/

#ifndef TCPREMOTE_DRIVER_H
#define TCPREMOTE_DRIVER_H

#include <libplayercore/playercore.h>
#include <libplayercore/remote_driver.h>
#include "playertcp.h"

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERTCP_EXPORT
  #elif defined (playertcp_EXPORTS)
    #define PLAYERTCP_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERTCP_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERTCP_EXPORT
#endif

#define DEFAULT_SETUP_TIMEOUT 3.0

class PLAYERTCP_EXPORT TCPRemoteDriverConnection: public RemoteConnection
{
public:
	TCPRemoteDriverConnection(PlayerTCP* ptcp, unsigned remote_host,
			unsigned short remote_port) :
		ptcp(ptcp), host(remote_host), port(remote_port), sock(-1), setup_timeout(DEFAULT_SETUP_TIMEOUT), kill_flag(0)
	{
#if defined (WIN32)
		// Initialise Windows sockets API (this can safely be done as many times as we like)
		WSADATA info;
		int result;
		if ((result = WSAStartup (MAKEWORD (2, 2), &info)) != 0)
		{
			PLAYER_ERROR1 ("Failed to initialise Windows sockets API with error %d", result);
		}
#endif
	}
	;

	virtual ~TCPRemoteDriverConnection()
	{
#if defined (WIN32)
		// Clean up the Windows sockets API (this can safely be done as many times as we like)
		if (WSACleanup () != 0)
			PLAYER_ERROR1 ("Failed to clean up Windows sockets API with error %s", WSAGetLastError ());
#endif
	}
	;

	virtual QueuePointer Connect();
	virtual QueuePointer Disconnect();

	virtual void Subscribe(player_devaddr_t addr);
	virtual void Unsubscribe(player_devaddr_t addr);

	void PutMsg(player_msghdr_t* hdr, void* src);

private:
	int SubscribeRemote(player_devaddr_t addr, unsigned char mode);

	PlayerTCP* ptcp;
	unsigned host;
	unsigned short port;
	char ipaddr[256];
#if defined (WIN32)
	SOCKET sock;
#else
	int sock;
#endif
	double setup_timeout;
	int kill_flag;
};

class PLAYERTCP_EXPORT TCPRemoteDriver: public RemoteDriver
{
private:
	PlayerTCP* ptcp;
	unsigned host;
	unsigned short port;
	//    QueuePointer queue, ret_queue;

	virtual RemoteConnection *CreateConnection()
	{
		return new TCPRemoteDriverConnection(ptcp, host, port);
	}
	;

/*	virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data)
	{
		PLAYER_MSG4(8, "RemoteDriver recieved message: %d %d %d %d",
				hdr->addr.host, hdr->addr.robot, hdr->addr.interf, hdr->addr.index);
		return RemoteDriver::ProcessMessage(resp_queue,hdr,data);
	}*/


public:
	TCPRemoteDriver(player_devaddr_t addr, void* arg);
	virtual ~TCPRemoteDriver();

	static Driver* TCPRemoteDriver_Init(player_devaddr_t addr, void* arg);

};

#endif
