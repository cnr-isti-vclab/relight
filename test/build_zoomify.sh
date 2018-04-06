#!/bin/bash
for i in plane*.jpg; do 
	rm -rf ${i%.jpg}_files;
	rm -rf ${i%.jpg};
	vips dzsave $i ${i%.jpg} --layout zoomify --tile-size 256 --overlap 0  --depth onetile --suffix .jpg[Q=95]; 
done
