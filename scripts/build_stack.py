#!/usr/bin/python3
#
# Convert Relight directory lauout into a single tiled multi-resolution TIFF image stack
# for use with the relight.js iip layout.
#
# Author: Ruven Pillay
#
# Requires pyvips
#
# Basic usage: build_stack -i <path to relight folder> -o <output TIFF file>
#
#

import os
import argparse
import pyvips


# Parse our command line arguments
parser = argparse.ArgumentParser(
  prog='build_stack',
  description='Convert relight output folder to tiled multi-resolution TIFF image stack'
)

parser.add_argument('-i', '--input', help='Input file in EDF format', required=True)
parser.add_argument('-o', '--output', help='Output TIFF file', required=True)
parser.add_argument('-c', '--compression', choices=['jpeg', 'deflate', 'lzw', 'webp', 'none'], default="jpeg", help='Compression format')
parser.add_argument('-q', '--quality', type=int, default=75, help='Compression quality for JPEG and WebP')
parser.add_argument('-v', '--verbose', action='store_true', help='verbose output')
args = parser.parse_args()


# Open file and extract info data
info = os.path.join( args.input, "info.json" )
if os.path.isfile(info):
  f = open(info, "r")
  info = f.read()
  f.close()


# Count number of planes
planes = [filename for filename in os.listdir(args.input) if os.path.isfile(os.path.join(args.input,filename)) and 'plane_' in filename]
planes = len( planes )


if args.verbose:
  print( "%d RTI planes found" % planes )


# Initialize an empty array of RTI planes
relight_planes = []


# Loop through RTI planes
for n in range(0,planes):

  plane = "plane_%d.jpg" % (n)

  # Open image and append to array
  image = pyvips.Image.new_from_file( os.path.join( args.input, plane ) )
  height = image.height
  image.set_type( pyvips.GValue.gint_type, "page-height", height )
  relight_planes.append( image )


# Join image stack vertically as a "toilet-roll"
stack = pyvips.Image.arrayjoin( relight_planes, across=1 )

# Embed info.json in XMP tag
stack.set_type(pyvips.GValue.blob_type, "xmp-data", str.encode(info))

if args.verbose:
  print("Saving to TIFF with compression %s at quality %d" % (args.compression, args.quality))

# Save as a stacked tiled pyramid TIFF using SubIFDs
stack.tiffsave( args.output, pyramid=True, subifd=True,
                compression=args.compression, Q=args.quality,
                tile=True, tile_width=256, tile_height=256 )
