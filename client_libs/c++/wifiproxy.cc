/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     John Sweeney, UMASS, Amherst, Laboratory for Perceptual Robotics
 *     Brian Gerkey & Andrew Howard
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
 *  Copyright (C) 2003
 *     John Sweeney, UMASS, Amherst, Laboratory for Perceptual Robotics
 *     Brian Gerkey & Andrew Howard
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
 * $Id$
 * implementation of WiFi proxy class
 */

#include <playerclient.h>
#include <netinet/in.h>

/* fills in all the data....
 *
 * returns:
 */
void
WiFiProxy::FillData(player_msghdr_t hdr, const char *buffer)
{
  if (hdr.size != sizeof(player_wifi_data_t)) {
    fprintf(stderr, "WIFIPROXY: expected %d but got %d\n",
	    sizeof(player_wifi_data_t), hdr.size);
  }

  /* TODO: fix
  link = ntohs( ((player_wifi_data_t *)buffer)->link );
  level = ntohs( ((player_wifi_data_t *)buffer)->level );
  noise = ntohs( ((player_wifi_data_t *)buffer)->noise );
  */
  
}

/* print it
 *
 * returns:
 */
void
WiFiProxy::Print()
{
  printf("#WiFi(%d:%d) - %c\n", device, index, access);
  printf("\tlink: %d\tlevel: %d\tnoise: %d\n", link, level, noise);
}
