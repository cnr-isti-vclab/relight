#!/bin/bash
for i in plane*.jpg; do 
	rm -rf ${i%.jpg}_files;
	rm -rf ${i%.jpg};
vips tiffsave $i ${i%.jpg}.tif --tile --pyramid --compression jpeg --tile-width 256 --tile-height 256 --rgbjpeg
#	vips im_vips2tiff $i ${i%.tif}:jpeg:95,tile256x256,pyramid
done


#rgb remove chroma subsampling. And that is a problem with IIP!
