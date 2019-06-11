#coin16 12783 coin10 statue fatt;

q=95
#sigma=0.125
sigma=0.2
rate=4

for i in coin16
do

echo $i;


cd $i;
echo name,type,colorspace,nplanes,nmaterials,nycc,size,psnr,error > ../$i.txt
relight 512 lptm -b lptm -p 9 -e -q $q -s $rate -S $sigma >> ../$i.txt
echo $'' >> ../$i.txt
relight 512 ptm -b ptm -p 18 -e -q $q -s $rate -S $sigma >> ../$i.txt
echo $'' >> ../$i.txt
relight 512 hsh -b hsh -p 27 -e -q $q -s $rate -S $sigma >> ../$i.txt
echo $'' >> ../$i.txt
relight 512 yhsh -b yhsh -p 27 -e -q $q -s $rate -S $sigma >> ../$i.txt
echo $'' >> ../$i.txt

relight 512 rbf6  -b rbf -p 6 -e -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 rbf9  -b rbf -p 9 -e -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 rbf12 -b rbf -p 12 -e -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 rbf18 -b rbf -p 18 -e -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 rbf27 -b rbf -p 27 -e -q $q -s $rate -S $sigma >> ../$i.txt
echo $'' >> ../$i.txt

relight 512 ycc522 -b yrbf -p 9 -y 5 -e -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 ycc711 -b yrbf -p 9 -y 7 -e -q $q -s $rate -S $sigma >> ../$i.txt

relight 512 ycc633 -b yrbf -p 12 -y 6 -e -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 ycc822 -b yrbf -p 12 -y 8 -e -q $q -s $rate -S $sigma >> ../$i.txt

relight 512 ycc1044 -b yrbf -p 18 -y 10 -e -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 ycc1233 -b yrbf -p 18 -y 12 -e -q $q -s $rate -S $sigma >> ../$i.txt

relight 512 ycc1755 -b yrbf -p 27 -y 17 -e -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 ycc1944 -b yrbf -p 27 -y 19 -e -q $q -s $rate -S $sigma >> ../$i.txt
echo $'' >> ../$i.txt

#relight 512 ycc444 -b yrbf -p 12 -y 4 -e -q $q -s $rate -S $sigma >> ../$i.txt
#relight 512 ycc1011 -b yrbf -p 12 -y 10 -e -q $q -s $rate -S $sigma >> ../$i.txt
#relight 512 ycc555 -b yrbf -p 15 -y 5 -e -q $q -s $rate -S $sigma >> ../$i.txt
#relight 512 ycc1122 -b yrbf -p 15 -y 11 -e -q $q -s $rate -S $sigma >> ../$i.txt

#relight 512 ycc666 -b yrbf -p 18 -y 6 -e -q $q -s $rate -S $sigma >> ../$i.txt
#relight 512 ycc1233 -b yrbf -p 18 -y 12 -e -q $q -s $rate -S $sigma >> ../$i.txt


relight 512 bilinear9  -b bilinear -p 9 -e -r 9 -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 bilinear12 -b bilinear -p 12 -e -r 9 -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 bilinear18 -b bilinear -p 18 -e -r 9 -q $q -s $rate -S $sigma >> ../$i.txt
relight 512 bilinear27 -b bilinear -p 27 -e -r 9 -q $q -s $rate -S $sigma >> ../$i.txt


#relight 512 ycc822 -b bilinear -p 12 -y 8 -e -q $q -s $rate -S $sigma >> ../$i.txt
#relight 512 ycc933 -b yrbf -p 15 -y 9 -e -q $q -s $rate -S $sigma >> ../$i.txt
#relight 512 ycc1044 -b yrbf -p 18 -y 10 -e -q $q -s $rate -S $sigma >> ../$i.txt

cd ..

done;
