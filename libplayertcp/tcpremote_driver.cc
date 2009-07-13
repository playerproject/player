/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) <insert dates here>
 *     <insert author's name(s) here>
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#if !defined (WIN32)
	#include <unistd.h>
#endif

#if ENABLE_TCP_NODELAY
#include <netinet/tcp.h>
#endif

#include <libplayercore/globals.h>
#include <libplayercommon/playercommon.h>
#include <libplayerinterface/playerxdr.h>
#include "tcpremote_driver.h"
#include "playertcp_errutils.h"

QueuePointer TCPRemoteDriverConnection::Connect()
{
	struct sockaddr_in server;
	char banner[PLAYER_IDENT_STRLEN];
	int numread, thisnumread;
	double t1, t2;
#if defined (WIN32)
  unsigned long setting = 1;
#else
  int flags;                  /* temp for old socket access flags */
#endif

	// if kill_flag is set then we cant use the ptcp object
	if (kill_flag)
		return QueuePointer();

	packedaddr_to_dottedip(this->ipaddr, sizeof(this->ipaddr), host);

	// We can't talk to ourselves
	if (this->ptcp->GetHost() == host && this->ptcp->Listening(port))
	{
		PLAYER_ERROR3("tried to connect to self (%s:%d:%d)\n", this->ipaddr,
				host, port);
		throw "Tried to connect to self";
	}

	// Construct socket
#if defined (WIN32)
	if((this->sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
#else
	if((this->sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
#endif
	{
		STRERROR (PLAYER_ERROR1, "socket() failed; socket not created: %s");
		throw "Connection failed";
	}

	server.sin_family = PF_INET;
	server.sin_addr.s_addr = host;
	server.sin_port = htons(port);

	// Connect the socket
	if (connect(this->sock, (struct sockaddr*) &server, sizeof(server)) < 0)
	{
		PLAYER_ERROR3("connect call on [%s:%u] failed with error [%s]",
				this->ipaddr, port, strerror(errno));
#if defined (WIN32)
		closesocket(this->sock);
#else
		close(this->sock);
#endif
		sock=-1;
		throw "Connection Failed";
	}

	PLAYER_MSG2(2, "connected to: %s:%u\n", this->ipaddr, host);

	// make the socket non-blocking
#if defined (WIN32)
	if (ioctlsocket(sock, FIONBIO, &setting) == SOCKET_ERROR)
	{
		STRERROR(PLAYER_ERROR1, "ioctlsocket error: %s");
		closesocket(sock);
		sock=INVALID_SOCKET;
		throw "Connection failed";
	}
#else
	/*
	* get the current access flags
	*/
	if((flags = fcntl(sock, F_GETFL)) == -1)
	{
		perror("create_and_bind_socket():fcntl() while getting socket "
				"access flags; socket not created.");
		close(sock);
		sock=-1;
		throw "Connection failed";
	}
	/*
	* OR the current flags with O_NONBLOCK (so we won't block),
	* and write them back
	*/
	if(fcntl(sock, F_SETFL, flags | O_NONBLOCK ) == -1)
	{
		perror("create_and_bind_socket():fcntl() failed while setting socket "
				"access flags; socket not created.");
		close(sock);
		sock=-1;
		throw "Connection failed";
	}
#endif // defined (WIN32)

#if ENABLE_TCP_NODELAY
	// Disable Nagel's algorithm for lower latency
	int yes = 1;
	if (setsockopt(this->sock, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int))
			== -1)
	{
		PLAYER_ERROR("failed to enable TCP_NODELAY - setsockopt failed");
		close(this->sock);
		sock=-1;
		throw "Connection Failed";
	}
#endif

	// Get the banner.
	GlobalTime->GetTimeDouble(&t1);
	numread = 0;
	while (numread < (int) sizeof(banner))
	{
		if ((thisnumread = recv(this->sock, banner + numread, sizeof(banner)
				- numread, 0)) < 0)
		{
			if (errno!= EAGAIN)
			{
				PLAYER_ERROR("error reading banner from remote device");
#if defined (WIN32)
				closesocket(this->sock);
#else
				close(this->sock);
#endif
				sock=-1;
				throw "Connection Failed";
			}
		}
		else
			numread += thisnumread;
		GlobalTime->GetTimeDouble(&t2);
		if ((t2 - t1) > this->setup_timeout)
		{
			PLAYER_ERROR("timed out reading banner from remote server");
#if defined (WIN32)
			closesocket(this->sock);
#else
			close(this->sock);
#endif
			sock=-1;
			throw "Connection Failed";
		}
	}

	PLAYER_MSG0(5, "Adding new TCPRemoteDriver to the PlayerTCP Client List");

	// Add this socket for monitoring
	this->kill_flag = 0;

	// create a place holder queue, but we cant actually add the client connection until we get a subscription
	ConnectionQueue = QueuePointer(false,PLAYER_MSGQUEUE_DEFAULT_MAXLEN);

	return ConnectionQueue;
}

QueuePointer TCPRemoteDriverConnection::Disconnect()
{
	QueuePointer ret=ConnectionQueue;
	ConnectionQueue = QueuePointer();
	return ret;
}

void TCPRemoteDriverConnection::PutMsg(player_msghdr_t* hdr, void* src)
{
	Message msg(*hdr,src,ConnectionQueue);
	ConnectionQueue->Push(msg);
}

void TCPRemoteDriverConnection::Subscribe(player_devaddr_t addr)
{
	if (SubscribeRemote(addr, PLAYER_OPEN_MODE) != 0)
		throw "Failed to subscribe";

	if (kill_flag)
		return;

	// we need to do this after the subscription as we directly
	// manipulate the socket in our subscribe and unsubscribe
	this->ptcp->AddClient(NULL, host, port, this->sock,
			false, &this->kill_flag, pthread_equal(this->ptcp->thread, pthread_self()),ConnectionQueue);

}

void TCPRemoteDriverConnection::Unsubscribe(player_devaddr_t addr)
{
	// Do this before we unsubscribe as we need direct access to the socket
	// Have we already been killed?
	if (!this->kill_flag)
	{
		// Set the delete flag, letting PlayerTCP close the connection and
		// clean up.
		this->ptcp->DeleteClient(ConnectionQueue, pthread_equal(this->ptcp->thread,
				pthread_self()));
		kill_flag = 1;
	}


	if (SubscribeRemote(addr, PLAYER_CLOSE_MODE) != 0)
		throw "Failed to unsubscribe";
}

// (un)subscribe to the remote device
int TCPRemoteDriverConnection::SubscribeRemote(player_devaddr_t addr,
		unsigned char mode)
{
	int encode_msglen;
	unsigned char buf[512];
	player_msghdr_t hdr;
	player_device_req_t req;
	double t1, t2;

	int numbytes, thisnumbytes;

	// if we have no socket then we dont need to unsub
#if defined (WIN32)
	if (sock == INVALID_SOCKET)
#else
	if (sock < 0)
#endif
		return 0;

	memset(&hdr, 0, sizeof(hdr));
	hdr.addr.interf = PLAYER_PLAYER_CODE;
	hdr.type = PLAYER_MSGTYPE_REQ;
	hdr.subtype = PLAYER_PLAYER_REQ_DEV;
	GlobalTime->GetTimeDouble(&hdr.timestamp);

	PLAYER_MSG4(8,"TCPRemote sub for: %d %d %d %d",addr.host, addr.robot,addr.interf,addr.index);


	req.addr = addr;
	req.access = mode;
	req.driver_name_count = 0;

	// Encode the body
	if ((encode_msglen = player_device_req_pack(buf + PLAYERXDR_MSGHDR_SIZE,
			sizeof(buf) - PLAYERXDR_MSGHDR_SIZE, &req, PLAYERXDR_ENCODE)) < 0)
	{
		PLAYER_ERROR("failed to encode request");
		return (-1);
	}

	// Rewrite the size in the header with the length of the encoded
	// body, then encode the header.
	hdr.size = encode_msglen;
	if (player_msghdr_pack(buf, PLAYERXDR_MSGHDR_SIZE, &hdr, PLAYERXDR_ENCODE)
			< 0)
	{
		PLAYER_ERROR("failed to encode header");
		return (-1);
	}

	encode_msglen += PLAYERXDR_MSGHDR_SIZE;

	// Send the request
	GlobalTime->GetTimeDouble(&t1);
	numbytes = 0;

	while (numbytes < encode_msglen)
	{
		if ((thisnumbytes = send(this->sock, reinterpret_cast<const char*> (buf + numbytes), encode_msglen
				- numbytes, 0)) < 0)
		{
			if (errno!= EAGAIN)
			{
				PLAYER_ERROR1("write failed: %s", strerror(errno));
				return (-1);
			}
		}
		else
		{
			numbytes += thisnumbytes;
		}
		GlobalTime->GetTimeDouble(&t2);
		if ((t2 - t1) > this->setup_timeout)
		{
			PLAYER_ERROR(
					"timed out sending subscription request to remote server");
			return (-1);
		}
	}

	// If we're closing, then stop here; we don't need to hear the response.
	// In any case, explicitly unsubscribing is just a courtesy.
	if (mode == PLAYER_CLOSE_MODE)
	{
		PLAYER_MSG5(5, "unsubscribed from remote device %s:%d:%d:%d (%s)",
				this->ipaddr, addr.robot, addr.interf,
				addr.index, req.driver_name);

		return (0);
	}

	// Receive the response header
	GlobalTime->GetTimeDouble(&t1);
	numbytes = 0;
	while (numbytes < PLAYERXDR_MSGHDR_SIZE)
	{
		if ((thisnumbytes = recv(this->sock, reinterpret_cast<char*> (buf + numbytes),
				PLAYERXDR_MSGHDR_SIZE - numbytes, 0)) < 0)
		{
			if (errno!= EAGAIN)
			{
				PLAYER_ERROR1("read failed: %s", strerror(errno));
				return (-1);
			}
		}
		else
			numbytes += thisnumbytes;
		GlobalTime->GetTimeDouble(&t2);
		if ((t2 - t1) > this->setup_timeout)
		{
			PLAYER_ERROR("timed out reading response header from remote server");
			return (-1);
		}
	}

	// Decode the header
	if (player_msghdr_pack(buf, PLAYERXDR_MSGHDR_SIZE, &hdr, PLAYERXDR_DECODE)
			< 0)
	{
		PLAYER_ERROR("failed to decode header");
		return (-1);
	}

	// Is it the right kind of message?
	if (!Message::MatchMessage(&hdr, PLAYER_MSGTYPE_RESP_ACK,
	PLAYER_PLAYER_REQ_DEV, hdr.addr))
	{
		PLAYER_ERROR4("got wrong kind of reply %d:%d %d:%d",hdr.type,hdr.subtype,hdr.addr.interf,hdr.addr.index);
		PLAYER_ERROR4("was trying to subscribe to  %d:%d %d:%d",addr.host,addr.robot,addr.interf,addr.index);
		return (-1);
	}

	// Receive the response body
	GlobalTime->GetTimeDouble(&t1);
	numbytes = 0;
	while (numbytes < (int) hdr.size)
	{
		if ((thisnumbytes = recv(this->sock, reinterpret_cast<char*> (buf + PLAYERXDR_MSGHDR_SIZE
				+ numbytes), hdr.size - numbytes, 0)) < 0)
		{
			if (errno!= EAGAIN)
			{
				PLAYER_ERROR1("read failed: %s", strerror(errno));
				return (-1);
			}
		}
		else
			numbytes += thisnumbytes;
		GlobalTime->GetTimeDouble(&t2);
		if ((t2 - t1) > this->setup_timeout)
		{
			PLAYER_ERROR("timed out reading response body from remote server");
			return (-1);
		}
	}

	memset(&req, 0, sizeof(req));
	// Decode the body
	if (player_device_req_pack(buf + PLAYERXDR_MSGHDR_SIZE, hdr.size, &req,
			PLAYERXDR_DECODE) < 0)
	{
		PLAYER_ERROR("failed to decode reply");
		return (-1);
	}

	// Did we get the right access?
	if (req.access != mode)
	{
		PLAYER_ERROR("got wrong access");
		return (-1);
	}


	PLAYER_MSG0(5,
			"Adding new TCPRemoteDriver to the PlayerTCP Client List...Success");


	// Success!
	PLAYER_MSG5(5, "subscribed to remote device %s:%d:%d:%d (%s)",
			this->ipaddr, addr.robot, addr.interf,
			addr.index, req.driver_name);

	return (0);
}

TCPRemoteDriver::TCPRemoteDriver(player_devaddr_t addr, void* arg) :
	RemoteDriver(NULL, 0)
{
	ptcp = reinterpret_cast<PlayerTCP*> (arg);
	host = addr.host;
	port = addr.robot;
	// no need to call add device, this is done already by device table for remote devices
}

TCPRemoteDriver::~TCPRemoteDriver()
{
}

Driver*
TCPRemoteDriver::TCPRemoteDriver_Init(player_devaddr_t addr, void* arg)
{
	return ((Driver*) (new TCPRemoteDriver(addr, arg)));
}
