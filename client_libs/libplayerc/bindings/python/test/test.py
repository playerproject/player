#!/usr/bin/env python

import sys

from playerc import *
from test_wifi import *



def test_position():

    c = playerc_client_create(None, 'localhost', 6665)
    if playerc_client_connect(c) != 0:
        raise playerc_error_str()

    pos = playerc_position_create(c, 0)

    if playerc_position_subscribe(pos, PLAYERC_READ_MODE) != 0:
        raise playerc_error_str()    

    while 1:

        proxy = playerc_client_read(c)

        print pos.pose
        print pos.px, pos.py, pos.pa
    
    return




def test_laser(client, index):
    """Basic test of the laser interface."""

    laser = playerc_laser(client, index)

    if laser.subscribe(PLAYERC_READ_MODE) != 0:
        raise playerc_error_str()    

    for i in range(10):

        proxy = client.read()
        print proxy == laser.this

        print "laser: [%14.3f] [%d] " % (laser.info.datatime, laser.scan_count),
        for i in range(3):
            print "[%6.3f, %6.3f] " % (laser.scan[i][0], laser.scan[i][1]),
        print
    
    return



if __name__ == '__main__':

    server = ('localhost', 6665)
    test = 'laser'
    index = 0

    # Connect to server
    c = playerc_client(None, 'localhost', 6665)
    if c.connect() != 0:
        raise playerc_error_str()

    # Get the list of available devices
    if c.get_devlist() != 0:
        raise playerc_error_str()

    # Print the device list
    for devinfo in c.devinfos:
        print '%d:%s:%d %20s' % \
              (devinfo.port, playerc_lookup_name(devinfo.code),
               devinfo.index, devinfo.drivername)

    eval('test_%s(c, index)' % test)
