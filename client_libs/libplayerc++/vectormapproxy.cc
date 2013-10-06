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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
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
VectorMapProxy::VectorMapProxy(PlayerClient *aPc, uint32_t aIndex) : ClientProxy(aPc, aIndex),
                               mDevice(NULL)
{
  Subscribe(aIndex);
  mInfo = &(mDevice->info);
  map_info_cached = false;
}

// Destructor
VectorMapProxy::~VectorMapProxy()
{
  Unsubscribe();
}

// Subscribe
void VectorMapProxy::Subscribe(uint32_t aIndex)
{
  scoped_lock_t lock(mPc->mMutex);
  mDevice = playerc_vectormap_create(mClient, aIndex);
  if (NULL==mDevice)
    throw PlayerError("VectorMapProxy::VectorMapProxy()", "could not create");

  if (0 != playerc_vectormap_subscribe(mDevice, PLAYER_OPEN_MODE))
    throw PlayerError("VectorMapProxy::VectorMapProxy()", "could not subscribe");
}

// Unsubscribe
void VectorMapProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  scoped_lock_t lock(mPc->mMutex);
  playerc_vectormap_unsubscribe(mDevice);
  playerc_vectormap_destroy(mDevice);
  mDevice = NULL;
}

void VectorMapProxy::GetMapInfo()
{
  scoped_lock_t lock(mPc->mMutex);
  playerc_vectormap_get_map_info(mDevice);
  map_info_cached = true;
}

int VectorMapProxy::GetLayerCount() const
{
  scoped_lock_t lock(mPc->mMutex);
  if (map_info_cached)
    return mDevice->layers_count;
  else
    return -1;
}

vector<string> VectorMapProxy::GetLayerNames() const
{
  vector<string> names;
  int layerCount = GetLayerCount();
  if (layerCount < 1)
    return names;

  // this lock needs to come after GetLayerCount which locks as well
  scoped_lock_t lock(mPc->mMutex);
  for (int i=0; i<layerCount; ++i)
  {
    names.push_back(string(mDevice->layers_info[i]->name));
  }

  return names;
}

void VectorMapProxy::GetLayerData(unsigned layer_index)
{
  if (map_info_cached)
  {
    scoped_lock_t lock(mPc->mMutex);
    playerc_vectormap_get_layer_data(mDevice, layer_index);
  }
  else
  {
    throw PlayerError("VectorMapProxy::GetLayerData()", "Map info not cached");
  }
}

int VectorMapProxy::GetFeatureCount(unsigned layer_index) const
{
  int layerCount = GetLayerCount();
  if (layerCount <= (int)layer_index)
    return -1;

  scoped_lock_t lock(mPc->mMutex);
  return mDevice->layers_data[layer_index]->features_count;
}

const uint8_t * VectorMapProxy::GetFeatureData(unsigned layer_index, unsigned feature_index) const
{
  scoped_lock_t lock(mPc->mMutex);
  return playerc_vectormap_get_feature_data(mDevice, layer_index, feature_index);
}

size_t VectorMapProxy::GetFeatureDataCount(unsigned layer_index, unsigned feature_index) const
{
  scoped_lock_t lock(mPc->mMutex);
  return playerc_vectormap_get_feature_data_count(mDevice, layer_index, feature_index);
}

ostream&
    std::operator << (ostream &os, const VectorMapProxy &c)
{
  os << "#VectorMap (" << c.GetInterface() << ":" << c.GetIndex() << ")" << endl;
  os << "#Layer Number\tName\tFeature Count" << endl;

  int layerCount = c.GetLayerCount();
  vector<string> names = c.GetLayerNames();
  for (int i=0; i<layerCount; ++i)
  {
    os << i << "\t" << names[i] << "\t" << c.GetFeatureCount(i) << endl;
  }
  os << "Total " << layerCount << " layers\n";

  return os;
}
