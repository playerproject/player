/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
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
 */

/* conditionally include device-specific headers */
#ifdef INCLUDE_LASER
#include <laserdevice.h>
#endif
#ifdef INCLUDE_SONAR
#include <sonardevice.h>
#endif
#ifdef INCLUDE_VISION
#include <visiondevice.h>
#endif
#ifdef INCLUDE_POSITION
#include <positiondevice.h>
#endif
#ifdef INCLUDE_GRIPPER
#include <gripperdevice.h>
#endif
#ifdef INCLUDE_MISC
#include <miscdevice.h>
#endif
#ifdef INCLUDE_PTZ
#include <ptzdevice.h>
#endif
#ifdef INCLUDE_AUDIO
#include <audiodevice.h>
#endif
#ifdef INCLUDE_LASERBEACON
#include <laserbeacondevice.hh>
#endif
#ifdef INCLUDE_BROADCAST
#include <broadcastdevice.hh>
#endif
#ifdef INCLUDE_SPEECH
#include <speechdevice.h>
#endif
#ifdef INCLUDE_BPS
#include <bpsdevice.h>
#endif
#ifdef INCLUDE_RWI_POSITION
#include <rwi_positiondevice.h>
#endif
#ifdef INCLUDE_RWI_SONAR
#include <rwi_sonardevice.h>
#endif
#ifdef INCLUDE_RWI_LASER
#include <rwi_laserdevice.h>
#endif
#ifdef INCLUDE_RWI_BUMPER
#include <rwi_bumperdevice.h>
#endif
// Not working yet
/*
#ifdef INLCUDE_RWI_JOYSTICK
#include <rwi_joystickdevice.h>
#endif
*/
#ifdef INCLUDE_RWI_POWER
#include <rwi_powerdevice.h>
#endif


#include <string.h> /* for strlen(3),strchr(3) */
#include <stdlib.h> /* for atoi(3) */

#include <devicetable.h> /* for CDeviceTable */
// this table holds all the currently *available* device types
// (it's declared in main.cc)
extern CDeviceTable* availableDeviceTable;
// this table holds all the currently *instantiated* devices
// (it's declared in main.cc)
extern CDeviceTable* deviceTable;

#include <playertime.h> /* for PlayerTime */
#include <wallclocktime.h> /* for WallclockTime */
// GlobalTime is declared in main.cc
extern PlayerTime* GlobalTime;

// global_playerport is declared in main.cc
extern int global_playerport;

/* 
 * this array constitutes the default (i.e., "sane") configuration when using 
 * Player with physical devices.  unless command-line arguments override, one
 * each of the following devices (with accompanying indices) will be 
 * instantiated.  this list has NO effect when using Stage (in that case,
 * device instantiations are controlled by the .world file)
 *
 * NOTE: the last element *MUST* be NULL
 *
 */
char* sane_spec[] = {"-misc:0",
                     "-gripper:0",
                     "-position:0",
                     "-sonar:0",
                     "-laser:0",
                     "-vision:0",
                     "-ptz:0",
                     "-laserbeacon:0",
                     "-broadcast:0",
                     "-speech:0",
                     "-bps:0",
                     NULL};

/*
 * this function will be called at startup.  all available devices should
 * be added to the availableDeviceTable here.  they will be instantiated later
 * as necessary.
 */
