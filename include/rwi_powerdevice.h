/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2002
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *     Nik Melchior
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

#ifndef _RWI_POWERDEVICE_H
#define _RWI_POWERDEVICE_H

#include <rwidevice.h>

class CRWIPowerDevice: public CRWIDevice {

public:
	CRWIPowerDevice (int argc, char *argv[])
		: CRWIDevice(argc, argv,
		             sizeof(player_power_data_t),
		             0,  /* power device takes no commands */
		             1,1)
		{}

	static CDevice *Init (int argc, char *argv[])
	{
		return((CDevice *)(new CRWIPowerDevice(argc, argv)));
	}
	
	virtual int Setup ();
	virtual int Shutdown ();
	
	virtual void Main ();
	
private:
	#ifdef USE_MOBILITY
	MobilityData::PowerManagementState_var  power_state;
	#endif // USE_MOBILITY
	
	// number of laser readings
	unsigned int range_count;

};

#endif // _RWI_POWERDEVICE_H
