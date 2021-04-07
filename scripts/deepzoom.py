import sys
from pyvips import Image


if __name__ == "__main__":
	plane = sys.argv[1]
	quality = sys.argv[2]

	filename = plane + '.jpg'
	image = Image.new_from_file(filename, access='sequential')
	image.dzsave(plane, overlap=0, tile_size=256, depth='onetile', suffix='.jpg[Q=' + str(quality) + ']')
