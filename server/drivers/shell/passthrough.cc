/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007 Alexis Maldonado Herrera
 *                     maldonad \/at/\ cs.tum.edu
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
 This is the new passthrough driver for Player 2.
 It does the following:
 - From the configuration file, reads requires ["SRC interface"], and provides ["DST interface"]
 - When a client connects to our DST interface, it subscribes to the SRC interface, and forwards all packets to the DST interface.
 - If the client sends a command to the DST, it gets forwarded to the SRC interfaces, and the ACK, NACK, or SYNCH answers get sent back to the DST interface
 - When the client disconnects from our DST interface, we unsubscribe from the SRC interface.
 - When forwarding packets, the header gets changed accordingly, specifically: devaddr (host, robot, interface, index)
 */

//Doxygen documentation taken partly from the old Player 1.x passthrough driver, and modified to fit the new one

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_passthrough passthrough
 * @brief General-purpose proxy driver

 The @p passthrough driver relays packets between two player interfaces.
 It connects as a @e client to an interface, and offers the same interface
 to other clients. All communication packets are forwarded between the two.

 This is specially useful to aggregate many player devices in a single Player
 Server, and control them using only one connection. Probably the most
 useful usage is to have a Player Server offering a stable set of interfaces,
 that get forwarded to the appropriate Player Servers connected to the hardware.
 If a device is moved around to a different computer, the clients don't have to
 be reconfigured, since the change is done only in the server running the passthrough
 driver.

 The passthrough driver is also able to change its remote address at runtime. To do
 this set the connect property to 0, and then change as needed the remote_host, remote_port
 and remote_index properties. When you set connect to 1, it will connect to the new address.
 Setting connect to -1 will trigger a disconnect followed by a connect allowing for seamless
 transfer to a new remote device.

 Subscribed clients will have all requests nack'd while the driver is disconnected.

 @par Compile-time dependencies

 - none

 @par Provides

 - The @p passthrough driver will support any of Player's interfaces,
 and can connect to any Player device.

 @par Requires

 - none

 @par Configuration requests

 - This driver will pass on any configuration requests.

 @par Configuration file options

 The @p passthrough driver can be used with local or remote interfaces using the
 requires section in the configuration file.

 For local interfaces, the format is:
 @verbatim
 driver
 (
 name "passthrough"
 requires ["interface:index"]  // example: ["dio:0"]
 provides ["interface:anotherindex"] // example: ["dio:25"]
 )
 @endverbatim

 To connect to an interface running on another server, the format is:
 @verbatim
 driver
 (
 name "passthrough"
 requires [":hostname:port:interface:index"]   // example: [":someserver:6665:dio:0"]
 provides ["interface:someindex"] //example: [dio:0]
 )
 @endverbatim

 Note that the in the case of connecting to remote interfaces, the provided interface can have
 any index number. The driver changes the header accordingly.


 @author Toby Collett (Inro Technologies) (Original version: Alexis Maldonado)
 */
/** @} */

#if !defined (WIN32)
	#include <unistd.h>
#endif
#include <string.h>

#include <libplayercore/playercore.h>
#include <libplayercore/remote_driver.h>

#include <vector>

using namespace std;

class PassthroughRemoteConnection: public RemoteConnection
{
public:
	PassthroughRemoteConnection(QueueList &list) :
		DriverQueuelist(list)
	{
		ConnectionQueue = QueuePointer(false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN);
		DriverQueuelist.push_back(ConnectionQueue);
	}
	;

	virtual ~PassthroughRemoteConnection()
	{
		DriverQueuelist.remove(ConnectionQueue);
	}
	;

	virtual void Subscribe(player_devaddr_t addr);
	virtual void Unsubscribe(player_devaddr_t addr);

	void PutMsg(player_msghdr_t* hdr, void* src);

	typedef map<player_devaddr_t, Device*, PlayerAddressCompare> DeviceMap_t;
	DeviceMap_t DeviceMap;

	QueueList &DriverQueuelist;
};

