for i in light/scalable/*.svg; do
	file="$(basename -- $i)"
	echo $file
	sed 's/currentColor/white/' "$i" > dark/scalable/"$file";
done;	

