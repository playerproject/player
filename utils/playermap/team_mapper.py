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

# Desc: Do global mapping stuff
# Author: Andrew Howard
# Date: 14 Jun 2003

import math
import sys
import rtk3

import geom
import map
import gui

from cmodules import scan
from cmodules import grid
from cmodules import relax



class TeamMapper:
    """Build the map using data from multiple robot."""

    def __init__(self, map, root_fig):

        self.map = map
        self.root_fig = root_fig

        # Direct all patch figure events back to us
        self.map.patch_callback = self.on_patch_press
        return


    def update(self):
        """Look for team-wide info."""

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
                group_a = scan.scan_group()
                group_a.add_scan((0, 0, 0), link.patch_a.scan)
                group_b = scan.scan_group()
                group_b.add_scan((0, 0, 0), link.patch_b.scan)
                match = scan.scan_match(group_a, group_b)
                link.pairs = match.pairs(link.patch_a.pose, link.patch_b.pose, outlier)
        return


    def fit_all(self):
        """Improve fit for all patches."""

        outlier = 1e16

        rgraph = relax.Relax()
        rnodes = {}
        rlinks = []

        # Create a node for geo-referencing
        geo_node = relax.Node(rgraph)
        geo_node.free = 0

        # Create nodes for all the patches
        for patch in self.map.patches:
            rnode = relax.Node(rgraph)
            rnode.pose = patch.pose
            rnode.free = 1
            rnodes[patch] = rnode

        # Create geo-reference links
        for pin in self.map.pins:
            for (patch, pos) in pin.patches:
                rlink = relax.Link(rgraph, geo_node, rnodes[patch])
                rlink.type = (0,)
                rlink.w = (pin.pos_var ** (-2),)  # HACK
                rlink.outlier = (1e6,) # HACK
                rlink.pa = pin.pos
                rlink.pb = pos
                rlinks += [rlink]

        # Create links between patches
        for link in self.map.links:
            for pair in link.pairs:
                rnode_a = rnodes[link.patch_a]
                rnode_b = rnodes[link.patch_b]                
                rlink = relax.Link(rgraph, rnode_a, rnode_b)
                rlink.type = (pair[0],)
                rlink.w = (pair[2],)
                rlink.outlier = (outlier,)
                rlink.pa = pair[3]
                rlink.pb = pair[4]
                rlink.la = pair[5]
                rlink.lb = pair[6]
                rlinks += [rlink]
                
        # Relax the graph
        (err, steps, m) = rgraph.relax_nl(1000, 1e-3, 1e-7, 1e-2, 1e-4) # Magic

        # Read new values
        for patch in self.map.patches:
            patch.pose = rnodes[patch].pose

        print 'global fit %d %.6f' % (steps, err)

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

        fig = rtk3.Fig(self.root_fig)

        # Add scan data to grid
        for i in range(len(self.map.patches)):

            patch = self.map.patches[i]

            print 'generating %d of %d patches\r' % (i, len(self.map.patches)),
            sys.stdout.flush()

            for (rpose, ranges) in patch.ranges:
                npose = geom.coord_add(rpose, patch.pose)
                npose = geom.coord_sub(npose, grid_pose)
                ngrid.add_ranges_slow(npose, ranges)
                #ngrid.add_ranges_fast(npose, ranges)

        # Save the occupancy grid
        print 'saved occ grid [%s]' % filename
        ngrid.save_occ(filename)

        # HACK
        path_filename = 'path' + filename

        # Save the path
        print 'saved path [%s]' % path_filename
        ngrid.save_path(path_filename)

        return