class PassThrough: public RemoteDriver
{
public:
	PassThrough(ConfigFile* cf, int section);
	virtual ~PassThrough()
	{
	}
	;

	virtual int Subscribe(QueuePointer &queue, player_devaddr_t addr);
	virtual int Unsubscribe(QueuePointer &queue, player_devaddr_t addr);
	virtual int ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr,
			void * data);

	void Update();

	virtual void SetMapping(const player_devaddr_t &local_address,
			const player_devaddr_t &remote_address);
	virtual void RemoveMapping(const player_devaddr_t &local_address);

	player_devaddr_t TranslateAddress(const player_devaddr_t &source_address);
	QueuePointer TranslateQueue(const QueuePointer &source_queue);

private:
	virtual RemoteConnection *CreateConnection()
	{
		return new PassthroughRemoteConnection(DriverQueueList);
	}
	;

	//Devices used to compare the header, and forward packets accordingly
	typedef pair<player_devaddr_t, player_devaddr_t> devpair;
	vector<devpair> devices;

	typedef map<player_devaddr_t, player_devaddr_t, PlayerAddressCompare>
			AddressMap_t;
	AddressMap_t AddressMap;

	QueueList DriverQueueList;

	// properties
	StringProperty RemoteHost;
	IntProperty RemotePort;

	IntProperty Connect;
};

Driver*
PassThrough_Init(ConfigFile* cf, int section)
{
	return ((Driver*) (new PassThrough(cf, section)));
}

void passthrough_Register(DriverTable* table)
{
	table->AddDriver("passthrough", PassThrough_Init);
}

void PassthroughRemoteConnection::Subscribe(player_devaddr_t addr)
{
	PLAYER_MSG4(5,"Passthrough Remote subscribing to: %d %d %d %d",addr.host,addr.robot,addr.interf,addr.index);

	Device *srcDevice = deviceTable->GetDevice(addr);

	if (!srcDevice)
	{
		PLAYER_ERROR3("Could not locate device [%d:%s:%d] for forwarding",
				addr.robot, ::lookup_interface_name(0, addr.interf), addr.index);
		throw 0;
	}
	if (srcDevice->Subscribe(ConnectionQueue) != 0)
	{
		PLAYER_ERROR("unable to subscribe to device");
		throw 0;
	}
	DeviceMap[addr] = srcDevice;

}

void PassthroughRemoteConnection::Unsubscribe(player_devaddr_t addr)
{
	if (DeviceMap.count(addr) && DeviceMap[addr])
	{
		DeviceMap[addr]->Unsubscribe(ConnectionQueue);
		DeviceMap.erase(addr);
	}
}

void PassthroughRemoteConnection::PutMsg(player_msghdr_t* hdr, void* src)
{
	if (DeviceMap.count(hdr->addr) && DeviceMap[hdr->addr])
	{
		DeviceMap[hdr->addr]->PutMsg(ConnectionQueue, hdr, src);
	}
	else
	{
		PLAYER_MSG4(8,"Passthrough recieved message for null device: %d %d %d %d",hdr->addr.host, hdr->addr.robot,hdr->addr.interf,hdr->addr.index);
	}
}

PassThrough::PassThrough(ConfigFile* cf, int section) :
	RemoteDriver(cf, section), RemoteHost("remote_host", "", false, this, cf,
			section), RemotePort("remote_port", -1, false, this, cf, section),
			Connect("connect", 1, false, this, cf, section)
{
	int device_count = cf->GetTupleCount(section, "provides");
	if (device_count != cf->GetTupleCount(section, "requires"))
	{
		PLAYER_ERROR("Mismatched number of entries in provides and requires");
		SetError(-1);
		return;
	}
	for (int i = 0; i < device_count; ++i)
	{
		player_devaddr_t local, remote;
		if (cf->ReadDeviceAddr(&local, section, "provides", -1, i, NULL) != 0)
		{
			PLAYER_ERROR("PassThrough: Bad 'provides' section, aborting.");
			SetError(-1);
			return;
		}
		if (cf->ReadDeviceAddr(&remote, section, "requires", -1, i, NULL) != 0)
		{
			PLAYER_ERROR("PassThrough: Bad 'requires' section, aborting.");
			SetError(-1);
			return;
		}

		devices.push_back(devpair(local, remote));
		if (AddInterface(local) != 0)
		{
			SetError(-1);
			return;
		}
		SetMapping(local, remote);
	}

	if (Connect.GetValue())
	{
		ConnectAll();
	}

	return;
}

