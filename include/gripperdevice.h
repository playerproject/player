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
 *   the Pioneer 2 gripper device
 */

#ifndef _GRIPPERDEVICE_H
#define _GRIPPERDEVICE_H

#include <p2osdevice.h>

class CGripperDevice: public CP2OSDevice 
{
 public:
   static CDevice* Init(int argc, char** argv)
   {
     return((CDevice*)(new CGripperDevice(argc,argv)));
   }
   ~CGripperDevice();
   CGripperDevice(int argc, char** argv):CP2OSDevice(argc,argv){}
   size_t GetData(unsigned char *, size_t maxsize,
                  uint32_t* timestamp_sec, uint32_t* timestamp_usec );
   void PutCommand( unsigned char *, size_t maxsize);
};

#endif
