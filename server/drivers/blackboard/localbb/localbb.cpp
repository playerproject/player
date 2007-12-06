/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004  Brian Gerkey gerkey@stanford.edu    
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
 * $Id$
 *
 * 
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_LocalBB LocalBB
 * @brief Local memory implementation of a blackboard. The data entries are stored internally in a hash-map.
 * Internally information is stored in two hash-maps. One hash-map contains a map of labels to the entry data.
 * This stores the actual data. The second hash-map stores a map of device queues which are listening to an entry.
 * These are the devices that are sent events when an entry is updated.
 * CAVEATS:
 *  -There is no checking to see if a device is already subscribed to a key. If a device subscribes to a key twice, it will receive two updates.
 *  -All listening devices are sent updates when an entry is set, even if that device set the entry.

 @par Provides

 - @ref interface_blackboard

 @par Requires

 - None

 @par Configuration requests

 - None

 @par Configuration file options

 @par Example 

 @verbatim
 driver
 (
		name "localbb"
		provides [ "blackboard:0" ]
 )
 @endverbatim

 @author Ben Morelli

 */

/** @} */

#include <sys/types.h> // required by Darwin
#include <stdlib.h>
#include <libplayercore/playercore.h>
#include <libplayercore/error.h>
#include <libplayerc++/playererror.h>
#include <vector>
#include <map>
#include <strings.h>
#include <iostream>

/**@brief Custom blackboard-entry data representation used internally by the driver. */
typedef struct EntryData
{
	/** Constructor. Sets all members to 0 or NULL. */
  EntryData() { interf = 0; type = 0; subtype = 0; data_count = 0; data = NULL; timestamp_sec = 0; timestamp_usec = 0; }
  //~EntryData() { if (data != NULL) delete [] data; } Why doesn't it like this?
  
  /** Player interface */
  uint16_t interf;
  /** Message type */
  uint8_t type;
  /** Message sub-type */
  uint8_t subtype;

  /** Data size */
  uint32_t data_count;
  /** Data */
  uint8_t* data;
  /** Time entry created */
  uint32_t timestamp_sec;
  uint32_t timestamp_usec;
} EntryData;

/**@brief Custom blackboard entry representation used internally by the driver.*/
typedef struct BlackBoardEntry
{
	/** Constructor. Sets key to an empty string. Data should be automatically set to empty values. */
  BlackBoardEntry() { key = ""; }

  /** Entry label */
  string key;
  /** Entry data */
  EntryData data;
} BlackBoardEntry;


// Function prototypes
/** Convienience function to convert from the internal blackboard entry representation to the player format. The user must clean-up the player blackboard entry. */
player_blackboard_entry_t ToPlayerBlackBoardEntry(const BlackBoardEntry &entry);

/** Convienience function to convert from the player blackboard entry to the internal representation. */
BlackBoardEntry FromPlayerBlackBoardEntry(const player_blackboard_entry_t &entry);

// Remember to clean-up the entry afterwards. Delete the key and data.
player_blackboard_entry_t ToPlayerBlackBoardEntry(const BlackBoardEntry &entry)
{
  player_blackboard_entry_t result;
  memset(&result, 0, sizeof(player_blackboard_entry_t));
  result.interf = entry.data.interf;
  result.type = entry.data.type;
  result.subtype = entry.data.subtype;
  result.key_count = strlen(entry.key.c_str()) + 1;
  result.key = new char[result.key_count]; //strdup(entry.key.c_str());
  memcpy(result.key, entry.key.c_str(), result.key_count);
  result.data_count = entry.data.data_count;
  result.data = new uint8_t[result.data_count];
  memcpy(result.data, entry.data.data, result.data_count);
  result.timestamp_sec = entry.data.timestamp_sec;
  result.timestamp_usec = entry.data.timestamp_usec;
  return result;
}

