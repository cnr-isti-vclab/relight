# Roadmap for RelightLab.



## Reinstate existing functions:

* lp saving

## New Features

* consider eventual modifications to the file format to mirror interface parameters.
* manual alignment
* ellipse reflections geometrically accurate
* cancel button on sphere reflection processing.
* better info on queue items.
* remove relight_vector and jus use eigen.
* allow resize for normals integration (it's too slow...).

## Long term new features

* preprocessing files and caching
* TIF/CR2/JPEGXL support
* floating point processing
* shadow and burned pixels filtering
* Assisted/automated alignment
* PBR texture creation
* shadow removal
* 3d lights on sphere should take into account sphere posistion/
* allow rescaling when building rti and normals


## Bugs

* dark theme on windows conflicts
* Accessing the crop tab before loading the project causes it to stop working.
* check for invalid inner circle when creating a sphere.
* test pause/stop/play in queue.
* deal with #lights  different from #images
* when the sphere changes size, the icon with the reflection overview is not resized properly

## Small Bugs

* Check version is properly written and read from the .relight file
* normal integrations callbacks.

