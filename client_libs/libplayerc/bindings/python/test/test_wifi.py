
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
    
    if playerc_wifi_subscribe(device, PLAYERC_READ_MODE) != 0:
        raise playerc_error_str()    

    for i in range(5):
        playerc_client_read(client)

        print device.scan

        for link in device.links:
            print "wifi: [%d] [%s] [%s] [%s] [%4d] [%4d]" % \
                  (i, link.mac, link.essid, link.ip, link.level, link.noise)
        
    
    return

