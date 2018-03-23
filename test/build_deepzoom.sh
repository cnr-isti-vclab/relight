#!/bin/bash
for i in plane*.jpg; do 
	vips dzsave $i ${i%.jpg} --layout dz --tile-size 256 --overlap 0  --depth onetile --suffix .jpeg[Q=95]; 
done
