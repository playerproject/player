
# Desc: Test the laser
# Author: Andrew Howard
# Date: 15 Sep 2004
# CVS: $Id$

from playerc import *


def test_laser(client, index, context):
    """Basic test of the laser interface."""

    laser = playerc_laser(client, index)
    if laser.subscribe(PLAYERC_READ_MODE) != 0:
        raise playerc_error_str()    

    for i in range(10):

        while 1:
            id = client.read()
            if id == laser.info.id:
                break

        if context:
            print context,            
        print "laser: [%14.3f] [%d]" % (laser.info.datatime, laser.scan_count),
        for i in range(3):
            print "[%06.3f, %06.3f]" % (laser.scan[i][0], laser.scan[i][1]),
        print

    laser.unsubscribe()    
    return
