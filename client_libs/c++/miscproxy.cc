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
 * client-side misc device 
 */

#include <miscproxy.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

void MiscProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_misc_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of misc data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_misc_data_t),hdr.size);
  }

  frontbumpers = ((player_misc_data_t*)buffer)->frontbumpers;
  rearbumpers = ((player_misc_data_t*)buffer)->rearbumpers;
  voltage = ((player_misc_data_t*)buffer)->voltage;
}

// interface that all proxies SHOULD provide
void MiscProxy::Print()
{
  printf("#Misc(%d:%d) - %c\n", device, index, access);
  puts("#front\trear\tvoltage");
  printf("%u\t%u\t%u\n", frontbumpers,rearbumpers,voltage);
}

