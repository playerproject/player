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

# Desc: Build the map using data from a single robot
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


class RobotMapper:
    """Build the map using data from a single robot."""

    def __init__(self, map, root_fig, options):

        self.map = map
        self.options = options
        self.root_fig = root_fig

        self.laser_pose = (0.05, 0, 0)
        self.update_dist = 0.20
        self.update_angle = float(options.get('--odom-angle', 15)) * math.pi / 180
        self.patch_dist = float(options.get('--patch-dist', '4.0'))
        self.outlier_dist = float(options.get('--outlier-dist', '0.20'))
        self.odom_w = float(options.get('--odom-w', '0.50'))
        self.no_scan_match = '--disable-scan-match' in options
    
        self.curr_odom_pose = None
        self.curr_odom_dist = 0.0
        self.laser_ranges = None
        self.anon_fids = []
        self.robot_fids = []

        self.incoming = []

        self.update_odom_dist = 0.0
        self.update_odom_pose = None
        self.front_time = 0.0
        
        self.curr_patch = None
        self.curr_pose = None
        self.curr_scan = None
        self.curr_npatches = []

        # Scan fitting diagnostics
        self.fit_err = 0
        self.fit_steps = 0
        self.fit_consts = 0

        # TESTING
        self.pairs = []

        if self.root_fig:
            self.robot_fig = rtk3.Fig(self.root_fig)
            self.nhood_fig = rtk3.Fig(self.root_fig)
            self.robot_fig.connect(self.on_robot_press, None)
        return


    def get_location(self):
        """Get the current robot pose."""

        patch = self.curr_patch
        if patch == None:
            return (None, None)

        # Offset pose by the accumulated odometry pose
        diff = geom.coord_sub(self.curr_odom_pose, self.update_odom_pose)
        pose = geom.coord_add(diff, self.curr_pose)

        return (patch, pose)
    

    def update_odom(self, odom):
        """Process new odometry data."""

        ws = 0.38
        odom_pose = odom.odom_pose

        if self.curr_odom_pose == None:
            diff = (0, 0, 0)
            dist = 0
        else:
            diff = geom.coord_sub(odom_pose, self.curr_odom_pose)
            l = diff[0] - diff[2] * ws / 2
            r = diff[0] + diff[2] * ws / 2
            dist = (abs(l) + abs(r)) / 2

        # Sanity check for bad odometry
        if dist > 0.50:
            print '*** bad odometry ***'
            print dist
            print '%.3f %.3f %.3f' % self.curr_odom_pose
            print '%.3f %.3f %.3f' % odom_pose

        self.odom_time = odom.datatime
        self.curr_odom_pose = odom_pose
        self.curr_odom_dist += dist
        return


    def update_laser(self, laser):
        """Process new laser data."""

        self.laser_ranges = laser.laser_ranges
        return
    

    def update_sync(self, sync):
        """Process sync packets."""

        if self.curr_odom_pose == None:
            return
        if self.laser_ranges == None:
            return

        # Queue up data to be processed
        self.incoming += [(self.curr_odom_dist, self.curr_odom_pose, self.laser_ranges)]
        if len(self.incoming) < 3:
            return

        odom_pose_a = self.incoming[0][1]
        odom_pose_b = self.incoming[1][1]
        odom_pose_c = self.incoming[2][1]
        
        dba = geom.coord_sub(odom_pose_b, odom_pose_a)
        dcb = geom.coord_sub(odom_pose_c, odom_pose_b)

        # Dont process fast turns
        #if abs(dba[2]) > 2 * math.pi / 180 or abs(dcb[2]) > 2 * math.pi / 180:
        #    print 'discarding (fast turn)'
        #    self.incoming.pop(0)
        #    return

        # Get the message that is second in the queue (throw the first away)
        (odom_dist, odom_pose, laser_ranges) = self.incoming[1]
        self.incoming.pop(0)

        update = 0

        # Update if we have no patches yet
        update = update or (self.curr_patch == None)

        # Update if we have accumulated some distance on the wheels
        update = update or (odom_dist - self.update_odom_dist > self.update_dist)

        # Update if we have changed odometric pose
        if self.update_odom_pose != None:
            diff = geom.coord_sub(odom_pose, self.update_odom_pose)
            dr = math.sqrt(diff[0] * diff[0] + diff[1] * diff[1])
            da = abs(diff[2])
            update = update or (dr > self.update_dist)
            update = update or (da > self.update_angle)

        if not update:
            return

        # Generate a scan from the laser data
        self.curr_scan = scan.Scan()
        self.curr_scan.add_ranges(self.laser_pose, laser_ranges)

        # Update the pose and the map
        self.update_pose(odom_dist, odom_pose, self.curr_scan, laser_ranges)

        # Draw the current state
        self.draw_state()

        # Something has changed
        return 1
    

    def update_pose(self, odom_dist, odom_pose, laser_scan, laser_ranges):
        """Update the robot pose."""

        if self.update_odom_pose == None:
            
            patch = None
            island = None
            pose = (0, 0, 0)
            npatches = []

        else:
        
            # Update the pose estimate using the odometry readings
            patch = self.curr_patch
            island = patch.island

            # Use odometry to update pose estimate
            pose = geom.coord_add(self.curr_pose, patch.pose)
            diff = geom.coord_sub(odom_pose, self.update_odom_pose)
            pose = geom.coord_add(diff, pose)

            # Find the set of overlapping patches
            npatches = self.find_overlapping(patch, pose, laser_scan)
            assert(patch in npatches)

            # Fit the scan
            if not self.no_scan_match:
                old_err = 1e6
                base_pose = pose
                for i in range(5):
                    (pose, err, consts) = self.fit(npatches, base_pose, pose, laser_scan)
                    if abs(err - old_err) / (old_err + 1e-16) < 1e-3:  # Magic
                        break
                    old_err = err

                print 'scan fit %d %d %.6f %.6f' % \
                      (i, consts, err, abs(err - old_err) / (old_err + 1e-16))

                self.fit_err = err
                self.fit_steps = i
                self.fit_consts = consts

        # See how far we are from the patch origin
        if patch == None:
            dist = 1e6
        else:
            dist = geom.coord_sub(pose, patch.pose)

            # HACK
            if abs(geom.normalize(dist[2])) > 45 * math.pi / 180:
                dist = 1e6
            else:
                dist = math.sqrt(dist[0] * dist[0] + dist[1] * dist[1])

                
        # Test if any patch in the neighborhood matches the criteria,
        # only if no patches match should a new patch be created.
        if dist > self.patch_dist:

            # Create a new patch in the map
            patch = self.map.create_patch(island)
            patch.pose = pose
            patch.odom_pose = odom_pose
            patch.odom_dist = odom_dist

            # Add this new patch to the neighborhood
            npatches.append(patch)

            # Update the scan
            rpose = geom.coord_sub(pose, patch.pose)
            rpose = geom.coord_add(self.laser_pose, rpose)
            patch.scan.add_ranges(rpose, laser_ranges)

        # Generate any missing links for the neighborhood
        for npatch in npatches:
            if npatch == patch:
                continue
            if npatch not in self.map.find_connected(patch):
                link = self.map.create_link(npatch, patch)
                link.scan_w = 1.0
                link.scan_pose_ba = geom.coord_sub(link.patch_b.pose, link.patch_a.pose)
                link.scan_pose_ab = geom.coord_sub(link.patch_a.pose, link.patch_b.pose)
                link.draw()

        # Remember the raw range scans
        rpose = geom.coord_sub(pose, patch.pose)
        rpose = geom.coord_add(self.laser_pose, rpose)
        patch.ranges.append((rpose, laser_ranges))

        # Redraw patch
        patch.draw()

        # Find the closest patch
        min_dist = 1e16
        min_patch = None
        for npatch in npatches:        
            diff = geom.coord_sub(npatch.pose, pose)
            dist = math.sqrt(diff[0] * diff[0] + diff[1] * diff[1])
            if dist < min_dist:
                min_dist = dist
                min_patch = npatch
        assert(min_patch != None)

        # Use closest patch as the current patch
        # and find the new set of overlapping patches
        if min_patch != patch:
            patch = min_patch
            npatches = self.find_overlapping(patch, pose, laser_scan)
            assert(patch in npatches)

        self.curr_patch = patch
        self.curr_pose = geom.coord_sub(pose, patch.pose)
        self.curr_npatches = npatches
        self.update_odom_dist = odom_dist
        self.update_odom_pose = odom_pose
        return


    def on_robot_press(self, fig, event, pos, dummy):
        """Process mouse events for the robot figure."""

        if event == rtk3.EVENT_RELEASE:

            pose = self.robot_fig.get_origin()

            npatches = self.curr_npatches
            laser_scan = self.curr_scan
            base_pose = pose

            old_err = 1e6
            for i in range(10):
                (pose, err, consts) = self.fit(npatches, base_pose, pose, laser_scan)
                if abs(old_err - err) < 1e-6:
                    break
                old_err = err

            print 'scan fit %d %d %.6f %.6f' % \
                  (i, consts, err, abs(err - old_err) / (old_err + 1e-16))

            self.curr_pose = geom.coord_sub(pose, self.curr_patch.pose)
            self.draw_state()

        return


    def find_overlapping(self, patch, pose, scan_):
        """Starting from the given patch, find the contiguous set of
        patches that overlap the given scan."""

        outlier = 1e16
        
        patches = [patch]
        opatches = [(patch, 0.0)]
        cpatches = [patch]

        while len(opatches) > 0:
            
            (npatch, ndist) = opatches.pop(0)

            # Get all the patches connected to this patch and check if
            # they overlap
            for link in self.map.get_patch_links(npatch):

                if link.patch_a == npatch:
                    mpatch = link.patch_b
                elif link.patch_b == npatch:
                    mpatch = link.patch_a
                else:
                    assert(0)

                # If this patch is already in the closed list, skip it
                if mpatch in cpatches:
                    continue

                # Compute the overlap area
                match = scan.ScanMatch(mpatch.scan, scan_)
                pairs = match.pairs(mpatch.pose, pose, outlier)

                #diff = geom.coord_sub(mpatch.pose, npatch.pose)
                #dist = math.sqrt(diff[0] * diff[0] + diff[1] * diff[1])
                #mdist = ndist + dist
                mdist = 0

                # If this patch overlaps, include it in the overlap list
                # and add its connected patches to the open list
                # Magic numbers
                if len(pairs) >= 4:
                    patches += [mpatch]
                    cpatches += [mpatch]
                    opatches += [(mpatch, mdist)]
                else:
                    cpatches += [mpatch]

        return patches


    def fit(self, npatches, base_pose, init_pose, scan_):
        """Improve fit for the given scan."""

        rgraph = relax.Relax()

        # Create relax patch for the scan
        rnode = relax.Node(rgraph)
        rnode.pose = init_pose
        rnode.free = 1

        # Create relax nodes for the connected patches
        for npatch in npatches:
            npatch.rnode = relax.Node(rgraph)
            npatch.rnode.pose = npatch.pose
            npatch.rnode.free = 0

        scan_w = 0.0
        rlinks = []

        # TESTING
        self.pairs = []
        
        # Create all the links
        for npatch in npatches:

            match = scan.ScanMatch(scan_, npatch.scan)                
            pairs = match.pairs(init_pose, npatch.pose, self.outlier_dist)

            for pair in pairs:
                rlink = relax.Link(rgraph, rnode, npatch.rnode)
                rlink.type = (pair[0],)
                rlink.w = (pair[1],) # Magic
                scan_w += pair[1]
                rlink.outlier = (self.outlier_dist,)
                rlink.pa = pair[2]
                rlink.pb = pair[3]
                rlink.la = pair[4]
                rlink.lb = pair[5]
                rlinks += [rlink]

        consts = len(rlinks)

        # Create a patch for the global pose
        base_rnode = relax.Node(rgraph)
        base_rnode.pose = base_pose
        base_rnode.free = 0

        # Create a link for the base pose estimate
        rlink = relax.Link(rgraph, base_rnode, rnode)
        rlink.type = (0,)
        rlink.w = (self.odom_w * scan_w,)
        rlink.outlier = (1e6,)
        rlink.pa = (0, 0)
        rlink.pb = (0, 0)
        rlinks += [rlink]

        # Relax the graph
        #err = rgraph.relax_ls(100, 1e-3, 0) # Magic
        #err = rgraph.relax_nl(100, 1e-3, 1e-4, 1e-4)
        err = self.fit_relax(rgraph)
        #print err

        # Read the relaxed pose
        pose = rnode.pose

        # Record the number of constraints
        #consts = len(rlinks)

        # Delete temp variables
        for npatch in npatches:
            del npatch.rnode

        return (pose, err, consts)


    def fit_relax(self, graph):
        """Dummy function for profiling  the relaxation part."""

        return graph.relax_nl(100, 1e-3, 1e-3, 1e-4)


    def draw_state(self):
        """Draw the current state"""

        if not self.root_fig:
            return

        pose = geom.coord_add(self.curr_pose, self.curr_patch.pose)

        self.robot_fig.clear()
        self.robot_fig.origin(pose)

        #self.robot_fig.text((0.50, 0.50),
        #'fit: %d %d %f' % (self.fit_consts, self.fit_steps, self.fit_err))

        # Draw the free space polygon
        self.robot_fig.fgcolor((255, 255, 255, 128))
        self.robot_fig.bgcolor((255, 255, 255, 128))
        self.robot_fig.polygon((0, 0, 0), self.curr_scan.get_free())

        # Draw the hit points
        self.robot_fig.fgcolor((0, 0, 0, 255))
        for (s, w) in self.curr_scan.get_hits():
            if w > 0:
                self.robot_fig.circle(s, 0.05)

        # Draw the robot body
        self.robot_fig.fgcolor((172, 0, 0, 255))
        self.robot_fig.bgcolor((172, 0, 0, 255))
        self.robot_fig.rectangle((-0.10, 0, 0), (0.52, 0.38))
        self.robot_fig.polyline([(-0.10, 0), (+0.10, 0)])
        self.robot_fig.polyline([(0, -0.10), (0, +0.10)])        

        self.robot_fig.elevate(1000)

        # Draw the neighborhood
        self.nhood_fig.clear()
        self.nhood_fig.fgcolor((255, 0, 0, 255))
        self.nhood_fig.bgcolor((255, 0, 0, 128))
        for patch in self.curr_npatches:
            self.nhood_fig.circle(patch.pose[:2], 0.40)

        return
