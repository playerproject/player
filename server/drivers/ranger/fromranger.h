/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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

///////////////////////////////////////////////////////////////////////////
//
// Desc: Base class for ranger-> interface converter drivers.
// Author: Geoffrey Biggs
// Date: 06/05/2007
//
///////////////////////////////////////////////////////////////////////////

#include <libplayercore/playercore.h>

class FromRanger : public Driver
{
	public:
		FromRanger (ConfigFile* cf, int section);
		~FromRanger (void);

		// Message processor - must be called first by child classes if overridden
		virtual int ProcessMessage (QueuePointer &resp_queue, player_msghdr *hdr, void *data);

		virtual int Setup (void);
		virtual int Shutdown (void);

	protected:
		// Input device
		Device *inputDevice;						// Input device interface
		player_devaddr_t inputDeviceAddr;			// Input device address
};
