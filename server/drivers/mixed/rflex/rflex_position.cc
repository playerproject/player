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
 *   modified from the P2 position device, the this device is identicle
 *   but designed for the RWI RFLEX interface instead
 *
 *   the RFLEX position device.  accepts commands for changing
 *   wheel speeds, and returns data on x,y,theta,compass, etc.
 */

#include <stdio.h>
#include <rflex.h>
#include <string.h>
#include <rflex_configs.h>
#include <stdlib.h>

class RFLEXPosition: public RFLEX 
{
 public:
   ~RFLEXPosition();
  RFLEXPosition(char* interface, ConfigFile* cf, int section) :
    RFLEX(interface, cf, section){}
  virtual size_t GetData(void*,unsigned char *, size_t maxsize,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);
  void PutCommand(void*, unsigned char *, size_t maxsize);
  void GetOptions(ConfigFile * cf, int section, rflex_config_t* rflex_config);
};

CDevice* RFLEXPosition_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
  {
    PLAYER_ERROR1("driver \"rfflex_position\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else{
    RFLEXPosition* tmp=new RFLEXPosition(interface, cf, section);
    tmp->GetOptions(cf,section,&rflex_configs);
    return (CDevice*) tmp;
  }
}

void RFLEXPosition::GetOptions(ConfigFile * cf,int section, rflex_config_t *rflex_configs){
  char temp[1024];
  Lock();
  //length
  rflex_configs->mm_length=atof(strncpy(temp,cf->ReadString(section, "mm_length", temp),sizeof(temp)));
  //width
  rflex_configs->mm_width=atof(strncpy(temp,cf->ReadString(section, "mm_width", temp),sizeof(temp)));
  //distance conversion
  rflex_configs->odo_distance_conversion=atof(strncpy(temp,cf->ReadString(section, "odo_distance_conversion", temp),sizeof(temp)));
  //angle conversion
  rflex_configs->odo_angle_conversion=atof(strncpy(temp,cf->ReadString(section, "odo_angle_conversion", temp),sizeof(temp)));
  //default trans accel
  rflex_configs->mmPsec2_trans_acceleration=atof(strncpy(temp,cf->ReadString(section, "default_trans_acceleration", temp),sizeof(temp)));
  //defautl rot accel
  rflex_configs->radPsec2_rot_acceleration=atof(strncpy(temp,cf->ReadString(section, "default_rot_acceleration", temp),sizeof(temp)));

  Unlock();
}

// a driver registration function
void 
RFLEXPosition_Register(DriverTable* table)
{
  table->AddDriver("rflex_position", PLAYER_ALL_MODE, RFLEXPosition_Init);
}

RFLEXPosition::~RFLEXPosition()
{
  ((player_rflex_cmd_t*)device_command)->position.xspeed = 0;
  ((player_rflex_cmd_t*)device_command)->position.yawspeed = 0;
}

size_t RFLEXPosition::GetData(void* client, unsigned char *dest, size_t maxsize,
                                 uint32_t* timestamp_sec, 
                                 uint32_t* timestamp_usec)
{
  Lock();
  *((player_position_data_t*)dest) = 
          ((player_rflex_data_t*)device_data)->position;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();
  return( sizeof( player_position_data_t) );
}

void RFLEXPosition::PutCommand(void* client, unsigned char *src, size_t size ) 
{
  if(size != sizeof( player_position_cmd_t ) )
  {
    puts("RFLEXPosition::PutCommand(): command wrong size. ignoring.");
    printf("expected %d; got %d\n", sizeof(player_position_cmd_t),size);
  }
  else
    ((player_rflex_cmd_t*)device_command)->position = 
            *((player_position_cmd_t*)src);
}