void
register_devices()
{
#ifdef INCLUDE_MISC
  availableDeviceTable->AddDevice(PLAYER_MISC_CODE, PLAYER_READ_MODE,
                                  PLAYER_MISC_STRING,CMiscDevice::Init);
#endif
#ifdef INCLUDE_GRIPPER
  availableDeviceTable->AddDevice(PLAYER_GRIPPER_CODE, PLAYER_ALL_MODE,
                                  PLAYER_GRIPPER_STRING,CGripperDevice::Init);
#endif
#ifdef INCLUDE_POSITION
  availableDeviceTable->AddDevice(PLAYER_POSITION_CODE, PLAYER_ALL_MODE,
                                  PLAYER_POSITION_STRING,CPositionDevice::Init);
#endif
#ifdef INCLUDE_SONAR
  availableDeviceTable->AddDevice(PLAYER_SONAR_CODE, PLAYER_READ_MODE,
                                  PLAYER_SONAR_STRING,CSonarDevice::Init);
#endif
#ifdef INCLUDE_LASERBEACON
  availableDeviceTable->AddDevice(PLAYER_LASERBEACON_CODE, PLAYER_READ_MODE,
                                  PLAYER_LASERBEACON_STRING,
                                  CLaserBeaconDevice::Init);
#endif
#ifdef INCLUDE_LASER
  availableDeviceTable->AddDevice(PLAYER_LASER_CODE, PLAYER_READ_MODE,
                                  PLAYER_LASER_STRING,CLaserDevice::Init);
#endif
#ifdef INCLUDE_VISION
  availableDeviceTable->AddDevice(PLAYER_VISION_CODE, PLAYER_READ_MODE,
                                  PLAYER_VISION_STRING,CVisionDevice::Init);
#endif
#ifdef INCLUDE_PTZ
  availableDeviceTable->AddDevice(PLAYER_PTZ_CODE, PLAYER_ALL_MODE,
                                  PLAYER_PTZ_STRING,CPtzDevice::Init);
#endif
#ifdef INCLUDE_AUDIO
  availableDeviceTable->AddDevice(PLAYER_AUDIO_CODE, PLAYER_ALL_MODE,
                                  PLAYER_AUDIO_STRING,CAudioDevice::Init);
#endif
#ifdef INCLUDE_BROADCAST
  availableDeviceTable->AddDevice(PLAYER_BROADCAST_CODE, PLAYER_ALL_MODE,
                                  PLAYER_BROADCAST_STRING,
                                  CBroadcastDevice::Init);
#endif
#ifdef INCLUDE_SPEECH
  availableDeviceTable->AddDevice(PLAYER_SPEECH_CODE, PLAYER_WRITE_MODE,
                                  PLAYER_SPEECH_STRING, CSpeechDevice::Init);
#endif
#ifdef INCLUDE_BPS
  availableDeviceTable->AddDevice(PLAYER_BPS_CODE, PLAYER_READ_MODE,
                                  PLAYER_BPS_STRING, CBpsDevice::Init);
#endif
#ifdef INCLUDE_RWI_POSITION
  availableDeviceTable->AddDevice(PLAYER_RWI_POSITION_CODE, PLAYER_ALL_MODE,
                                  PLAYER_RWI_POSITION_STRING, CRWIPositionDevice::Init);
#endif
#ifdef INCLUDE_RWI_SONAR
  availableDeviceTable->AddDevice(PLAYER_RWI_SONAR_CODE, PLAYER_READ_MODE,
                                  PLAYER_RWI_SONAR_STRING, CRWISonarDevice::Init);
#endif
#ifdef INCLUDE_RWI_LASER
  availableDeviceTable->AddDevice(PLAYER_RWI_LASER_CODE, PLAYER_READ_MODE,
                                  PLAYER_RWI_LASER_STRING, CRWILaserDevice::Init);
#endif
#ifdef INCLUDE_RWI_BUMPER
  availableDeviceTable->AddDevice(PLAYER_RWI_BUMPER_CODE, PLAYER_READ_MODE,
                                  PLAYER_RWI_BUMPER_STRING, CRWIBumperDevice::Init);
#endif
#ifdef INCLUDE_RWI_JOYSTICK
  availableDeviceTable->AddDevice(PLAYER_RWI_JOYSTICK_CODE, PLAYER_READ_MODE,
		                  PLAYER_RWI_JOYSTICK_STRING, CRWIJoystickDevice::Init);
#endif
#ifdef INCLUDE_RWI_POWER
  availableDeviceTable->AddDevice(PLAYER_RWI_POWER_CODE, PLAYER_READ_MODE,
		                  PLAYER_RWI_POWER_STRING, CRWIPowerDevice::Init);
#endif

    
}

/*
 * parses strings that look like "-laser:2"
 *   <str1> is the device string; <str2> is the argument string for the device
 *
 * if the string can be parsed, then the appropriate device will be created, 
 *    and 0 will be returned.
 * otherwise, -1 is retured
 */
int
parse_device_string(char* str1, char* str2)
{
  char* colon;
  uint16_t index = 0;
  CDevice* tmpdevice;
  CDeviceEntry* entry;
  char devicename[PLAYER_MAX_DEVICE_STRING_LEN];

  // if we haven't initialized the PlayerTime pointer yet, do it now,
  // because it might be referenced by CDevice::PutData(), which can be
  // called by a device's constructor
  if(!GlobalTime)
    GlobalTime = (PlayerTime*)(new WallclockTime());

  // did we get a valid device option string?
  if(!str1 || (str1[0] != '-') || (strlen(str1) < 2))
    return(-1);

  /* if there's a colon, try to get the index out */
  if((colon = strchr(str1,':')))
  {
    // is there a number after the colon?
    if(strlen(colon) < 2)
      return(-1);
    // extract the number
    index = atoi(colon+1);

    // copy the raw device name (minus the leading '-' and trailing colon 
    // and number)
    strncpy(devicename,str1+1,min(sizeof(devicename),
                                  (unsigned int)(colon-(str1+1))));

    // NULLify the end
    devicename[colon-(str1+1)] = '\0';
  }
  else
  {
    // copy the raw device name (minus the leading '-')
    strncpy(devicename,str1+1,sizeof(devicename));
    // NULLify the end (just in case)
    devicename[sizeof(devicename)-1] = '\0';
  }

  /* parse the config string into argc/argv format */
  int argc = 0;
  char* argv[32];
  char* tmpptr;
  if(str2 && strlen(str2))
  {
    argv[argc++] = strtok(str2, " ");
    while(argc < (int)sizeof(argv))
    {
      tmpptr = strtok(NULL, " ");
      if(tmpptr)
        argv[argc++] = tmpptr;
      else
        break;
    }
  }
  
  /* look for the indicated device in the available device table */
  if(!(entry = availableDeviceTable->GetDeviceEntry(devicename)))
  {
    fprintf(stderr,"\nError: couldn't instantiate requested device \"%s\";\n"
            "  Perhaps support for it was not compiled into this binary?\n", 
            str1+1);
    return(-1);
  }
  else
  {
    player_device_id_t id;
    id = entry->id;
    id.port = global_playerport;
    id.index = index;

    tmpdevice = (*(entry->initfunc))(argc,argv);
    deviceTable->AddDevice(id, entry->access, tmpdevice);
  }
  return(0);
}
