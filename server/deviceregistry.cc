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
  #include <config.h>
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
#ifdef INCLUDE_SICKLMS200
void SickLMS200_Register(DriverTable* table);
#endif

#ifdef INCLUDE_ACTS
void Acts_Register(DriverTable* table);
#endif

#ifdef INCLUDE_CMVISION
void CMVision_Register(DriverTable* table);
#endif

#ifdef INCLUDE_FESTIVAL
void Festival_Register(DriverTable* table);
#endif 

#ifdef INCLUDE_LASERFIDUCIAL
void LaserBar_Register(DriverTable* table);
void LaserBarcode_Register(DriverTable* table);
void LaserVisualBarcode_Register(DriverTable* table);
#endif

#ifdef INCLUDE_LASERCSPACE
void LaserCSpace_Register(DriverTable* table);
#endif

#ifdef INCLUDE_SONYEVID30
void SonyEVID30_Register(DriverTable* table);
#endif

#ifdef INCLUDE_UDPBROADCAST
void UDPBroadcast_Register(DriverTable* table);
#endif

#ifdef INCLUDE_PASSTHROUGH
void PassThrough_Register(DriverTable* table);
#endif

#ifdef INCLUDE_P2OS
void P2OSGripper_Register(DriverTable* table);
void P2OSPower_Register(DriverTable* table);
void P2OSaio_Register(DriverTable* table);
void P2OSdio_Register(DriverTable* table);
void P2OSBumper_Register(DriverTable* table);
void P2OSPosition_Register(DriverTable* table);
void P2OSSonar_Register(DriverTable* table);
void P2OSSound_Register(DriverTable* table);
#endif

#ifdef INCLUDE_LINUXWIFI
void LinuxWiFi_Register(DriverTable *table);
#endif

#ifdef INCLUDE_AODV
void Aodv_Register(DriverTable *table);
#endif

#ifdef INCLUDE_IWSPY
void Iwspy_Register(DriverTable *table);
#endif

#ifdef INCLUDE_REB
void REBPosition_Register(DriverTable *table);
void REBIR_Register(DriverTable *table);
void REBPower_Register(DriverTable *table);
#endif

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

#ifdef INCLUDE_ISENSE
void InertiaCube2_Register(DriverTable* table);
#endif

#ifdef INCLUDE_MICROSTRAIN
void MicroStrain3DMG_Register(DriverTable* table);
#endif

#ifdef INCLUDE_WAVEAUDIO
void Waveaudio_Register(DriverTable* table);
#endif

#ifdef INCLUDE_MCL
void RegularMCL_Register(DriverTable* table);
#endif

#ifdef INCLUDE_AMCL
void AdaptiveMCL_Register(DriverTable* table);
#endif

#ifdef INCLUDE_LIFOMCOM
void LifoMCom_Register(DriverTable* table);
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
  {PLAYER_WIFI_CODE, PLAYER_WIFI_STRING, "linuxwifi"},
  {PLAYER_IR_CODE, PLAYER_IR_STRING, "reb_ir"},
  {PLAYER_WAVEFORM_CODE, PLAYER_WAVEFORM_STRING, "wave_audio"},
  {PLAYER_LOCALIZE_CODE, PLAYER_LOCALIZE_STRING, "adaptive_mcl"},
  {PLAYER_MCOM_CODE, PLAYER_MCOM_STRING, "lifomcom"},
  {PLAYER_SOUND_CODE, PLAYER_SOUND_STRING, "p2os_sound"},
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
#ifdef INCLUDE_SICKLMS200
  SickLMS200_Register(driverTable);
#endif

#ifdef INCLUDE_ACTS
  Acts_Register(driverTable);
#endif

#ifdef INCLUDE_CMVISION
  CMVision_Register(driverTable);
#endif

#ifdef INCLUDE_FESTIVAL
  Festival_Register(driverTable);
#endif

#ifdef INCLUDE_LASERFIDUCIAL
  LaserBar_Register(driverTable);
  LaserBarcode_Register(driverTable);
  LaserVisualBarcode_Register(driverTable);
#endif

#ifdef INCLUDE_LASERCSPACE
  LaserCSpace_Register(driverTable);
#endif

#ifdef INCLUDE_SONYEVID30
  SonyEVID30_Register(driverTable);
#endif

#ifdef INCLUDE_UDPBROADCAST
  UDPBroadcast_Register(driverTable);
#endif

#ifdef INCLUDE_PASSTHROUGH
  PassThrough_Register(driverTable);
#endif

#ifdef INCLUDE_P2OS
  P2OSGripper_Register(driverTable);
  P2OSPower_Register(driverTable);
  P2OSaio_Register(driverTable);
  P2OSdio_Register(driverTable);
  P2OSBumper_Register(driverTable);
  P2OSPosition_Register(driverTable);
  P2OSSonar_Register(driverTable);
  P2OSSound_Register(driverTable);
#endif

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
  
#ifdef INCLUDE_LINUXWIFI
  LinuxWiFi_Register(driverTable);
#endif

#ifdef INCLUDE_AODV
  Aodv_Register(driverTable);
#endif

#ifdef INCLUDE_IWSPY
  Iwspy_Register(driverTable);
#endif

#ifdef INCLUDE_REB
  REBPosition_Register(driverTable);
  REBIR_Register(driverTable);
  REBPower_Register(driverTable);
#endif

#ifdef INCLUDE_ISENSE
  InertiaCube2_Register(driverTable);
#endif

#ifdef INCLUDE_MICROSTRAIN
  MicroStrain3DMG_Register(driverTable);
#endif

#ifdef INCLUDE_WAVEAUDIO
  Waveaudio_Register(driverTable);
#endif

#ifdef INCLUDE_MCL
  RegularMCL_Register(driverTable);
#endif

#ifdef INCLUDE_AMCL
  AdaptiveMCL_Register(driverTable);
#endif

#ifdef INCLUDE_LIFOMCOM
  LifoMCom_Register(driverTable);
#endif
}

