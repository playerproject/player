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
 *
 * the miscellanous device for the Pioneer 2.  this is a good place
 * to return any random bits of data that don't fit well into other
 * categories, from battery voltage and bumper state to digital and
 * analog in/out.
 *
 */

#include <p2os.h>

class P2OSMisc: public P2OS 
{
 public:

   P2OSMisc(int argc, char** argv):P2OS(argc,argv){}
   size_t GetData( unsigned char *, size_t maxsize, 
                   uint32_t* timestamp_sec, uint32_t* timestamp_usec);
};

CDevice* P2OSMisc_Init(int argc, char** argv)
{
  return((CDevice*)(new P2OSMisc(argc,argv)));
}


size_t P2OSMisc::GetData(unsigned char *dest, size_t maxsize,
                            uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  Lock();
  *((player_misc_data_t*)dest) = ((player_p2os_data_t*)device_data)->misc;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();

  return( sizeof( player_misc_data_t ));
}

