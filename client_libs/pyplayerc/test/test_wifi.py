
# Desc: Test wifi device
# Author: Andrew Howard
# Date: 20 Nov 2002
# CVS: $Id$

import playerc

def test_wifi(client, index):
    """Test the wifi device."""

    device = playerc.wifi(client, 0, index)
    device.subscribe('r')

    
    for i in range(3):

        while client.read() != device:
            pass
        print device
        print "wifi: [%4d] [%4d] [%4d]" % \
              (device.link, device.level, device.noise)
    return

