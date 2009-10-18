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

#include "remote_driver.h"

int RemoteDriver::Subscribe(QueuePointer &queue,
		player_devaddr_t local_addr)
{
	try
	{
		// create our connection proxy
		if (ConnectionMap[queue].first == NULL)
		{
			PLAYER_MSG0(9, "Creating new remote connection mapping");
			ConnectionMap[queue].first = CreateConnection();
		}
		// register the subscription
		ConnectionMap[queue].second.push_back(local_addr);

		Connect(queue, local_addr);
	} catch (...)
	{
		PLAYER_MSG4(4, "Connection to remote device failed: %d %d %d %d",local_addr.host,local_addr.robot,local_addr.interf,local_addr.index);
		// ignore exceptions from Unsubscribe here
		try
		{
			Unsubscribe(queue,local_addr);
		} catch (...) {}
		return -1;
	}
	return Driver::Subscribe(queue, local_addr);

}

void RemoteDriver::Connect(const QueuePointer &queue, const player_devaddr_t &local_addr)
{

	// if our driver is in a connected state actually connect to the remote end
	if (Connected)
	{
		PLAYER_MSG0(9, "Remote driver connecting to remote device");
		// check if we have a queue yet
		if (ConnectionMap[queue].first->subscription_count == 0)
		{
			QueuePointer RemoteQueue = ConnectionMap[queue].first->Connect();
			QueueMap[RemoteQueue] = queue;
		}
		// now subscribe to the new address
		ConnectionMap[queue].first->Subscribe(local_addr);
	}

}

int RemoteDriver::Unsubscribe(QueuePointer &queue, player_devaddr_t local_addr)
{
	RemoteDriver::Disconnect(queue, local_addr);

	// remove the subscription record
	for (list<player_devaddr_t>::iterator itr =
			ConnectionMap[queue].second.begin(); itr
			!= ConnectionMap[queue].second.end(); ++itr)
	{
		if (memcmp(&(*itr), &local_addr, sizeof(local_addr)) == 0)
		{
			ConnectionMap[queue].second.erase(itr);
			break;
		}
	}
	// if last subscription from the client remote its connection info
	if (ConnectionMap[queue].second.empty())
	{
		delete ConnectionMap[queue].first;
		ConnectionMap.erase(queue);
	}
	Driver::Unsubscribe(queue, local_addr);
	return 0;
}

void RemoteDriver::Disconnect(const QueuePointer &queue, const player_devaddr_t &local_addr)
{
	if (Connected)
	{
		try
		{
			ConnectionMap[queue].first->Unsubscribe(local_addr);
		}
		catch (...)
		{
			PLAYER_ERROR("Failed to correctly unsubscribe from remote driver, may result in driver not getting correctly cleaned up");
			return;
		}
		if (ConnectionMap[queue].first->subscription_count == 0)
		{
			QueuePointer RemoteQueue = ConnectionMap[queue].first->Disconnect();
			QueueMap.erase(RemoteQueue);
		}
	}
}

int RemoteDriver::ProcessMessage(QueuePointer & resp_queue,
		player_msghdr * hdr, void * data)
{
/*
	PLAYER_MSG4(8, "RemoteDriver recieved message: %d %d %d %d",
			hdr->addr.host, hdr->addr.robot, hdr->addr.interf, hdr->addr.index);
*/

	// silence warning etc while we are not connected
	if (!Connected)
	{
		if (hdr->type == PLAYER_MSGTYPE_REQ)
		{
			Publish(hdr->addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK,
					hdr->subtype);
		}
		return 0;
	}

	// if it is an incoming message post it to the connection object
	if (hdr->type == PLAYER_MSGTYPE_REQ || hdr->type == PLAYER_MSGTYPE_CMD)
	{
		// if it came from a client, forward it to the remote
		if (ConnectionMap.count(resp_queue) < 1 || ConnectionMap[resp_queue].first == NULL)
		{
			PLAYER_ERROR4("RemoteDriver recieved message from a client that was not subscribed to it: %d %d %d %d",
						hdr->addr.host, hdr->addr.robot, hdr->addr.interf, hdr->addr.index);
			return -1;
		}
		ConnectionMap[resp_queue].first->PutMsg(hdr, data);
	}
	else
	{
		// if it is another type of message then forward it to a client
		if (QueueMap.count(resp_queue) > 0)
		{
			Publish(QueueMap[resp_queue], hdr, data);
		}
	}

	return 0;
}

void RemoteDriver::ConnectAll()
{
	if (Connected)
		return;

	// we are changing connection state so all subscribed devices from all clients need set up
	for (ConnectionMap_t::iterator itr = ConnectionMap.begin(); itr
			!= ConnectionMap.end(); ++itr)
	{

		QueueMap[itr->first] = itr->second.first->Connect();
		// add the reverse mapping as well
		QueueMap[QueueMap[itr->first]] = itr->first;
		for (list<player_devaddr_t>::iterator d_itr =
				itr->second.second.begin(); d_itr != itr->second.second.end(); ++d_itr)
		{
			Connect(itr->first,*d_itr);
		}
	}
	Connected = true;
}

void RemoteDriver::DisconnectAll()
{
	if (!Connected)
		return;

	// we are changing connection state so all subscribed devices from all clients need disconnected
	for (ConnectionMap_t::iterator itr = ConnectionMap.begin(); itr
			!= ConnectionMap.end(); ++itr)
	{
		for (list<player_devaddr_t>::iterator d_itr =
				itr->second.second.begin(); d_itr != itr->second.second.end(); ++d_itr)
		{
			Disconnect(itr->first,*d_itr);
		}
	}
}

