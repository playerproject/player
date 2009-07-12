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

#ifndef REMOTE_DRIVER_H
#define REMOTE_DRIVER_H

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERCORE_EXPORT
  #elif defined (playercore_EXPORTS)
    #define PLAYERCORE_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERCORE_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERCORE_EXPORT
#endif

#include <libplayercore/playercore.h>
#include <map>
#include <list>
#include <string.h>

using namespace std;

struct PlayerAddressCompare
{
	bool operator()(const player_devaddr_t &lhs, const player_devaddr_t &rhs) const
	{
		return memcmp(&lhs,&rhs,sizeof(lhs)) < 0;
	}

};

struct PlayerQueueCompare
{
	bool operator()(const QueuePointer &lhs, const QueuePointer &rhs) const
	{
		return lhs.get() < rhs.get();
	}
};

typedef list<QueuePointer> QueueList;

class PLAYERCORE_EXPORT RemoteConnection
{
public:
	RemoteConnection() : subscription_count(0) {};
	virtual ~RemoteConnection() {if (subscription_count != 0) Disconnect();};

	virtual QueuePointer Connect() {return ConnectionQueue;};
	virtual QueuePointer Disconnect() {return ConnectionQueue;};

	virtual void Subscribe(player_devaddr_t addr) {subscription_count++;};
	virtual void Unsubscribe(player_devaddr_t addr) {subscription_count--;};

	virtual void PutMsg(player_msghdr_t* hdr, void* src) {throw "unimplemented";};

	int subscription_count;
	QueuePointer ConnectionQueue;
};

class PLAYERCORE_EXPORT RemoteDriver: public Driver {
public:
	RemoteDriver(ConfigFile *cf=NULL,int section=-1) : Driver(cf,section,false), Connected(true){};
	virtual ~RemoteDriver() {};

	virtual int Subscribe(QueuePointer &queue, player_devaddr_t addr);
	virtual int Unsubscribe(QueuePointer &queue, player_devaddr_t addr);
	virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);

protected:
	virtual void Connect(const QueuePointer &queue, const player_devaddr_t &addr);
	virtual void Disconnect(const QueuePointer &queue, const player_devaddr_t &addr);


	virtual RemoteConnection *CreateConnection() = 0;

	void ConnectAll();
	void DisconnectAll();

private:
	typedef pair<RemoteConnection*, list<player_devaddr_t> > ConnectionInfo_t;
	typedef map <QueuePointer,ConnectionInfo_t,PlayerQueueCompare> ConnectionMap_t;
	ConnectionMap_t ConnectionMap;

	typedef map <QueuePointer,QueuePointer,PlayerQueueCompare> QueueMap_t;
	QueueMap_t QueueMap;


	bool Connected;
};

#endif
