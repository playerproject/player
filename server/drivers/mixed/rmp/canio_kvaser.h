/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  John Sweeney & Brian Gerkey
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

#ifndef _KVASER_CANLIB_
#define _KVASER_CANLIB_

#include "canlib.h"
#include "canio.h"

class CANIOKvaser : public DualCANIO
{
private:
	canHandle channels[2];

public:
	CANIOKvaser();
	virtual ~CANIOKvaser();
	virtual int Init(long channel_freq);
	virtual int ReadPacket(CanPacket *pkt, int channel);
	virtual int WritePacket(CanPacket &pkt);
	virtual int Shutdown();
};

#endif
