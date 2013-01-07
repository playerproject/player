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
