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

/*
 * $Id$
 *
 * Currently equiped only to interface with the mobility drivers, this base class
 * handles the connection and common logic between the devices on the RWI robot
 * and some underlying system.
 */

#ifndef _RWIDEVICE_H
#define _RWIDEVICE_H

#include <pthread.h>

#include <device.h>
#include <playercommon.h>
#include <messages.h>
#include <netinet/in.h>  // for htonl()

#ifdef USE_MOBILITY
#include <mobilitycomponents_i.h>
#include <mobilitydata_i.h>
#include <mobilityutil.h>
#include <mobilitygeometry_i.h>
#include <mobilityactuator_i.h>
#endif // USE_MOBILITY

class CRWIDevice : public CDevice  {

public:
	CRWIDevice (int argc, char *argv[],
	            size_t datasize, size_t commandsize,
	            int reqqueuelen, int repqueuelen);
	~CRWIDevice ();
		
protected:

	static pthread_mutex_t rwi_counter_mutex;
	static unsigned int rwi_device_count;
#ifdef USE_MOBILITY
	#define RWI_ROBOT_NAME_MAX 25
	#define RWI_MOBILITY_PATH_MAX 100
	#define RWI_ROBOT_NAME_DEFAULT "B21R"
	
	// Used to access the devices managed by mobility.  Most rwi_devices
	// should not need to use this helper directly.  Instead, use the
	// RWIConnect function below.	
	static mbyClientHelper *helper;
	
	// Keep track of whether a `name' parameter was passed on the command
	// line for this device, or the default name is being used.
	bool name_provided;
	
	// This name is the first part of the "path" used to access any of
	// your robot's devices in mobility (or MOM).  It is necessary for
	// RwiConnect().
	char name[RWI_ROBOT_NAME_MAX];
	
	// Attempts to fill the first argument with a pointer to the requested
	// mobility device.  Wraps the call to helper->find_object().
	int RWIConnect (CORBA::Object_ptr *corba_ptr, const char *path) const;
#endif // USE_MOBILITY

};

#endif // _RWIDEVICE_H
