import re, os, sys, json, glob, shutil

#tarzoom for RTI has a problem: a lot of server requests which are not really needed as if you want 
#a tile, you need all the planes.
#itarzoom just zip all the planes in a single (interleaved) plane.

#arguments: all the plane_*.tzi and the output as the last argument (PATH/planes for example, the extensions will be added.)

def itarzoom(planes, output):
	big = {}
	sizes = []
	tzb = []
	for plane in planes:
		print(plane)
		file = open(plane)
		tzi = json.load(file);

#convert offsets in sizes (more manageable)
		offsets = tzi['offsets']
		size = []
		for i in range(0, len(offsets) -1):
			size.append(offsets[i+1] - offsets[i])
		sizes.append(size)


		filename = plane.replace('.tzi', '.tzb')
		tzb.append(open(filename, 'br'))
		if tzb is None:
			print("Could not open file: " + filename)
			exit();

		big = tzi

	big['offsets'] = []
	big['mode'] = 'interleaved'
	big['stride'] = len(planes)
	sizes = [i for tup in zip(*sizes) for i in tup]

	data = open(output + ".tzb", 'wb')

	stride = len(planes)
	p = 0
	offset = 0
	for size in sizes:
		data.write(tzb[p].read(size))
		big['offsets'].append(offset)
		offset += size
		p = (p+1)%stride

	big['offsets'].append(offset)


	with open(output + ".tzi", 'w') as outfile:
		json.dump(big, outfile)

if __name__ == "__main__":
	if len(sys.argv) < 3:
		print("Specyfy 'path/*.tzi <output>' as argument of the shell command");
		exit()

	planes = sys.argv[1:-1]
	itarzoom(planes, sys.argv[-1])
