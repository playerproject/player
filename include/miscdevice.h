/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
#ifndef MISCDEVICE
#define MISCDEVICE

#include "p2osdevice.h"

class CMiscDevice: public CP2OSDevice {
 public:
  CMiscDevice::CMiscDevice(char* port):CP2OSDevice(port){}
  int GetData( unsigned char * );
};

#endif

