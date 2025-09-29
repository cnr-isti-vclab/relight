# Roadmap for RelightLab.


## Todo for release

* installer for windows.
* default dome(s) in preferences (when number of light matches).
* ellipse reflections geometrically accurate.
* better info on queue items.
* metadata
* history + export in json + return status to (normal/rti crop parameters date version).
* align group of thumbs.
* Settings->Casting options: port.
* Settings->Performances: memory, workers
* Show lens parameters (and check we make use of them)
* BRDF interface rewamp
## Long term new features

* preprocessing files and caching
* TIF/CR2/JPEGXL support
* floating point processing
* shadow and burned pixels filtering
* Assisted/automated alignment
* PBR texture creation
* shadow removal
* 3d lights on sphere should take into account sphere posistion
* DStretch (should maybe go into openlime?)

## Bugs

* icons and font size on linux large screen are too small.
* check for invalid inner circle when creating a sphere.
* test pause/stop/play in queue.
* when the sphere changes size, the icon with the reflection overview is not resized properly

## Small Bugs

* Check version is properly written and read from the .relight file

