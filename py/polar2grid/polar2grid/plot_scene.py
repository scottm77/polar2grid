#!/usr/bin/env python
# encoding: utf-8
# Copyright (C) 2014 Space Science and Engineering Center (SSEC),
# University of Wisconsin-Madison.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# This file is part of the polar2grid software package. Polar2grid takes
# satellite observation data, remaps it, and writes it to a file format for
#     input into another program.
# Documentation: http://www.ssec.wisc.edu/software/polar2grid/
#
# Written by David Hoese    October 2014
# University of Wisconsin-Madison
# Space Science and Engineering Center
# 1225 West Dayton Street
# Madison, WI  53706
# david.hoese@ssec.wisc.edu
"""Simple script to plot P2G JSON objects and save them as png files using matplotlib.

:author:       David Hoese (davidh)
:contact:      david.hoese@ssec.wisc.edu
:organization: Space Science and Engineering Center (SSEC)
:copyright:    Copyright (c) 2014 University of Wisconsin SSEC. All rights reserved.
:date:         Oct 2014
:license:      GNU GPLv3

"""
__docformat__ = "restructuredtext en"

import os
import numpy
import matplotlib
matplotlib.use('agg')
from matplotlib import pyplot as plt

from polar2grid.core.containers import BaseP2GObject, BaseProduct, BaseScene

DEFAULT_FILL_VALUE   = numpy.nan
DEFAULT_DPI          = 150


def plot_binary(arr, output_fn, dpi_to_use=DEFAULT_DPI, vmin=None, vmax=None):
    plt.figure()
    print("Minimum: %f | Maximum: %f" % (numpy.nanmin(arr), numpy.nanmax(arr)))
    plt.imshow(arr, vmin=vmin, vmax=vmax)
    plt.bone()
    plt.colorbar()
    plt.savefig(output_fn, dpi=dpi_to_use)
    plt.close()


def _parse_binary_info(parts):
    from polar2grid.core.dtype import str_to_dtype
    fn = parts[0]
    if not os.path.exists(fn):
        raise ValueError("File '%s' does not exist" % (fn,))

    dtype = str_to_dtype(parts[1])
    rows = int(parts[2])
    cols = int(parts[3])
    fill = float(parts[4])
    arr = numpy.memmap(fn, dtype=dtype, mode='r', shape=(rows, cols))
    return (fn, numpy.ma.masked_array(arr, numpy.isnan(arr) | (arr == fill)))


def main():
    from argparse import ArgumentParser
    description = """
Plot binary files using matplotlib.
    """
    parser = ArgumentParser(description=description)
    parser.add_argument('--vmin', dest="vmin", default=None, type=float,
            help="Specify minimum brightness value. Defaults to minimum value of data.")
    parser.add_argument('--vmax', dest="vmax", default=None, type=float,
            help="Specify maximum brightness value. Defaults to maximum value of data.")
    parser.add_argument("scene_files", nargs="*",
            help="list of scene json files to be plotted in the current directory")
    parser.add_argument("--binary", nargs=5, dest="binary_array",
                        help="Specify a binary file and its information instead of a scene (<fn> <dtype> <rows> <cols> <fill value>")
    parser.add_argument('-d', '--dpi', dest="dpi", default=DEFAULT_DPI, type=float,
            help="Specify the dpi for the resulting figure, higher dpi will result in larger figures and longer run times")
    parser.add_argument("-p", "--products", dest="products", nargs="*", default=None,
                        help="Specify frontend products to process")
    args = parser.parse_args()
    args.binary_array = _parse_binary_info(args.binary_array) if args.binary_array is not None else None

    if args.binary_array is not None:
        print("Plotting binary file instead of scene...")
        fn, arr = args.binary_array
        output_fn = "plot_product.%s.png" % (os.path.splitext(os.path.basename(fn))[0],)
        plot_binary(arr, output_fn, dpi_to_use=args.dpi, vmin=args.vmin, vmax=args.vmax)
        return

    for scene_fn in args.scene_files:
        try:
            scene = BaseP2GObject.load(scene_fn)
            print("Loaded %s" % (scene_fn,))
        except StandardError:
            print("Couldn't load object from JSON file '%s'" % (scene_fn,))
            continue

        if isinstance(scene, BaseProduct):
            products = [scene]
        elif isinstance(scene, BaseScene):
            products = scene.values()
            if args.products:
                products = [p for p in products if p["product_name"] in args.products]
        else:
            print("ERROR: Unknown object type loaded")
            continue

        for product in products:
            try:
                print("Plotting '%s'" % (product["product_name"],))
                output_fn = "plot_product.%s.png" % (product["product_name"],)
                arr = numpy.ma.masked_array(product.get_data_array(), product.get_data_mask())
                plot_binary(arr, output_fn, dpi_to_use=args.dpi, vmin=args.vmin, vmax=args.vmax)
            except StandardError as e:
                print("Could not plot '%s'" % (product["product_name"],))
                if hasattr(e, "msg"):
                    print(e, e.msg)
                else:
                    print(e)

if __name__ == "__main__":
    import sys
    sys.exit(main())
