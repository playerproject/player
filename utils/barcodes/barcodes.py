#!/usr/bin/env python

# *  Player - One Hell of a Robot Server
# *  Copyright (C) 2004
# *     Andrew Howard
# *                      
# *
# *  This program is free software; you can redistribute it and/or modify
# *  it under the terms of the GNU General Public License as published by
# *  the Free Software Foundation; either version 2 of the License, or
# *  (at your option) any later version.
# *
# *  This program is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# *  GNU General Public License for more details.
# *
# *  You should have received a copy of the GNU General Public License
# *  along with this program; if not, write to the Free Software
# *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

# Desc: Simple barcode printer
# Author: Andrew Howard
# Date: 11 Jan 2004
# CVS: $Id$

import os
import string
import sys



def mangle_xfig(filename, id, bitmask):
    """Mangle an xfig file to produce the correct bitmask."""

    outfilename = 'mangled.fig'
    infile = open(filename, 'r')
    outfile = open(outfilename, 'w+')

    while 1:
        line = infile.readline()
        if not line:
            break

        tokens = string.split(line)

        # Discard certain layers

        # Boxes
        if tokens[0] == '2':
            layer = int(tokens[6])
            if layer >= 0 and layer < 8 and bitmask[layer] == 0:
                infile.readline()
                continue

        # Text
        elif tokens[0] == '4':
            line = string.replace(line, '$ID$', '%d' % id)

        outfile.write(line)

    return outfilename





def main(options):
    """Print barcodes using template file."""

    # These are the UPC bit masks for digits.
    # http://www.ee.washington.edu/conselec/Sp96/projects/ajohnson/proposal/project.htm
    bitmasks = {
        0 : (0,0,0,1,1,0,1),
        1 : (0,0,1,1,0,0,1),
        2 : (0,0,1,0,0,1,1),
        3 : (0,1,1,1,1,0,1),
        4 : (0,1,0,0,0,1,1),
        5 : (0,1,1,0,0,0,1),
        6 : (0,1,0,1,1,1,1),
        7 : (0,1,1,1,0,1,1),
        8 : (0,1,1,0,1,1,1),
        9 : (0,0,0,1,0,1,1),
        }

    template = options['template']

    for id in bitmasks.keys():
        outfile = mangle_xfig(template, id, bitmasks[id])
        os.system('fig2dev -L ps %s bc-%s.ps' % (outfile, id))
        os.system('ps2pdf bc-%s.ps' % (id))

    return






if __name__ == '__main__':

    options = {}
    options['template'] = sys.argv[1] #'upc-4-4-0.fig'

    main(options)
