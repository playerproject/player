/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007
 *     Brian Gerkey
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
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007
 *     Brian Gerkey
 *                      
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Common (i.e., implementation-independent) libplayersd functions.
 *
 * $Id$
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <libplayercore/addr_util.h>
#include <libplayercore/interface_util.h>
#include "playersd.h"

#define PLAYER_SD_DEVS_LEN_INITIAL 4
#define PLAYER_SD_DEVS_LEN_MULTIPLIER 2

player_sd_dev_t* 
player_sd_get_device(player_sd_t* sd, const char* name)
{
  int i;

  for(i=0;i<sd->devs_len;i++)
  {
    if(sd->devs[i].valid && !strcmp(sd->devs[i].name,name))
      return(sd->devs + i);
  }
  
  return(NULL);
}

player_sd_dev_t*
_player_sd_add_device(player_sd_t* sd, const char* name)
{
  int i,j;

  // Look for an empty spot.
  for(i=0;i<sd->devs_len;i++)
  {
    if(!sd->devs[i].valid)
      break;
  }

  // Grow the list if necessary
  if(i == sd->devs_len)
  {
    if(!sd->devs_len)
      sd->devs_len = PLAYER_SD_DEVS_LEN_INITIAL;
    else
      sd->devs_len *= PLAYER_SD_DEVS_LEN_MULTIPLIER;

    sd->devs = 
            (player_sd_dev_t*)realloc(sd->devs,
                                      sizeof(player_sd_dev_t) * sd->devs_len);
    assert(sd->devs);

    for(j=i;j<sd->devs_len;j++)
      sd->devs[j].valid = 0;
  }

  return(sd->devs + i);
}

void
player_sd_printcache(player_sd_t* sd)
{
  char ip[16];
  int i;

  puts("Device cache:");
  for(i=0;i<sd->devs_len;i++)
  {
    if(sd->devs[i].valid)
    {
      printf("  name:%s\n", sd->devs[i].name);
      if(sd->devs[i].addr_valid)
      {
        packedaddr_to_dottedip(ip,sizeof(ip),sd->devs[i].addr.host);
        printf("    host:    %s\n"
               "    robot:   %d\n"
               "    interf:  %d(%s)\n"
               "    index:   %d\n",
               ip, 
               sd->devs[i].addr.robot, 
               sd->devs[i].addr.interf, 
               interf_to_str(sd->devs[i].addr.interf), 
               sd->devs[i].addr.index);
      }
    }
  }
}

