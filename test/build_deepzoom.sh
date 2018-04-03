#!/bin/bash
for i in plane*.jpg; do 
	rm -rf ${i%.jpg}_files;
	vips dzsave $i ${i%.jpg} --layout dz --tile-size 256 --overlap 8  --depth onetile --suffix .jpeg[Q=95]; 
done
