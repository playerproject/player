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
#include <drivertable.h>

#include <string.h> // for strncpy(3)

// initialize the table
DriverTable::DriverTable()
{
  numdrivers = 0;
  head = NULL;
}

// tear down the table
DriverTable::~DriverTable()
{
  DriverEntry* thisentry = head;
  DriverEntry* tmpentry;
  // for each registered driver, delete it.
  while(thisentry)
  {
    tmpentry = thisentry->next;
    delete thisentry;
    numdrivers--;
    thisentry = tmpentry;
  }
}
    
// add a new driver to the table
int 
DriverTable::AddDriver(char* name, char access, 
                       CDevice* (*initfunc)(char*,ConfigFile*,int))
{
  DriverEntry* thisentry;
  DriverEntry* preventry;
  // don't check for preexisting driver, just overwrite the old driver.  
  // shouldn't really come up.
  for(thisentry = head,preventry=NULL; thisentry; 
      preventry=thisentry, thisentry=thisentry->next)
  {
    if(!strncmp(thisentry->name, name, sizeof(thisentry->name)))
      break;
  }

  if(!thisentry)
  {
    thisentry = new DriverEntry;
    if(preventry)
      preventry->next = thisentry;
    else
      head = thisentry;
    numdrivers++;
  }

  strncpy(thisentry->name, name, sizeof(thisentry->name));
  thisentry->name[sizeof(thisentry->name)-1] = '\0';
  thisentry->access = access;
  thisentry->initfunc = initfunc;

  return(0);
}

// matches on the string name
DriverEntry* 
DriverTable::GetDriverEntry(char* name)
{
  DriverEntry* thisentry;
  DriverEntry* retval = NULL;
  for(thisentry=head;thisentry;thisentry=thisentry->next)
  {
    if(!strcmp(thisentry->name,name))
    {
      retval = thisentry;
      break;
    }
  }
  return(retval);
}

// get the ith driver name; returns NULL if there is no such driver
char* 
DriverTable::GetDriverName(int idx)
{
  DriverEntry* thisentry;
  char* retval = NULL;
  int i=0;
  for(thisentry=head;thisentry;thisentry=thisentry->next)
  {
    if(i == idx)
    {
      retval = thisentry->name;
      break;
    }
    i++;
  }
  return(retval);
}

