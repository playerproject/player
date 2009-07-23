/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006
 *     Raymond Sheh, Luke Gumbley
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

#include <libplayercore/playercore.h>

#include "UvcInterface.h"

class CameraUvc;

#ifndef CAMERAUVC_H_
#define CAMERAUVC_H_

class CameraUvc : public ThreadedDriver
{
	public:
		CameraUvc(ConfigFile* cf, int section);
    ~CameraUvc();
		int MainSetup();
		void MainQuit();

		int ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data);
	private:
		virtual void Main();

		UvcInterface *ui;

		player_camera_data_t data;	// Data to send to client (through the server)
};

#endif /*CAMERAUVC_H_*/
