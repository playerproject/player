#!/usr/bin/env python

# Desc: Map maker for SDR; file reader for raw log files
# Author: Andrew Howard
# Date: 29 Mar 2003
# CVS: $Id$

import gzip
import string
import sys
import cPickle


class LogFileEntry:
    """Polymorphic data class (a dummy class, really)"""

    def __init__(self):
        return



class LogFile:
    """Read text log files."""

    def __init__(self, filename):

        if filename[-3:] == '.gz':
            self.file = gzip.open(filename, 'r')
        else:
            self.file = open(filename, 'r')
        self.format = 'pyplayerc'
        return
    

    def read_entry(self):
        """Read one line from the file."""

        while 1:

            line = self.file.readline()
            if not line:
                return None

            # Tokenize the string; skip blank lines
            tokens = string.split(line)
            if len(tokens) == 0:
                continue

            # Skip comments
            if tokens[0] == '#':
                continue

            # Check for meta-data
            if tokens[0] == '##':
                if len(tokens) == 4 and tokens[3] == '0.0.0':
                    self.format = 'writelog'
                continue

            if len(tokens) < 4:
                print 'bad line:', line
                continue
            if line[-1] != '\n':
                print 'incomplete line:', line
                continue

            ctime = float(tokens[0])
            rid = tokens[1]
            iface = tokens[3]

            # Ignore sync entries
            if iface == 'sync':

                data = LogFileEntry()
                data.ctime = ctime
                data.rid = rid
                data.iface = iface

                return data

            # Process odometry
            elif iface == 'position':

                index = int(tokens[4])
                datatime = float(tokens[5])

                data = LogFileEntry()
                data.ctime = ctime
                data.rid = rid
                data.iface = iface
                data.index = index
                data.datatime = datatime
                data.odom_pose = (float(tokens[6]), float(tokens[7]), float(tokens[8]))

                # TESTING
                data.odom_pose = (data.odom_pose[1], data.odom_pose[0], data.odom_pose[2])

                return data

            # Process odometry
            elif iface == 'position3d':

                index = int(tokens[4])
                datatime = float(tokens[5])

                data = LogFileEntry()
                data.ctime = ctime
                data.rid = rid
                data.iface = iface
                data.index = index
                data.datatime = datatime
                data.odom_pose = (float(tokens[6]), float(tokens[7]), float(tokens[11]))

                return data

            # Process laser
            elif iface == 'laser':

                index = int(tokens[4])
                datatime = float(tokens[5])

                if index == 1:
                    continue

                if self.format == 'pyplayerc':
                    ranges = []
                    for i in range(6, len(tokens), 3):
                        r = float(tokens[i + 0])
                        b = float(tokens[i + 1])
                        ranges += [(r, b)]

                elif self.format == 'writelog':
                    min = float(tokens[6])
                    max = float(tokens[7])
                    res = float(tokens[8])
                    ranges = []
                    for i in range(10, len(tokens), 2):
                        r = float(tokens[i + 0])
                        b = min + res * ((i - 10) / 2)
                        ranges += [(r, b)]

                data = LogFileEntry()
                data.ctime = ctime
                data.rid = rid
                data.iface = iface
                data.index = index
                data.datatime = datatime
                data.laser_ranges = ranges

                return data

            # Process fiducials
            elif iface == 'fiducial':

                index = int(tokens[4])
                datatime = float(tokens[5])

                fids = []
                for i in range(6, len(tokens), 4):
                    id = int(tokens[i + 0])
                    r = float(tokens[i + 1])
                    b = float(tokens[i + 2])
                    fids += [(id, r, b)]

                data = LogFileEntry()
                data.ctime = ctime
                data.rid = rid
                data.iface = iface
                data.index = index
                data.datatime = datatime
                data.fids = fids

                return data

            # Process old-style fiducials
            elif iface == 'robot':

                index = int(tokens[4])
                datatime = float(tokens[5])

                fids = []
                for i in range(6, len(tokens), 5):
                    id = tokens[i + 0]
                    r = float(tokens[i + 1])
                    b = float(tokens[i + 2])
                    fids += [(id, r, b)]

                data = LogFileEntry()
                data.ctime = ctime
                data.rid = rid
                data.iface = 'mutual'
                data.index = index
                data.datatime = datatime
                data.fids = fids

                return data
            

##             # Process scans
##             elif iface == 'scan':

##                 index = int(tokens[4])
##                 datatime = float(tokens[5])

##                 pose = (float(tokens[6]), float(tokens[7]), float(tokens[8]))
                
##                 ranges = []
##                 for i in range(9, len(tokens), 2):
##                     r = float(tokens[i + 0])
##                     b = float(tokens[i + 1])
##                     ranges += [(r, b)]

##                 data = LogFileEntry()
##                 data.ctime = ctime
##                 data.rid = rid
##                 data.iface = iface
##                 data.index = index
##                 data.datatime = datatime
##                 data.odom_pose = pose
##                 data.laser_ranges = ranges

##                 return data

        return None

