#Lights geometry

<!-- many RTI software consider the light direction only, as if the lights were infinitely distant,
this approximation is somewhat ok for a visual inspection, but severely distorts the normals.
We can instead take into account for each pixel the real light direction if we can reconstruct
the geometry of the scene: 3d positions of the lights and size of the image -->
