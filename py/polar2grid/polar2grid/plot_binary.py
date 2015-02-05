#!/usr/bin/env python
# encoding: utf-8
"""Simple script to plot flat binary files
onto a png file using matplotlib.

:author:       David Hoese (davidh)
:contact:      david.hoese@ssec.wisc.edu
:organization: Space Science and Engineering Center (SSEC)
:copyright:    Copyright (c) 2013 University of Wisconsin SSEC. All rights reserved.
:date:         Jan 2013
:license:      GNU GPLv3

Copyright (C) 2013 Space Science and Engineering Center (SSEC),
 University of Wisconsin-Madison.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

This file is part of the polar2grid software package. Polar2grid takes
satellite observation data, remaps it, and writes it to a file format for
input into another program.
Documentation: http://www.ssec.wisc.edu/software/polar2grid/

    Written by David Hoese    January 2013
    University of Wisconsin-Madison 
    Space Science and Engineering Center
    1225 West Dayton Street
    Madison, WI  53706
    david.hoese@ssec.wisc.edu

"""
__docformat__ = "restructuredtext en"

from glob import glob
import numpy
import matplotlib
matplotlib.use('agg')
from matplotlib import pyplot as plt
from polar2grid.core import Workspace

import os

DEFAULT_FILE_PATTERN = "result_*.real4.*.*"
DEFAULT_FILL_VALUE   = -999.0
DEFAULT_DPI          = 150

def plot_binary(bf, workspace='.',
                fill_value=DEFAULT_FILL_VALUE,
                dpi_to_use=DEFAULT_DPI,
                vmin=None, vmax=None):
    W=Workspace(workspace)

    plt.figure()
    fbf_attr = bf.split(".")[0]
    result = getattr(W, fbf_attr)
    result = numpy.ma.masked_where(result == fill_value, result)
    print result.min(),result.max()
    plt.imshow(result, vmin=vmin, vmax=vmax)
    plt.bone()
    plt.colorbar()
    plt.savefig("plot_binary.%s.png" % fbf_attr, dpi=dpi_to_use)
    plt.close()

def sci_float(x):
    x = x.replace("\"", "")
    x = x.replace("\'", "")
    return float(str(x))

def main():
    from argparse import ArgumentParser
    description = """
Plot binary files using matplotlib.
    """
    parser = ArgumentParser(description=description)
    parser.add_argument("-f", dest="fill_value", default=DEFAULT_FILL_VALUE, type=sci_float,
            help="Specify the fill_value of the input file(s)")
    parser.add_argument('--vmin', dest="vmin", default=None, type=float,
            help="Specify minimum brightness value. Defaults to minimum value of data.")
    parser.add_argument('--vmax', dest="vmax", default=None, type=float,
            help="Specify maximum brightness value. Defaults to maximum value of data.")
    parser.add_argument("-p", dest="pattern",
            help="filename pattern to search the current directory for")
    parser.add_argument("binary_files", nargs="*",
            help="list of flat binary files to be plotted in the current directory")
    parser.add_argument('-d', '--dpi', dest="dpi", default=DEFAULT_DPI, type=float,
            help="Specify the dpi for the resulting figure, higher dpi will result in larger figures and longer run times")
    args = parser.parse_args()

    workspace = '.'
    binary_files = args.binary_files
    if not args.binary_files and not args.pattern:
        args.pattern = DEFAULT_FILE_PATTERN
    if args.pattern:
        workspace = os.path.split(args.pattern)[0]
        binary_files = [ os.path.split(x)[1] for x in glob(args.pattern) ]

    for bf in binary_files:
        print "Plotting '%s'" % (bf,)
        try:
            plot_binary(bf, workspace='.',
                        fill_value=args.fill_value,
                        dpi_to_use=args.dpi,
                        vmin=args.vmin, vmax=args.vmax)
        except StandardError as e:
            print "Could not plot '%s'" % (bf,)
            if hasattr(e, "msg"): print e,e.msg
            else: print e

if __name__ == "__main__":
    import sys

    sys.exit(main())

