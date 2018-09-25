#!/bin/bash

#-q 3 best interpolation
#-b 0.8  brightness (lower increases brightness)

OPTIONS=""

#properly expand *'s
shopt -s nullglob

if [ "$1" = "camera" ]; then
	OPTIONS="-w"
fi

if [ "$1" = "warm" ]; then
	OPTIONS="-r 1.202498 1.000000 1.8114 1"
fi

OPTIONS+=" -q 3 -o 1"

echo "dcraw options: " $OPTIONS

for i in *.CR2 *.NEF; do

	BASE=${i%.*}
	echo $BASE
	dcraw $OPTIONS $i;
	convert -quality 95% $BASE.ppm $BASE.jpg;
	rm $BASE.ppm;
done;
