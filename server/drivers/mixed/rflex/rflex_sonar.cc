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

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <rflex.h>
#include <string.h>
#include <stdlib.h>

class RFLEXSonar: public RFLEX
{
 public:
  RFLEXSonar(char* interface, ConfigFile* cf, int section) :
    RFLEX(interface, cf, section){}
  
  virtual size_t GetData(void*, unsigned char *, size_t maxsize,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);
  void GetOptions(ConfigFile * cf, int section, rflex_config_t * rflex_configs);
};

CDevice* RFLEXSonar_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_SONAR_STRING))
    {
      PLAYER_ERROR1("driver \"rflex_sonar\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else{
    RFLEXSonar* tmp=new RFLEXSonar(interface, cf, section);
    tmp->GetOptions(cf,section,&rflex_configs);
    return (CDevice*)tmp;
  }
}

/* read out all of our configuration stuff into the config structure
 * for detauls on what each of these settings does, check the structure
 * deffinition
 */
void RFLEXSonar::GetOptions(ConfigFile *cf,int section,rflex_config_t * rflex_configs){
  int x;
  Lock();
  rflex_configs->range_distance_conversion=
    cf->ReadFloat(section, "range_distance_conversion",1);
  rflex_configs->max_num_sonars=
    cf->ReadInt(section, "max_num_sonars",64);
  rflex_configs->num_sonars=
    cf->ReadInt(section, "num_sonars",24);
  rflex_configs->sonar_age=
    cf->ReadInt(section, "sonar_age",1);
  rflex_configs->num_sonar_banks=
    cf->ReadInt(section, "num_sonar_banks",8);
  rflex_configs->num_sonars_possible_per_bank=
    cf->ReadInt(section, "num_sonars_possible_per_bank",16);
  rflex_configs->num_sonars_in_bank=(int *) malloc(rflex_configs->num_sonar_banks*sizeof(double));
  for(x=0;x<rflex_configs->num_sonar_banks;x++)
    rflex_configs->num_sonars_in_bank[x]=
      (int) cf->ReadTupleFloat(section, "num_sonars_in_bank",x,8);
  rflex_configs->sonar_echo_delay=
    cf->ReadInt(section, "sonar_echo_delay",3000);
  rflex_configs->sonar_ping_delay=
    cf->ReadInt(section, "sonar_ping_delay",0);
  rflex_configs->sonar_set_delay=
    cf->ReadInt(section, "sonar_set_delay", 0);
  rflex_configs->mmrad_sonar_poses=(sonar_pose_t *) malloc(rflex_configs->num_sonars*sizeof(sonar_pose_t));
  for(x=0;x<rflex_configs->num_sonars;x++){
    rflex_configs->mmrad_sonar_poses[x].x=
      cf->ReadTupleFloat(section, "mmrad_sonar_poses",3*x+1,0.0);
    rflex_configs->mmrad_sonar_poses[x].y=
      cf->ReadTupleFloat(section, "mmrad_sonar_poses",3*x+2,0.0);
    rflex_configs->mmrad_sonar_poses[x].t=
      cf->ReadTupleFloat(section, "mmrad_sonar_poses",3*x,0.0);
  }

	rflex_configs->run |= cf->ReadInt(section, "rflex_done",0);

  Unlock();
}  


// a driver registration function
void 
RFLEXSonar_Register(DriverTable* table)
{
  table->AddDriver("rflex_sonar", PLAYER_READ_MODE, RFLEXSonar_Init);
}

size_t RFLEXSonar::GetData(void* client,unsigned char *dest, size_t maxsize,
                             uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  Lock();
  *((player_sonar_data_t*)dest) = ((player_rflex_data_t*)device_data)->sonar;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();

  return( sizeof(player_sonar_data_t) );
}






