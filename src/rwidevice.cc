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

#define PLAYER_ENABLE_MSG 1
#define PLAYER_ENABLE_TRACE 1

#include <rwidevice.h>
#include <playertime.h>
#include <devicetable.h>
#include <playerqueue.h>
#include <messages.h>
#include <string.h>

extern pthread_mutex_t  CRWIDevice::rwi_counter_mutex;
extern unsigned int     CRWIDevice::rwi_device_count = 0;

#ifdef USE_MOBILITY
extern mbyClientHelper *CRWIDevice::helper;
#endif				// USE_MOBILITY

CRWIDevice::CRWIDevice(int argc, char *argv[],
                       size_t datasize, size_t commandsize,
                       int reqqueuelen, int repqueuelen)
	: CDevice(datasize, commandsize, reqqueuelen, repqueuelen)
{
	// does not hurt to call this multiple times (I'm told)
	pthread_mutex_init(&rwi_counter_mutex, NULL);
	
	pthread_mutex_lock(&rwi_counter_mutex);
	
	#ifdef USE_MOBILITY
	if (rwi_device_count == 0) {
		PLAYER_TRACE0("Initializing helper pointer\n");
		helper = new mbyClientHelper(argc, argv);
	}
	#endif				// USE_MOBILITY

	rwi_device_count++;
	pthread_mutex_unlock(&rwi_counter_mutex);
	
	#ifdef USE_MOBILITY
	bool name_found = false;
	strcpy(name, "NoName");
	
	// parse cmd line args to find name
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "name") && (i+1 < argc)) {
			strcpy(name, argv[++i]);;
			name_found = true;
		}
	}
	
	if (!name_found) {
		fprintf(stderr, "Unable to locate robot name in device argument"
		        " string.  Mobility connections MAY fail.  Please pass robot"
		        " name in the form: -rwi_foo:0 \"name B21R"
		        " extra_option\"\n");
	}
	#endif				// USE_MOBILITY
}

CRWIDevice::~CRWIDevice()
{
	pthread_mutex_lock(&rwi_counter_mutex);
	rwi_device_count--;
	pthread_mutex_unlock(&rwi_counter_mutex);

	#ifdef USE_MOBILITY
	if (rwi_device_count == 0) {
		PLAYER_TRACE0("Destroying mbyClientHelper\n");
		helper->~mbyClientHelper();
	}
	#endif				// USE_MOBILITY
}

#ifdef USE_MOBILITY
int
CRWIDevice::RWIConnect (CORBA::Object_ptr *corba_ptr, const char *path) const
{
	char full_path[RWI_MOBILITY_PATH_MAX];
	//FIXME: does snprintf count the '\0'?
	snprintf(full_path, RWI_MOBILITY_PATH_MAX, "%s%s", name, path);
	
	try {
	    *corba_ptr = helper->find_object(full_path);
	    return 0;
	} catch(...) {
	    fprintf(stderr, "Unable to locate device %s for robot %s.\n",
	            path, name);
	    return -1;
	}
}
#endif				// USE_MOBILITY