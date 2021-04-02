#!/usr/bin/python3

import re, os, sys, json, glob, shutil

width_re = re.compile('Width="(\d+)"')
height_re = re.compile('Height="(\d+)"')
pos_re = re.compile("(\d+)_(\d+).jp.*g")

tilesize = 256
overlap = 0
format = 'jpg'

def tarzoom(basename):
	index =  { "tilesize": tilesize, "overlap": 0, "format":format, "offsets": [0] }

	print(basename)
	with open(basename + '.dzi') as f:
		contents = f.read()
		w = index['width']  = int(width_re.search(contents).group(1))
		h = index['height'] = int(height_re.search(contents).group(1))


	data = open(basename + ".tzb", 'wb')

	levels = sorted([(f.name, f.path) for f in os.scandir(basename + "_files") if f.is_dir()])
	index['nlevels'] = len(levels)

	offset = 0
	offsets = [0]

	for level in levels:

		files = [(f.name, f.path) for f in os.scandir(level[1]) if f.is_file()]
		maxx = 0
		maxy = 0
		for file in files:
			found = pos_re.search(file[0])
			x = int(found.group(1))
			y = int(found.group(2))
			maxx = max(x, maxx)
			maxy = max(y, maxy)

		ordered = [None] *(maxx+1)*(maxy+1)
		for file in files:
			found = pos_re.search(file[0])
			x = int(found.group(1))
			y = int(found.group(2))
			i = x + (maxx+1)*y
			ordered[i] = file


		for file in ordered:
			size = os.stat(file[1]).st_size
#			offset += int(size/256)
			offset += size
			index['offsets'].append(offset)
			offsets.append(offset)
			img = open(file[1], 'br')
			data.write(img.read())

#		index['offsets'].append(offsets)


	with open(basename + ".tzi", 'w') as outfile:
		json.dump(index, outfile)

	os.remove(basename + ".dzi")
	shutil.rmtree(basename + '_files')


if __name__ == "__main__":
	plane = sys.argv[1]
	tarzoom(plane)

