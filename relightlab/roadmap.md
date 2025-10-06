# Roadmap for RelightLab.


## Todo for release

* installer for windows.
* default dome(s) in preferences (when number of light matches).
* ellipse reflections geometrically accurate.
* better info on queue items.
* metadata
* history + export in json + return status to (normal/rti crop parameters date version).
* align group of thumbs.
* Settings->Performances, port: add tab title.
* Show lens parameters (and check we make use of them)
* BRDF interface rewamp
* check for zero value on scale (or unreasonable ones)
* change max value for scale.
* change maxFixedZoom for Openlime.

## Long term new features

* Normals: Robust photometric stereo (https://people.eecs.berkeley.edu/~yima/matrix-rank/Files/robust_stereo.pdf)
* preprocessing files and caching
* TIF/CR2/JPEGXL support
* floating point processing
* shadow and burned pixels filtering
* Assisted/automated alignment
* PBR texture creation
* shadow removal
* 3d lights on sphere should take into account sphere posistion
* DStretch (should maybe go into openlime?)
* alighment usinc ECC https://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=4515873&casa_token=C2A7tCG4BUAAAAAA:n468ofOZeADfU5JwjrKLM68g9ngnxqf4T9y0Yvdc-wyREuqQj7XfSOPuN4f19ijScwhBG00s&tag=1
  This should work as follow: 1) compute gradients, for each couple of images select a subset of high gradients along the orthogonal direction to the light change.
  this should take care of the moving shadow problem.
  the ecc algorithm works on pairs, we need bundle adjustment.

## Bugs
* brdrf adds jpg to folder
* downsampling doesn't work properly with crop
* How HSH with 18 planes is possible? (missing 27?)
* crop initialized with wrong handles size. (too small)
* icons and font size on linux large screen are too small.
* check for invalid inner circle when creating a sphere.
* Crash by delete queue items.
* test pause/stop/play in queue.
* when the sphere changes size, the icon with the reflection overview is not resized properly

## Small Bugs

* Check version is properly written and read from the .relight file

