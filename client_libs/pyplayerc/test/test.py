#!/usr/local/bin/python

# Desc: Test program for the Python player client (extension module).
# Author: Andrew Howard
# Date: 21 Aug 2002
# CVS: $Id$

import playerc
import sys

def test():
    """Run the tests."""

    host = 'ant'
    port = 6665

    client = playerc.client(None, host, port)

    client.connect()
    
    client.get_devlist()

    print client.devlist



    client.disconnect()

    return




if __name__ == '__main__':

    print 'running'

    test()
