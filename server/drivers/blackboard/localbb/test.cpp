/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007
 *     Ben Morelli
 *
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
 */


#include <libplayerc/playerc.h>
#include <iostream>

#define KEY "test"

using namespace std;

void On_First_Device_Event(player_blackboard_entry_t event)
{
	printf("First device event fired for key '%s'\n", event.key);
	printf("Key value = %d,%d,%d,%d\n", event.data[0], event.data[1], event.data[2], event.data[3]);
}

void On_Second_Device_Event(player_blackboard_entry_t event)
{
	printf("Second device event fired for key '%s'\n", event.key);
	printf("Key value = %d,%d,%d,%d\n", event.data[0], event.data[1], event.data[2], event.data[3]);
}

class BlackBoardTester
{
	public:
		bool Initialise();
		bool CreateFirstDevice();
		bool CreateSecondDevice();
		bool SubscribeFirstDevice();
		bool SubscribeSecondDevice();
		bool SubscribeFirstDeviceToKey();
		bool SubscribeSecondDeviceToKey();
		bool SetFirstDeviceEntry();
		bool SetSecondDeviceEntry();
		bool UnsubscribeFirstDeviceKey();
		bool UnsubscribeSecondDeviceKey();
		bool UnsubscribeFirstDevice();
		bool UnsubscribeSecondDevice();
		void Read();
		bool Shutdown();

	private:
		playerc_client_t* client_first;
		playerc_client_t* client_second;
		playerc_blackboard_t* first;
		playerc_blackboard_t* second;

};

bool BlackBoardTester::Initialise()
{
	client_first = NULL;
	client_second = NULL;
	first = NULL;
	second = NULL;

	client_first = playerc_client_create(NULL, "localhost", 6665);
	if (0 != playerc_client_connect(client_first))
	{
		printf("Error connecting first client\n");
		return false;
	}
	//playerc_client_datamode(client_first, PLAYERC_DATAMODE_PULL);

	client_second = playerc_client_create(NULL, "localhost", 6665);
	if (0 != playerc_client_connect(client_second))
	{
		printf("Error connecting second client\n");
		return false;
	}
	//playerc_client_datamode(client_second, PLAYERC_DATAMODE_PULL);

	return true;
}

bool BlackBoardTester::CreateFirstDevice()
{
	first = playerc_blackboard_create(client_first, 0);
	if (first == NULL)
	{
		printf("Failed to create first device.\n");
		return false;
	}
	first->on_blackboard_event = On_First_Device_Event;
	return true;

}

bool BlackBoardTester::CreateSecondDevice()
{
	second = playerc_blackboard_create(client_second, 0);
	if (second == NULL)
	{
		printf("Failed to create second device.\n");
		return false;
	}
	second->on_blackboard_event = On_Second_Device_Event;
	return true;
}

bool BlackBoardTester::SubscribeFirstDevice()
{
	if (playerc_blackboard_subscribe(first, PLAYER_OPEN_MODE))
	{
		printf("Error subscribing first\n");
		return false;
	}
	return true;
}

bool BlackBoardTester::SubscribeSecondDevice()
{
	if (playerc_blackboard_subscribe(second, PLAYER_OPEN_MODE))
	{
		printf("Error subscribing second\n");
		return false;
	}
	return true;
}

bool BlackBoardTester::SubscribeFirstDeviceToKey()
{
	player_blackboard_entry_t *t = new player_blackboard_entry_t;
	memset(t, 0, sizeof(t));
	const char* key = strdup(KEY);
	if (playerc_blackboard_subscribe_to_key(first, key, &t))
	{
		printf("Error subscribing to key '%s'\n", KEY);
		return false;
	}

	if (t->data_count == 0)
	{
		printf("Key '%s' does not exist (EMPTY)\n", KEY);
	}
	else
	{
		printf("First subscribed to key '%s'\n", KEY);
		printf("Key value = %d,%d,%d,%d\n", t->data[0], t->data[1], t->data[2], t->data[3]);
	}
	delete t->data;
	delete t->key;
	delete t;
	return true;
}

bool BlackBoardTester::SubscribeSecondDeviceToKey()
{
	player_blackboard_entry_t *t = new player_blackboard_entry_t;
	memset(t, 0, sizeof(t));
	if (playerc_blackboard_subscribe_to_key(second, KEY, &t))
	{
		printf("Error subscribing to key '%s'\n", KEY);
		return false;
	}

	if (t->data_count == 0)
	{
		printf("Key '%s' does not exist (EMPTY)\n", KEY);
	}
	else
	{
		printf("First subscribed to key '%s'\n", KEY);
		printf("Key value = %d,%d,%d,%d\n", t->data[0], t->data[1], t->data[2], t->data[3]);
	}
	delete t->data;
	delete t->key;
	delete t;
	return true;
}

