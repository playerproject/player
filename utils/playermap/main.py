#!/usr/bin/env python
# Desc: Generate maps
# Author: Andrew Howard
# Date: 14 Jun 2003

import time
import sys, os, getopt
import math
import random
import gtk

#sys.path += ['cmodules']
#print sys.path

import gui
import geom
from logfile import LogFile, LogFileEntry
from map import Map
from robot_mapper import RobotMapper
from team_mapper import TeamMapper
from geo_mapper import GeoMapper

class Robot:
    """Combined info for a single robot."""

    def __init__(self, map, map_fig, options):

        self.map = map
        self.mapper = RobotMapper(map, map_fig, options)
        return


    def update(self, data):
        """Process new data from this robot."""

        if data.iface == 'position' and data.index == 0:
            self.mapper.update_odom(data)
        elif data.iface == 'laser':
            self.mapper.update_laser(data)
        elif data.iface == 'sync':
            self.mapper.update_sync(data)
        return

    



def makemap(gladepath, logfilename, options):

    wallstart = None
    walltime = 0
    logstart = None
    logtime = 0
    savemap = 0
    saveframe = 0
    savetime = 0

    if '--nogui' not in options.keys():
        mainwin = gui.MainWin(gladepath)
        mainwin.show()
        maproot = mainwin.root_fig
    else:
        mainwin = None
        maproot = None

    # Open the log file
    logfile = LogFile(logfilename)
        
    # Construct the map
    map = Map(maproot)

    # Construct proxy for each robot
    robot = Robot(map, maproot, options)

    # Construct proxy for the team
    team_mapper = TeamMapper(map, maproot)

    # Geo mapping
    geo_mapper = GeoMapper(map, maproot)
    pin_file = options.get('--pin-file', None)
    if pin_file:
        geo_mapper.load_pins(pin_file)
    
    try:
        
        while not gui.quit:

            if mainwin:

                # Process gui events
                while gtk.events_pending():
                    gtk.main_iteration_do()
                gui_time = time.time()

##                 # Save canvas
##                 if save_request and logtime - savetime > 0.400:
##                     filename = 'map-%05d.png' % saveframe
##                     #print 'saving', filename
##                     #mainwin.canvas.save(filename)
##                     save_request = 0
##                     saveframe += 1
##                     savetime = logtime

                # Rotate the map
                if mainwin.rotate:
                    a = mainwin.rotate * 5 * math.pi / 180
                    for patch in map.patches:
                        patch.pose = geom.coord_add(patch.pose, (0, 0, a))
                    for patch in map.patches:
                        patch.draw()
                    for link in map.links:
                        link.draw()
                    robot.mapper.draw_state()
                    mainwin.rotate = 0

                # Infer links
                if mainwin.infer_links:
                    team_mapper.infer_links()
                    for patch in map.patches:
                        patch.draw()
                    for link in map.links:
                        link.draw()
                    mainwin.infer_links = 0

                # Do a strong fit
                if mainwin.fit_strong:
                    while time.time() - gui_time < 0.050:
                        team_mapper.match_all()
                        team_mapper.fit_all()                
                    for patch in map.patches:
                        patch.draw()
                    for link in map.links:
                        link.draw()

                # Generate occupancy grid
                if mainwin.make_grid:
                    team_mapper.export_grid('map-%02d.pgm' % savemap)
                    savemap += 1
                    mainwin.make_grid = 0

                # Pause processing
                if mainwin.pause:
                    time.sleep(0.050)
                    continue

            # Process log data in batches (saves a lot of time)
            while 1:
                # Process log data
                data = logfile.read_entry()
                if not data:
                    time.sleep(0.001)
                    break

                # Process new data from robot
                robot.update(data)

                # Do global updates
                if data.iface == 'sync':
                    if team_mapper.update():
                        if mainwin:
                            #mainwin.pause_button.clicked()
                            for patch in map.patches:
                                patch.draw()
                            for link in map.links:
                                link.draw()

                if not mainwin or time.time() - gui_time > 0.100:
                    break

            if data == None:
                continue

            # Monitor time
            logtime = data.ctime
            if logstart == None:
                logstart = logtime
            walltime = time.time()
            if wallstart == None:
                wallstart = walltime

            #if (logtime - logstart) % 5.00 < 0.05:
            #    print '%f %f : %f' % (walltime - wallstart,
            #                          logtime - logstart,
            #                          (logtime - logstart) / (walltime - wallstart + 1e-16))

            #if logtime - logstart > 1000:
            #    break



    except KeyboardInterrupt:
        pass

    print 'Processed %.3f sec (log) in %.3f sec (real)' % \
          (logtime - logstart, walltime - wallstart)

    return



def main(gladepath):

    logfile = None
    options = {}

    # The -p switch is for profiling; it is caught at a higher level
    (opts, args) = getopt.getopt(sys.argv[1:], 'p',
                                 ['nogui',
                                  'disable-scan-match',
                                  'update-dist=',
                                  'update-angle=',
                                  'patch-dist=',
                                  'outlier-dist=',
                                  'odom-w=',
                                  'pin-file='])
    
    for opt in opts:
        if opt[0] == '-p':
            pass
        elif len(opt) == 1:
            options[opt] = '1'
        else:
            options[opt[0]] = opt[1]

    # Check for minimal arg list
    if len(args) != 1:
        sys.exit(-1)

    logfile = args[0]

    makemap(gladepath, logfile, options)
        




if __name__ == '__main__':

    main('.')
    
