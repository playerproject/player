#
# Map maker
# Copyright (C) 2003 Andrew Howard 
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

# Desc: Do geo-referencing (push-pins)
# Author: Andrew Howard
# Date: 14 Jun 2003

import math
import string
import sys
import rtk3

import geom
import map





class GeoMapper:
    """Geo-referencing of the map."""

    def __init__(self, nmap, root_fig):

        self.map = nmap
        self.root_fig = root_fig

        self.pins = []
        return


    def load_pins(self, filename):
        """Load pins from a file."""

        file = open(filename)

        while 1:
            line = file.readline()
            if not line:
                break
            tokens = string.split(line)
            if not tokens:
                continue
            if len(tokens) < 4:
                continue
            if tokens[0][0] == '#':
                continue

            id = tokens[0]
            pos = (float(tokens[1]), float(tokens[2]))
            pos_var = float(tokens[3])

            # HACK scale
            scale = 0.05
            pos = (pos[0] * scale, -pos[1] * scale)
            pos_var = pos_var * scale

            pin = map.MapPin()

            pin.id = id
            pin.pos = pos
            pin.pos_var = pos_var
            pin.fig = rtk3.Fig(self.root_fig)
            pin.fig.origin((pin.pos[0], pin.pos[1], 0.0))
            pin.fig.fgcolor((128, 0, 128, 255))
            pin.fig.bgcolor((128, 0, 128, 32))
            pin.fig.circle((0, 0), 5.0)
            pin.fig.circle((0, 0), pin.pos_var)
            pin.fig.text((0, 0.50), '%s %.3f %.3f' % (pin.id, pin.pos[0], pin.pos[1]))
            pin.fig.connect(self.on_pin_event, pin)
        
            self.pins.append(pin)
            self.map.pins.append(pin)

        return


    def on_pin_event(self, fig, event, pos, pin):
        """Process mouse events for a pin."""

        if event == rtk3.EVENT_RELEASE:

            pin.patches = []

            # Pin all overlapping patches
            for patch in self.map.patches:
                npos = geom.coord_sub(pos, patch.pose)
                if patch.scan.test_free(npos) > 0.0:
                    print 'pinned %d' % patch.id
                    pin.patches.append((patch, npos))

            # Restore pin pose
            pin.fig.origin((pin.pos[0], pin.pos[1], 0.0))

        return


    def draw(self):
        """Draw the pins."""

        for pin in self.pins:
            pass

        return