bool BlackBoardTester::SetFirstDeviceEntry()
{
	player_blackboard_entry_t *entry = new player_blackboard_entry_t;
	memset(entry, 0, sizeof(entry));
	entry->key = strdup(KEY);
	entry->key_count = strlen(KEY) + 1;
	entry->data = new uint8_t[4];
	entry->data[0] = 0;
	entry->data[1] = 1;
	entry->data[2] = 2;
	entry->data[3] = 3;
	entry->data_count = 4;

	if (playerc_blackboard_set_entry(first, entry))
	{
		printf("Error setting first entry\n");
		return false;
	}
	delete entry->key;
	delete entry->data;
	delete entry;

	return true;
}

bool BlackBoardTester::SetSecondDeviceEntry()
{
	player_blackboard_entry_t *entry = new player_blackboard_entry_t;
	memset(entry, 0, sizeof(entry));
	entry->key = strdup(KEY);
	entry->key_count = strlen(KEY) + 1;
	entry->data = new uint8_t[4];
	entry->data[0] = 0;
	entry->data[1] = 1;
	entry->data[2] = 2;
	entry->data[3] = 3;
	entry->data_count = 4;

	if (playerc_blackboard_set_entry(second, entry))
	{
		printf("Error setting second entry\n");
		return false;
	}
	delete entry->key;
	delete entry->data;
	delete entry;

	return true;
}

bool BlackBoardTester::UnsubscribeFirstDeviceKey()
{
	if (playerc_blackboard_unsubscribe_from_key(first, KEY))
	{
		printf("Error unsubscribing first from key\n");
		return false;
	}
	return true;
}

bool BlackBoardTester::UnsubscribeSecondDeviceKey()
{
	if (playerc_blackboard_unsubscribe_from_key(second, KEY))
	{
		printf("Error unsubscribing second from key\n");
		return false;
	}
	return true;
}

bool BlackBoardTester::UnsubscribeFirstDevice()
{
	if (playerc_blackboard_unsubscribe(first))
	{
		printf("Error unsubscribing first from client\n");
		return false;
	}
	playerc_blackboard_destroy(first);

	return true;
}

bool BlackBoardTester::UnsubscribeSecondDevice()
{
	if (playerc_blackboard_unsubscribe(second))
	{
		printf("Error unsubscribing second from client\n");
		return false;
	}
	playerc_blackboard_destroy(second);

	return true;
}

void BlackBoardTester::Read()
{
	playerc_client_read(client_first);
	playerc_client_read(client_second);
}

bool BlackBoardTester::Shutdown()
{
	playerc_client_disconnect(client_first);
	playerc_client_disconnect(client_second);
	playerc_client_destroy(client_first);
	playerc_client_destroy(client_second);
	return true;
}

int main()
{
	BlackBoardTester t;
	printf("Initialise\n");
	bool res = t.Initialise();

	if (res)
	{
		printf("CreateFirstDevice\n");
		res = t.CreateFirstDevice();
	}
	if (res)
	{
		printf("CreateSecondDevice\n");
		res = t.CreateSecondDevice();
	}
	if (res)
	{
		printf("SubscribeFirstDevice\n");
		res = t.SubscribeFirstDevice();
	}
	if (res)
	{
		printf("SubscribeSecondDevice\n");
		res = t.SubscribeSecondDevice();
	}
	if (res)
	{
		printf("SubscribeFirstDeviceToKey\n");
		res = t.SubscribeFirstDeviceToKey();
	}
	if (res)
	{
		printf("SubscribeSecondDeviceToKey\n");
		res = t.SubscribeSecondDeviceToKey();
	}
	if (res)
	{
		printf("SetFirstDeviceEntry\n");
		res = t.SetFirstDeviceEntry();
	}
	if (res)
	{
		printf("SetSecondDeviceEntry\n");
		res = t.SetSecondDeviceEntry();
	}

	printf("Read\n");
	t.Read();

	if (res)
	{
		printf("UnsubscribeFirstDeviceKey\n");
		res = t.UnsubscribeFirstDeviceKey();
	}
	if (res)
	{
		printf("UnsubscribeSecondDeviceKey\n");
		res = t.UnsubscribeSecondDeviceKey();
	}
	if (res)
	{
		printf("UnsubscribeFirstDevice\n");
		res = t.UnsubscribeFirstDevice();
	}
	if (res)
	{
		printf("UnsubscribeSecondDevice\n");
		res = t.UnsubscribeSecondDevice();
	}
}
