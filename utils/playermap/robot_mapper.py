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


class FitData:
    """Data from scan fitting."""
    pass


class RobotMapper:
    """Build the map using data from a single robot."""

    def __init__(self, map, root_fig, options):

        self.map = map
        self.options = options
        self.root_fig = root_fig

        self.laser_pose = (0.05, 0, 0)
        self.update_dist = float(options.get('--update-dist', 0.20))
        self.update_angle = float(options.get('--update-angle', 10)) * math.pi / 180
        self.patch_dist = float(options.get('--patch-dist', '2.0'))
        self.patch_angle = float(options.get('--patch-angle', 45)) * math.pi / 180
        self.nhood_dist = float(options.get('--nhood-dist', '8.0'))
        self.outlier_dist = float(options.get('--outlier-dist', '0.40'))
        self.fit_steps = int(options.get('--fit-steps', 100))
        self.odom_w = float(options.get('--odom-w', '0.10'))
        self.no_scan_match = '--disable-scan-match' in options
    
        self.curr_odom_pose = None
        self.curr_odom_dist = 0.0
        self.curr_laser_ranges = None

        self.curr_patch = None
        self.curr_pose = None
        self.curr_nhood = []
        self.curr_scan = None
        self.curr_local_group = scan.scan_group()

        self.update_odom_dist = 0.0
        self.update_odom_pose = None

        if self.root_fig:
            self.robot_fig = rtk3.Fig(self.root_fig)
            self.nhood_fig = rtk3.Fig(self.root_fig)
            self.robot_fig.connect(self.on_robot_press, None)
        return
    

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

        self.curr_laser_ranges = laser.laser_ranges
        return
    

    def update_sync(self, sync):
        """Process sync packets."""

        if self.curr_odom_dist == None:
            return
        if self.curr_odom_pose == None:
            return
        if self.curr_laser_ranges == None:
            return

        # Get the new sensor data
        odom_dist = self.curr_odom_dist
        odom_pose = self.curr_odom_pose
        laser_ranges = self.curr_laser_ranges

        update = 0

        # Update if we have no patches yet
        update = update or (self.curr_patch == None)

        # Update if we have accumulated some distance on the wheels
        update = update or (odom_dist - self.update_odom_dist > self.update_dist)

        # Update if we have changed odometric pose significantly
        if self.update_odom_pose != None:
            diff = geom.coord_sub(odom_pose, self.update_odom_pose)
            dr = math.sqrt(diff[0] * diff[0] + diff[1] * diff[1])
            da = abs(diff[2])
            update = update or (dr > self.update_dist)
            update = update or (da > self.update_angle)

        if not update:
            return


        # Update the pose and the map
        self.update_pose(odom_dist, odom_pose, laser_ranges)

        # Draw the current state
        self.draw_state()

        # Something has changed
        return 1
    

    def update_pose(self, odom_dist, odom_pose, laser_ranges):
        """Update the robot pose."""

        # Generate a scan from the laser data
        laser_scan = scan.scan()
        laser_scan.add_ranges(self.laser_pose, laser_ranges)

        if self.update_odom_pose == None:

            patch = None
            island = None
            pose = (0, 0, 0)
            nhood = []

        else:
        
            # Update the pose estimate using the odometry readings
            patch = self.curr_patch
            island = patch.island

            # Use odometry to update pose estimate
            diff = geom.coord_sub(odom_pose, self.update_odom_pose)
            pose = geom.coord_add(diff, self.curr_pose)

            # Find the neighborhood
            nhood = self.find_nhood(patch, pose, laser_scan)
            assert(patch in nhood)

            # Generate local scan group
            self.curr_local_group.reset()
            for npatch in nhood:
                self.curr_local_group.add_scan(npatch.pose, npatch.scan)

            # Refine the pose estimate with scan matching
            if not self.no_scan_match:
                fit = self.fit(patch, nhood, pose, self.curr_local_group, laser_scan)
                pose = fit.pose
            else:
                fit = None

        # See if we should switch to a new patch
        min_dist = 1e16
        new_patch = None
        for npatch in nhood:

            # Compute robot pose relative to patch
            npose = geom.coord_add(pose, patch.pose)
            npose = geom.coord_sub(npose, npatch.pose)

            dist = math.sqrt(npose[0] * npose[0] + npose[1] * npose[1])
            angle = abs(geom.normalize(npose[2]))

            # Robot must lie within free space of patch or near
            # to the patch origin (for turning in place)
            if dist > 0.10 and npatch.scan.test_free(npose[:2]) < 0:
                continue

            # Robot must lie within certain distance of patch
            if dist > self.patch_dist:
                continue

            # Robot must lie within certain angle of patch
            if angle > self.patch_angle:
                continue

            # If all criteria are met, choose closest patch as switch patch
            if dist < min_dist:
                min_dist = dist
                new_patch = npatch                
                new_pose = npose

        # If no patches are valid, we need to create a brand-new patch
        if new_patch == None:

            print 'new patch'

            # Create a new patch in the map
            new_patch = self.map.create_patch(island)
            if patch == None:
                new_patch.pose = (0, 0, 0)
            else:
                new_patch.pose = geom.coord_add(pose, patch.pose)
            new_patch.odom_pose = odom_pose
            new_patch.odom_dist = odom_dist
            new_patch.scan = laser_scan
            new_patch.draw()

            # Create links to all patches in the neighborhood (we got
            # fitted to all of them)
            for npatch in nhood:
                link = self.map.create_link(new_patch, npatch)
                link.scan_w = 1.0
                link.scan_pose_ba = geom.coord_sub(link.patch_b.pose, link.patch_a.pose)
                link.scan_pose_ab = geom.coord_sub(link.patch_a.pose, link.patch_b.pose)
                link.draw()

            # Add this new patch to the neighborhood
            nhood.append(new_patch)

            # Set new pose
            new_pose = (0, 0, 0)

        # If the patch has changed, we need to recompute the neighborhood
        if new_patch != patch:

            if patch:
                print 'switch patch %d to %d' % (patch.id, new_patch.id)
                                
                # Refresh the old patch with any changes we made
                patch.draw()
                            
            patch = new_patch
            pose = new_pose

        # Remember the raw range scans for grid reconstruction
        rpose = geom.coord_add(self.laser_pose, pose)
        patch.ranges.append((rpose, laser_ranges))

        # Update state variables
        self.curr_patch = patch
        self.curr_pose = pose
        self.curr_nhood = nhood
        self.curr_scan = laser_scan
        self.update_odom_dist = odom_dist
        self.update_odom_pose = odom_pose

        # Redraw patch
        patch.draw()
        return


    def on_robot_press(self, fig, event, pos, dummy):
        """Process mouse events for the robot figure."""

        if event == rtk3.EVENT_RELEASE:

            pose = self.robot_fig.get_origin()

            nhood = self.curr_nhood
            laser_scan = self.curr_scan
            base_pose = pose

            old_err = 1e6
            for i in range(10):
                (pose, err, consts) = self.fit(nhood, base_pose, pose, laser_scan)
                if abs(old_err - err) < 1e-6:
                    break
                old_err = err

            print 'scan fit %d %d %.6f %.6f' % \
                  (i, consts, err, abs(err - old_err) / (old_err + 1e-16))

            self.curr_pose = geom.coord_sub(pose, self.curr_patch.pose)
            self.draw_state()

        return


    def find_nhood(self, patch, pose, laser_scan):
        """Determine the current neighborhood."""

        # TODO: this should constrain the nhood based on certainty of
        # fit

        outlier = 1e16
        pose = geom.coord_add(pose, patch.pose)
        patches = [patch]

        for npatch in self.map.find_nhood(patch, self.nhood_dist):

            # Generate link only if there are some points to match
            #match = scan.scan_match(scan_, npatch.scan)
            #pairs = match.pairs(pose, npatch.pose, outlier)

            # If this patch overlaps, include it in the overlap list
            # and add its connected patches to the open list
            # Magic numbers
            #if len(pairs) >= 4:

            # TESTING
            patches += [npatch]

        return patches


    def fit(self, patch, nhood, pose, local_group, laser_scan):
        """Fit the scan; may reject the initial pose."""

        # HACK
        fit_thresh = 0.05 ** 2

        # Do the full fit against the entire neighborhood        
        best_fit = self.fit_for_pose(patch, nhood, pose, local_group, laser_scan)

        # If the fit is ok...
        if best_fit.stats[2] < fit_thresh:
            return best_fit

        # TESTING
        return best_fit

        print 'bad fit - trying other angles'

        # Try fitting other initial angles
        for i in range(-45, 45, 5):  # HACK
            pa = i * math.pi / 180 
            npose = (pose[0], pose[1], pose[2] + pa)
            fit = self.fit_for_pose(patch, nhood, npose, laser_scan)

            # If this looks better initially, do the full fit for this value
            #if fit.err < best_fit.err:
            if fit.stats[2] < fit_thresh and fit.err < best_fit.err:
                best_fit = fit
        
        return best_fit


    def fit_diagnostics(self, patch, nhood, pose, local_group, laser_scan):
        """Save fitting diagnostics."""

        # Try fitting other initial angles
        for i in range(-90, 90, 1):  # HACK
            pa = i * math.pi / 180 
            npose = (pose[0], pose[1], pose[2] + pa)
            #fit = self.fit_for_pose(patch, nhood, npose, laser_scan)
            fit = self.fit_step(0, patch, nhood, pose, npose, local_group, laser_scan)

            self.fitfile.write('%f %f %f %f %f\n' %
                               (pa, fit.err,
                                fit.stats[0], fit.stats[1], fit.stats[2]))

        self.fitfile.write('\n\n')
        return


    def fit_for_pose(self, patch, nhood, pose, local_group, laser_scan):
        """Fit the scan for one initial pose."""

        steps = self.fit_steps
        last_err = 1e6
        init_pose = pose

        for i in range(20):
            fit = self.fit_step(steps, patch, nhood, init_pose, pose, local_group, laser_scan)
            if abs(fit.err - last_err) / (last_err + 1e-16) < 1e-3:  # Magic
                break
            print 'scan fit %d %d %.6f %.6f : %d %.6f %.6f' % \
                  (i, fit.steps, fit.err, abs(fit.err - last_err) / (last_err + 1e-16),
                   fit.stats[0], fit.stats[1], fit.stats[2])
            pose = fit.pose
            last_err = fit.err

            
        return fit


    def fit_step(self, steps, patch, nhood, base_pose, init_pose, local_group, laser_scan):
        """Improve fit for the given scan (one step)."""

        outlier = self.outlier_dist

        # Compute poses in the global cs
        base_pose = geom.coord_add(base_pose, patch.pose)
        init_pose = geom.coord_add(init_pose, patch.pose)
        
        graph = relax.Relax()

        # Create relax node for the scan
        scan_node = relax.Node(graph)
        scan_node.pose = init_pose
        scan_node.free = 1

        # Create relax node for the local map
        map_node = relax.Node(graph)
        map_node.pose = (0, 0, 0)
        map_node.free = 0

        # Create a scan group for the laser
        laser_group = scan.scan_group()
        laser_group.add_scan((0, 0, 0), laser_scan)

        # Find correspondance points
        match = scan.scan_match(local_group, laser_group)
        pairs = match.pairs((0, 0, 0), init_pose, self.outlier_dist)

        scan_w = 0.0
        links = []
        scan_hits = {}
        
        # Create all the links
        for pair in pairs:
            link = relax.Link(graph, map_node, scan_node)
            link.type = (pair[0],)
            link.w = (pair[2],)
            scan_w += pair[2]
            link.outlier = (outlier,)
            link.pa = pair[3]
            link.pb = pair[4]
            link.la = pair[5]
            link.lb = pair[6]
            links += [link]

            # Record which hit points got matched
            if pair[0] == 1:
                scan_hits[pair[1][0]] = 1

        # Create a link for the base pose estimate
        link = relax.Link(graph, map_node, scan_node)
        link.type = (0,)
        link.w = (self.odom_w * scan_w,)
        link.outlier = (1e6,)
        link.pa = base_pose[:2]
        link.pb = (0, 0)
        links += [link]

        # Create another link to capture the orientation
        link = relax.Link(graph, map_node, scan_node)
        link.type = (0,)
        link.w = (self.odom_w * scan_w * 0.1,)  # HACK: use different weight
        link.outlier = (1e6,)
        link.pa = geom.coord_add((1, 0), base_pose)
        link.pb = (1, 0)
        links += [link]

        # Relax the graph
        (err, steps, m) = self.fit_relax(steps, graph)
        stats = (m[0], m[1] / m[0], m[2] / m[0] - (m[1] / m[0]) ** 2)

        # Read the relaxed pose
        pose = scan_node.pose
        pose = geom.coord_sub(pose, patch.pose)

        # Return fit data
        fit = FitData()
        fit.err = err
        fit.pose = pose
        fit.steps = steps
        fit.stats = stats
        fit.scan_hits = len(scan_hits)

        return fit


    def fit_match(self, pose_a, scan_a, pose_b, scan_b):
        """Dummy function for profiling the matching part."""

        match = scan.scan_match(scan_a, scan_b)                
        pairs = match.pairs(pose_a, pose_b, self.outlier_dist)
        return pairs
        

    def fit_relax(self, steps, graph):
        """Dummy function for profiling the relaxation part."""

        return graph.relax_nl(steps, 1e-3, 1e-7, 1e-2, 1e-2)


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
        for patch in self.curr_nhood:
            self.nhood_fig.circle(patch.pose[:2], 0.40)

        # Draw the local scan group
        self.nhood_fig.bgcolor((255, 0, 0, 64))
        for c in self.curr_local_group.get_free():
            self.nhood_fig.polygon((0, 0, 0), c)
        for (s, w) in self.curr_local_group.get_hits():
            self.nhood_fig.circle((s[0], s[1]), 0.05)

        return
