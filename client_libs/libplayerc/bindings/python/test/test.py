#!/usr/bin/env python

# Desc: Test Python bindings
# Author: Andrew Howard
# Date: 15 Sep 2004
# CVS: $Id$

import getopt
import string
import sys
import threading

from playerc import *

from test_camera import *
from test_position import *
from test_laser import *
from test_wifi import *



def main(server, test, context):
    """Open a connection and run a test."""

    # Connect to server
    c = playerc_client(None, 'localhost', 6665)
    if c.connect() != 0:
        raise playerc_error_str()

    # Get the list of available devices
    if c.get_devlist() != 0:
        raise playerc_error_str()

    # Print the device list
    print '\033[m'
    print '----------------------------------------------------------------------'
    for devinfo in c.devinfos:
        print '%d:%s:%d %20s' % \
              (devinfo.port, playerc_lookup_name(devinfo.code),
               devinfo.index, devinfo.drivername)
    print '----------------------------------------------------------------------'

    # Switch to async mode
    c.datamode(PLAYERC_DATAMODE_PUSH_ASYNC)

    eval('test_%s(c, %d, context)' % (test[0], test[1]))

    c.disconnect()
    return



class TestThread(threading.Thread):
    """Dummy class for testing multiple threads."""

    def __init__(self, target, args):

        threading.Thread.__init__(self, None, target, None, args)
        return





if __name__ == '__main__':

    threaded = False
    server = ['localhost', 6665]
    tests = []

    (opts, args) = getopt.getopt(sys.argv[1:], "h:p:t")

    for opt in opts:
        if opt[0] == '-h':
            server[0] = opt[1]
        elif opt[0] == '-p':
            server[1] = int(opt[1])
        elif opt[0] == '-t':
            threaded = True

    for arg in args:
        tokens = string.split(arg, ':')
        test = (tokens[0], int(tokens[1]))
        tests.append(test)


    if not threaded:

        # Run tests sequentially
        for (i, test) in enumerate(tests):
            context = 'T%d' % i
            main(server, test, context)

    else:

        # Run tests in parallel
        tids = []
        for (i, test) in enumerate(tests):
            context = '\033[%dmT%d' % (31 + i % 2, i)
            tid = TestThread(main, (server, test, context))
            tid.start()
            tids.append(tid)

        # Wait for threads to complete
        for tid in tids:
            tid.join()

    print '\033[m'
            
