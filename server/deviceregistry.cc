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
#ifdef INCLUDE_GARMINNMEA
void GarminNMEA_Register(DriverTable* table);
#endif

#ifdef INCLUDE_MAPFILE
void MapFile_Register(DriverTable* table);
#endif

#ifdef INCLUDE_MAPCSPACE
void MapCspace_Register(DriverTable* table);
#endif

#ifdef INCLUDE_AMTECPOWERCUBE
void AmtecPowerCube_Register(DriverTable* table);
#endif

#ifdef INCLUDE_CLODBUSTER
void ClodBuster_Register(DriverTable* table);
#endif

#ifdef INCLUDE_TROGDOR
void Trogdor_Register(DriverTable* table);
#endif

#ifdef INCLUDE_ER
void ER_Register(DriverTable* table);
#endif

#ifdef INCLUDE_WAVEFRONT
void Wavefront_Register(DriverTable* table);
#endif

#ifdef INCLUDE_SEGWAYRMP
void SegwayRMPPosition_Register(DriverTable* table);
void SegwayRMPPower_Register(DriverTable* table);
#endif

#ifdef INCLUDE_SICKLMS200
void SickLMS200_Register(DriverTable* table);
#endif
                                                                               
#ifdef INCLUDE_SICKPLS
void SickPLS_Register(DriverTable* table);
#endif

#ifdef INCLUDE_ACTS
void Acts_Register(DriverTable* table);
#endif

#ifdef INCLUDE_CMVISION
void CMVision_Register(DriverTable* table);
#endif


#ifdef INCLUDE_CMUCAM2
void Cmucam2_Register(DriverTable* table);
#endif

#ifdef INCLUDE_UPCBARCODE
void UPCBarcode_Register(DriverTable* table);
#endif

#ifdef INCLUDE_SHAPETRACKER
void ShapeTracker_Register(DriverTable* table);
#endif

#ifdef INCLUDE_FESTIVAL
void Festival_Register(DriverTable* table);
#endif 

#ifdef INCLUDE_LASERBAR
void LaserBar_Register(DriverTable* table);
#endif

#ifdef INCLUDE_LASERBARCODE
void LaserBarcode_Register(DriverTable* table);
#endif

#ifdef INCLUDE_LASERVISUALBARCODE
void LaserVisualBarcode_Register(DriverTable* table);
#endif

#ifdef INCLUDE_LASERVISUALBW
void LaserVisualBW_Register(DriverTable* table);
#endif

#ifdef INCLUDE_LASERCSPACE
void LaserCSpace_Register(DriverTable* table);
#endif

#ifdef INCLUDE_SONYEVID30
void SonyEVID30_Register(DriverTable* table);
#endif

#ifdef INCLUDE_PTU46
void PTU46_Register(DriverTable* table);
#endif

#ifdef INCLUDE_FLOCKOFBIRDS
void FlockOfBirds_Register(DriverTable* table);
#endif

#ifdef INCLUDE_UDPBROADCAST
void UDPBroadcast_Register(DriverTable* table);
#endif

#ifdef INCLUDE_PASSTHROUGH
void PassThrough_Register(DriverTable* table);
#endif

#ifdef INCLUDE_LOGFILE
void WriteLog_Register(DriverTable* table);
void ReadLog_Register(DriverTable* table);
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
void P2OSCMUcam_Register(DriverTable* table);
void P2OSCompass_Register(DriverTable* table);
void P2OSGyro_Register(DriverTable* table);
#endif

#ifdef INCLUDE_RFLEX
void RFLEXPower_Register(DriverTable* table);
void RFLEXaio_Register(DriverTable* table);
void RFLEXdio_Register(DriverTable* table);
void RFLEXPosition_Register(DriverTable* table);
void RFLEXSonar_Register(DriverTable* table);
void RFLEXbumper_Register(DriverTable* table);
void RFLEXIr_Register(DriverTable* table);
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

#ifdef INCLUDE_KHEPERA
void KheperaPosition_Register(DriverTable *table);
void KheperaIR_Register(DriverTable *table);
#endif

#ifdef INCLUDE_FIXEDTONES
void FixedTones_Register(DriverTable* table);
#endif

