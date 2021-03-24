#!/usr/bin/python3

import re, os, sys, json, glob


width_re = re.compile('Width="(\d+)"')
#width_re = re.compile('[a-z]+')
height_re = re.compile('Height="(\d+)"')
pos_re = re.compile("(\d+)_(\d+).jpg")

tilesize = 256
overlap = 0
format = 'jpg'

def tarzoom(basename):
	index =  { "tilesize": tilesize, "overlap": 0, "format":format, "offsets": [0] }

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
		print(maxx * maxy)
		for file in files:
			found = pos_re.search(file[0])
			x = int(found.group(1))
			y = int(found.group(2))
			i = x + (maxx+1)*y
			print(x, y, i)
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


for plane in glob.glob('plane_*.dzi'):
	tarzoom(plane[0:-4])









#!/bin/bash
#for i in plane*.jpg; do 
#	rm -rf ${i%.jpg}_files;
#	rm -rf ${i%.jpg};
#	vips dzsave $i ${i%.jpg} --layout dz --tile-size 256 --overlap 1	  --depth onetile --suffix .jpg[Q=95]; 
#	
#done
