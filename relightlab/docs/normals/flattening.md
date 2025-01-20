#Flattening normals

<!-- The object might be flat, but the normals are computed using the light direction, 
and failing to account for exact light position (or worse just using the direction) and other
phenomena like vignetting, we will get a deformed surface normals -->

##Radial flattening
<!-- we fit a second degree polinomial throuugh the normals assuming the normals will be flat on average,
the resulting normals wont show 'banana' effects -->

##Fourier flattening
<!-- Here we decompoose normal frequencies (like sound) and filter low frequencies (there is a parameter
to tune it, this not only account for the 'banana' problems but also for local deformations (depending on the
parameter value -->
