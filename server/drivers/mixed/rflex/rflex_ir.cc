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

class RFLEXIr: public RFLEX
{
 public:
  RFLEXIr(char* interface, ConfigFile* cf, int section) :
    RFLEX(interface, cf, section){}
  
  virtual size_t GetData(void*, unsigned char *, size_t maxsize,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);
  void GetOptions(ConfigFile * cf, int section, rflex_config_t * rflex_configs);
};

CDevice* RFLEXIr_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_IR_STRING))
    {
      PLAYER_ERROR1("driver \"rflex_ir\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else{
    RFLEXIr* tmp=new RFLEXIr(interface, cf, section);
    tmp->GetOptions(cf,section,&rflex_configs);
    return (CDevice*)tmp;
  }
}

/* read out all of our configuration stuff into the config structure
 * for details on what each of these settings does, check the structure
 * definition
 */
void RFLEXIr::GetOptions(ConfigFile *cf,int section,rflex_config_t * rflex_configs){
  int x;
  Lock();
  int pose_count=cf->ReadInt(section, "pose_count",8);
  rflex_configs->ir_base_bank=cf->ReadInt(section, "rflex_base_bank",0);
  rflex_configs->ir_bank_count=cf->ReadInt(section, "rflex_bank_count",0);
  rflex_configs->ir_min_range=cf->ReadInt(section,"ir_min_range",100);
  rflex_configs->ir_max_range=cf->ReadInt(section,"ir_max_range",800); 
  rflex_configs->ir_count=new int[rflex_configs->ir_bank_count];
  rflex_configs->ir_a=new double[pose_count];
  rflex_configs->ir_b=new double[pose_count];
  rflex_configs->ir_poses.pose_count=pose_count;
  int RunningTotal = 0;
  for (int i=0; i < rflex_configs->ir_bank_count; ++i)
  	RunningTotal += (rflex_configs->ir_count[i]=(int) cf->ReadTupleFloat(section, "rflex_banks",i,0));

	// posecount is actually unnecasary, but for consistancy will juse use it for error checking :)
	if (RunningTotal != rflex_configs->ir_poses.pose_count)
	{
		fprintf(stderr,"Error in config file, pose_count not equal to total poses in bank description\n");  
		rflex_configs->ir_poses.pose_count = RunningTotal;
	}		
  
//  rflex_configs->ir_poses.poses=new int16_t[rflex_configs->ir_poses.pose_count];
  for(x=0;x<rflex_configs->ir_poses.pose_count;x++)
  {
  	rflex_configs->ir_poses.poses[x][0]=(int) cf->ReadTupleFloat(section, "poses",x*3,0);
  	rflex_configs->ir_poses.poses[x][1]=(int) cf->ReadTupleFloat(section, "poses",x*3+1,0);
  	rflex_configs->ir_poses.poses[x][2]=(int) cf->ReadTupleFloat(section, "poses",x*3+2,0);
	
	// Calibration parameters for ir in form range=(a*voltage)^b
	rflex_configs->ir_a[x] = cf->ReadTupleFloat(section, "rflex_ir_calib",x*2,1);
	rflex_configs->ir_b[x] = cf->ReadTupleFloat(section, "rflex_ir_calib",x*2+1,1);	
  }
  Unlock();
}  


// a driver registration function
void 
RFLEXIr_Register(DriverTable* table)
{
  table->AddDriver("rflex_ir", PLAYER_READ_MODE, RFLEXIr_Init);
}

size_t RFLEXIr::GetData(void* client,unsigned char *dest, size_t maxsize,
                             uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  Lock();
  *((player_ir_data*)dest) = ((player_rflex_data_t*)device_data)->ir;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();

  return( sizeof(player_ir_data) );
}






