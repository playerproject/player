/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey
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
 * A driver to provide access to the ALSA sound system.
 */

#include <libplayercore/playercore.h>

#include "alsa.h"
#include <alsa/asoundlib.h>

// Initialisation function
Driver* Alsa_Init (ConfigFile* cf, int section)
{
	return reinterpret_cast<Driver*> (new Alsa (cf, section));
}

// Register function
void Alsa_Register (DriverTable* table)
{
	table->AddDriver("alsa", Alsa_Init);
}


////////////////////////////////////////////////////////////////////////////////
//	Driver management
////////////////////////////////////////////////////////////////////////////////

// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
Alsa::Alsa (ConfigFile* cf, int section)
    : Driver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_AUDIO_CODE)
{
	// Read the config file options
	block = cf->ReadBool (section, "block", false);

	return;
}

// Set up the device. Return 0 if things go well, and -1 otherwise.
int Alsa::Setup (void)
{
	StartThread ();
	printf ("Alsa driver initialised\n");
	return 0;
}


// Shutdown the device
int Alsa::Shutdown (void)
{
	printf ("Alsa driver shutting down\n");
	StopThread ();
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//	Thread stuff
////////////////////////////////////////////////////////////////////////////////

void Alsa::Main (void)
{
	while (1)
	{
		pthread_testcancel ();

	    // Handle pending messages
		if (!InQueue->Empty ())
		{
			ProcessMessages ();
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//	Message handling
////////////////////////////////////////////////////////////////////////////////

int Alsa::ProcessMessage (MessageQueue *resp_queue, player_msghdr *hdr, void *data)
{
	return 0;
}
