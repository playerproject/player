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

# Desc: Build the map using data from all robots robot
# Author: Andrew Howard
# Date: 14 Jun 2003

import math
import sys
import rtk3

import geom
import map

from cmodules import scan
from cmodules import grid
from cmodules import relax





class TeamMapper:
    """Build the map using data from multiple robot."""

    def __init__(self, map, root_fig):

        self.map = map
        self.root_fig = root_fig
        self.mutuals = {}
        self.fixed = []
        self.connected = []
        return


    def update(self):
        """Look for team-wide info."""

        # Install handlers for patches
        #if self.root_fig:
        #    for patch in self.map.patches:
        #        if not patch in self.connected:
        #            patch.fig.connect(self.on_patch_press, patch)
        #            self.connected.append(patch)

        return 0


    def on_patch_press(self, fig, event, pos, patch):
        """Process mouse events for a map patch."""

        patch.pose = patch.fig.get_origin()
            
        if event == rtk3.EVENT_RELEASE:

            patch.draw()
            for link in self.map.get_patch_links(patch):
                link.draw()

        return


    def infer_links(self):
        """Look for missing links."""

        # Add links for patches that overlap, but for which
        # we have no links
        for patch in self.map.patches:
            for npatch in self.map.patches:
                if npatch == patch:
                    continue
                if npatch in self.map.find_connected(patch):
                    continue

                match = scan.ScanMatch(patch.scan, npatch.scan)
                pairs = match.pairs(patch.pose, npatch.pose, 1e16)

                if len(pairs) < 4:
                    continue

                link = self.map.create_link(patch, npatch)
                link.scan_w = 1.0
                link.scan_pose_ba = geom.coord_sub(link.patch_b.pose, link.patch_a.pose)
                link.scan_pose_ab = geom.coord_sub(link.patch_a.pose, link.patch_b.pose)
                link.draw()

                print 'added link %d %d' % (link.patch_a.id, link.patch_b.id)

        return


    def match_all(self):
        """Update scan matches for all patches."""

        outlier = 1e16
        for patch in self.map.patches:
            for link in self.map.get_patch_links(patch):
                match = scan.ScanMatch(link.patch_a.scan, link.patch_b.scan)
                link.pairs = match.pairs(link.patch_a.pose, link.patch_b.pose, outlier)
        return


    def fit_all(self):
        """Improve fit for all patches."""

        outlier = 1e16

        rgraph = relax.Relax()

        # Create relax node for everything
        for patch in self.map.patches:
            patch.rnode = relax.Node(rgraph)
            patch.rnode.pose = patch.pose
            patch.rnode.free = 1 #not patch.fixed

        # Create all the links
        for link in self.map.links:

            link.rlinks = []
            for pair in link.pairs:
                rlink = relax.Link(rgraph, link.patch_a.rnode, link.patch_b.rnode)
                rlink.type = (pair[0],)
                rlink.w = (max(pair[1], 2.0),) # Magic
                rlink.outlier = (outlier,)
                rlink.pa = pair[2]
                rlink.pb = pair[3]
                rlink.la = pair[4]
                rlink.lb = pair[5]
                link.rlinks += [rlink]
                
        # Relax the graph
        #err = rgraph.relax_ls(100, 1e-4, 0) # Magic
        err = rgraph.relax_nl(1000, 1e-7, 1e-3, 1e-4) # Magic

        # Read new values
        for patch in self.map.patches:
            #if not patch.fixed:
            patch.pose = patch.rnode.pose

        # Delete everything
        for link in self.map.links:
            del link.rlinks
        for patch in self.map.patches:
            del patch.rnode
        del rgraph

        print 'global fit %.6f' % (err)

        return err


    def export_grid(self, filename):
        """Export an occupancy grid."""

        # Compute the bounding box
        (min_x, min_y) = (+1e6, +1e6)
        (max_x, max_y) = (-1e6, -1e6)
        for patch in self.map.patches:
            min_x = min(min_x, patch.pose[0])
            min_y = min(min_y, patch.pose[1])
            max_x = max(max_x, patch.pose[0])
            max_y = max(max_y, patch.pose[1])

        grid_pose = ((max_x + min_x) / 2, (max_y + min_y) / 2, 0.0)
        grid_size = ((max_x - min_x) + 20, (max_y - min_y) + 20)
        grid_scale = 0.05

        ngrid = grid.grid(grid_size[0], grid_size[1], grid_scale)
        ngrid.set_model(10, -1, 100, -20)

        # Add scan data to grid
        for i in range(len(self.map.patches)):
            patch = self.map.patches[i]
            print 'generating %d of %d scans\r' % (i, len(self.map.patches)),
            sys.stdout.flush()
            for (rpose, ranges) in patch.ranges:
                npose = geom.coord_add(rpose, patch.pose)
                npose = geom.coord_sub(npose, grid_pose)
                ngrid.add_ranges_slow(npose, ranges)

        # Save the occupancy grid
        print 'saved occ grid [%s]' % filename
        ngrid.save_occ(filename)
        return