#ifdef INCLUDE_ACOUSTICS
void Acoustics_Register(DriverTable* table);
#endif

#ifdef INCLUDE_MIXER
void Mixer_Register(DriverTable* table);
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

#ifdef INCLUDE_INAV
void INav_Register(DriverTable *table);
#endif

#ifdef INCLUDE_VFH
void VFH_Register(DriverTable *table);
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

#ifdef INCLUDE_CAMERAV4L
void CameraV4L_Register(DriverTable *table);
#endif

#ifdef INCLUDE_CAMERA1394
void Camera1394_Register(DriverTable *table);
#endif


#ifdef INCLUDE_GAZEBO_CAMERA
void GzCamera_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_FACTORY
void GzFactory_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_FIDUCIAL
void GzFiducial_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_GPS
void GzGps_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_LASER
void GzLaser_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_POSITION
void GzPosition_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_POSITION3D
void GzPosition3d_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_POWER
void GzPower_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_PTZ
void GzPtz_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_TRUTH
void GzTruth_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_GRIPPER
void GzGripper_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_SONARS
void GzSonars_Register(DriverTable *table);
#endif

#ifdef INCLUDE_GAZEBO_HUD
void GzHUD_Register(DriverTable *table);
#endif


#ifdef INCLUDE_SERVICE_ADV_LSD
void ServiceAdvLSD_Register(DriverTable* table);
#endif

#ifdef INCLUDE_STAGE1P4
void StgSimulation_Register(DriverTable *table);
void StgLaser_Register(DriverTable *table);
void StgPosition_Register(DriverTable *table);
void StgSonar_Register(DriverTable *table);
void StgEnergy_Register(DriverTable *table);
void StgBlobfinder_Register(DriverTable *table);
void StgFiducial_Register(DriverTable *table);
//void StgBlinkenlight_Register(DriverTable *table);
#endif

#ifdef INCLUDE_NOMAD
void Nomad_Register(DriverTable *driverTable);
void NomadPosition_Register(DriverTable *driverTable);
void NomadSonar_Register(DriverTable *driverTable);
//void NomadBumper_Register(DriverTable *driverTable);
//void NomadSpeech_Register(DriverTable *driverTable);
#endif


/* this array lists the interfaces that Player knows how to load, along with
 * the default driver for each.
 *
 * NOTE: the last element *must* be NULL
 */
