
# Desc: Test the position interface
# Author: Andrew Howard
# Date: 15 Sep 2004
# CVS: $Id$

from playerc import *



def test_position(client, index, context):
    """Basic test of the position interface."""

    position = playerc_position(client, index)
    if position.subscribe(PLAYERC_READ_MODE) != 0:
        raise playerc_error_str()    

    for i in range(20):
        while 1:
            id = client.read()
            if id == position.info.id:
                break

        if context:
            print context,
        print "position: [%14.3f] " % (position.info.datatime),
        print '[%6.3f %6.3f %6.3f]' % (position.px, position.py, position.pa)

    position.unsubscribe()
    return
