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

/*
 * $Id$
 *
 * client-side camera device
 */

#if HAVE_CONFIG_H
  #include "config.h"
#endif

#include <cassert>
#include <sstream>
#include <iomanip>

#include "playercc.h"

#define DEBUG_LEVEL HIGH
#include "debug.h"

using namespace PlayerCc;

CameraProxy::CameraProxy(PlayerClient *aPc, uint aIndex)
  : ClientProxy(aPc),
  mCamera(NULL),
  mPrefix("image"),
  mFrameNo(0)
{
  assert(NULL != aPc);
  mCamera = playerc_camera_create(mPc->mClient, aIndex);
  if (NULL==mCamera)
    throw PlayerError("CameraProxy::CameraProxy()", "could not create");

  if (0 != playerc_camera_subscribe(mCamera, PLAYER_OPEN_MODE))
    throw PlayerError("CameraProxy::CameraProxy()", "could not subscribe");

  mInfo = &(mCamera->info);
}

CameraProxy::~CameraProxy()
{
  assert(NULL!=mCamera);
  playerc_camera_unsubscribe(mCamera);
  playerc_camera_destroy(mCamera);
}

void
CameraProxy::SaveFrame(const std::string aPrefix, uint aWidth)
{
  std::ostringstream filename;
  filename.imbue(std::locale(""));
  filename.fill('0');

  filename << mPrefix << std::setw(aWidth) << mFrameNo++;
  if (GetCompression())
    filename << ".jpg";
  else
    filename << ".ppm";

  playerc_camera_save(mCamera, filename.str().c_str());
}

void
CameraProxy::Decompress()
{
  playerc_camera_decompress(mCamera);
}


std::ostream& operator << (std::ostream& os, const PlayerCc::CameraProxy& c)
{
  return os << "[" << c.GetWidth()
            << ", " << c.GetHeight() << "] "
            << 1/c.GetElapsedTime() << " fps, "
            << "compressed(" << (c.GetCompression() ? "yes" : "no") << ")";
}
