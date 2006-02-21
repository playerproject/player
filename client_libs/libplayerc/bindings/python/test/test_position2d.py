
# Desc: Test the position2d interface
# Author: Andrew Howard
# Date: 15 Sep 2004
# CVS: $Id$

from playerc import *



def test_position2d(client, index, context):
    """Basic test of the position2d interface."""

    position2d = playerc_position2d(client, index)
    if position2d.subscribe(PLAYERC_OPEN_MODE) != 0:
        raise playerc_error_str()    

    for i in range(20):
        while 1:
            id = client.read()
            if id == position2d.info.id:
                break

        if context:
            print context,
        print "position2d: [%14.3f] " % (position2d.info.datatime),
        print '[%6.3f %6.3f %6.3f]' % (position2d.px, position2d.py, position2d.pa)

    position2d.unsubscribe()
    return