// This entry will be cleaned-up automatically by the destructor.
BlackBoardEntry FromPlayerBlackBoardEntry(const player_blackboard_entry_t &entry)
{
  BlackBoardEntry result;
  result.data.interf = entry.interf;
  result.data.type = entry.type;
  result.data.subtype = entry.subtype;
  assert(entry.key != NULL);
  result.key = string(entry.key);
  result.data.data_count = entry.data_count;
  result.data.data = new uint8_t[result.data.data_count];
  memcpy(result.data.data, entry.data, result.data.data_count);
  result.data.timestamp_sec = entry.timestamp_sec;
  result.data.timestamp_usec = entry.timestamp_usec;
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/** Local memory blackboard driver. Stores entries in a hash-map in local memory. */
class LocalBB : public Driver
{
	public:
		/** Default constructor. */
		LocalBB(ConfigFile* cf, int section);
		/** Default destructor. */
		~LocalBB();
		/** Driver initialisation. */
		int Setup();
		/** Driver de-initialisation. */
		int Shutdown();

		// MessageHandler
		/** Process incoming messages. */
		int ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data);
	private:
		// Message subhandlers
		/** Process a subscribe to key message. */
		int ProcessSubscribeKeyMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data);
		/** Process an unsubscribe from key message. */
		int ProcessUnsubscribeKeyMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data);
		/** Process a set entry message. */
		int ProcessSetEntryMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data);

		// Blackboard handler functions
		/** Add the key and queue combination to the listeners hash-map and return the entry for the key. */
		BlackBoardEntry SubscribeKey(const string &key, const QueuePointer &resp_queue);
		/** Remove the key and queue combination from the listeners hash-map. */
		void UnsubscribeKey(const string &key, const QueuePointer &qp);
		/** Set the entry in the entries hashmap. */
		void SetEntry(const BlackBoardEntry &entry);
		
		// Helper functions
		/** Check that the message has a valid size. */
		bool CheckHeader(player_msghdr * hdr);

		// Internal blackboard data
		/** Map of labels to entry data. */
		map<string, BlackBoardEntry> entries;
		/** Map of labels to listening queues. */
		map<string, vector<QueuePointer> > listeners;
};

////////////////////////////////////////////////////////////////////////////////
// Factory method.
Driver* LocalBB_Init(ConfigFile* cf, int section)
{
	return ((Driver*)(new LocalBB(cf, section)));
}

////////////////////////////////////////////////////////////////////////////////
// Driver registration function
void LocalBB_Register(DriverTable* table)
{
	table->AddDriver("localbb", LocalBB_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.
LocalBB::LocalBB(ConfigFile* cf, int section) :
	Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_BLACKBOARD_CODE)
{
	// No settings needed currently.
}

////////////////////////////////////////////////////////////////////////////////
// Destructor.
LocalBB::~LocalBB()
{
	// maps clean up themselves
}

////////////////////////////////////////////////////////////////////////////////
// Load resources.
int LocalBB::Setup()
{
	PLAYER_MSG0(2, "LocalBB ready");
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Clean up resources.
int LocalBB::Shutdown()
{
	PLAYER_MSG0(2, "LocalBB shut down");
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message.
int LocalBB::ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data)
{
	// Request for a subscription
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_BLACKBOARD_REQ_SUBSCRIBE_TO_KEY, this->device_addr))
	{
		return ProcessSubscribeKeyMessage(resp_queue, hdr, data);
	}
	// Request for unsubscribe
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_BLACKBOARD_REQ_UNSUBSCRIBE_FROM_KEY, this->device_addr))
	{
		return ProcessUnsubscribeKeyMessage(resp_queue, hdr, data);
	}
	// Request to update an entry
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_BLACKBOARD_REQ_SET_ENTRY, this->device_addr))
	{
		return ProcessSetEntryMessage(resp_queue, hdr, data);
	}
	// Don't know how to handle this message
	return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Subscribe a device to a key.
int LocalBB::ProcessSubscribeKeyMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data)
{
	if (!CheckHeader(hdr))
		return -1;

	// Add the device to the listeners map
	player_blackboard_entry_t *request = reinterpret_cast<player_blackboard_entry_t*>(data);
	BlackBoardEntry current_value = SubscribeKey(request->key , resp_queue);

	// Get the entry for the given key
	player_blackboard_entry_t response = ToPlayerBlackBoardEntry(current_value);
	size_t response_size = sizeof(player_blackboard_entry_t) + response.key_count + response.data_count;

	// Publish the blackboard entry
	this->Publish(
		this->device_addr,
		resp_queue,
		PLAYER_MSGTYPE_RESP_ACK,
		PLAYER_BLACKBOARD_REQ_SUBSCRIBE_TO_KEY,
		&response,
		response_size,
		NULL);

	if (response.key)
	{
		delete [] response.key;
	}
	if (response.data)
	{
		delete [] response.data;
	}	

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Unsubscribe a device from a key.
int LocalBB::ProcessUnsubscribeKeyMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data)
{
	if (!CheckHeader(hdr))
		return -1;

	// Remove the device from the listeners map
	player_blackboard_entry_t *request = reinterpret_cast<player_blackboard_entry_t*>(data);
	UnsubscribeKey(request->key, resp_queue);

	// Send back an empty ack
	this->Publish(
		this->device_addr,
		resp_queue,
		PLAYER_MSGTYPE_RESP_ACK,
		PLAYER_BLACKBOARD_REQ_UNSUBSCRIBE_FROM_KEY,
		NULL,
		0,
		NULL);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Set an entry and send out update events to all listeners.
int LocalBB::ProcessSetEntryMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data)
{
	if (!CheckHeader(hdr))
		return -1;

	player_blackboard_entry_t *request = reinterpret_cast<player_blackboard_entry_t*>(data);
	BlackBoardEntry entry = FromPlayerBlackBoardEntry(*request);

	SetEntry(entry);
	
	// Send out update events to other listening devices
	vector<QueuePointer> &devices = listeners[entry.key];
	for (vector<QueuePointer>::iterator itr=devices.begin(); itr != devices.end(); itr++)
	{
		QueuePointer device_queue = (*itr);
		this->Publish(this->device_addr,
			device_queue,
			PLAYER_MSGTYPE_DATA,
			PLAYER_BLACKBOARD_DATA_UPDATE,
			data,
			hdr->size,
			NULL);
	}
	
	// Send back an empty ack
	this->Publish(this->device_addr,
		resp_queue,
		PLAYER_MSGTYPE_RESP_ACK,
		PLAYER_BLACKBOARD_REQ_SET_ENTRY,
		NULL,
		0,
		NULL);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Add a device to the listener list for a key. Return the current value of the entry.
BlackBoardEntry LocalBB::SubscribeKey(const string &key, const QueuePointer &resp_queue)
{
	listeners[key].push_back(resp_queue);
	BlackBoardEntry entry = entries[key];
	if (entry.key == "")
	{
		entry.key = key;
	}
	return entry;
}

////////////////////////////////////////////////////////////////////////////////
// Remove a device from the listener list for a key.
void LocalBB::UnsubscribeKey(const string &key, const QueuePointer &qp)
{
	vector<QueuePointer> &devices = listeners[key];

	for (vector<QueuePointer>::iterator itr = devices.begin(); itr != devices.end(); itr++)
	{
		if ((*itr) == qp)
		{
			devices.erase(itr);
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// Set entry value in the entries map.
void LocalBB::SetEntry(const BlackBoardEntry &entry)
{
	entries[entry.key] = entry;
}

////////////////////////////////////////////////////////////////////////////////
// Check that we have some data in the message.
bool LocalBB::CheckHeader(player_msghdr * hdr)
{
	if (hdr->size <= 0)
	{
		PLAYER_ERROR2("request is wrong length (%d <= %d); ignoring", hdr->size, 0);
		return false;
	}
	return true;
}
