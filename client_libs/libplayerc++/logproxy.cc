/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2005-
 *     Brian Gerkey
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
 * client-side log device
 */

#include "playerc++.h"

using namespace PlayerCc;

LogProxy::LogProxy(PlayerClient *aPc, uint aIndex)
  : ClientProxy(aPc, aIndex),
  mDevice(NULL)
{
  Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
  mInfo = &(mDevice->info);
}

LogProxy::~LogProxy()
{
  Unsubscribe();
}

void
LogProxy::Subscribe(uint aIndex)
{
  mDevice = playerc_log_create(mClient, aIndex);
  if (NULL==mDevice)
    throw PlayerError("LogProxy::LogProxy()", "could not create");

  if (0 != playerc_log_subscribe(mDevice, PLAYER_OPEN_MODE))
    throw PlayerError("LogProxy::LogProxy()", "could not subscribe");
}

void
LogProxy::Unsubscribe()
{
  assert(NULL!=mDevice);
  playerc_log_unsubscribe(mDevice);
  playerc_log_destroy(mDevice);
  mDevice = NULL;
}

std::ostream& std::operator << (std::ostream &os, const PlayerCc::LogProxy &c)
{
  os << "#Log (" << c.GetInterface() << ":" << c.GetIndex() << ")" << std::endl;
  
  return os;
}


/** Start/stop (1/0) writing to the log file. */
int LogProxy::SetWriteState(int aState)
{
  return playerc_log_set_write_state(mDevice,aState);
}

/** Start/stop (1/0) reading from the log file. */
int LogProxy::SetReadState(int aState)
{
  return playerc_log_set_read_state(mDevice,aState);
}

/** Rewind the log file.*/
int LogProxy::Rewind()
{
  return playerc_log_set_read_rewind(mDevice);
}

/** Set the name of the logfile to write to. */
int LogProxy::SetFilename(const std::string aFilename)
{
  return playerc_log_set_filename(mDevice,aFilename.c_str());
}
