#!/usr/bin/env python

# Desc: Test program for the Python player client (extension module).
# Author: Andrew Howard
# Date: 21 Aug 2002
# CVS: $Id$

import sys
import playerc


from test_wifi import test_wifi


def test():
    """Run the tests."""

    host = 'fly'
    port = 6665

    client = playerc.client(None, host, port)

    client.connect()
    
    client.get_devlist()

    print client.devlist

    test_wifi(client, 0)

    client.disconnect()

    return




if __name__ == '__main__':

    print 'running'

    test()