player_interface_t interfaces[] = {
  {PLAYER_NULL_CODE, PLAYER_NULL_STRING, ""},
  {PLAYER_LOG_CODE, PLAYER_LOG_STRING, "writelog"},
  {PLAYER_LASER_CODE, PLAYER_LASER_STRING, "sicklms200"},
  {PLAYER_BLOBFINDER_CODE, PLAYER_BLOBFINDER_STRING, "acts"},
  {PLAYER_SPEECH_CODE, PLAYER_SPEECH_STRING, "festival"},
  {PLAYER_AUDIO_CODE, PLAYER_AUDIO_STRING, "fixedtones"},
  {PLAYER_AUDIODSP_CODE, PLAYER_AUDIODSP_STRING, "acoustics"},
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
  {PLAYER_LOCALIZE_CODE, PLAYER_LOCALIZE_STRING, "amcl"},
  {PLAYER_MCOM_CODE, PLAYER_MCOM_STRING, "lifomcom"},
  {PLAYER_SOUND_CODE, PLAYER_SOUND_STRING, "p2os_sound"},
  {PLAYER_AUDIOMIXER_CODE, PLAYER_AUDIOMIXER_STRING, "mixer"},
  {PLAYER_POSITION3D_CODE, PLAYER_POSITION3D_STRING, "segwayrmp"},
  {PLAYER_TRUTH_CODE, PLAYER_TRUTH_STRING, "passthrough"},
  {PLAYER_GPS_CODE, PLAYER_GPS_STRING, "garminnmea"},
  {PLAYER_SERVICE_ADV_CODE, PLAYER_SERVICE_ADV_STRING, "service_adv_lsd"},
  {PLAYER_SIMULATION_CODE, PLAYER_SIMULATION_STRING, "stg_simulation"},
  {PLAYER_BLINKENLIGHT_CODE, PLAYER_BLINKENLIGHT_STRING, "stg_blinkenlight"},
  {PLAYER_LASER_CODE, PLAYER_LASER_STRING, "sickpls"},
  {PLAYER_CAMERA_CODE, PLAYER_CAMERA_STRING, "camerav4l"},
  {PLAYER_CAMERA_CODE, PLAYER_CAMERA_STRING, "camera1394"},
  {PLAYER_BLOBFINDER_CODE, PLAYER_BLOBFINDER_STRING, "shapetracker"},
  {PLAYER_NOMAD_CODE, PLAYER_NOMAD_STRING, "nomad"},
  {PLAYER_ENERGY_CODE, PLAYER_ENERGY_STRING, "stg_energy"},
  {PLAYER_MAP_CODE, PLAYER_MAP_STRING, "mapfile"},
  {PLAYER_HUD_CODE, PLAYER_HUD_STRING, "gz_hud"},
  {PLAYER_PLANNER_CODE, PLAYER_PLANNER_STRING, "wavefront"},
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
 * looks through the array of available interfaces for one which the given
 * code.  if found, interface is filled out (the caller must provide storage)
 * and zero is returned.  otherwise, -1 is returned.
 */
int
lookup_interface_code(int code, player_interface_t* interface)
{
  for(int i=0; interfaces[i].code; i++)
  {
    if(code == interfaces[i].code)
    {
      *interface = interfaces[i];
      return(0);
    }
  }
  return(-1);
}




/*
 * looks through the array of interfaces, starting at startpos, for the first
 * entry that has the given code, and return the name.
 * leturns 0 when the end of the * array is reached.
 */
char*
lookup_device_name(unsigned int startpos, int code) {
    if(startpos > sizeof(interfaces))
            return 0;
    for(int i = startpos; interfaces[i].code != 0; i++)
    {
        if(code == interfaces[i].code) {
            return interfaces[i].name;
        }
    }
    return 0;
} 

/*
 * this function will be called at startup.  all available devices should
 * be added to the driverTable here.  they will be instantiated later as 
 * necessary.
 */
void
register_devices()
{
#ifdef INCLUDE_GARMINNMEA
  GarminNMEA_Register(driverTable);
#endif

#ifdef INCLUDE_MAPFILE
  MapFile_Register(driverTable);
#endif

#ifdef INCLUDE_MAPCSPACE
  MapCspace_Register(driverTable);
#endif

#ifdef INCLUDE_AMTECPOWERCUBE
  AmtecPowerCube_Register(driverTable);
#endif

#ifdef INCLUDE_CLODBUSTER
  ClodBuster_Register(driverTable);
#endif

#ifdef INCLUDE_TROGDOR
  Trogdor_Register(driverTable);
#endif

#ifdef INCLUDE_ER
  ER_Register(driverTable);
#endif

#ifdef INCLUDE_WAVEFRONT
  Wavefront_Register(driverTable);
#endif

#ifdef INCLUDE_SEGWAYRMP
  SegwayRMPPosition_Register(driverTable);
  SegwayRMPPower_Register(driverTable);
#endif

#ifdef INCLUDE_SICKLMS200
  SickLMS200_Register(driverTable);
#ifdef INCLUDE_SICKPLS
  SickPLS_Register(driverTable);
#endif

#endif

#ifdef INCLUDE_ACTS
  Acts_Register(driverTable);
#endif

#ifdef INCLUDE_CMVISION
  CMVision_Register(driverTable);
#endif

#ifdef INCLUDE_CMUCAM2
  Cmucam2_Register(driverTable);
#endif

#ifdef INCLUDE_UPCBARCODE
  UPCBarcode_Register(driverTable);
#endif

#ifdef INCLUDE_SHAPETRACKER
  ShapeTracker_Register(driverTable);
#endif

#ifdef INCLUDE_FESTIVAL
  Festival_Register(driverTable);
#endif

#ifdef INCLUDE_LASERBAR
  LaserBar_Register(driverTable);
#endif

#ifdef INCLUDE_LASERBARCODE
  LaserBarcode_Register(driverTable);
#endif

#ifdef INCLUDE_LASERVISUALBARCODE
  LaserVisualBarcode_Register(driverTable);
#endif

#ifdef INCLUDE_LASERVISUALBW
  LaserVisualBW_Register(driverTable);
#endif

#ifdef INCLUDE_LASERCSPACE
  LaserCSpace_Register(driverTable);
#endif
  
#ifdef INCLUDE_RFLEX
  RFLEXPower_Register(driverTable);
  RFLEXaio_Register(driverTable);
  RFLEXdio_Register(driverTable);
  RFLEXPosition_Register(driverTable);
  RFLEXSonar_Register(driverTable);
  RFLEXbumper_Register(driverTable);
  RFLEXIr_Register(driverTable); 
#endif

#ifdef INCLUDE_SONYEVID30
  SonyEVID30_Register(driverTable);
#endif

#ifdef INCLUDE_PTU46
  PTU46_Register(driverTable);
#endif

#ifdef INCLUDE_FLOCKOFBIRDS
  FlockOfBirds_Register(driverTable);
#endif

#ifdef INCLUDE_UDPBROADCAST
  UDPBroadcast_Register(driverTable);
#endif

#ifdef INCLUDE_PASSTHROUGH
  PassThrough_Register(driverTable);
#endif

#ifdef INCLUDE_LOGFILE
  WriteLog_Register(driverTable);  
  ReadLog_Register(driverTable);
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
  P2OSCMUcam_Register(driverTable);
  P2OSCompass_Register(driverTable);
  P2OSGyro_Register(driverTable);
#endif

#ifdef INCLUDE_FIXEDTONES
  FixedTones_Register(driverTable);
#endif

#ifdef INCLUDE_ACOUSTICS
  Acoustics_Register(driverTable);
#endif

#ifdef INCLUDE_MIXER
  Mixer_Register(driverTable);
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

#ifdef INCLUDE_KHEPERA
  KheperaPosition_Register(driverTable);
  KheperaIR_Register(driverTable);
#endif

#ifdef INCLUDE_ISENSE
  InertiaCube2_Register(driverTable);
#endif

#ifdef INCLUDE_MICROSTRAIN
  MicroStrain3DMG_Register(driverTable);
#endif
  
#ifdef INCLUDE_INAV
  INav_Register(driverTable);
#endif

#ifdef INCLUDE_VFH
  VFH_Register(driverTable);
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

#ifdef INCLUDE_CAMERAV4L
  CameraV4L_Register(driverTable);
#endif

#ifdef INCLUDE_CAMERA1394
  Camera1394_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_CAMERA
  GzCamera_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_FACTORY
  GzFactory_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_FIDUCIAL
  GzFiducial_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_GPS
  GzGps_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_LASER
  GzLaser_Register(driverTable);
#endif
  
#ifdef INCLUDE_GAZEBO_POSITION
  GzPosition_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_POSITION3D
  GzPosition3d_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_POWER
  GzPower_Register(driverTable);
#endif
  
#ifdef INCLUDE_GAZEBO_PTZ
  GzPtz_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_TRUTH
  GzTruth_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_GRIPPER
  GzGripper_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_SONARS
  GzSonars_Register(driverTable);
#endif

#ifdef INCLUDE_GAZEBO_HUD
  GzHUD_Register(driverTable);
#endif


#ifdef INCLUDE_SERVICE_ADV_LSD
  ServiceAdvLSD_Register(driverTable);
#endif

#ifdef INCLUDE_NOMAD
  Nomad_Register(driverTable);
  NomadPosition_Register(driverTable);
  NomadSonar_Register(driverTable);
  //NomadBumper_Register(driverTable);
  //NomadSpeech_Register(driverTable);
#endif

#ifdef INCLUDE_STAGE1P4
  StgSimulation_Register(driverTable);
  StgLaser_Register(driverTable);
  StgPosition_Register(driverTable);
  StgSonar_Register(driverTable);
  StgEnergy_Register(driverTable);
  StgBlobfinder_Register(driverTable);
  StgFiducial_Register(driverTable);
  //StgBlinkenlight_Register(driverTable);
#endif
}
