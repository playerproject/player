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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <device.h>
#include <deviceregistry.h>

#include <drivertable.h>
// this table holds all the currently *available* drivers
// (it's declared in main.cc)
extern DriverTable* driverTable;

#include <wallclocktime.h> /* for WallclockTime */
// GlobalTime is declared in main.cc
extern PlayerTime* GlobalTime;

/* prototype device-specific init funcs */
void SickLMS200_Register(DriverTable* table);
void Acts_Register(DriverTable* table);
void Festival_Register(DriverTable* table);
void LaserReflector_Register(DriverTable* table);
void LaserBarcode_Register(DriverTable* table);
void LaserVisualBarcode_Register(DriverTable* table);
void SonyEVID30_Register(DriverTable* table);
void UDPBroadcast_Register(DriverTable* table);
void P2OSGripper_Register(DriverTable* table);
void P2OSPower_Register(DriverTable* table);
void P2OSaio_Register(DriverTable* table);
void P2OSdio_Register(DriverTable* table);
void P2OSBumper_Register(DriverTable* table);
void P2OSPosition_Register(DriverTable* table);
void P2OSSonar_Register(DriverTable* table);

#ifdef INCLUDE_FIXEDTONES
void FixedTones_Register(DriverTable* table);
#endif

#ifdef INCLUDE_RWI
void RWIBumper_Register(DriverTable* table);
void RWILaser_Register(DriverTable* table);
void RWIPosition_Register(DriverTable* table);
void RWIPower_Register(DriverTable* table);
void RWISonar_Register(DriverTable* table);
#endif

/* this array lists the interfaces that Player knows how to load, along with
 * the default driver for each.
 *
 * NOTE: the last element *must* be NULL
 */
player_interface_t interfaces[] = { 
  {PLAYER_LASER_CODE, PLAYER_LASER_STRING, "sicklms200"},
  {PLAYER_BLOBFINDER_CODE, PLAYER_BLOBFINDER_STRING, "acts"},
  {PLAYER_SPEECH_CODE, PLAYER_SPEECH_STRING, "festival"},
  {PLAYER_AUDIO_CODE, PLAYER_AUDIO_STRING, "fixedtones"},
  {PLAYER_FIDUCIAL_CODE, PLAYER_FIDUCIAL_STRING, "laserbarcode"},
  {PLAYER_PTZ_CODE, PLAYER_PTZ_STRING, "sonyevid30"},
  {PLAYER_COMMS_CODE, PLAYER_COMMS_STRING, "udpbroadcast"},
  {PLAYER_GRIPPER_CODE, PLAYER_GRIPPER_STRING, "p2os_gripper"},
  {PLAYER_POWER_CODE, PLAYER_POWER_STRING, "p2os_power"},
  {PLAYER_BUMPER_CODE, PLAYER_BUMPER_STRING, "p2os_bumper"},
  {PLAYER_AIO_CODE, PLAYER_AIO_STRING, "p2os_aio"},
  {PLAYER_DIO_CODE, PLAYER_DIO_STRING, "p2os_dio"},
  {PLAYER_POSITION_CODE, PLAYER_POSITION_STRING, "p2os_position"},
  {PLAYER_SONAR_CODE, PLAYER_SONAR_STRING, "p2os_sonar"},
  {0,NULL,NULL}
};

/* 
 * looks through the array of available interfaces for one which the given
 * name.  if found, interface is filled out (the caller must provide storage)
 * and zero is returned.  otherwise, -1 is returned.
 */
int
lookup_interface(char* name, player_interface_t* interface)
{
  for(int i=0; interfaces[i].code; i++)
  {
    if(!strcmp(name, interfaces[i].name))
    {
      *interface = interfaces[i];
      return(0);
    }
  }
  return(-1);
}

/*
 * this function will be called at startup.  all available devices should
 * be added to the driverTable here.  they will be instantiated later as 
 * necessary.
 */
void
register_devices()
{
  SickLMS200_Register(driverTable);
  Acts_Register(driverTable);
  Festival_Register(driverTable);
  LaserReflector_Register(driverTable);
  LaserBarcode_Register(driverTable);
  LaserVisualBarcode_Register(driverTable);
  SonyEVID30_Register(driverTable);
  UDPBroadcast_Register(driverTable);
  P2OSGripper_Register(driverTable);
  P2OSPower_Register(driverTable);
  P2OSaio_Register(driverTable);
  P2OSdio_Register(driverTable);
  P2OSBumper_Register(driverTable);
  P2OSPosition_Register(driverTable);
  P2OSSonar_Register(driverTable);

#ifdef INCLUDE_FIXEDTONES
  FixedTones_Register(driverTable);
#endif

#ifdef INCLUDE_RWI
  RWIPosition_Register(driverTable);
  RWISonar_Register(driverTable);
  RWILaser_Register(driverTable);
  RWIBumper_Register(driverTable);
  RWIPower_Register(driverTable);
#endif
}

