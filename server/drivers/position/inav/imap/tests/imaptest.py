#!/usr/bin/env python

# Desc: Local map test program
# Date: 22 Jan 2003
# Author: Andrew H
# CVS: $Id$


import getopt
import math
import profile
import pstats
import random
import string
import sys

import rtk
import imap

import geom



class MapTest:
    """Local map test."""

    def __init__(self, fit, scale, noise, canvas):

        # Enable fitting
        self.fit = fit

        # Map scale
        self.map_scale = scale

        # Extra noise
        self.noise = noise

        # Pose of laser in robot cs
        self.laser_pose = (0.15, 0, 0)

        # Create the local map
        self.map = imap.imap(16.0, 16.0, self.map_scale, 0.25, 0.25)

        # Poses
        self.odom_pose = None
        self.map_pose = None
        self.est_pose = None

        self.draw_map_time = 0

        # Create figures
        self.canvas = canvas
        self.map_fig = rtk.fig(self.canvas, None, -1)
        self.robot_fig = rtk.fig(self.canvas, None, 50)
        return


    def update_map(self, datatime, odom_pose, laser_ranges):
        """Update the map with pose/range scan."""

        # Add in some noise to the odometry
        #odom_pose = (odom_pose[0] + random.normalvariate(0, +self.noise[0]), 
        #             odom_pose[1] + random.normalvariate(0, +self.noise[1]),
        #             odom_pose[2] + random.normalvariate(0, +self.noise[2]))

        # If this is the first update, initialize the estimates
        if self.odom_pose == None:
            self.odom_pose = odom_pose
            self.map_pose = (0, 0, 0)
            self.est_pose = (0, 0, 0)
            return

        # Update the estimated pose
        diff = geom.gtol(odom_pose, self.odom_pose)
        self.est_pose = geom.ltog(diff, self.est_pose)
        self.odom_pose = odom_pose

        # The laser pose
        pose = geom.ltog(self.laser_pose, self.est_pose)

        # Add in some noise to the initial pose
        pose = (pose[0] + random.normalvariate(0, self.noise[0]), 
                pose[1] + random.normalvariate(0, self.noise[1]),
                pose[2] + random.normalvariate(0, self.noise[2]))

        # Fit the range scan to the current map,
        # and compute the new pose estimate induced by the laser fit.
        if self.fit:
            (pose, err) = self.map.fit_ranges(pose, laser_ranges)
            if err >= 0:
                self.est_pose = geom.ltog(geom.gtol((0, 0, 0), self.laser_pose), pose)

        # Update the map
        self.map.add_ranges(pose, laser_ranges)            

        # Translate the map if we have moved away from the center
        diff = geom.gtol(self.est_pose, self.map_pose)
        (di, dj) = (int(diff[0] / self.map_scale), int(diff[1] / self.map_scale))
        if abs(di) > 2 or abs(dj) > 2:
            self.map.translate(di, dj)
            self.map_pose = (self.map_pose[0] + di * self.map_scale,
                             self.map_pose[1] + dj * self.map_scale,
                             self.map_pose[2])

        # Draw the local map
        if datatime - self.draw_map_time > 5.00:
            self.draw_map_time = datatime
            self.draw_map()

        # Draw the robot
        self.draw_robot()
        return


    def draw_map(self):
        """Draw the map."""

        self.map_fig.clear()
        self.map.draw(self.map_fig)
        return


    def draw_robot(self):
        """Draw the robot."""
        
        self.robot_fig.clear()
        self.robot_fig.color(0.7, 0, 0)
        pose = self.est_pose
        self.robot_fig.origin(pose[0], pose[1], pose[2])
        self.robot_fig.rectangle(-0.1, 0, 0, 0.40, 0.30, 0)
        return


def main(logfile, fit, scale, noise, export):
    """Process the given log file."""

    app = rtk.app()
    canvas = rtk.canvas(app)
    canvas.size(400, 400)
    canvas.scale(0.05, 0.05)
    app.main_init()

    map = MapTest(fit, scale, noise, canvas)

    file = open(logfile, 'r')

    skip_count = 0
    export_count = 0

    app.main_init()

    try:
        
        while 1:

            app.main_loop()

            line = file.readline()
            if not line:
                continue

            tokens = string.split(line)
            if len(tokens) < 5:
                continue

            type = tokens[3]
            index = int(tokens[4])

            if type == 'position' and index == 0:  # HACK
                datatime = float(tokens[5])
                #print datatime
                odom_pose = (float(tokens[6]),
                             float(tokens[7]),
                             float(tokens[8]))

            elif type == 'laser':

                skip_count += 1
                if skip_count % 4 != 0:
                    continue
                
                laser_ranges = []
                for i in range(6, len(tokens), 3):
                    r = float(tokens[i + 0])
                    b = float(tokens[i + 1])
                    laser_ranges.append((r, b))

                # Update the map
                map.update_map(datatime, odom_pose, laser_ranges)

                # Re-draw the canvas
                canvas.render()

                # Save the map
                if export:
                    canvas.export_image('imaptest_%04d.ppm' % export_count, rtk.IMAGE_FORMAT_PPM)
                    export_count += 1

    except KeyboardInterrupt:
        pass

    app.main_term()
    return




if __name__ == '__main__':

    (opts, args) = getopt.getopt(sys.argv[1:], 'pef:n:')

    prof = 0
    logfile = None
    fit = 1
    scale = 0.10
    noise = (0, 0, 0)
    export = 0

    for opt in opts:

        # Profile code
        if opt[0] == '-p':
            prof = 1

        # Export maps
        elif opt[0] == '-e':
            export = 1

        # Enable fitting
        elif opt[0] == '-f':
            fit = int(opt[1])
            
        elif opt[0] == '-n':
            #noise = (float(opt[1]), float(opt[1]), float(opt[1]))
            noise = (0, 0, float(opt[1]))

    logfile = args[0]

    if not prof:
        main(logfile, fit, scale, noise, export)
    else:
        profile.run('main(logfile, fit, scale, noise, export)', '.profile')
        p = pstats.Stats('.profile')
        p.sort_stats('time').print_stats()
