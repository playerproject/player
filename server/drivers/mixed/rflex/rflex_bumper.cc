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
 * Bumper device added by Toby Collett, University of Auckland, 2003-02-25
 * Tested with b21r robot
 * based on rflex_sonar device
 */

#include <rflex.h>
#include <string.h>
#include "rflex_configs.h"

class RFLEXbumper: public RFLEX 
{
 public:

   RFLEXbumper( ConfigFile* cf, int section) : 
           RFLEX( cf, section){}
   size_t GetData(void*, unsigned char *, size_t maxsize, 
                   uint32_t* timestamp_sec, uint32_t* timestamp_usec);
   void GetOptions(ConfigFile * cf, int section, rflex_config_t * rflex_configs);
};

Driver* RFLEXbumper_Init( ConfigFile* cf, int section)
{
  if(strcmp( PLAYER_BUMPER_STRING))
  {
    PLAYER_ERROR1("driver \"rflex_bumper\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else{
  	RFLEXbumper * tmp =new RFLEXbumper( cf, section);
	tmp->GetOptions(cf,section,&rflex_configs);
	RFLEX::BumperDev = static_cast<Driver *> (tmp);
    return static_cast<Driver *> (tmp);
  }
}

/* read out all of our configuration stuff into the config structure
 * for detauls on what each of these settings does, check the structure
 * deffinition
 */
void RFLEXbumper::GetOptions(ConfigFile *cf,int section,rflex_config_t * rflex_configs){
	int x;
	Lock();
  
	rflex_configs->bumper_count = cf->ReadInt(section, "bumper_count",0);
	rflex_configs->bumper_def = new player_bumper_define_t[rflex_configs->bumper_count];
  	for(x=0;x<rflex_configs->bumper_count;++x)
	{
		rflex_configs->bumper_def[x].x_offset = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x,0)); //mm
		rflex_configs->bumper_def[x].y_offset = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+1,0)); //mm
		rflex_configs->bumper_def[x].th_offset = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+2,0)); //deg
		rflex_configs->bumper_def[x].length = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+3,0)); //mm
		rflex_configs->bumper_def[x].radius = static_cast<int> (cf->ReadTupleFloat(section, "bumper_def",5*x+4,0));	//mm  	
	}
	rflex_configs->bumper_address = cf->ReadInt(section, "rflex_bumper_address",DEFAULT_RFLEX_BUMPER_ADDRESS);


	const char *bumperStyleStr = cf->ReadString(section, "rflex_bumper_style",DEFAULT_RFLEX_BUMPER_STYLE);
	
	if(strcmp(bumperStyleStr,RFLEX_BUMPER_STYLE_BIT) == 0)
	{
	   rflex_configs->bumper_style = BUMPER_BIT;
	}
	else if(strcmp(bumperStyleStr,RFLEX_BUMPER_STYLE_ADDR) == 0)
	{
	   rflex_configs->bumper_style = BUMPER_ADDR;
	}
	else
	{
	   //Invalid value
	   rflex_configs->bumper_style = BUMPER_ADDR;
	}
	
	rflex_configs->run |= cf->ReadInt(section, "rflex_done",0);
	
	Unlock();
}  


// a driver registration function
void 
RFLEXbumper_Register(DriverTable* table)
{
  table->AddDriver("rflex_bumper", PLAYER_READ_MODE, RFLEXbumper_Init);
}

size_t RFLEXbumper::GetData(void* client,unsigned char *dest, size_t maxsize,
                            uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  Lock();
  *((player_bumper_data_t*)dest) = ((player_rflex_data_t*)device_data)->bumper;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();

  return( sizeof( player_bumper_data_t ));
}