int PassThrough::Subscribe(QueuePointer &queue, player_devaddr_t addr)
{
	return RemoteDriver::Subscribe(queue, TranslateAddress(addr));
}

int PassThrough::Unsubscribe(QueuePointer &queue, player_devaddr_t addr)
{
	return RemoteDriver::Unsubscribe(queue, TranslateAddress(addr));
}

int PassThrough::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr,
		void * data)
{
	//PLAYER_MSG4(8,"Passthrough recieved message: %d %d %d %d",hdr->addr.host, hdr->addr.robot,hdr->addr.interf,hdr->addr.index);


	// let our properties through
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_STRPROP_REQ))
	{
		player_strprop_req_t req =
				*reinterpret_cast<player_strprop_req_t*> (data);
		if (strcmp("remote_host", req.key) == 0)
		{
			uint32_t newhost;
			// assume it's a string containing a hostname or IP address
			if (hostname_to_packedaddr(&newhost, req.value) < 0)
			{
				PLAYER_ERROR1("name lookup failed for host \"%s\"", req.value);
				return -1;
			}
			for (vector<devpair>::iterator itr = devices.begin(); itr
					!= devices.end(); ++itr)
			{
				itr->second.host = newhost;
				SetMapping(itr->first, itr->second);
			}
			return -1;
		}
	}

	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_INTPROP_REQ))
	{
		player_intprop_req_t req =
				*reinterpret_cast<player_intprop_req_t*> (data);
		if (strcmp("remote_port", req.key) == 0)
		{
			for (vector<devpair>::iterator itr = devices.begin(); itr
					!= devices.end(); ++itr)
			{
				itr->second.robot = req.value;
				SetMapping(itr->first, itr->second);
			}
			return -1;
		}
		if (strcmp("connect", req.key) == 0)
		{
			if (req.value == 0) // disconnect
			{
				DisconnectAll();
			}
			else if (req.value == 1) // connect
			{
				ConnectAll();
			}
			else if (req.value == -1) // reconnect (with new address if its changed)
			{
				DisconnectAll();
				ConnectAll();
			}

			return -1;
		}
	}

	player_msghdr newhdr = *hdr;
	// if we have an address mapping use it
	newhdr.addr = TranslateAddress(hdr->addr);

	return RemoteDriver::ProcessMessage(resp_queue, &newhdr, data);
}

void PassThrough::Update()
{
	// here we also need to process the messages that are on out RemoteConnection queues, i.e. the data for our clients
	ProcessMessages();
	// also need to check for messages on any of our remote connections
	for (QueueList::iterator itr = DriverQueueList.begin(); itr
			!= DriverQueueList.end(); ++itr)
	{

		Message * msg;
		while ((msg = (*itr)->Pop()))
		{
			player_msghdr * hdr = msg->GetHeader();
			void * data = msg->GetPayload();
			ProcessMessage(*itr, hdr, data);
			delete msg;
		}

	}
}

void PassThrough::SetMapping(const player_devaddr_t &local_address,
		const player_devaddr_t &remote_address)
{
	AddressMap[local_address] = remote_address;
	AddressMap[remote_address] = local_address;
}

void PassThrough::RemoveMapping(const player_devaddr_t &local_address)
{
	AddressMap.erase(AddressMap[local_address]);
	AddressMap.erase(local_address);
}

player_devaddr_t PassThrough::TranslateAddress(
		const player_devaddr_t &source_address)
{
	if (AddressMap.count(source_address) > 0)
		return AddressMap[source_address];
	return source_address;
}
