#!/bin/bash

for i in coin10 12783 coin16
do

cd $i

echo name,type,colorspace,nplanes,nmaterials,nycc,size,psnr,error > ../$i.rbf27.txt
echo name,type,colorspace,nplanes,nmaterials,nycc,size,psnr,error > ../$i.bilinear27.txt
echo name,type,colorspace,nplanes,nmaterials,nycc,size,psnr,error > ../$i.bilinear18.txt
echo name,type,colorspace,nplanes,nmaterials,nycc,size,psnr,error > ../$i.hsh.txt
echo name,type,colorspace,nplanes,nmaterials,nycc,size,psnr,error > ../$i.ptm.txt


for k in $(seq 0 115)
do
	relight 512 rbf27      -b rbf      -p 27 -q 95 -s 10 -S 0.40 -e -E $k >> ../$i.rbf27.txt
	relight 512 bilinear27 -b bilinear -p 27 -q 95 -s 10 -S 1.25 -e -E $k >> ../$i.bilinear27.txt
	relight 512 bilinear18 -b bilinear -p 18 -q 95 -s 10 -S 1.25 -e -E $k >> ../$i.bilinear18.txt
	relight 512 hsh        -b hsh      -p 27 -q 95 -s 10 -S 1.25 -e -E $k >> ../$i.hsh.txt
	relight 512 ptm        -b ptm      -p 18 -q 95 -s 10 -S 1.25 -e -E $k >> ../$i.ptm.txt
done

cd ..

done
