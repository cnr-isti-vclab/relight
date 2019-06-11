for i in coin16 12783 coin10 statue fatt
do

echo $i;

gnuplot -e "file='$i.txt'" gnuplot.txt > $i.eps

done;

