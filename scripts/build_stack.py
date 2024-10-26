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

parser.add_argument('-i', '--input', help='Input directory containing relight RTI layout', required=True)
parser.add_argument('-o', '--output', help='Output TIFF file', required=True)
parser.add_argument('-c', '--compression', choices=['jpeg', 'deflate', 'lzw', 'webp', 'none'], default="jpeg", help='Compression format (default: %(default)s)')
parser.add_argument('-q', '--quality', type=int, default=90, help='Compression quality for JPEG and WebP (default: %(default)s)')
parser.add_argument('-t', '--tile', type=int, default=256, help="Tile size (default: %(default)s)")
parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
args = parser.parse_args()


# Open file and extract info data
info = os.path.join( args.input, "info.json" )
if os.path.isfile(info):
  f = open(info, "r")
  info = f.read()
  f.close()


# Update format field within the info data depending on choice of compression
if args.compression != "jpeg":
  format = '"format": "' + args.compression + '",'
  info = info.replace( '"format": "jpg",', format )


# Check for spatial resolution within info file
dpi = 0.0
n = info.find("pixelSizeInMM")
if n > 0:
  # Vips uses pixels/mm internally, so no scaling necessary
  dpi = 1.0 / float( info[n+16:n+24] )


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


# Embed info.json in ImageDescription tag (tag 270)
stack.set_type(pyvips.GValue.gstr_type, "image-description", str.encode(info))


if args.verbose:
  print("Saving to TIFF with a tile size %d and %s compression at quality %d" % (args.tile, args.compression, args.quality))

# Save as a stacked tiled pyramid TIFF using SubIFDs
if dpi == 0.0:
stack.tiffsave( args.output, pyramid=True, subifd=True,
                compression=args.compression, Q=args.quality,
                  tile=True, tile_width=args.tile, tile_height=args.tile )
else:
  # Include spatial resolution if we have it
  stack.tiffsave( args.output, pyramid=True, subifd=True,
                  compression=args.compression, Q=args.quality,
                  tile=True, tile_width=args.tile, tile_height=args.tile,
                  xres=dpi, yres=dpi, resunit="cm" )
