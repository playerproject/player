#/*
# *  Player - One Hell of a Robot Server
# *  Copyright (C) 2004
# *     Andrew Howard
# *                      
# *
# *  This library is free software; you can redistribute it and/or
# *  modify it under the terms of the GNU Lesser General Public
# *  License as published by the Free Software Foundation; either
# *  version 2.1 of the License, or (at your option) any later version.
# *
# *  This library is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# *  Lesser General Public License for more details.
# *
# *  You should have received a copy of the GNU Lesser General Public
# *  License along with this library; if not, write to the Free Software
# *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
# */

# Desc: Test wifi device
# Author: Andrew Howard
# Date: 1 Aug 2004
# CVS: $Id$

from playerc import *


def test_wifi(client, index):

    device = playerc_wifi_create(client, index)

    #for i in range(device.link_count + 10):
    #    link = playerc_wifi_get_link(device, i)
    #    #print '[%s] [%d]' % (link.mac, link.level)

    link = device.get_link(0)
    print '[%s] [%d]' % (link.mac, link.level)
    
    if playerc_wifi_subscribe(device, PLAYERC_OPEN_MODE) != 0:
        raise playerc_error_str()    

    for i in range(5):
        playerc_client_read(client)

        print device.scan

        for link in device.links:
            print "wifi: [%d] [%s] [%s] [%s] [%4d] [%4d]" % \
                  (i, link.mac, link.essid, link.ip, link.level, link.noise)
        
    
    return

