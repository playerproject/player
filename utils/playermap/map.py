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

# Desc: Graph representation of the map
# Author: Andrew Howard
# Date: 30 Jun 2003

import math
import string
import sys
import rtk3

import geom
from cmodules import scan
import cmodules.grid


class MapPin:
    """A global reference point for the map."""

    def __init__(self):

        self.id = None
        self.pos = None
        self.pos_var = None
        self.patches = []
        return


class MapPatch:
    """A map patch."""

    def __init__(self):

        self.id = None
        self.island = None
        self.pose = None
        self.odom_pose = None
        self.odom_dist = None
        self.scan = None
        self.ranges = []
        self.fig = None
        self.site_fig = None
        return


    def draw(self):
        """Draw or redraw the node."""

        if not self.fig:
            return

        self.fig.clear()
        self.fig.elevate(1000)
        self.fig.origin(self.pose)

        self.fig.fgcolor((0, 0, 255, 255))
        self.fig.bgcolor((0, 0, 255, 255))
        self.fig.circle((0, 0), 0.20)

        self.fig.text((0.50, 0.50), '%d' % (self.id))

        #self.fig.polyline([(-0.10, 0), (+0.10, 0)])
        #self.fig.polyline([(0, -0.10), (0, +0.10)])

        # Draw free space
        self.fig.fgcolor((255, 255, 255, 16))
        self.fig.bgcolor((255, 255, 255, 16))
        self.fig.polygon((0, 0, 0), self.scan.get_free())

        # Draw the hit points
        self.fig.fgcolor((0, 0, 0, 255))
        self.fig.bgcolor((0, 0, 0, 255))
        for (s, w) in self.scan.get_hits():
            if w > 1:
                col = int(max(0, 128 - w * 128 / 10))
                self.fig.fgcolor((col, col, col, 255))
                self.fig.bgcolor((col, col, col, 255))
                #self.fig.circle(s, 0.05)
                self.fig.rectangle((s[0], s[1], 0), (0.05, 0.05))

        # REMOVE
        # Draw pins
        #for (pos, pin) in self.pins:
        #    self.fig.fgcolor((255, 0, 255, 255))
        #    self.fig.bgcolor((255, 0, 255, 32))
        #    pos_a = pos
        #    pos_b = geom.coord_sub(pin.pos, self.pose)
        #    self.fig.polyline((pos_a, pos_b))

        return




class MapLink:
    """A single link in the map."""

    def __init__(self, patch_a, patch_b):

        self.patch_a = patch_a
        self.patch_b = patch_b
        self.mutual_w = 0.0
        self.scan_w = 0.0
        self.fig = None
        return


    def draw(self):
        """Draw or redraw the link."""

        if not self.fig:
            return

        self.fig.clear()
        if self.mutual_w > 0:
            self.fig.fgcolor((128, 0, 128, 128))
        else:
            self.fig.fgcolor((0, 0, 255, 128))
        self.fig.polyline([self.patch_a.pose[:2], self.patch_b.pose[:2]])
        self.fig.lower(1000)
        return
    


class Map:
    """The map."""

    def __init__(self, root_fig):

        self.root_fig = root_fig
        self.patches = []
        self.links = []
        self.links_a = {}
        self.links_b = {}
        self.pins = []
        self.patch_id = 0
        self.patch_callback = None
        return
    

    def create_patch(self, island):
        """Create a new patch in the map."""

        patch = MapPatch()

        patch.id = self.patch_id
        if island != None:
            patch.island = island
        else:
            patch.island = patch.id

        if self.root_fig:
            patch.fig = rtk3.Fig(self.root_fig)
            patch.site_fig = rtk3.Fig(patch.fig)
            if self.patch_callback:
                patch.fig.connect(self.patch_callback, patch)

        self.patch_id += 1
        self.patches += [patch]
        self.links_a[patch] = []
        self.links_b[patch] = []

        return patch


    def destroy_patch(self, patch):
        """Destroy a patch and all of its associted links."""

        # Get a list of all the links connected to this patch
        links = self.get_patch_links(patch)

        # Remove all the links
        for link in links:
            self.destroy_link(link)

        assert(len(self.links_a[patch]) == 0)
        assert(len(self.links_b[patch]) == 0)        
        del self.links_a[patch]
        del self.links_b[patch]

        # Remove the patch from the patch list
        self.patches.remove(patch)
        return


    def create_link(self, patch_a, patch_b):
        """Create a link between two patches."""

        assert(patch_a != patch_b)

        link = MapLink(patch_a, patch_b)
        self.links += [link]

        # Add link to list of all outgoing links for patch_a
        self.links_a[patch_a] += [link]

        # Add link to list of all incoming links for patch_b
        self.links_b[patch_b] += [link]

        if self.root_fig:
            link.fig = rtk3.Fig(self.root_fig)
            link.draw()
        
        return link


    def destroy_link(self, link):
        """Destroy a link between two patches."""

        # Remove link from the outgoing list for patch_a
        self.links_a[link.patch_a].remove(link)

        # Remove link from the incoming list for patch_b
        self.links_b[link.patch_b].remove(link)

        # Remove from the unsorted list of links
        self.links.remove(link)

        link.patch_a = None
        link.patch_b = None
        return


    def get_patch_links(self, patch):
        """Get a list of all the links connected to the given patch."""

        links = []

        # Get all outgoing links
        if self.links_a.has_key(patch):
            links += self.links_a[patch]

        # Get all incoming links
        if self.links_b.has_key(patch):
            links += self.links_b[patch]
                
        return links
    

    def find_connected(self, patch):
        """Get a list of all the patches connected to the given patch."""

        patches = []

        # Get all outgoing links
        if self.links_a.has_key(patch):
            for link in self.links_a[patch]:
                patches += [link.patch_b]

        # Get all incoming links
        if self.links_b.has_key(patch):
            for link in self.links_b[patch]:
                patches += [link.patch_a]
                    
        return patches


    def find_nhood(self, patch, max_dist):
        """Find all the patches in the neighborhood of the given patch
        (does not include the givne patch)."""

        open = self.find_connected(patch)
        closed = [patch]
        patches = []

        while open:

            npatch = open.pop(0)

            pose = geom.coord_sub(npatch.pose, patch.pose)
            dist = math.sqrt(pose[0] * pose[0] + pose[1] * pose[1])
            
            if dist > max_dist:
                continue
            patches.append(npatch)
            closed.append(npatch)

            for mpatch in self.find_connected(npatch):
                if mpatch in open:
                    continue
                if mpatch in closed:
                    continue
                open.append(mpatch)

        assert(patch not in patches)
        return patches
    
