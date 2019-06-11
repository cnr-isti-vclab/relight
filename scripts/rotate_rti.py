#!/usr/bin/python

import sys, json, subprocess
from pprint import pprint

def help():
	print "Usage: " + sys.argv[0] + " <folder> [degrees]\n"

def rotateLights(angle, lights):
	i = 0
	while i < len(lights):
		if angle == '90':
			lights[i], lights[i+1] = -lights[i+1], lights[i]
		elif angle == '180':
			lights[i], lights[i+1] = -lights[i], -lights[i+1]
		elif angle == '270':
			lights[i], lights[i+1] = lights[i+1], -lights[i]
		i += 3
	return lights


if len(sys.argv) >= 4 or len(sys.argv) == 1:
	help()
	exit(1);

dir = sys.argv[1]

try:
	with open(dir + '/info.json') as f:
		data = json.load(f)

except IOError as e:
	print "Error loding the file: " + dir + "info.json:";
	print e.strerror
	exit(1)

if len(sys.argv) == 2:
	angle = '90'
else:
	angle = sys.argv[2]

if angle == '90':
	data["width"], data["height"] = data["height"], data["width"]

elif angle == '180':
	100

elif angle == '270':
	data["width"], data["height"] = data["height"], data["width"]

else:
	print "Rotation supported only for 90, 180, 270 degrees\n"

data['lights'] = rotateLights(angle, data['lights'])


with open(dir + '/info.json', 'w') as outfile:
	json.dump(data, outfile)

njpegs = data['nplanes']/3;

for i in range(0, njpegs):
	subprocess.call(['mogrify', '-rotate', angle, dir + '/plane_%d.jpg' % i])

#for i in plane*.jpg; do
#	mogrify -rotate "90" $i;
#done
