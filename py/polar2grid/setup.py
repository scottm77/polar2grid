#!/usr/bin/env python
# encoding: utf-8
"""Script for installing the polar2grid package.

See http://packages.python.org/distribute/ for use details.

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
from setuptools import setup, find_packages

classifiers = ""
version = '1.2.1'

setup(
    name='polar2grid',
    version=version,
    description="Library and scripts to remap imager data to a grid",
    classifiers=filter(None, classifiers.split("\n")),
    keywords='',
    author='David Hoese, SSEC',
    author_email='david.hoese@ssec.wisc.edu',
    license='GPLv3',
    url='http://www.ssec.wisc.edu/software/polar2grid/',
    packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
    namespace_packages=["polar2grid"],
    include_package_data=True,
    package_data={'polar2grid': ["grids/*.gpd","grids/*.ncml","*.conf"]},
    zip_safe=False,
    install_requires=[
        'numpy',
        'matplotlib',
        'netCDF4',          # AWIPS backend
        'pyproj',           # Python ll2cr, grids
        'gdal',             # Geotiff backend
        'shapely',          # Grid determination
        'pylibtiff',
        'polar2grid.core',
        'polar2grid.viirs',
        'polar2grid.modis'
        ],
    dependency_links = ['http://larch.ssec.wisc.edu/cgi-bin/repos.cgi'],
    entry_points = {'console_scripts' : [
            'viirs2awips = polar2grid.viirs2awips:main',
            'modis2awips = polar2grid.modis2awips:main'
            ]}
)

