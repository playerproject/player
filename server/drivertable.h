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
 *   class to keep track of available drivers.  
 */

#ifndef _DRIVERTABLE_H
#define _DRIVERTABLE_H

#include <device.h>
#include <pthread.h>
#include <configfile.h>

typedef CDevice* (*DriverInitFn) (char *interface, ConfigFile *cf, int section);
typedef CDevice* (*DriverInitFnEx) (ConfigFile *cf, int section);



// one element in a linked list
class DriverEntry
{
  public:
    char name[PLAYER_MAX_DEVICE_STRING_LEN]; // the string name for the driver
    unsigned char access;   // allowed access mode: 'r', 'w', or 'a'

    // factory creation function
    DriverInitFn initfunc;
    //CDevice* (*initfunc)(char*,ConfigFile*,int);
    
    // factory creation function (new-style)
    DriverInitFnEx initfuncEx;

    // next in list
    DriverEntry* next;  

    DriverEntry() { name[0]='\0'; next = NULL; }
};

class DriverTable
{
  private:
    // we'll keep the driver info here.
    DriverEntry* head;
    int numdrivers;

  public:
    DriverTable();
    ~DriverTable();

    // how many drivers we have
    int Size() { return(numdrivers); }
    
    // add a new driver to the table
    int AddDriver(char* name, char access, DriverInitFn initfunc);
                  //CDevice* (*initfunc)(char*,ConfigFile*,int));

    // add a new driver to the table (new-style)
    int AddDriverEx(char* name, DriverInitFnEx initfuncEx);

    // sort drivers, based on name.  the return value points at newly
    // malloc()ed memory, which the user should free().
    char** SortDrivers();

    // matches on the driver name
    DriverEntry* GetDriverEntry(const char* name);
   
    // get the ith driver name; returns NULL if there is no such driver
    char* GetDriverName(int idx);
};

#endif
