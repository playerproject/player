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

#include "config.h"

#include <cassert>
#include <sstream>
#include <iomanip>
#if __GNUC__ > 2
  #include <locale>
#endif

#include "playerc++.h"
#include "debug.h"

using namespace PlayerCc;

StereoProxy::StereoProxy(PlayerClient *aPc, uint32_t aIndex)
  : ClientProxy(aPc, aIndex),
  mDevice(NULL),
  mPrefix("image")
{
  Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
  mInfo = &(mDevice->info);
  for (int i=0; i < 3; i++)
  {
    this->mFrameNo[i] = 0;
  }
}

StereoProxy::~StereoProxy()
{
  Unsubscribe();
}

void
StereoProxy::Subscribe(uint32_t aIndex)
{
  scoped_lock_t lock(mPc->mMutex);
  mDevice = playerc_stereo_create(mClient, aIndex);
  if (NULL==mDevice)
    throw PlayerError("StereoProxy::StereoProxy()", "could not create");

  if (0 != playerc_stereo_subscribe(mDevice, PLAYER_OPEN_MODE))
    throw PlayerError("StereoProxy::StereoProxy()", "could not subscribe");
}

void
StereoProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  scoped_lock_t lock(mPc->mMutex);
  playerc_stereo_unsubscribe(mDevice);
  playerc_stereo_destroy(mDevice);
  mDevice = NULL;
}


void
StereoProxy::SaveFrame(const std::string aPrefix, uint32_t aWidth, playerc_camera_t aDevice, uint8_t aIndex)
{
  std::ostringstream filename;
  std::string im;
#if __GNUC__ > 2
  filename.imbue(std::locale(""));
#endif
  filename.fill('0');

  if (aIndex == 0)
  {
    im = "L_";
  }
  else if (aIndex == 1)
  {
    im = "R_";
  } 
  else
  {
    im = "D_";
  }
		

  filename << im << aPrefix << std::setw(aWidth) << mFrameNo[aIndex]++;
  //if (aDevice.compression)
  //  filename << ".jpg";
  //else
    filename << ".ppm";

  scoped_lock_t lock(mPc->mMutex);
  playerc_camera_save(&aDevice, filename.str().c_str());
}

void
StereoProxy::Decompress(playerc_camera_t aDevice)
{
  scoped_lock_t lock(mPc->mMutex);
  playerc_camera_decompress(&aDevice);
}


std::ostream& std::operator << (std::ostream& os, const PlayerCc::StereoProxy& c)
{
  return os << c.GetLeftWidth() << "\t"
            << c.GetLeftHeight() << "\t"
            << c.GetRightWidth() << "\t"
            << c.GetRightHeight() << "\t"
            << c.GetDisparityWidth() << "\t"
            << c.GetDisparityHeight() << "\t"
            << 1/c.GetElapsedTime() << "\t"
            << c.GetDataTime() << "\t"
            << (c.GetLeftCompression() ? "compressed" : "uncompressed") << "\t"
            << (c.GetRightCompression() ? "compressed" : "uncompressed") << "\t"
            << (c.GetDisparityCompression() ? "compressed" : "uncompressed");
}

