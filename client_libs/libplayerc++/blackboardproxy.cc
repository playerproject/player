/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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

/*
 * $Id$
 */


#include "playerc++.h"
#include "string.h"

using namespace std;
using namespace PlayerCc;

// Constructor
BlackBoardProxy::BlackBoardProxy(PlayerClient *aPc, uint aIndex) : ClientProxy(aPc, aIndex),
                               mDevice(NULL)
{
  Subscribe(aIndex);
  mInfo = &(mDevice->info);
}

// Destructor
BlackBoardProxy::~BlackBoardProxy()
{
  Unsubscribe();
}

// Subscribe
void BlackBoardProxy::Subscribe(uint aIndex)
{
  scoped_lock_t lock(mPc->mMutex);
  mDevice = playerc_blackboard_create(mClient, aIndex);
  if (NULL==mDevice)
  {
    throw PlayerError("BlackBoardProxy::Subscribe(uint aIndex)", "could not create");
  }

  if (0 != playerc_blackboard_subscribe(mDevice, PLAYER_OPEN_MODE))
  {
    throw PlayerError("BlackBoardProxy::Subscribe(uint aIndex)", "could not subscribe");
  }
  
  mDevice->on_blackboard_event = NULL;
}

// Unsubscribe
void BlackBoardProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  scoped_lock_t lock(mPc->mMutex);
  playerc_blackboard_unsubscribe(mDevice);
  playerc_blackboard_destroy(mDevice);
  mDevice = NULL;
}

player_blackboard_entry_t *BlackBoardProxy::SubscribeToKey(const char *key)
{
  scoped_lock_t lock(mPc->mMutex);
  player_blackboard_entry_t *pointer;
  if (0 != playerc_blackboard_subscribe_to_key(mDevice, key, &pointer))
  {
  	throw PlayerError("BlackBoardProxy::SubscribeToKey(const string& key)", "could not subscribe to key");
  }

  // We don't want a mix of malloc and new, so make a copy using only new
  player_blackboard_entry_t *result = new player_blackboard_entry_t;
  memset(result, 0, sizeof(player_blackboard_entry_t));
  result->type = pointer->type;
  result->subtype = pointer->subtype;

  result->key_count = pointer->key_count;
  result->key = new char[result->key_count];
  memcpy(result->key, pointer->key, result->key_count);
  
  result->data_count = pointer->data_count;
  result->data = new uint8_t[result->data_count];
  memcpy(result->data, pointer->data, result->data_count);

  // Get rid of the original
  free(pointer->key);
  free(pointer->data);
  free(pointer);
  
  return result;
}

void BlackBoardProxy::UnsubscribeFromKey(const char *key)
{
	scoped_lock_t lock(mPc->mMutex);
	if (0 != playerc_blackboard_unsubscribe_from_key(mDevice, key))
	{
		throw PlayerError("BlackBoardProxy::UnsubscribeFromKey(const string& key)", "could not unsubscribe from key");
	}
}

void BlackBoardProxy::SetEntry(const player_blackboard_entry_t &entry)
{
	scoped_lock_t lock(mPc->mMutex);
	player_blackboard_entry_t *copy = new player_blackboard_entry_t;
	// shallow copy
	memcpy(copy, &entry, sizeof(player_blackboard_entry_t));
	
	if (0 != playerc_blackboard_set_entry(mDevice, copy))
	{
		throw PlayerError("BlackBoardProxy::SetEntry(const player_blackboard_entry_t &entry)", "could not set entry");
	}

	delete copy;
}

void BlackBoardProxy::SetEventHandler(void (*on_blackboard_event)(player_blackboard_entry_t))
{
	mDevice->on_blackboard_event = on_blackboard_event;
}

